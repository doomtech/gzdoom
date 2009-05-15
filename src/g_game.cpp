// Emacs style mode select	 -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:  none
//
//-----------------------------------------------------------------------------



#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <time.h>

#include "templates.h"
#include "version.h"
#include "doomdef.h" 
#include "doomstat.h"
#include "d_protocol.h"
#include "d_netinf.h"
#include "f_finale.h"
#include "m_argv.h"
#include "m_misc.h"
#include "m_menu.h"
#include "m_random.h"
#include "m_crc32.h"
#include "i_system.h"
#include "p_saveg.h"
#include "p_tick.h"
#include "d_main.h"
#include "wi_stuff.h"
#include "hu_stuff.h"
#include "st_stuff.h"
#include "am_map.h"
#include "c_console.h"
#include "c_cvars.h"
#include "c_bind.h"
#include "c_dispatch.h"
#include "v_video.h"
#include "w_wad.h"
#include "p_local.h" 
#include "s_sound.h"
#include "gstrings.h"
#include "r_data.h"
#include "r_sky.h"
#include "r_draw.h"
#include "g_game.h"
#include "g_level.h"
#include "b_bot.h"			//Added by MC:
#include "sbar.h"
#include "m_swap.h"
#include "m_png.h"
#include "gi.h"
#include "a_keys.h"
#include "a_artifacts.h"
#include "r_translate.h"
#include "cmdlib.h"
#include "d_net.h"
#include "d_event.h"
#include "p_acs.h"

#include <zlib.h>

#include "g_hub.h"


static FRandom pr_dmspawn ("DMSpawn");

const int SAVEPICWIDTH = 216;
const int SAVEPICHEIGHT = 162;

bool	G_CheckDemoStatus (void);
void	G_ReadDemoTiccmd (ticcmd_t *cmd, int player);
void	G_WriteDemoTiccmd (ticcmd_t *cmd, int player, int buf);
void	G_PlayerReborn (int player);

void	G_DoNewGame (void);
void	G_DoLoadGame (void);
void	G_DoPlayDemo (void);
void	G_DoCompleted (void);
void	G_DoVictory (void);
void	G_DoWorldDone (void);
void	G_DoSaveGame (bool okForQuicksave, FString filename, const char *description);
void	G_DoAutoSave ();

FIntCVar gameskill ("skill", 2, CVAR_SERVERINFO|CVAR_LATCH);
CVAR (Int, deathmatch, 0, CVAR_SERVERINFO|CVAR_LATCH);
CVAR (Bool, chasedemo, false, 0);
CVAR (Bool, storesavepic, true, CVAR_ARCHIVE|CVAR_GLOBALCONFIG)
CVAR (Bool, longsavemessages, true, CVAR_ARCHIVE|CVAR_GLOBALCONFIG)
CVAR (String, save_dir, "", CVAR_ARCHIVE|CVAR_GLOBALCONFIG);

gameaction_t	gameaction;
gamestate_t 	gamestate = GS_STARTUP;

int 			paused;
bool 			sendpause;				// send a pause event next tic 
bool			sendsave;				// send a save event next tic 
bool			sendturn180;			// [RH] send a 180 degree turn next tic
bool 			usergame;				// ok to save / end game
bool			insave;					// Game is saving - used to block exit commands

bool			timingdemo; 			// if true, exit with report on completion 
bool 			nodrawers;				// for comparative timing purposes 
bool 			noblit; 				// for comparative timing purposes 

bool	 		viewactive;

bool 			netgame;				// only true if packets are broadcast 
bool			multiplayer;
player_t		players[MAXPLAYERS];
bool			playeringame[MAXPLAYERS];

int 			consoleplayer;			// player taking events
int 			gametic;

CVAR(Bool, demo_compress, true, CVAR_ARCHIVE|CVAR_GLOBALCONFIG);
char			demoname[256];
bool 			demorecording;
bool 			demoplayback;
bool 			netdemo;
bool			demonew;				// [RH] Only used around G_InitNew for demos
int				demover;
BYTE*			demobuffer;
BYTE*			demo_p;
BYTE*			democompspot;
BYTE*			demobodyspot;
size_t			maxdemosize;
BYTE*			zdemformend;			// end of FORM ZDEM chunk
BYTE*			zdembodyend;			// end of ZDEM BODY chunk
bool 			singledemo; 			// quit after playing a demo from cmdline 
 
bool 			precache = true;		// if true, load all graphics at start 
 
wbstartstruct_t wminfo; 				// parms for world map / intermission 
 
short			consistancy[MAXPLAYERS][BACKUPTICS];
 
BYTE*			savebuffer;
 
 
#define MAXPLMOVE				(forwardmove[1]) 
 
#define TURBOTHRESHOLD	12800

float	 		normforwardmove[2] = {0x19, 0x32};		// [RH] For setting turbo from console
float	 		normsidemove[2] = {0x18, 0x28};			// [RH] Ditto

fixed_t			forwardmove[2], sidemove[2];
fixed_t 		angleturn[4] = {640, 1280, 320, 320};		// + slow turn
fixed_t			flyspeed[2] = {1*256, 3*256};
int				lookspeed[2] = {450, 512};

#define SLOWTURNTICS	6 

CVAR (Bool,		cl_run,			false,	CVAR_GLOBALCONFIG|CVAR_ARCHIVE)		// Always run?
CVAR (Bool,		invertmouse,	false,	CVAR_GLOBALCONFIG|CVAR_ARCHIVE)		// Invert mouse look down/up?
CVAR (Bool,		freelook,		false,	CVAR_GLOBALCONFIG|CVAR_ARCHIVE)		// Always mlook?
CVAR (Bool,		lookstrafe,		false,	CVAR_GLOBALCONFIG|CVAR_ARCHIVE)		// Always strafe with mouse?
CVAR (Float,	m_pitch,		1.f,	CVAR_GLOBALCONFIG|CVAR_ARCHIVE)		// Mouse speeds
CVAR (Float,	m_yaw,			1.f,	CVAR_GLOBALCONFIG|CVAR_ARCHIVE)
CVAR (Float,	m_forward,		1.f,	CVAR_GLOBALCONFIG|CVAR_ARCHIVE)
CVAR (Float,	m_side,			2.f,	CVAR_GLOBALCONFIG|CVAR_ARCHIVE)
 
int 			turnheld;								// for accelerative turning 
 
// mouse values are used once 
int 			mousex;
int 			mousey; 		

FString			savegamefile;
char			savedescription[SAVESTRINGSIZE];

// [RH] Name of screenshot file to generate (usually NULL)
FString			shotfile;

AActor* 		bodyque[BODYQUESIZE]; 
int 			bodyqueslot; 

void R_ExecuteSetViewSize (void);

FString savename;
FString BackupSaveName;

bool SendLand;
const AInventory *SendItemUse, *SendItemDrop;

EXTERN_CVAR (Int, team)

CVAR (Bool, teamplay, false, CVAR_SERVERINFO)

// [RH] Allow turbo setting anytime during game
CUSTOM_CVAR (Float, turbo, 100.f, 0)
{
	if (self < 10.f)
	{
		self = 10.f;
	}
	else if (self > 256.f)
	{
		self = 256.f;
	}
	else
	{
		double scale = self * 0.01;

		forwardmove[0] = (int)(normforwardmove[0]*scale);
		forwardmove[1] = (int)(normforwardmove[1]*scale);
		sidemove[0] = (int)(normsidemove[0]*scale);
		sidemove[1] = (int)(normsidemove[1]*scale);
	}
}

CCMD (turnspeeds)
{
	if (argv.argc() == 1)
	{
		Printf ("Current turn speeds: %d %d %d %d\n", angleturn[0],
			angleturn[1], angleturn[2], angleturn[3]);
	}
	else
	{
		int i;

		for (i = 1; i <= 4 && i < argv.argc(); ++i)
		{
			angleturn[i-1] = atoi (argv[i]);
		}
		if (i <= 2)
		{
			angleturn[1] = angleturn[0] * 2;
		}
		if (i <= 3)
		{
			angleturn[2] = angleturn[0] / 2;
		}
		if (i <= 4)
		{
			angleturn[3] = angleturn[2];
		}
	}
}

CCMD (slot)
{
	if (argv.argc() > 1)
	{
		int slot = atoi (argv[1]);

		if (slot < NUM_WEAPON_SLOTS)
		{
			SendItemUse = players[consoleplayer].weapons.Slots[slot].PickWeapon (&players[consoleplayer]);
		}
	}
}

CCMD (centerview)
{
	Net_WriteByte (DEM_CENTERVIEW);
}

CCMD(crouch)
{
	Net_WriteByte(DEM_CROUCH);
}

CCMD (land)
{
	SendLand = true;
}

CCMD (pause)
{
	sendpause = true;
}

CCMD (turn180)
{
	sendturn180 = true;
}

CCMD (weapnext)
{
	SendItemUse = players[consoleplayer].weapons.PickNextWeapon (&players[consoleplayer]);
}

CCMD (weapprev)
{
	SendItemUse = players[consoleplayer].weapons.PickPrevWeapon (&players[consoleplayer]);
}

CCMD (invnext)
{
	AInventory *next;

	if (who == NULL)
		return;

	if (who->InvSel != NULL)
	{
		if ((next = who->InvSel->NextInv()) != NULL)
		{
			who->InvSel = next;
		}
		else
		{
			// Select the first item in the inventory
			if (!(who->Inventory->ItemFlags & IF_INVBAR))
			{
				who->InvSel = who->Inventory->NextInv();
			}
			else
			{
				who->InvSel = who->Inventory;
			}
		}
	}
	who->player->inventorytics = 5*TICRATE;
}

CCMD (invprev)
{
	AInventory *item, *newitem;

	if (who == NULL)
		return;

	if (who->InvSel != NULL)
	{
		if ((item = who->InvSel->PrevInv()) != NULL)
		{
			who->InvSel = item;
		}
		else
		{
			// Select the last item in the inventory
			item = who->InvSel;
			while ((newitem = item->NextInv()) != NULL)
			{
				item = newitem;
			}
			who->InvSel = item;
		}
	}
	who->player->inventorytics = 5*TICRATE;
}

CCMD (invuseall)
{
	SendItemUse = (const AInventory *)1;
}

CCMD (invuse)
{
	if (players[consoleplayer].inventorytics == 0 || gameinfo.gametype == GAME_Strife)
	{
		if (players[consoleplayer].mo) SendItemUse = players[consoleplayer].mo->InvSel;
	}
	players[consoleplayer].inventorytics = 0;
}

CCMD(invquery)
{
	AInventory *inv = players[consoleplayer].mo->InvSel;
	if (inv != NULL)
	{
		const char *description = inv->GetClass()->Meta.GetMetaString(AMETA_StrifeName);
		if (description == NULL) description = inv->GetClass()->TypeName;
		Printf(PRINT_HIGH, "%s (%dx)\n", description, inv->Amount);
	}
}

CCMD (use)
{
	if (argv.argc() > 1 && who != NULL)
	{
		SendItemUse = who->FindInventory (PClass::FindClass (argv[1]));
	}
}

CCMD (invdrop)
{
	if (players[consoleplayer].mo) SendItemDrop = players[consoleplayer].mo->InvSel;
}

CCMD (weapdrop)
{
	SendItemDrop = players[consoleplayer].ReadyWeapon;
}

CCMD (drop)
{
	if (argv.argc() > 1 && who != NULL)
	{
		SendItemDrop = who->FindInventory (PClass::FindClass (argv[1]));
	}
}

CCMD (useflechette)
{ // Select from one of arti_poisonbag1-3, whichever the player has
	static const ENamedName bagnames[3] =
	{
		NAME_ArtiPoisonBag1,
		NAME_ArtiPoisonBag2,
		NAME_ArtiPoisonBag3
	};
	int i, j;

	if (who == NULL)
		return;

	if (who->IsKindOf (PClass::FindClass (NAME_ClericPlayer)))
		i = 0;
	else if (who->IsKindOf (PClass::FindClass (NAME_MagePlayer)))
		i = 1;
	else
		i = 2;

	for (j = 0; j < 3; ++j)
	{
		AInventory *item;
		if ( (item = who->FindInventory (bagnames[(i+j)%3])) )
		{
			SendItemUse = item;
			break;
		}
	}
}

CCMD (select)
{
	if (argv.argc() > 1)
	{
		AInventory *item = who->FindInventory (PClass::FindClass (argv[1]));
		if (item != NULL)
		{
			who->InvSel = item;
		}
	}
	who->player->inventorytics = 5*TICRATE;
}

//
// G_BuildTiccmd
// Builds a ticcmd from all of the available inputs
// or reads it from the demo buffer.
// If recording a demo, write it out
//
void G_BuildTiccmd (ticcmd_t *cmd)
{
	int 		strafe;
	int 		speed;
	int 		forward;
	int 		side;
	int			fly;

	ticcmd_t	*base;

	base = I_BaseTiccmd (); 			// empty, or external driver
	*cmd = *base;

	cmd->consistancy = consistancy[consoleplayer][(maketic/ticdup)%BACKUPTICS];

	strafe = Button_Strafe.bDown;
	speed = Button_Speed.bDown ^ (int)cl_run;

	forward = side = fly = 0;

	// [RH] only use two stage accelerative turning on the keyboard
	//		and not the joystick, since we treat the joystick as
	//		the analog device it is.
	if (Button_Left.bDown || Button_Right.bDown)
		turnheld += ticdup;
	else
		turnheld = 0;

	// let movement keys cancel each other out
	if (strafe)
	{
		if (Button_Right.bDown)
			side += sidemove[speed];
		if (Button_Left.bDown)
			side -= sidemove[speed];
	}
	else
	{
		int tspeed = speed;

		if (turnheld < SLOWTURNTICS)
			tspeed *= 2;		// slow turn
		
		if (Button_Right.bDown)
		{
			G_AddViewAngle (angleturn[tspeed]);
			LocalKeyboardTurner = true;
		}
		if (Button_Left.bDown)
		{
			G_AddViewAngle (-angleturn[tspeed]);
			LocalKeyboardTurner = true;
		}
	}

	if (Button_LookUp.bDown)
		G_AddViewPitch (lookspeed[speed]);
	if (Button_LookDown.bDown)
		G_AddViewPitch (-lookspeed[speed]);

	if (Button_MoveUp.bDown)
		fly += flyspeed[speed];
	if (Button_MoveDown.bDown)
		fly -= flyspeed[speed];

	if (Button_Klook.bDown)
	{
		if (Button_Forward.bDown)
			G_AddViewPitch (lookspeed[speed]);
		if (Button_Back.bDown)
			G_AddViewPitch (-lookspeed[speed]);
	}
	else
	{
		if (Button_Forward.bDown)
			forward += forwardmove[speed];
		if (Button_Back.bDown)
			forward -= forwardmove[speed];
	}

	if (Button_MoveRight.bDown)
		side += sidemove[speed];
	if (Button_MoveLeft.bDown)
		side -= sidemove[speed];

	// buttons
	if (Button_Attack.bDown)		cmd->ucmd.buttons |= BT_ATTACK;
	if (Button_AltAttack.bDown)		cmd->ucmd.buttons |= BT_ALTATTACK;
	if (Button_Use.bDown)			cmd->ucmd.buttons |= BT_USE;
	if (Button_Jump.bDown)			cmd->ucmd.buttons |= BT_JUMP;
	if (Button_Crouch.bDown)		cmd->ucmd.buttons |= BT_CROUCH;
	if (Button_Zoom.bDown)			cmd->ucmd.buttons |= BT_ZOOM;
	if (Button_Reload.bDown)		cmd->ucmd.buttons |= BT_RELOAD;

	if (Button_User1.bDown)			cmd->ucmd.buttons |= BT_USER1;
	if (Button_User2.bDown)			cmd->ucmd.buttons |= BT_USER2;
	if (Button_User3.bDown)			cmd->ucmd.buttons |= BT_USER3;
	if (Button_User4.bDown)			cmd->ucmd.buttons |= BT_USER4;

	if (Button_Speed.bDown)			cmd->ucmd.buttons |= BT_SPEED;
	if (Button_Strafe.bDown)		cmd->ucmd.buttons |= BT_STRAFE;
	if (Button_MoveRight.bDown)		cmd->ucmd.buttons |= BT_MOVERIGHT;
	if (Button_MoveLeft.bDown)		cmd->ucmd.buttons |= BT_MOVELEFT;
	if (Button_LookDown.bDown)		cmd->ucmd.buttons |= BT_LOOKDOWN;
	if (Button_LookUp.bDown)		cmd->ucmd.buttons |= BT_LOOKUP;
	if (Button_Back.bDown)			cmd->ucmd.buttons |= BT_BACK;
	if (Button_Forward.bDown)		cmd->ucmd.buttons |= BT_FORWARD;
	if (Button_Right.bDown)			cmd->ucmd.buttons |= BT_RIGHT;
	if (Button_Left.bDown)			cmd->ucmd.buttons |= BT_LEFT;
	if (Button_MoveDown.bDown)		cmd->ucmd.buttons |= BT_MOVEDOWN;
	if (Button_MoveUp.bDown)		cmd->ucmd.buttons |= BT_MOVEUP;
	if (Button_ShowScores.bDown)	cmd->ucmd.buttons |= BT_SHOWSCORES;

	// [RH] Scale joystick moves to full range of allowed speeds
	if (JoyAxes[JOYAXIS_PITCH] != 0)
	{
		G_AddViewPitch (int((JoyAxes[JOYAXIS_PITCH] * 2048) / 256));
		LocalKeyboardTurner = true;
	}
	if (JoyAxes[JOYAXIS_YAW] != 0)
	{
		G_AddViewAngle (int((-1280 * JoyAxes[JOYAXIS_YAW]) / 256));
		LocalKeyboardTurner = true;
	}

	side += int((MAXPLMOVE * JoyAxes[JOYAXIS_SIDE]) / 256);
	forward += int((JoyAxes[JOYAXIS_FORWARD] * MAXPLMOVE) / 256);
	fly += int(JoyAxes[JOYAXIS_UP] * 8);

	if (!Button_Mlook.bDown && !freelook)
	{
		forward += (int)((float)mousey * m_forward);
	}

	cmd->ucmd.pitch = LocalViewPitch >> 16;

	if (SendLand)
	{
		SendLand = false;
		fly = -32768;
	}

	if (strafe || lookstrafe)
		side += (int)((float)mousex * m_side);

	mousex = mousey = 0;

	if (forward > MAXPLMOVE)
		forward = MAXPLMOVE;
	else if (forward < -MAXPLMOVE)
		forward = -MAXPLMOVE;
	if (side > MAXPLMOVE)
		side = MAXPLMOVE;
	else if (side < -MAXPLMOVE)
		side = -MAXPLMOVE;

	cmd->ucmd.forwardmove += forward;
	cmd->ucmd.sidemove += side;
	cmd->ucmd.yaw = LocalViewAngle >> 16;
	cmd->ucmd.upmove = fly;
	LocalViewAngle = 0;
	LocalViewPitch = 0;

	// special buttons
	if (sendturn180)
	{
		sendturn180 = false;
		cmd->ucmd.buttons |= BT_TURN180;
	}
	if (sendpause)
	{
		sendpause = false;
		Net_WriteByte (DEM_PAUSE);
	}
	if (sendsave)
	{
		sendsave = false;
		Net_WriteByte (DEM_SAVEGAME);
		Net_WriteString (savegamefile);
		Net_WriteString (savedescription);
		savegamefile = "";
	}
	if (SendItemUse == (const AInventory *)1)
	{
		Net_WriteByte (DEM_INVUSEALL);
		SendItemUse = NULL;
	}
	else if (SendItemUse != NULL)
	{
		Net_WriteByte (DEM_INVUSE);
		Net_WriteLong (SendItemUse->InventoryID);
		SendItemUse = NULL;
	}
	if (SendItemDrop != NULL)
	{
		Net_WriteByte (DEM_INVDROP);
		Net_WriteLong (SendItemDrop->InventoryID);
		SendItemDrop = NULL;
	}

	cmd->ucmd.forwardmove <<= 8;
	cmd->ucmd.sidemove <<= 8;
}

//[Graf Zahl] This really helps if the mouse update rate can't be increased!
CVAR (Bool,		smooth_mouse,	false,	CVAR_GLOBALCONFIG|CVAR_ARCHIVE)

void G_AddViewPitch (int look)
{
	if (gamestate == GS_TITLELEVEL)
	{
		return;
	}
	look <<= 16;
	if (!level.IsFreelookAllowed())
	{
		LocalViewPitch = 0;
	}
	else if (look > 0)
	{
		// Avoid overflowing
		if (LocalViewPitch + look <= LocalViewPitch)
		{
			LocalViewPitch = 0x78000000;
		}
		else
		{
			LocalViewPitch = MIN(LocalViewPitch + look, 0x78000000);
		}
	}
	else if (look < 0)
	{
		// Avoid overflowing
		if (LocalViewPitch + look >= LocalViewPitch)
		{
			LocalViewPitch = -0x78000000;
		}
		else
		{
			LocalViewPitch = MAX(LocalViewPitch + look, -0x78000000);
		}
	}
	if (look != 0)
	{
		LocalKeyboardTurner = smooth_mouse;
	}
}

void G_AddViewAngle (int yaw)
{
	if (gamestate == GS_TITLELEVEL)
	{
		return;
	}
	LocalViewAngle -= yaw << 16;
	if (yaw != 0)
	{
		LocalKeyboardTurner = smooth_mouse;
	}
}

CVAR (Bool, bot_allowspy, false, 0)

// [RH] Spy mode has been separated into two console commands.
//		One goes forward; the other goes backward.
static void ChangeSpy (bool forward)
{
	// If you're not in a level, then you can't spy.
	if (gamestate != GS_LEVEL)
	{
		return;
	}

	// If not viewing through a player, return your eyes to your own head.
	if (players[consoleplayer].camera->player == NULL)
	{
		players[consoleplayer].camera = players[consoleplayer].mo;
		return;
	}

	// We may not be allowed to spy on anyone.
	if (dmflags2 & DF2_DISALLOW_SPYING)
		return;

	// Otherwise, cycle to the next player.
	bool checkTeam = !demoplayback && deathmatch;
	int pnum = int(players[consoleplayer].camera->player - players);
	int step = forward ? 1 : -1;

	do
	{
		pnum += step;
		pnum &= MAXPLAYERS-1;
		if (playeringame[pnum] &&
			(!checkTeam || players[pnum].mo->IsTeammate (players[consoleplayer].mo) ||
			(bot_allowspy && players[pnum].isbot)))
		{
			break;
		}
	} while (pnum != consoleplayer);

	players[consoleplayer].camera = players[pnum].mo;
	S_UpdateSounds(players[consoleplayer].camera);
	StatusBar->AttachToPlayer (&players[pnum]);
	if (demoplayback || multiplayer)
	{
		StatusBar->ShowPlayerName ();
	}
}

CCMD (spynext)
{
	// allow spy mode changes even during the demo
	ChangeSpy (true);
}

CCMD (spyprev)
{
	// allow spy mode changes even during the demo
	ChangeSpy (false);
}


//
// G_Responder
// Get info needed to make ticcmd_ts for the players.
//
bool G_Responder (event_t *ev)
{
	// any other key pops up menu if in demos
	// [RH] But only if the key isn't bound to a "special" command
	if (gameaction == ga_nothing && 
		(demoplayback || gamestate == GS_DEMOSCREEN || gamestate == GS_TITLELEVEL))
	{
		const char *cmd = C_GetBinding (ev->data1);

		if (ev->type == EV_KeyDown)
		{

			if (!cmd || (
				strnicmp (cmd, "menu_", 5) &&
				stricmp (cmd, "toggleconsole") &&
				stricmp (cmd, "sizeup") &&
				stricmp (cmd, "sizedown") &&
				stricmp (cmd, "togglemap") &&
				stricmp (cmd, "spynext") &&
				stricmp (cmd, "spyprev") &&
				stricmp (cmd, "chase") &&
				stricmp (cmd, "+showscores") &&
				stricmp (cmd, "bumpgamma") &&
				stricmp (cmd, "screenshot")))
			{
				M_StartControlPanel (true);
				return true;
			}
			else
			{
				return C_DoKey (ev);
			}
		}
		if (cmd && cmd[0] == '+')
			return C_DoKey (ev);

		return false;
	}

	if (CT_Responder (ev))
		return true;			// chat ate the event

	if (gamestate == GS_LEVEL)
	{
		if (ST_Responder (ev))
			return true;		// status window ate it
		if (!viewactive && AM_Responder (ev))
			return true;		// automap ate it
	}
	else if (gamestate == GS_FINALE)
	{
		if (F_Responder (ev))
			return true;		// finale ate the event
	}

	switch (ev->type)
	{
	case EV_KeyDown:
		if (C_DoKey (ev))
			return true;
		break;

	case EV_KeyUp:
		C_DoKey (ev);
		break;

	// [RH] mouse buttons are sent as key up/down events
	case EV_Mouse: 
		mousex = (int)(ev->x * mouse_sensitivity);
		mousey = (int)(ev->y * mouse_sensitivity);
		break;
	}

	// [RH] If the view is active, give the automap a chance at
	// the events *last* so that any bound keys get precedence.

	if (gamestate == GS_LEVEL && viewactive)
		return AM_Responder (ev);

	return (ev->type == EV_KeyDown ||
			ev->type == EV_Mouse);
}



//
// G_Ticker
// Make ticcmd_ts for the players.
//
extern FTexture *Page;


void G_Ticker ()
{
	int i;
	gamestate_t	oldgamestate;

	// do player reborns if needed
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i] &&
			(players[i].playerstate == PST_REBORN || players[i].playerstate == PST_ENTER))
		{
			G_DoReborn (i, false);
		}
	}

	if (ToggleFullscreen)
	{
		static char toggle_fullscreen[] = "toggle fullscreen";
		ToggleFullscreen = false;
		AddCommandString (toggle_fullscreen);
	}

	// do things to change the game state
	oldgamestate = gamestate;
	while (gameaction != ga_nothing)
	{
		if (gameaction == ga_newgame2)
		{
			gameaction = ga_newgame;
			break;
		}
		switch (gameaction)
		{
		case ga_loadlevel:
			G_DoLoadLevel (-1, false);
			break;
		case ga_newgame2:	// Silence GCC (see above)
		case ga_newgame:
			G_DoNewGame ();
			break;
		case ga_loadgame:
		case ga_autoloadgame:
			G_DoLoadGame ();
			break;
		case ga_savegame:
			G_DoSaveGame (true, savegamefile, savedescription);
			gameaction = ga_nothing;
			savegamefile = "";
			savedescription[0] = '\0';
			break;
		case ga_autosave:
			G_DoAutoSave ();
			gameaction = ga_nothing;
			break;
		case ga_playdemo:
			G_DoPlayDemo ();
			break;
		case ga_completed:
			G_DoCompleted ();
			break;
		case ga_slideshow:
			F_StartSlideshow ();
			break;
		case ga_worlddone:
			G_DoWorldDone ();
			break;
		case ga_screenshot:
			M_ScreenShot (shotfile);
			shotfile = "";
			gameaction = ga_nothing;
			break;
		case ga_fullconsole:
			C_FullConsole ();
			gameaction = ga_nothing;
			break;
		case ga_togglemap:
			AM_ToggleMap ();
			gameaction = ga_nothing;
			break;
		case ga_nothing:
			break;
		}
		C_AdjustBottom ();
	}

	if (oldgamestate != gamestate)
	{
		if (oldgamestate == GS_DEMOSCREEN && Page != NULL)
		{
			Page->Unload();
			Page = NULL;
		}
		else if (oldgamestate == GS_FINALE)
		{
			F_EndFinale ();
		}
	}

	// get commands, check consistancy, and build new consistancy check
	int buf = (gametic/ticdup)%BACKUPTICS;

	// [RH] Include some random seeds and player stuff in the consistancy
	// check, not just the player's x position like BOOM.
	DWORD rngsum = FRandom::StaticSumSeeds ();

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i])
		{
			ticcmd_t *cmd = &players[i].cmd;
			ticcmd_t *newcmd = &netcmds[i][buf];

			if ((gametic % ticdup) == 0)
			{
				RunNetSpecs (i, buf);
			}
			if (demorecording)
			{
				G_WriteDemoTiccmd (newcmd, i, buf);
			}
			players[i].oldbuttons = cmd->ucmd.buttons;
			// If the user alt-tabbed away, paused gets set to -1. In this case,
			// we do not want to read more demo commands until paused is no
			// longer negative.
			if (demoplayback && paused >= 0)
			{
				G_ReadDemoTiccmd (cmd, i);
			}
			else
			{
				memcpy (cmd, newcmd, sizeof(ticcmd_t));
			}

			// check for turbo cheats
			if (cmd->ucmd.forwardmove > TURBOTHRESHOLD &&
				!(gametic&31) && ((gametic>>5)&(MAXPLAYERS-1)) == i )
			{
				Printf ("%s is turbo!\n", players[i].userinfo.netname);
			}

			if (netgame && !players[i].isbot && !netdemo && (gametic%ticdup) == 0)
			{
				//players[i].inconsistant = 0;
				if (gametic > BACKUPTICS*ticdup && consistancy[i][buf] != cmd->consistancy)
				{
					players[i].inconsistant = gametic - BACKUPTICS*ticdup;
				}
				if (players[i].mo)
				{
					DWORD sum = rngsum + players[i].mo->x + players[i].mo->y + players[i].mo->z
						+ players[i].mo->angle + players[i].mo->pitch;
					sum ^= players[i].health;
					consistancy[i][buf] = sum;
				}
				else
				{
					consistancy[i][buf] = rngsum;
				}
			}
		}
	}

	// do main actions
	switch (gamestate)
	{
	case GS_LEVEL:
		P_Ticker ();
		AM_Ticker ();
		break;

	case GS_TITLELEVEL:
		P_Ticker ();
		break;

	case GS_INTERMISSION:
		WI_Ticker ();
		break;

	case GS_FINALE:
		F_Ticker ();
		break;

	case GS_DEMOSCREEN:
		D_PageTicker ();
		break;

	case GS_STARTUP:
		if (gameaction == ga_nothing)
		{
			gamestate = GS_FULLCONSOLE;
			gameaction = ga_fullconsole;
		}
		break;

	default:
		break;
	}
}


//
// PLAYER STRUCTURE FUNCTIONS
// also see P_SpawnPlayer in P_Mobj
//

//
// G_PlayerFinishLevel
// Called when a player completes a level.
//
void G_PlayerFinishLevel (int player, EFinishLevelType mode, bool resetinventory)
{
	AInventory *item, *next;
	player_t *p;

	p = &players[player];

	// Strip all current powers, unless moving in a hub and the power is okay to keep.
	item = p->mo->Inventory;
	while (item != NULL)
	{
		next = item->Inventory;
		if (item->IsKindOf (RUNTIME_CLASS(APowerup)))
		{
			if (deathmatch || mode != FINISH_SameHub || !(item->ItemFlags & IF_HUBPOWER))
			{
				item->Destroy ();
			}
		}
		item = next;
	}
	if (p->ReadyWeapon != NULL &&
		p->ReadyWeapon->WeaponFlags&WIF_POWERED_UP &&
		p->PendingWeapon == p->ReadyWeapon->SisterWeapon)
	{
		// Unselect powered up weapons if the unpowered counterpart is pending
		p->ReadyWeapon=p->PendingWeapon;
	}
	p->mo->flags &= ~MF_SHADOW; 		// cancel invisibility
	p->mo->RenderStyle = STYLE_Normal;
	p->mo->alpha = FRACUNIT;
	p->extralight = 0;					// cancel gun flashes
	p->fixedcolormap = 0;				// cancel ir goggles
	p->damagecount = 0; 				// no palette changes
	p->bonuscount = 0;
	p->poisoncount = 0;
	p->inventorytics = 0;

	if (mode != FINISH_SameHub)
	{
		// Take away flight and keys (and anything else with IF_INTERHUBSTRIP set)
		item = p->mo->Inventory;
		while (item != NULL)
		{
			next = item->Inventory;
			if (item->ItemFlags & IF_INTERHUBSTRIP)
			{
				item->Destroy ();
			}
			item = next;
		}
	}

	if (mode == FINISH_NoHub && !(level.flags2 & LEVEL2_KEEPFULLINVENTORY))
	{ // Reduce all owned (visible) inventory to 1 item each
		for (item = p->mo->Inventory; item != NULL; item = item->Inventory)
		{
			// There may be depletable items with an amount of 0.
			// Those need to stay at 0; the rest get dropped to 1.
			if (item->ItemFlags & IF_INVBAR && item->Amount > 1)
			{
				item->Amount = 1;
			}
		}
	}

	if (p->morphTics)
	{ // Undo morph
		P_UndoPlayerMorph (p, p, true);
	}

	// Clears the entire inventory and gives back the defaults for starting a game
	if (resetinventory)
	{
		AInventory *inv = p->mo->Inventory;

		while (inv != NULL)
		{
			AInventory *next = inv->Inventory;
			if (!(inv->ItemFlags & IF_UNDROPPABLE))
			{
				inv->Destroy ();
			}
			else if (inv->GetClass() == RUNTIME_CLASS(AHexenArmor))
			{
				AHexenArmor *harmor = static_cast<AHexenArmor *> (inv);
				harmor->Slots[3] = harmor->Slots[2] = harmor->Slots[1] = harmor->Slots[0] = 0;
			}
			inv = next;
		}
		p->ReadyWeapon = NULL;
		p->PendingWeapon = WP_NOCHANGE;
		p->psprites[ps_weapon].state = NULL;
		p->psprites[ps_flash].state = NULL;
		p->mo->GiveDefaultInventory();
	}
}


//
// G_PlayerReborn
// Called after a player dies
// almost everything is cleared and initialized
//
void G_PlayerReborn (int player)
{
	player_t*	p;
	int 		frags[MAXPLAYERS];
	int			fragcount;	// [RH] Cumulative frags
	int 		killcount;
	int 		itemcount;
	int 		secretcount;
	int			chasecam;
	BYTE		currclass;
	userinfo_t  userinfo;	// [RH] Save userinfo
	botskill_t  b_skill;//Added by MC:
	APlayerPawn *actor;
	const PClass *cls;
	FString		log;

	p = &players[player];

	memcpy (frags, p->frags, sizeof(frags));
	fragcount = p->fragcount;
	killcount = p->killcount;
	itemcount = p->itemcount;
	secretcount = p->secretcount;
	currclass = p->CurrentPlayerClass;
    b_skill = p->skill;    //Added by MC:
	memcpy (&userinfo, &p->userinfo, sizeof(userinfo));
	actor = p->mo;
	cls = p->cls;
	log = p->LogText;
	chasecam = p->cheats & CF_CHASECAM;

	// Reset player structure to its defaults
	p->~player_t();
	::new(p) player_t;

	memcpy (p->frags, frags, sizeof(p->frags));
	p->fragcount = fragcount;
	p->killcount = killcount;
	p->itemcount = itemcount;
	p->secretcount = secretcount;
	p->CurrentPlayerClass = currclass;
	memcpy (&p->userinfo, &userinfo, sizeof(userinfo));
	p->mo = actor;
	p->cls = cls;
	p->LogText = log;
	p->cheats |= chasecam;

    p->skill = b_skill;	//Added by MC:

	p->oldbuttons = ~0, p->attackdown = true;	// don't do anything immediately
	p->original_oldbuttons = ~0;
	p->playerstate = PST_LIVE;

	if (gamestate != GS_TITLELEVEL)
	{
		actor->GiveDefaultInventory ();
		p->ReadyWeapon = p->PendingWeapon;
	}

    //Added by MC: Init bot structure.
    if (bglobal.botingame[player])
        bglobal.CleanBotstuff (p);
    else
		p->isbot = false;
}

//
// G_CheckSpot	
// Returns false if the player cannot be respawned
// at the given mapthing spot  
// because something is occupying it 
//

bool G_CheckSpot (int playernum, FMapThing *mthing)
{
	fixed_t x;
	fixed_t y;
	fixed_t z, oldz;
	int i;

	x = mthing->x;
	y = mthing->y;
	z = mthing->z;

	z += P_PointInSector (x, y)->floorplane.ZatPoint (x, y);

	if (!players[playernum].mo)
	{ // first spawn of level, before corpses
		for (i = 0; i < playernum; i++)
			if (players[i].mo && players[i].mo->x == x && players[i].mo->y == y)
				return false;
		return true;
	}

	oldz = players[playernum].mo->z;	// [RH] Need to save corpse's z-height
	players[playernum].mo->z = z;		// [RH] Checks are now full 3-D

	// killough 4/2/98: fix bug where P_CheckPosition() uses a non-solid
	// corpse to detect collisions with other players in DM starts
	//
	// Old code:
	// if (!P_CheckPosition (players[playernum].mo, x, y))
	//    return false;

	players[playernum].mo->flags |=  MF_SOLID;
	i = P_CheckPosition(players[playernum].mo, x, y);
	players[playernum].mo->flags &= ~MF_SOLID;
	players[playernum].mo->z = oldz;	// [RH] Restore corpse's height
	if (!i)
		return false;

	return true;
}


//
// G_DeathMatchSpawnPlayer 
// Spawns a player at one of the random death match spots 
// called at level load and each death 
//

// [RH] Returns the distance of the closest player to the given mapthing
static fixed_t PlayersRangeFromSpot (FMapThing *spot)
{
	fixed_t closest = INT_MAX;
	fixed_t distance;
	int i;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || !players[i].mo || players[i].health <= 0)
			continue;

		distance = P_AproxDistance (players[i].mo->x - spot->x,
									players[i].mo->y - spot->y);

		if (distance < closest)
			closest = distance;
	}

	return closest;
}

// [RH] Select the deathmatch spawn spot farthest from everyone.
static FMapThing *SelectFarthestDeathmatchSpot (size_t selections)
{
	fixed_t bestdistance = 0;
	FMapThing *bestspot = NULL;
	unsigned int i;

	for (i = 0; i < selections; i++)
	{
		fixed_t distance = PlayersRangeFromSpot (&deathmatchstarts[i]);

		if (distance > bestdistance)
		{
			bestdistance = distance;
			bestspot = &deathmatchstarts[i];
		}
	}

	return bestspot;
}

// [RH] Select a deathmatch spawn spot at random (original mechanism)
static FMapThing *SelectRandomDeathmatchSpot (int playernum, unsigned int selections)
{
	unsigned int i, j;

	for (j = 0; j < 20; j++)
	{
		i = pr_dmspawn() % selections;
		if (G_CheckSpot (playernum, &deathmatchstarts[i]) )
		{
			return &deathmatchstarts[i];
		}
	}

	// [RH] return a spot anyway, since we allow telefragging when a player spawns
	return &deathmatchstarts[i];
}

void G_DeathMatchSpawnPlayer (int playernum)
{
	unsigned int selections;
	FMapThing *spot;

	selections = deathmatchstarts.Size ();
	// [RH] We can get by with just 1 deathmatch start
	if (selections < 1)
		I_Error ("No deathmatch starts");

	// At level start, none of the players have mobjs attached to them,
	// so we always use the random deathmatch spawn. During the game,
	// though, we use whatever dmflags specifies.
	if ((dmflags & DF_SPAWN_FARTHEST) && players[playernum].mo)
		spot = SelectFarthestDeathmatchSpot (selections);
	else
		spot = SelectRandomDeathmatchSpot (playernum, selections);

	if (!spot)
	{ // no good spot, so the player will probably get stuck
		spot = &playerstarts[playernum];
	}
	else
	{
		if (playernum < 4)
			spot->type = playernum+1;
		else if (gameinfo.gametype != GAME_Hexen)
			spot->type = playernum+4001-4;	// [RH] > 4 players
		else
			spot->type = playernum+9100-4;
	}

	AActor *mo = P_SpawnPlayer (spot);
	if (mo != NULL) P_PlayerStartStomp(mo);
}

//
// G_QueueBody
//
static void G_QueueBody (AActor *body)
{
	// flush an old corpse if needed
	int modslot = bodyqueslot%BODYQUESIZE;

	if (bodyqueslot >= BODYQUESIZE && bodyque[modslot] != NULL)
	{
		bodyque[modslot]->Destroy ();
	}
	bodyque[modslot] = body;

	// Copy the player's translation, so that if they change their color later, only
	// their current body will change and not all their old corpses.
	if (GetTranslationType(body->Translation) == TRANSLATION_Players ||
		GetTranslationType(body->Translation) == TRANSLATION_PlayersExtra)
	{
		*translationtables[TRANSLATION_PlayerCorpses][modslot] = *TranslationToTable(body->Translation);
		body->Translation = TRANSLATION(TRANSLATION_PlayerCorpses,modslot);
	}

	bodyqueslot++;
}

//
// G_DoReborn
//
void G_DoReborn (int playernum, bool freshbot)
{
	if (!multiplayer && !(level.flags2 & LEVEL2_ALLOWRESPAWN))
	{
		if (BackupSaveName.Len() > 0 && FileExists (BackupSaveName.GetChars()))
		{ // Load game from the last point it was saved
			savename = BackupSaveName;
			gameaction = ga_autoloadgame;
		}
		else
		{ // Reload the level from scratch
			bool indemo = demoplayback;
			BackupSaveName = "";
			G_InitNew (level.mapname, false);
			demoplayback = indemo;
//			gameaction = ga_loadlevel;
		}
	}
	else
	{
		// respawn at the start
		int i;

		// first disassociate the corpse
		if (players[playernum].mo)
		{
			G_QueueBody (players[playernum].mo);
			players[playernum].mo->player = NULL;
		}

		// spawn at random spot if in death match
		if (deathmatch)
		{
			G_DeathMatchSpawnPlayer (playernum);
			return;
		}

		if (G_CheckSpot (playernum, &playerstarts[playernum]) )
		{
			AActor *mo = P_SpawnPlayer (&playerstarts[playernum]);
			if (mo != NULL) P_PlayerStartStomp(mo);
		}
		else
		{
			// try to spawn at one of the other players' spots
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (G_CheckSpot (playernum, &playerstarts[i]) )
				{
					int oldtype = playerstarts[i].type;

					// fake as other player
					// [RH] These numbers should be common across all games. Or better yet, not
					// used at all outside P_SpawnMapThing().
					if (playernum < 4 || gameinfo.gametype == GAME_Strife)
					{
						playerstarts[i].type = playernum + 1;
					}
					else if (gameinfo.gametype == GAME_Hexen)
					{
						playerstarts[i].type = playernum + 9100 - 4;
					}
					else
					{
						playerstarts[i].type = playernum + 4001 - 4;
					}
					AActor *mo = P_SpawnPlayer (&playerstarts[i]);
					if (mo != NULL) P_PlayerStartStomp(mo);
					playerstarts[i].type = oldtype; 			// restore 
					return;
				}
				// he's going to be inside something.  Too bad.
			}
			AActor *mo = P_SpawnPlayer (&playerstarts[playernum]);
			if (mo != NULL) P_PlayerStartStomp(mo);
		}
	}
}

void G_ScreenShot (char *filename)
{
	shotfile = filename;
	gameaction = ga_screenshot;
}





//
// G_InitFromSavegame
// Can be called by the startup code or the menu task.
//
void G_LoadGame (const char* name)
{
	if (name != NULL)
	{
		savename = name;
		gameaction = ga_loadgame;
	}
}

static bool CheckSingleWad (char *name, bool &printRequires, bool printwarn)
{
	if (name == NULL)
	{
		return true;
	}
	if (Wads.CheckIfWadLoaded (name) < 0)
	{
		if (printwarn)
		{
			if (!printRequires)
			{
				Printf ("This savegame needs these wads:\n%s", name);
			}
			else
			{
				Printf (", %s", name);
			}
		}
		printRequires = true;
		delete[] name;
		return false;
	}
	delete[] name;
	return true;
}

// Return false if not all the needed wads have been loaded.
bool G_CheckSaveGameWads (PNGHandle *png, bool printwarn)
{
	char *text;
	bool printRequires = false;

	text = M_GetPNGText (png, "Game WAD");
	CheckSingleWad (text, printRequires, printwarn);
	text = M_GetPNGText (png, "Map WAD");
	CheckSingleWad (text, printRequires, printwarn);

	if (printRequires)
	{
		if (printwarn)
		{
			Printf ("\n");
		}
		return false;
	}

	return true;
}


void G_DoLoadGame ()
{
	char sigcheck[20];
	char *text = NULL;
	char *map;

	if (gameaction != ga_autoloadgame)
	{
		demoplayback = false;
	}
	gameaction = ga_nothing;

	FILE *stdfile = fopen (savename.GetChars(), "rb");
	if (stdfile == NULL)
	{
		Printf ("Could not read savegame '%s'\n", savename.GetChars());
		return;
	}

	PNGHandle *png = M_VerifyPNG (stdfile);
	if (png == NULL)
	{
		fclose (stdfile);
		Printf ("'%s' is not a valid (PNG) savegame\n", savename.GetChars());
		return;
	}

	SaveVersion = 0;

	// Check whether this savegame actually has been created by a compatible engine.
	// Since there are ZDoom derivates using the exact same savegame format but
	// with mutual incompatibilities this check simplifies things significantly.
	char *engine = M_GetPNGText (png, "Engine");
	if (engine == NULL || 0 != strcmp (engine, GAMESIG))
	{
		// Make a special case for the message printed for old savegames that don't
		// have this information.
		if (engine == NULL)
		{
			Printf ("Savegame is from an incompatible version\n");
		}
		else
		{
			Printf ("Savegame is from another ZDoom-based engine: %s\n", engine);
			delete[] engine;
		}
		delete png;
		fclose (stdfile);
		return;
	}
	if (engine != NULL)
	{
		delete[] engine;
	}

	if (!M_GetPNGText (png, "ZDoom Save Version", sigcheck, 20) ||
		0 != strncmp (sigcheck, SAVESIG, 9) ||		// ZDOOMSAVE is the first 9 chars
		(SaveVersion = atoi (sigcheck+9)) < MINSAVEVER)
	{
		Printf ("Savegame is from an incompatible version\n");
		delete png;
		fclose (stdfile);
		return;
	}

	if (!G_CheckSaveGameWads (png, true))
	{
		fclose (stdfile);
		return;
	}

	map = M_GetPNGText (png, "Current Map");
	if (map == NULL)
	{
		Printf ("Savegame is missing the current map\n");
		fclose (stdfile);
		return;
	}

	// Read intermission data for hubs
	G_ReadHubInfo(png);

	bglobal.RemoveAllBots (true);

	text = M_GetPNGText (png, "Important CVARs");
	if (text != NULL)
	{
		BYTE *vars_p = (BYTE *)text;
		C_ReadCVars (&vars_p);
		delete[] text;
	}

	// dearchive all the modifications
	if (M_FindPNGChunk (png, MAKE_ID('p','t','I','c')) == 8)
	{
		DWORD time[2];
		fread (&time, 8, 1, stdfile);
		time[0] = BigLong((unsigned int)time[0]);
		time[1] = BigLong((unsigned int)time[1]);
		level.time = Scale (time[1], TICRATE, time[0]);
	}
	else
	{ // No ptIc chunk so we don't know how long the user was playing
		level.time = 0;
	}

	G_ReadSnapshots (png);
	FRandom::StaticReadRNGState (png);
	P_ReadACSDefereds (png);

	// load a base level
	savegamerestore = true;		// Use the player actors in the savegame
	bool demoplaybacksave = demoplayback;
	G_InitNew (map, false);
	demoplayback = demoplaybacksave;
	delete[] map;
	savegamerestore = false;

	P_ReadACSVars(png);

	NextSkill = -1;
	if (M_FindPNGChunk (png, MAKE_ID('s','n','X','t')) == 1)
	{
		BYTE next;
		fread (&next, 1, 1, stdfile);
		NextSkill = next;
	}

	if (level.info->snapshot != NULL)
	{
		delete level.info->snapshot;
		level.info->snapshot = NULL;
	}

	BackupSaveName = savename;

	delete png;
	fclose (stdfile);

	// At this point, the GC threshold is likely a lot higher than the
	// amount of memory in use, so bring it down now by starting a
	// collection.
	GC::StartCollection();
}


//
// G_SaveGame
// Called by the menu task.
// Description is a 24 byte text string
//
void G_SaveGame (const char *filename, const char *description)
{
	if (sendsave || gameaction == ga_savegame)
	{
		Printf ("A game save is still pending.\n");
		return;
	}
	savegamefile = filename;
	strncpy (savedescription, description, sizeof(savedescription)-1);
	savedescription[sizeof(savedescription)-1] = '\0';
	sendsave = true;
}

FString G_BuildSaveName (const char *prefix, int slot)
{
	FString name;
	FString leader;
	const char *slash = "";

	leader = Args->CheckValue ("-savedir");
	if (leader.IsEmpty())
	{
#ifndef unix
		if (Args->CheckParm ("-cdrom"))
		{
			leader = CDROM_DIR "/";
		}
		else
#endif
		{
			leader = save_dir;
		}
	}
	size_t len = leader.Len();
	if (leader[0] != '\0' && leader[len-1] != '\\' && leader[len-1] != '/')
	{
		slash = "/";
	}
	name.Format("%s%s%s", leader.GetChars(), slash, prefix);
	if (slot >= 0)
	{
		name.AppendFormat("%d.zds", slot);
	}
#ifdef unix
	if (leader[0] == 0)
	{
		return GetUserFile (name);
	}
#endif
	return NicePath(name);
}

CVAR (Int, autosavenum, 0, CVAR_NOSET|CVAR_ARCHIVE|CVAR_GLOBALCONFIG)
CVAR (Int, disableautosave, 0, CVAR_ARCHIVE|CVAR_GLOBALCONFIG)
CUSTOM_CVAR (Int, autosavecount, 4, CVAR_ARCHIVE|CVAR_GLOBALCONFIG)
{
	if (self < 0)
		self = 0;
	if (self > 20)
		self = 20;
}

extern void P_CalcHeight (player_t *);

void G_DoAutoSave ()
{
	char description[SAVESTRINGSIZE];
	FString file;
	// Keep up to four autosaves at a time
	UCVarValue num;
	const char *readableTime;
	int count = autosavecount != 0 ? autosavecount : 1;
	
	num.Int = (autosavenum + 1) % count;
	autosavenum.ForceSet (num, CVAR_Int);

	file = G_BuildSaveName ("auto", num.Int);

	readableTime = myasctime ();
	strcpy (description, "Autosave ");
	strncpy (description+9, readableTime+4, 12);
	description[9+12] = 0;

	G_DoSaveGame (false, file, description);
}


static void PutSaveWads (FILE *file)
{
	const char *name;

	// Name of IWAD
	name = Wads.GetWadName (FWadCollection::IWAD_FILENUM);
	M_AppendPNGText (file, "Game WAD", name);

	// Name of wad the map resides in
	if (Wads.GetLumpFile (level.lumpnum) > 1)
	{
		name = Wads.GetWadName (Wads.GetLumpFile (level.lumpnum));
		M_AppendPNGText (file, "Map WAD", name);
	}
}

static void PutSaveComment (FILE *file)
{
	char comment[256];
	const char *readableTime;
	WORD len;
	int levelTime;

	// Get the current date and time
	readableTime = myasctime ();

	strncpy (comment, readableTime, 10);
	strncpy (comment+10, readableTime+19, 5);
	strncpy (comment+15, readableTime+10, 9);
	comment[24] = 0;

	M_AppendPNGText (file, "Creation Time", comment);

	// Get level name
	//strcpy (comment, level.level_name);
	mysnprintf(comment, countof(comment), "%s - %s", level.mapname, level.LevelName.GetChars());
	len = (WORD)strlen (comment);
	comment[len] = '\n';

	// Append elapsed time
	levelTime = level.time / TICRATE;
	mysnprintf (comment + len + 1, countof(comment) - len - 1, "time: %02d:%02d:%02d",
		levelTime/3600, (levelTime%3600)/60, levelTime%60);
	comment[len+16] = 0;

	// Write out the comment
	M_AppendPNGText (file, "Comment", comment);
}

static void PutSavePic (FILE *file, int width, int height)
{
	if (width <= 0 || height <= 0 || !storesavepic)
	{
		M_CreateDummyPNG (file);
	}
	else
	{
		P_CheckPlayerSprites();
		screen->WriteSavePic(&players[consoleplayer], file, width, height);
	}
}

void G_DoSaveGame (bool okForQuicksave, FString filename, const char *description)
{
	// Do not even try, if we're not in a level. (Can happen after
	// a demo finishes playback.)
	if (lines == NULL || sectors == NULL)
	{
		return;
	}

	if (demoplayback)
	{
		filename = G_BuildSaveName ("demosave.zds", -1);
	}

	insave = true;
	G_SnapshotLevel ();

	FILE *stdfile = fopen (filename.GetChars(), "wb");

	if (stdfile == NULL)
	{
		Printf ("Could not create savegame '%s'\n", filename.GetChars());
		insave = false;
		return;
	}

	SaveVersion = SAVEVER;
	PutSavePic (stdfile, SAVEPICWIDTH, SAVEPICHEIGHT);
	M_AppendPNGText (stdfile, "Software", "ZDoom " DOTVERSIONSTR);
	M_AppendPNGText (stdfile, "Engine", GAMESIG);
	M_AppendPNGText (stdfile, "ZDoom Save Version", SAVESIG);
	M_AppendPNGText (stdfile, "Title", description);
	M_AppendPNGText (stdfile, "Current Map", level.mapname);
	PutSaveWads (stdfile);
	PutSaveComment (stdfile);

	// Intermission stats for hubs
	G_WriteHubInfo(stdfile);

	{
		BYTE vars[4096], *vars_p;
		vars_p = vars;
		C_WriteCVars (&vars_p, CVAR_SERVERINFO);
		*vars_p = 0;
		M_AppendPNGText (stdfile, "Important CVARs", (char *)vars);
	}

	if (level.time != 0 || level.maptime != 0)
	{
		DWORD time[2] = { BigLong(TICRATE), BigLong(level.time) };
		M_AppendPNGChunk (stdfile, MAKE_ID('p','t','I','c'), (BYTE *)&time, 8);
	}

	G_WriteSnapshots (stdfile);
	FRandom::StaticWriteRNGState (stdfile);
	P_WriteACSDefereds (stdfile);

	P_WriteACSVars(stdfile);

	if (NextSkill != -1)
	{
		BYTE next = NextSkill;
		M_AppendPNGChunk (stdfile, MAKE_ID('s','n','X','t'), &next, 1);
	}

	M_NotifyNewSave (filename.GetChars(), description, okForQuicksave);

	M_FinishPNG (stdfile);
	fclose (stdfile);

	// Check whether the file is ok.
	bool success = false;
	stdfile = fopen (filename.GetChars(), "rb");
	if (stdfile != NULL)
	{
		PNGHandle *pngh = M_VerifyPNG(stdfile);
		if (pngh != NULL)
		{
			success = true;
			delete pngh;
		}
		fclose(stdfile);
	}
	if (success) 
	{
		if (longsavemessages) Printf ("%s (%s)\n", GStrings("GGSAVED"), filename.GetChars());
		else Printf ("%s\n", GStrings("GGSAVED"));
	}
	else Printf(PRINT_HIGH, "Save failed\n");

	BackupSaveName = filename;

	// We don't need the snapshot any longer.
	if (level.info->snapshot != NULL)
	{
		delete level.info->snapshot;
		level.info->snapshot = NULL;
	}
		
	insave = false;
}




//
// DEMO RECORDING
//

void G_ReadDemoTiccmd (ticcmd_t *cmd, int player)
{
	int id = DEM_BAD;

	while (id != DEM_USERCMD && id != DEM_EMPTYUSERCMD)
	{
		if (!demorecording && demo_p >= zdembodyend)
		{
			// nothing left in the BODY chunk, so end playback.
			G_CheckDemoStatus ();
			break;
		}

		id = ReadByte (&demo_p);

		switch (id)
		{
		case DEM_STOP:
			// end of demo stream
			G_CheckDemoStatus ();
			break;

		case DEM_USERCMD:
			UnpackUserCmd (&cmd->ucmd, &cmd->ucmd, &demo_p);
			break;

		case DEM_EMPTYUSERCMD:
			// leave cmd->ucmd unchanged
			break;

		case DEM_DROPPLAYER:
			{
				BYTE i = ReadByte (&demo_p);
				if (i < MAXPLAYERS)
				{
					playeringame[i] = false;
				}
			}
			break;

		default:
			Net_DoCommand (id, &demo_p, player);
			break;
		}
	}
} 

bool stoprecording;

CCMD (stop)
{
	stoprecording = true;
}

extern BYTE *lenspot;

void G_WriteDemoTiccmd (ticcmd_t *cmd, int player, int buf)
{
	BYTE *specdata;
	int speclen;

	if (stoprecording)
	{ // use "stop" console command to end demo recording
		G_CheckDemoStatus ();
		if (!netgame)
		{
			gameaction = ga_fullconsole;
		}
		return;
	}

	// [RH] Write any special "ticcmds" for this player to the demo
	if ((specdata = NetSpecs[player][buf].GetData (&speclen)) && gametic % ticdup == 0)
	{
		memcpy (demo_p, specdata, speclen);
		demo_p += speclen;
		NetSpecs[player][buf].SetData (NULL, 0);
	}

	// [RH] Now write out a "normal" ticcmd.
	WriteUserCmdMessage (&cmd->ucmd, &players[player].cmd.ucmd, &demo_p);

	// [RH] Bigger safety margin
	if (demo_p > demobuffer + maxdemosize - 64)
	{
		ptrdiff_t pos = demo_p - demobuffer;
		ptrdiff_t spot = lenspot - demobuffer;
		ptrdiff_t comp = democompspot - demobuffer;
		ptrdiff_t body = demobodyspot - demobuffer;
		// [RH] Allocate more space for the demo
		maxdemosize += 0x20000;
		demobuffer = (BYTE *)M_Realloc (demobuffer, maxdemosize);
		demo_p = demobuffer + pos;
		lenspot = demobuffer + spot;
		democompspot = demobuffer + comp;
		demobodyspot = demobuffer + body;
	}
}



//
// G_RecordDemo
//
void G_RecordDemo (char* name)
{
	char *v;

	usergame = false;
	strcpy (demoname, name);
	FixPathSeperator (demoname);
	DefaultExtension (demoname, ".lmp");
	v = Args->CheckValue ("-maxdemo");
	maxdemosize = 0x20000;
	demobuffer = (BYTE *)M_Malloc (maxdemosize);

	demorecording = true; 
}


// [RH] Demos are now saved as IFF FORMs. I've also removed support
//		for earlier ZDEMs since I didn't want to bother supporting
//		something that probably wasn't used much (if at all).

void G_BeginRecording (const char *startmap)
{
	int i;

	if (startmap == NULL)
	{
		startmap = level.mapname;
	}
	demo_p = demobuffer;

	WriteLong (FORM_ID, &demo_p);			// Write FORM ID
	demo_p += 4;							// Leave space for len
	WriteLong (ZDEM_ID, &demo_p);			// Write ZDEM ID

	// Write header chunk
	StartChunk (ZDHD_ID, &demo_p);
	WriteWord (DEMOGAMEVERSION, &demo_p);	// Write ZDoom version
	*demo_p++ = 2;							// Write minimum version needed to use this demo.
	*demo_p++ = 3;							// (Useful?)
	for (i = 0; i < 8; i++)					// Write name of map demo was recorded on.
	{
		*demo_p++ = startmap[i];
	}
	WriteLong (rngseed, &demo_p);			// Write RNG seed
	*demo_p++ = consoleplayer;
	FinishChunk (&demo_p);

	// Write player info chunks
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i])
		{
			StartChunk (UINF_ID, &demo_p);
			WriteByte ((BYTE)i, &demo_p);
			D_WriteUserInfoStrings (i, &demo_p);
			FinishChunk (&demo_p);
		}
	}

	// It is possible to start a "multiplayer" game with only one player,
	// so checking the number of players when playing back the demo is not
	// enough.
	if (multiplayer)
	{
		StartChunk (NETD_ID, &demo_p);
		FinishChunk (&demo_p);
	}

	// Write cvars chunk
	StartChunk (VARS_ID, &demo_p);
	C_WriteCVars (&demo_p, CVAR_SERVERINFO|CVAR_DEMOSAVE);
	FinishChunk (&demo_p);

	// Write weapon ordering chunk
	StartChunk (WEAP_ID, &demo_p);
	P_WriteDemoWeaponsChunk(&demo_p);
	FinishChunk (&demo_p);

	// Indicate body is compressed
	StartChunk (COMP_ID, &demo_p);
	democompspot = demo_p;
	WriteLong (0, &demo_p);
	FinishChunk (&demo_p);

	// Begin BODY chunk
	StartChunk (BODY_ID, &demo_p);
	demobodyspot = demo_p;
}


//
// G_PlayDemo
//

FString defdemoname;

void G_DeferedPlayDemo (char *name)
{
	defdemoname = name;
	gameaction = ga_playdemo;
}

CCMD (playdemo)
{
	if (argv.argc() > 1)
	{
		G_DeferedPlayDemo (argv[1]);
		singledemo = true;
	}
}

CCMD (timedemo)
{
	if (argv.argc() > 1)
	{
		G_TimeDemo (argv[1]);
		singledemo = true;
	}
}

// [RH] Process all the information in a FORM ZDEM
//		until a BODY chunk is entered.
bool G_ProcessIFFDemo (char *mapname)
{
	bool headerHit = false;
	bool bodyHit = false;
	int numPlayers = 0;
	int id, len, i;
	uLong uncompSize = 0;
	BYTE *nextchunk;

	demoplayback = true;

	for (i = 0; i < MAXPLAYERS; i++)
		playeringame[i] = 0;

	len = ReadLong (&demo_p);
	zdemformend = demo_p + len + (len & 1);

	// Check to make sure this is a ZDEM chunk file.
	// TODO: Support multiple FORM ZDEMs in a CAT. Might be useful.

	id = ReadLong (&demo_p);
	if (id != ZDEM_ID)
	{
		Printf ("Not a ZDoom demo file!\n");
		return true;
	}

	// Process all chunks until a BODY chunk is encountered.

	while (demo_p < zdemformend && !bodyHit)
	{
		id = ReadLong (&demo_p);
		len = ReadLong (&demo_p);
		nextchunk = demo_p + len + (len & 1);
		if (nextchunk > zdemformend)
		{
			Printf ("Demo is mangled!\n");
			return true;
		}

		switch (id)
		{
		case ZDHD_ID:
			headerHit = true;

			demover = ReadWord (&demo_p);	// ZDoom version demo was created with
			if (demover < MINDEMOVERSION)
			{
				Printf ("Demo requires an older version of ZDoom!\n");
				//return true;
			}
			if (ReadWord (&demo_p) > DEMOGAMEVERSION)	// Minimum ZDoom version
			{
				Printf ("Demo requires a newer version of ZDoom!\n");
				return true;
			}
			memcpy (mapname, demo_p, 8);	// Read map name
			mapname[8] = 0;
			demo_p += 8;
			rngseed = ReadLong (&demo_p);
			FRandom::StaticClearRandom ();
			consoleplayer = *demo_p++;
			break;

		case VARS_ID:
			C_ReadCVars (&demo_p);
			break;

		case UINF_ID:
			i = ReadByte (&demo_p);
			if (!playeringame[i])
			{
				playeringame[i] = 1;
				numPlayers++;
			}
			D_ReadUserInfoStrings (i, &demo_p, false);
			break;

		case NETD_ID:
			multiplayer = true;
			break;

		case WEAP_ID:
			P_ReadDemoWeaponsChunk(&demo_p);
			break;

		case BODY_ID:
			bodyHit = true;
			zdembodyend = demo_p + len;
			break;

		case COMP_ID:
			uncompSize = ReadLong (&demo_p);
			break;
		}

		if (!bodyHit)
			demo_p = nextchunk;
	}

	if (!numPlayers)
	{
		Printf ("Demo has no players!\n");
		return true;
	}

	if (!bodyHit)
	{
		zdembodyend = NULL;
		Printf ("Demo has no BODY chunk!\n");
		return true;
	}

	if (numPlayers > 1)
		multiplayer = netgame = netdemo = true;

	if (uncompSize > 0)
	{
		BYTE *uncompressed = new BYTE[uncompSize];
		int r = uncompress (uncompressed, &uncompSize, demo_p, uLong(zdembodyend - demo_p));
		if (r != Z_OK)
		{
			Printf ("Could not decompress demo!\n");
			delete[] uncompressed;
			return true;
		}
		M_Free (demobuffer);
		zdembodyend = uncompressed + uncompSize;
		demobuffer = demo_p = uncompressed;
	}

	return false;
}

void G_DoPlayDemo (void)
{
	char mapname[9];
	int demolump;

	gameaction = ga_nothing;

	// [RH] Allow for demos not loaded as lumps
	demolump = Wads.CheckNumForFullName (defdemoname, true);
	if (demolump >= 0)
	{
		int demolen = Wads.LumpLength (demolump);
		demobuffer = (BYTE *)M_Malloc(demolen);
		Wads.ReadLump (demolump, demobuffer);
	}
	else
	{
		FixPathSeperator (defdemoname);
		DefaultExtension (defdemoname, ".lmp");
		M_ReadFile (defdemoname, &demobuffer);
	}
	demo_p = demobuffer;

	Printf ("Playing demo %s\n", defdemoname.GetChars());

	C_BackupCVars ();		// [RH] Save cvars that might be affected by demo

	if (ReadLong (&demo_p) != FORM_ID)
	{
		const char *eek = "Cannot play non-ZDoom demos.\n(They would go out of sync badly.)\n";

		C_ForgetCVars();
		M_Free(demobuffer);
		demo_p = demobuffer = NULL;
		if (singledemo)
		{
			I_Error ("%s", eek);
		}
		else
		{
			Printf (PRINT_BOLD, "%s", eek);
			gameaction = ga_nothing;
		}
	}
	else if (G_ProcessIFFDemo (mapname))
	{
		C_RestoreCVars();
		gameaction = ga_nothing;
		demoplayback = false;
	}
	else
	{
		// don't spend a lot of time in loadlevel 
		precache = false;
		demonew = true;
		G_InitNew (mapname, false);
		C_HideConsole ();
		demonew = false;
		precache = true;

		usergame = false;
		demoplayback = true;
	}
}

//
// G_TimeDemo
//
void G_TimeDemo (char* name)
{
	nodrawers = !!Args->CheckParm ("-nodraw");
	noblit = !!Args->CheckParm ("-noblit");
	timingdemo = true;
	singletics = true;

	defdemoname = name;
	gameaction = ga_playdemo;
}


/*
===================
=
= G_CheckDemoStatus
=
= Called after a death or level completion to allow demos to be cleaned up
= Returns true if a new demo loop action will take place
===================
*/

bool G_CheckDemoStatus (void)
{
	if (!demorecording)
	{ // [RH] Restore the player's userinfo settings.
		D_SetupUserInfo();
	}

	if (demoplayback)
	{
		extern int starttime;
		int endtime = 0;

		if (timingdemo)
			endtime = I_GetTime (false) - starttime;

		C_RestoreCVars ();		// [RH] Restore cvars demo might have changed
		M_Free (demobuffer);

		P_SetupWeapons_ntohton();
		demoplayback = false;
		netdemo = false;
		netgame = false;
		multiplayer = false;
		singletics = false;
		for (int i = 1; i < MAXPLAYERS; i++)
			playeringame[i] = 0;
		consoleplayer = 0;
		players[0].camera = NULL;
		StatusBar->AttachToPlayer (&players[0]);

		if (singledemo || timingdemo)
		{
			if (timingdemo)
			{
				// Trying to get back to a stable state after timing a demo
				// seems to cause problems. I don't feel like fixing that
				// right now.
				I_FatalError ("timed %i gametics in %i realtics (%.1f fps)\n"
							  "(This is not really an error.)", gametic,
							  endtime, (float)gametic/(float)endtime*(float)TICRATE);
			}
			else
			{
				Printf ("Demo ended.\n");
			}
			gameaction = ga_fullconsole;
			timingdemo = false;
			return false;
		}
		else
		{
			D_AdvanceDemo (); 
		}

		return true; 
	}

	if (demorecording)
	{
		BYTE *formlen;

		WriteByte (DEM_STOP, &demo_p);

		if (demo_compress)
		{
			// Now that the entire BODY chunk has been created, replace it with
			// a compressed version. If the BODY successfully compresses, the
			// contents of the COMP chunk will be changed to indicate the
			// uncompressed size of the BODY.
			uLong len = uLong(demo_p - demobodyspot);
			uLong outlen = (len + len/100 + 12);
			Byte *compressed = new Byte[outlen];
			int r = compress2 (compressed, &outlen, demobodyspot, len, 9);
			if (r == Z_OK && outlen < len)
			{
				formlen = democompspot;
				WriteLong (len, &democompspot);
				memcpy (demobodyspot, compressed, outlen);
				demo_p = demobodyspot + outlen;
			}
			delete[] compressed;
		}
		FinishChunk (&demo_p);
		formlen = demobuffer + 4;
		WriteLong (int(demo_p - demobuffer - 8), &formlen);

		M_WriteFile (demoname, demobuffer, int(demo_p - demobuffer)); 
		M_Free (demobuffer); 
		demorecording = false;
		stoprecording = false;
		Printf ("Demo %s recorded\n", demoname); 
	}

	return false; 
}
