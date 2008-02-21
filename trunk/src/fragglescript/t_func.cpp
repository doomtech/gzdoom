// Emacs style mode select -*- C++ -*-
//---------------------------------------------------------------------------
//
// Copyright(C) 2000 Simon Howard
// Copyright(C) 2005 Christoph Oelckers
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//--------------------------------------------------------------------------
//
// Functions
//
// functions are stored as variables(see variable.c), the
// value being a pointer to a 'handler' function for the
// function. Arguments are stored in an argc/argv-style list
//
// this module contains all the handler functions for the
// basic FraggleScript Functions.
//
// By Simon Howard
//
//---------------------------------------------------------------------------

/*
FraggleScript is from SMMU which is under the GPL. Technically, therefore, 
combining the FraggleScript code with the non-free ZDoom code is a violation of the GPL.

As this may be a problem for you, I hereby grant an exception to my copyright on the 
SMMU source (including FraggleScript). You may use any code from SMMU in GZDoom, provided that:

    * For any binary release of the port, the source code is also made available.
    * The copyright notice is kept on any file containing my code.
*/

#include "templates.h"
#include "p_local.h"
#include "t_script.h"
#include "s_sound.h"
#include "p_lnspec.h"
#include "m_random.h"
#include "c_console.h"
#include "c_dispatch.h"
#include "d_player.h"
#include "a_doomglobal.h"
#include "w_wad.h"
#include "gi.h"
#include "t_util.h"

#include "gl/gl_data.h"

static FRandom pr_script("FScript");

#define AngleToFixed(x)  ((((double) x) / ((double) ANG45/45)) * FRACUNIT)
#define FixedToAngle(x)  ((((double) x) / FRACUNIT) * ANG45/45)

// Disables Legacy-incompatible bug fixes.
CVAR(Bool, fs_forcecompatible, false, CVAR_ARCHIVE|CVAR_SERVERINFO)

// functions. SF_ means Script Function not, well.. heh, me

/////////// actually running a function /////////////

//==========================================================================

//FUNCTIONS

// the actual handler functions for the
// functions themselves

// arguments are evaluated and passed to the
// handler functions using 't_argc' and 't_argv'
// in a similar way to the way C does with command
// line options.

// values can be returned from the functions using
// the variable 't_return'
//
//==========================================================================

//==========================================================================
//
// Creates a string out of a print argument list. This version does not
// have any length restrictions like the original FS versions had.
//
//==========================================================================
FString DFraggleThinker::GetFormatString(int startarg)
{
	FString fmt="";
	for(int i=startarg; i<t_argc; i++) fmt += stringvalue(t_argv[i]);
	return fmt;
}

//==========================================================================
//
//
//
//==========================================================================
bool DFraggleThinker::CheckArgs(int cnt)
{
	if (t_argc<cnt)
	{
		script_error("Insufficient parameters for '%s'\n", t_func);
		return false;
	}
	return true;
}


//==========================================================================
//
// prints some text to the console and the notify buffer
//
//==========================================================================
void DFraggleThinker::SF_Print()
{
	Printf(PRINT_HIGH, "%s\n", GetFormatString(0).GetChars());
}


//==========================================================================
//
// return a random number from 0 to 255
//
//==========================================================================
void DFraggleThinker::SF_Rnd()
{
	t_return.type = svt_int;
	t_return.value.i = pr_script();
}

//==========================================================================
//
// "continue;" in FraggleScript is a function
//
//==========================================================================

void DFraggleThinker::SF_Continue()
{
	section_t *section;
	
	if(!(section = looping_section()) )       // no loop found
    {
		script_error("continue() not in loop\n");
		return;
    }
	
	rover = section->end;      // jump to the closing brace
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_Break()
{
	section_t *section;
	
	if(!(section = looping_section()) )
    {
		script_error("break() not in loop\n");
		return;
    }
	
	rover = section->end+1;   // jump out of the loop
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_Goto()
{
	if (CheckArgs(1))
	{
		// check argument is a labelptr
		
		if(t_argv[0].type != svt_label)
		{
			script_error("goto argument not a label\n");
			return;
		}
		
		// go there then if everythings fine
		rover = t_argv[0].value.labelptr;
	}	
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_Return()
{
	killscript = true;      // kill the script
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_Include()
{
	char tempstr[12];
	
	if (CheckArgs(1))
	{
		if(t_argv[0].type == svt_string)
		{
			strncpy(tempstr, t_argv[0].value.s, 8);
			tempstr[8]=0;
		}
		else
			sprintf(tempstr, "%i", (int)t_argv[0].value.i);
		
		parse_include(tempstr);
	}
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_Input()
{
	Printf(PRINT_BOLD,"input() function not available in doom\n");
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_Beep()
{
	S_Sound(CHAN_AUTO, "misc/chat", 1.0f, ATTN_IDLE);
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_Clock()
{
	t_return.type = svt_int;
	t_return.value.i = (gametic*100)/TICRATE;
}

/**************** doom stuff ****************/

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_ExitLevel()
{
	G_ExitLevel(0, false);
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_Tip()
{
	if (t_argc>0 && current_script->trigger &&
		current_script->trigger->CheckLocalView(consoleplayer)) 
	{
		C_MidPrint(GetFormatString(0).GetChars());
	}
}

//==========================================================================
//
// SF_TimedTip
//
// Implements: void timedtip(int clocks, ...)
//
//==========================================================================

EXTERN_CVAR(Float, con_midtime)

void DFraggleThinker::SF_TimedTip()
{
	if (CheckArgs(2))
	{
		float saved = con_midtime;
		con_midtime = intvalue(t_argv[0])/100.0f;
		C_MidPrint(GetFormatString(1).GetChars());
		con_midtime=saved;
	}
}


//==========================================================================
//
// tip to a particular player
//
//==========================================================================

void DFraggleThinker::SF_PlayerTip()
{
	if (CheckArgs(1))
	{
		int plnum = T_GetPlayerNum(t_argv[0]);
		if (plnum!=-1 && players[plnum].mo->CheckLocalView(consoleplayer)) 
		{
			C_MidPrint(GetFormatString(1).GetChars());
		}
	}
}

//==========================================================================
//
// message player
//
//==========================================================================

void DFraggleThinker::SF_Message()
{
	if (t_argc>0 && current_script->trigger &&
		current_script->trigger->CheckLocalView(consoleplayer))
	{
		Printf(PRINT_HIGH, "%s\n", GetFormatString(0).GetChars());
	}
}

//==========================================================================
//
// message to a particular player
//
//==========================================================================

void DFraggleThinker::SF_PlayerMsg()
{
	if (CheckArgs(1))
	{
		int plnum = T_GetPlayerNum(t_argv[0]);
		if (plnum!=-1 && players[plnum].mo->CheckLocalView(consoleplayer)) 
		{
			Printf(PRINT_HIGH, "%s\n", GetFormatString(1).GetChars());
		}
	}
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_PlayerInGame()
{
	if (CheckArgs(1))
	{
		int plnum = T_GetPlayerNum(t_argv[0]);

		t_return.type = svt_int;
		t_return.value.i = plnum!=-1;
	}
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_PlayerName()
{
	int plnum;
	
	if(!t_argc)
    {
		player_t *pl=NULL;
		if (current_script->trigger) pl = current_script->trigger->player;
		if(pl) plnum = pl - players;
		else plnum=-1;
    }
	else
		plnum = T_GetPlayerNum(t_argv[0]);
	
	if(plnum !=-1)
	{
		t_return.type = svt_string;
		t_return.value.s = players[plnum].userinfo.netname;
	}
	else
	{
		script_error("script not started by player\n");
	}
}

//==========================================================================
//
// object being controlled by player
//
//==========================================================================

void DFraggleThinker::SF_PlayerObj()
{
	int plnum;

	if(!t_argc)
	{
		player_t *pl=NULL;
		if (current_script->trigger) pl = current_script->trigger->player;
		if(pl) plnum = pl - players;
		else plnum=-1;
	}
	else
		plnum = T_GetPlayerNum(t_argv[0]);

	if(plnum !=-1)
	{
		t_return.type = svt_mobj;
		t_return.value.mobj = players[plnum].mo;
	}
	else
	{
		script_error("script not started by player\n");
	}
}

/*********** Mobj code ***************/

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_Player()
{
	AActor *mo = t_argc ? MobjForSvalue(t_argv[0]) : current_script->trigger;
	
	t_return.type = svt_int;
	
	if(mo && mo->player) // haleyjd: added mo->player
	{
		t_return.value.i = (int)(mo->player - players);
	}
	else
	{
		t_return.value.i = -1;
	}
}

//==========================================================================
//
// SF_Spawn
// 
// Implements: mobj spawn(int type, int x, int y, [int angle], [int z], [bool zrel])
//
//==========================================================================

void DFraggleThinker::SF_Spawn()
{
	int x, y, z;
	const PClass *PClass;
	angle_t angle = 0;
	
	if (CheckArgs(3))
	{
		if (!(PClass=T_GetMobjType(t_argv[0]))) return;
		
		x = fixedvalue(t_argv[1]);
		y = fixedvalue(t_argv[2]);

		if(t_argc >= 5)
		{
			z = fixedvalue(t_argv[4]);
			// [Graf Zahl] added option of spawning with a relative z coordinate
			if(t_argc > 5)
			{
				if (intvalue(t_argv[5])) z+=P_PointInSector(x, y)->floorplane.ZatPoint(x,y);
			}
		}
		else
		{
			// Legacy compatibility is more important than correctness.
			z = ONFLOORZ;// (GetDefaultByType(PClass)->flags & MF_SPAWNCEILING) ? ONCEILINGZ : ONFLOORZ;
		}
		
		if(t_argc >= 4)
		{
			angle = intvalue(t_argv[3]) * (SQWORD)ANG45 / 45;
		}
		
		t_return.type = svt_mobj;
		t_return.value.mobj = Spawn(PClass, x, y, z, ALLOW_REPLACE);

		if (t_return.value.mobj)		
		{
			t_return.value.mobj->angle = angle;

			if (!fs_forcecompatible)
			{
				if (!P_TestMobjLocation(t_return.value.mobj))
				{
					if (t_return.value.mobj->flags&MF_COUNTKILL) level.total_monsters--;
					if (t_return.value.mobj->flags&MF_COUNTITEM) level.total_items--;
					t_return.value.mobj->Destroy();
					t_return.value.mobj = NULL;
				}
			}
		}
	}	
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_RemoveObj()
{
	if (CheckArgs(1))
	{
		AActor * mo = MobjForSvalue(t_argv[0]);
		if(mo)  // nullptr check
		{
			if (mo->flags&MF_COUNTKILL && mo->health>0) level.total_monsters--;
			if (mo->flags&MF_COUNTITEM) level.total_items--;
			mo->Destroy();
		}
	}
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_KillObj()
{
	AActor *mo;
	
	if(t_argc) mo = MobjForSvalue(t_argv[0]);
	else mo = current_script->trigger;  // default to trigger object
	
	if(mo) 
	{
		// ensure the thing can be killed
		mo->flags|=MF_SHOOTABLE;
		mo->flags2&=~(MF2_INVULNERABLE|MF2_DORMANT);
		// [GrafZahl] This called P_KillMobj directly 
		// which is a very bad thing to do!
		P_DamageMobj(mo, NULL, NULL, mo->health, NAME_Massacre);
	}
}


//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_ObjX()
{
	AActor *mo = t_argc ? MobjForSvalue(t_argv[0]) : current_script->trigger;
	
	t_return.type = svt_fixed;           // haleyjd: SoM's fixed-point fix
	t_return.value.f = mo ? mo->x : 0;   // null ptr check
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_ObjY()
{
	AActor *mo = t_argc ? MobjForSvalue(t_argv[0]) : current_script->trigger;
	
	t_return.type = svt_fixed;         // haleyjd
	t_return.value.f = mo ? mo->y : 0; // null ptr check
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_ObjZ()
{
	AActor *mo = t_argc ? MobjForSvalue(t_argv[0]) : current_script->trigger;
	
	t_return.type = svt_fixed;         // haleyjd
	t_return.value.f = mo ? mo->z : 0; // null ptr check
}


//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_ObjAngle()
{
	AActor *mo = t_argc ? MobjForSvalue(t_argv[0]) : current_script->trigger;
	
	t_return.type = svt_fixed; // haleyjd: fixed-point -- SoM again :)
	t_return.value.f = mo ? (fixed_t)AngleToFixed(mo->angle) : 0;   // null ptr check
}


//==========================================================================
//
//
//
//==========================================================================

// teleport: object, sector_tag
void DFraggleThinker::SF_Teleport()
{
	int tag;
	AActor *mo;
	
	if (CheckArgs(1))
	{
		if(t_argc == 1)    // 1 argument: sector tag
		{
			mo = current_script->trigger;   // default to trigger
			tag = intvalue(t_argv[0]);
		}
		else    // 2 or more
		{                       // teleport a given object
			mo = MobjForSvalue(t_argv[0]);
			tag = intvalue(t_argv[1]);
		}
		
		if(mo)
			EV_Teleport(0, tag, NULL, 0, mo, true, true, false);
	}
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_SilentTeleport()
{
	int tag;
	AActor *mo;
	
	if (CheckArgs(1))
	{
		if(t_argc == 1)    // 1 argument: sector tag
		{
			mo = current_script->trigger;   // default to trigger
			tag = intvalue(t_argv[0]);
		}
		else    // 2 or more
		{                       // teleport a given object
			mo = MobjForSvalue(t_argv[0]);
			tag = intvalue(t_argv[1]);
		}
		
		if(mo)
			EV_Teleport(0, tag, NULL, 0, mo, false, false, true);
	}
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_DamageObj()
{
	AActor *mo;
	int damageamount;
	
	if (CheckArgs(1))
	{
		if(t_argc == 1)    // 1 argument: damage trigger by amount
		{
			mo = current_script->trigger;   // default to trigger
			damageamount = intvalue(t_argv[0]);
		}
		else    // 2 or more
		{                       // damage a given object
			mo = MobjForSvalue(t_argv[0]);
			damageamount = intvalue(t_argv[1]);
		}
		
		if(mo)
			P_DamageMobj(mo, NULL, current_script->trigger, damageamount, NAME_None);
	}
}

//==========================================================================
//
//
//
//==========================================================================

// the tag number of the sector the thing is in
void DFraggleThinker::SF_ObjSector()
{
	// use trigger object if not specified
	AActor *mo = t_argc ? MobjForSvalue(t_argv[0]) : current_script->trigger;
	
	t_return.type = svt_int;
	t_return.value.i = mo ? mo->Sector->tag : 0; // nullptr check
}

//==========================================================================
//
//
//
//==========================================================================

// the health number of an object
void DFraggleThinker::SF_ObjHealth()
{
	// use trigger object if not specified
	AActor *mo = t_argc ? MobjForSvalue(t_argv[0]) : current_script->trigger;
	
	t_return.type = svt_int;
	t_return.value.i = mo ? mo->health : 0;
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_ObjFlag()
{
	AActor *mo;
	int flagnum;
	
	if (CheckArgs(1))
	{
		if(t_argc == 1)         // use trigger, 1st is flag
		{
			// use trigger:
			mo = current_script->trigger;
			flagnum = intvalue(t_argv[0]);
		}
		else if(t_argc == 2)	// specified object
		{
			mo = MobjForSvalue(t_argv[0]);
			flagnum = intvalue(t_argv[1]);
		}
		else                     // >= 3 : SET flags
		{
			mo = MobjForSvalue(t_argv[0]);
			flagnum = intvalue(t_argv[1]);
			
			if(mo && flagnum<26)          // nullptr check
			{
				// remove old bit
				mo->flags &= ~(1 << flagnum);
				
				// make the new flag
				mo->flags |= (!!intvalue(t_argv[2])) << flagnum;
			}     
		}
		t_return.type = svt_int;  
		if (mo && flagnum<26)
		{
			t_return.value.i = !!(mo->flags & (1 << flagnum));
		}
		else t_return.value.i = 0;
	}
}

//==========================================================================
//
//
//
//==========================================================================

// apply momentum to a thing
void DFraggleThinker::SF_PushThing()
{
	if (CheckArgs(3))
	{
		AActor * mo = MobjForSvalue(t_argv[0]);
		if(!mo) return;
	
		angle_t angle = (angle_t)FixedToAngle(fixedvalue(t_argv[1]));
		fixed_t force = fixedvalue(t_argv[2]);
	
		P_ThrustMobj(mo, angle, force);
	}
}

//==========================================================================
//
//  SF_ReactionTime -- useful for freezing things
//
//==========================================================================


void DFraggleThinker::SF_ReactionTime()
{
	if (CheckArgs(1))
	{
		AActor *mo = MobjForSvalue(t_argv[0]);
	
		if(t_argc > 1)
		{
			if(mo) mo->reactiontime = (intvalue(t_argv[1]) * TICRATE) / 100;
		}
	
		t_return.type = svt_int;
		t_return.value.i = mo ? mo->reactiontime : 0;
	}
}

//==========================================================================
//
//  SF_MobjTarget   -- sets a thing's target field
//
//==========================================================================

// Sets a mobj's Target! >:)
void DFraggleThinker::SF_MobjTarget()
{
	AActor*  mo;
	AActor*  target;
	
	if (CheckArgs(1))
	{
		mo = MobjForSvalue(t_argv[0]);
		if(t_argc > 1)
		{
			target = MobjForSvalue(t_argv[1]);
			if(mo && target && mo->SeeState) // haleyjd: added target check -- no NULL allowed
			{
				mo->target=target;
				mo->SetState(mo->SeeState);
				mo->flags|=MF_JUSTHIT;
			}
		}
		
		t_return.type = svt_mobj;
		t_return.value.mobj = mo ? mo->target : NULL;
	}
}

//==========================================================================
//
//  SF_MobjMomx, MobjMomy, MobjMomz -- momentum functions
//
//==========================================================================

void DFraggleThinker::SF_MobjMomx()
{
	AActor*   mo;
	
	if (CheckArgs(1))
	{
		mo = MobjForSvalue(t_argv[0]);
		if(t_argc > 1)
		{
			if(mo) 
				mo->momx = fixedvalue(t_argv[1]);
		}
		
		t_return.type = svt_fixed;
		t_return.value.f = mo ? mo->momx : 0;
	}
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_MobjMomy()
{
	AActor*   mo;
	
	if (CheckArgs(1))
	{
		mo = MobjForSvalue(t_argv[0]);
		if(t_argc > 1)
		{
			if(mo)
				mo->momy = fixedvalue(t_argv[1]);
		}
		
		t_return.type = svt_fixed;
		t_return.value.f = mo ? mo->momy : 0;
	}
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_MobjMomz()
{
	AActor*   mo;
	
	if (CheckArgs(1))
	{
		mo = MobjForSvalue(t_argv[0]);
		if(t_argc > 1)
		{
			if(mo)
				mo->momz = fixedvalue(t_argv[1]);
		}
		
		t_return.type = svt_fixed;
		t_return.value.f = mo ? mo->momz : 0;
	}
}


//==========================================================================
//
//
//
//==========================================================================

/****************** Trig *********************/

void DFraggleThinker::SF_PointToAngle()
{
	if (CheckArgs(4))
	{
		fixed_t x1 = fixedvalue(t_argv[0]);
		fixed_t y1 = fixedvalue(t_argv[1]);
		fixed_t x2 = fixedvalue(t_argv[2]);
		fixed_t y2 = fixedvalue(t_argv[3]);
		
		angle_t angle = R_PointToAngle2(x1, y1, x2, y2);
		
		t_return.type = svt_fixed;
		t_return.value.f = (fixed_t)AngleToFixed(angle);
	}
}


//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_PointToDist()
{
	if (CheckArgs(4))
	{
		// Doing this in floating point is actually faster with modern computers!
		float x = floatvalue(t_argv[2]) - floatvalue(t_argv[0]);
		float y = floatvalue(t_argv[3]) - floatvalue(t_argv[1]);
	    
		t_return.type = svt_fixed;
		t_return.value.f = (fixed_t)(sqrtf(x*x+y*y)*65536.f);
	}
}


//==========================================================================
//
// setcamera(obj, [angle], [height], [pitch])
//
// [GrafZahl] This is a complete rewrite.
// Although both Eternity and Legacy implement this function
// they are mutually incompatible with each other and with ZDoom...
//
//==========================================================================

void DFraggleThinker::SF_SetCamera()
{
	angle_t angle;
	player_t * player;
	AActor * newcamera;
	
	if (CheckArgs(1))
	{
		player=current_script->trigger->player;
		if (!player) player=&players[0];
		
		newcamera = MobjForSvalue(t_argv[0]);
		if(!newcamera)
		{
			script_error("invalid location object for camera\n");
			return;         // nullptr check
		}
		
		angle = t_argc < 2 ? newcamera->angle : (fixed_t)FixedToAngle(fixedvalue(t_argv[1]));

		newcamera->special1=newcamera->angle;
		newcamera->special2=newcamera->z;
		newcamera->z = t_argc < 3 ? (newcamera->z + (41 << FRACBITS)) : (intvalue(t_argv[2]) << FRACBITS);
		newcamera->angle = angle;
		if(t_argc < 4) newcamera->pitch = 0;
		else
		{
			fixed_t pitch = fixedvalue(t_argv[3]);
			if(pitch < -50*FRACUNIT) pitch = -50*FRACUNIT;
			if(pitch > 50*FRACUNIT)  pitch =  50*FRACUNIT;
			newcamera->pitch=(angle_t)((pitch/65536.0f)*(ANGLE_45/45.0f)*(20.0f/32.0f));
		}
		player->camera=newcamera;
	}
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_ClearCamera()
{
	player_t * player;
	player=current_script->trigger->player;
	if (!player) player=&players[0];

	AActor * cam=player->camera;
	if (cam)
	{
		player->camera=player->mo;
		cam->angle=cam->special1;
		cam->z=cam->special2;
	}

}



/*********** sounds ******************/

//==========================================================================
//
//
//
//==========================================================================

// start sound from thing
void DFraggleThinker::SF_StartSound()
{
	AActor *mo;
	
	if (CheckArgs(2))
	{
		mo = MobjForSvalue(t_argv[0]);
		
		if (mo)
		{
			S_SoundID(mo, CHAN_BODY, T_FindSound(stringvalue(t_argv[1])), 1, ATTN_NORM);
		}
	}
}

//==========================================================================
//
//
//
//==========================================================================

// start sound from sector
void DFraggleThinker::SF_StartSectorSound()
{
	sector_t *sector;
	int tagnum;
	
	if (CheckArgs(2))
	{
		tagnum = intvalue(t_argv[0]);
		
		int i=-1;
		while ((i = T_FindSectorFromTag(tagnum, i)) >= 0)
		{
			sector = &sectors[i];
			S_SoundID(sector->soundorg, CHAN_BODY, T_FindSound(stringvalue(t_argv[1])), 1.0f, ATTN_NORM);
		}
	}
}

/************* Sector functions ***************/

//DMover::EResult P_MoveFloor (sector_t * m_Sector, fixed_t speed, fixed_t dest, int crush, int direction, int flags=0);
//DMover::EResult P_MoveCeiling (sector_t * m_Sector, fixed_t speed, fixed_t dest, int crush, int direction, int flags=0);

class DFloorChanger : public DFloor
{
public:
	DFloorChanger(sector_t * sec)
		: DFloor(sec) {}

	bool Move(fixed_t speed, fixed_t dest, int crush, int direction)
	{
		bool res = DMover::crushed != MoveFloor(speed, dest, crush, direction);
		Destroy();
		m_Sector->floordata=NULL;
		stopinterpolation (INTERP_SectorFloor, m_Sector);
		m_Sector=NULL;
		return res;
	}
};


//==========================================================================
//
//
//
//==========================================================================

// floor height of sector
void DFraggleThinker::SF_FloorHeight()
{
	int tagnum;
	int secnum;
	fixed_t dest;
	int returnval = 1; // haleyjd: SoM's fixes
	
	if (CheckArgs(1))
	{
		tagnum = intvalue(t_argv[0]);
		
		if(t_argc > 1)          // > 1: set floor height
		{
			int i;
			int crush = (t_argc >= 3) ? intvalue(t_argv[2]) : false;
			
			i = -1;
			dest = fixedvalue(t_argv[1]);
			
			// set all sectors with tag
			
			while ((i = T_FindSectorFromTag(tagnum, i)) >= 0)
			{
				if (sectors[i].floordata) continue;	// don't move floors that are active!

				DFloorChanger * f = new DFloorChanger(&sectors[i]);
				if (!f->Move(
					abs(dest - sectors[i].CenterFloor()), 
					sectors[i].floorplane.PointToDist (CenterSpot(&sectors[i]), dest), 
					crush? 10:-1, 
					(dest > sectors[i].CenterFloor()) ? 1 : -1))
				{
					returnval = 0;
				}
			}
		}
		else
		{
			secnum = T_FindSectorFromTag(tagnum, -1);
			if(secnum < 0)
			{ 
				script_error("sector not found with tagnum %i\n", tagnum); 
				return;
			}
			returnval = sectors[secnum].CenterFloor() >> FRACBITS;
		}
		
		// return floor height
		
		t_return.type = svt_int;
		t_return.value.i = returnval;
	}
}


//=============================================================================
//
//
//=============================================================================
class DMoveFloor : public DFloor
{
public:
	DMoveFloor(sector_t * sec,int moveheight,int _m_Direction,int crush,int movespeed)
	: DFloor(sec)
	{
		m_Type = floorRaiseByValue;
		m_Crush = crush;
		m_Speed=movespeed;
		m_Direction = _m_Direction;
		m_FloorDestDist = moveheight;
		StartFloorSound();
	}
};



//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_MoveFloor()
{
	int secnum = -1;
	sector_t *sec;
	int tagnum, platspeed = 1, destheight, crush;
	
	if (CheckArgs(2))
	{
		tagnum = intvalue(t_argv[0]);
		destheight = intvalue(t_argv[1]) * FRACUNIT;
		platspeed = t_argc > 2 ? fixedvalue(t_argv[2]) : FRACUNIT;
		crush = (t_argc > 3 ? intvalue(t_argv[3]) : -1);
		
		// move all sectors with tag
		
		while ((secnum = T_FindSectorFromTag(tagnum, secnum)) >= 0)
		{
			sec = &sectors[secnum];
			// Don't start a second thinker on the same floor
			if (sec->floordata) continue;
			
			new DMoveFloor(sec,sec->floorplane.PointToDist(CenterSpot(sec),destheight),
				destheight < sec->CenterFloor() ? -1:1,crush,platspeed);
		}
	}
}

//==========================================================================
//
//
//
//==========================================================================

class DCeilingChanger : public DCeiling
{
public:
	DCeilingChanger(sector_t * sec)
		: DCeiling(sec) {}

	bool Move(fixed_t speed, fixed_t dest, int crush, int direction)
	{
		bool res = DMover::crushed != MoveCeiling(speed, dest, crush, direction);
		Destroy();
		m_Sector->ceilingdata=NULL;
		stopinterpolation (INTERP_SectorCeiling, m_Sector);
		m_Sector=NULL;
		return res;
	}
};

//==========================================================================
//
//
//
//==========================================================================

// ceiling height of sector
void DFraggleThinker::SF_CeilingHeight()
{
	fixed_t dest;
	int secnum;
	int tagnum;
	int returnval = 1;
	
	if (CheckArgs(1))
	{
		tagnum = intvalue(t_argv[0]);
		
		if(t_argc > 1)          // > 1: set ceilheight
		{
			int i;
			int crush = (t_argc >= 3) ? intvalue(t_argv[2]) : false;
			
			i = -1;
			dest = fixedvalue(t_argv[1]);
			
			// set all sectors with tag
			while ((i = T_FindSectorFromTag(tagnum, i)) >= 0)
			{
				if (sectors[i].ceilingdata) continue;	// don't move ceilings that are active!

				DCeilingChanger * c = new DCeilingChanger(&sectors[i]);
				if (!c->Move(
					abs(dest - sectors[i].CenterCeiling()), 
					sectors[i].ceilingplane.PointToDist (CenterSpot(&sectors[i]), dest), 
					crush? 10:-1,
					(dest > sectors[i].CenterCeiling()) ? 1 : -1))
				{
					returnval = 0;
				}
			}
		}
		else
		{
			secnum = T_FindSectorFromTag(tagnum, -1);
			if(secnum < 0)
			{ 
				script_error("sector not found with tagnum %i\n", tagnum); 
				return;
			}
			returnval = sectors[secnum].CenterCeiling() >> FRACBITS;
		}
		
		// return ceiling height
		t_return.type = svt_int;
		t_return.value.i = returnval;
	}
}


//==========================================================================
//
//
//
//==========================================================================

class DMoveCeiling : public DCeiling
{
public:

	DMoveCeiling(sector_t * sec,int tag,fixed_t destheight,fixed_t speed,int silent,int crush)
		: DCeiling(sec)
	{
		m_Crush = crush;
		m_Speed2 = m_Speed = m_Speed1 = speed;
		m_Silent = silent;
		m_Type = DCeiling::ceilLowerByValue;	// doesn't really matter as long as it's no special value
		m_Tag=tag;			
		vertex_t * spot=CenterSpot(sec);
		m_TopHeight=m_BottomHeight=sec->ceilingplane.PointToDist(spot,destheight);
		m_Direction=destheight>sec->ceilingtexz? 1:-1;

		// Do not interpolate instant movement ceilings.
		fixed_t movedist = abs(sec->ceilingplane.d - m_BottomHeight);
		if (m_Speed >= movedist)
		{
			stopinterpolation (INTERP_SectorCeiling, sec);
			m_Silent=2;
		}
		PlayCeilingSound();
	}
};


//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_MoveCeiling()
{
	int secnum = -1;
	sector_t *sec;
	int tagnum, platspeed = 1, destheight;
	int crush;
	int silent;
	
	if (CheckArgs(2))
	{
		tagnum = intvalue(t_argv[0]);
		destheight = intvalue(t_argv[1]) * FRACUNIT;
		platspeed = /*FLOORSPEED **/ (t_argc > 2 ? fixedvalue(t_argv[2]) : FRACUNIT);
		crush=t_argc>3 ? intvalue(t_argv[3]):-1;
		silent=t_argc>4 ? intvalue(t_argv[4]):1;
		
		// move all sectors with tag
		while ((secnum = T_FindSectorFromTag(tagnum, secnum)) >= 0)
		{
			sec = &sectors[secnum];
			
			// Don't start a second thinker on the same floor
			if (sec->ceilingdata) continue;
			new DMoveCeiling(sec, tagnum, destheight, platspeed, silent, crush);
		}
	}
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_LightLevel()
{
	sector_t *sector;
	int secnum;
	int tagnum;
	
	if (CheckArgs(1))
	{
		tagnum = intvalue(t_argv[0]);
		
		// argv is sector tag
		secnum = T_FindSectorFromTag(tagnum, -1);
		
		if(secnum < 0)
		{ 
			return;
		}
		
		sector = &sectors[secnum];
		
		if(t_argc > 1)          // > 1: set light level
		{
			int i = -1;
			
			// set all sectors with tag
			while ((i = T_FindSectorFromTag(tagnum, i)) >= 0)
			{
				sectors[i].lightlevel = (short)intvalue(t_argv[1]);
			}
		}
		
		// return lightlevel
		t_return.type = svt_int;
		t_return.value.i = sector->lightlevel;
	}
}


//==========================================================================
// sf 13/10/99:
//
// P_FadeLight()
//
// Fade all the lights in sectors with a particular tag to a new value
//
//==========================================================================
void DFraggleThinker::SF_FadeLight()
{
	int sectag, destlevel, speed = 1;
	int i;
	
	if (CheckArgs(2))
	{
		sectag = intvalue(t_argv[0]);
		destlevel = intvalue(t_argv[1]);
		speed = t_argc>2 ? intvalue(t_argv[2]) : 1;
		
		for (i = -1; (i = P_FindSectorFromTag(sectag,i)) >= 0;) 
		{
			if (!sectors[i].lightingdata) new DLightLevel(&sectors[i],destlevel,speed);
		}
	}
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_FloorTexture()
{
	int tagnum, secnum;
	sector_t *sector;
	
	if (CheckArgs(1))
	{
		tagnum = intvalue(t_argv[0]);
		
		// argv is sector tag
		secnum = T_FindSectorFromTag(tagnum, -1);
		
		if(secnum < 0)
		{ script_error("sector not found with tagnum %i\n", tagnum); return;}
		
		sector = &sectors[secnum];
		
		if(t_argc > 1)
		{
			int i = -1;
			int picnum = TexMan.GetTexture(t_argv[1].value.s, FTexture::TEX_Flat, FTextureManager::TEXMAN_Overridable);
			
			// set all sectors with tag
			while ((i = T_FindSectorFromTag(tagnum, i)) >= 0)
			{
				sectors[i].floorpic=picnum;
				sectors[i].AdjustFloorClip();
			}
		}
		
		t_return.type = svt_string;
		FTexture * tex = TexMan[sector->floorpic];
		t_return.value.s = tex? tex->Name : NULL;
	}
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_SectorColormap()
{
	// This doesn't work properly and it never will.
	// Whatever was done here originally, it is incompatible 
	// with Boom and ZDoom and doesn't work properly in Legacy either.
	
	// Making it no-op is probably the best thing one can do in this case.
	
	/*
	int tagnum, secnum;
	sector_t *sector;
	int c=2;
	int i = -1;

	if(t_argc<2)
    { script_error("insufficient arguments to function\n"); return; }
	
	tagnum = intvalue(t_argv[0]);
	
	// argv is sector tag
	secnum = T_FindSectorFromTag(tagnum, -1);
	
	if(secnum < 0)
    { script_error("sector not found with tagnum %i\n", tagnum); return;}
	
	sector = &sectors[secnum];

	if (t_argv[1].type==svt_string)
	{
		DWORD cm = R_ColormapNumForName(t_argv[1].value.s);

		while ((i = T_FindSectorFromTag(tagnum, i)) >= 0)
		{
			sectors[i].midmap=cm;
			sectors[i].heightsec=&sectors[i];
		}
	}
	*/	
}


//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_CeilingTexture()
{
	int tagnum, secnum;
	sector_t *sector;
	
	if (CheckArgs(1))
	{
		tagnum = intvalue(t_argv[0]);
		
		// argv is sector tag
		secnum = T_FindSectorFromTag(tagnum, -1);
		
		if(secnum < 0)
		{ script_error("sector not found with tagnum %i\n", tagnum); return;}
		
		sector = &sectors[secnum];
		
		if(t_argc > 1)
		{
			int i = -1;
			int picnum = TexMan.GetTexture(t_argv[1].value.s, FTexture::TEX_Flat, FTextureManager::TEXMAN_Overridable);
			
			// set all sectors with tag
			while ((i = T_FindSectorFromTag(tagnum, i)) >= 0)
			{
				sectors[i].ceilingpic=picnum;
			}
		}
		
		t_return.type = svt_string;
		FTexture * tex = TexMan[sector->ceilingpic];
		t_return.value.s = tex? tex->Name : NULL;
	}
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_ChangeHubLevel()
{
	I_Error("FS hub system permanently disabled\n");
}

//==========================================================================
//
// for start map: start new game on a particular skill
//
//==========================================================================

void DFraggleThinker::SF_StartSkill()
{
	I_Error("startskill is not supported by this implementation!\n");
}

//==========================================================================
//
// Doors
//
// opendoor(sectag, [delay], [speed])
//
//==========================================================================


void DFraggleThinker::SF_OpenDoor()
{
	int speed, wait_time;
	int sectag;
	
	if (CheckArgs(1))
	{
		// got sector tag
		sectag = intvalue(t_argv[0]);
		if (sectag==0) return;	// tag 0 not allowed
		
		// door wait time
		if(t_argc > 1) wait_time = (intvalue(t_argv[1]) * TICRATE) / 100;
		else wait_time = 0;  // 0= stay open
		
		// door speed
		if(t_argc > 2) speed = intvalue(t_argv[2]);
		else speed = 1;    // 1= normal speed

		EV_DoDoor(wait_time? DDoor::doorRaise:DDoor::doorOpen,NULL,NULL,sectag,2*FRACUNIT*clamp(speed,1,127),wait_time,0,0);
	}
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_CloseDoor()
{
	int speed;
	int sectag;
	
	if (CheckArgs(1))
	{
		// got sector tag
		sectag = intvalue(t_argv[0]);
		if (sectag==0) return;	// tag 0 not allowed
		
		// door speed
		if(t_argc > 1) speed = intvalue(t_argv[1]);
		else speed = 1;    // 1= normal speed
		
		EV_DoDoor(DDoor::doorClose,NULL,NULL,sectag,2*FRACUNIT*clamp(speed,1,127),0,0,0);
	}
}

//==========================================================================
//
//
//
//==========================================================================

// run console cmd
void DFraggleThinker::SF_RunCommand()
{
	FS_EmulateCmd(GetFormatString(0).LockBuffer());
}

//==========================================================================
//
//
//
//==========================================================================

// any linedef type
extern void P_TranslateLineDef (line_t *ld, maplinedef_t *mld);

void DFraggleThinker::SF_LineTrigger()
{
	if (CheckArgs(1))
	{
		line_t line;
		maplinedef_t mld;
		mld.special=intvalue(t_argv[0]);
		mld.tag=t_argc > 1 ? intvalue(t_argv[1]) : 0;
		P_TranslateLineDef(&line, &mld);
		LineSpecials[line.special](NULL, current_script->trigger, false, 
			line.args[0],line.args[1],line.args[2],line.args[3],line.args[4]); 
	}
}

//==========================================================================
//
//
//
//==========================================================================
void DFraggleThinker::SF_ChangeMusic()
{
	if (CheckArgs(1))
	{
		FS_ChangeMusic(stringvalue(t_argv[0]));
	}
}


//==========================================================================
//
//
//
//==========================================================================

inline line_t * P_FindLine(int tag,int * searchPosition)
{
	*searchPosition=P_FindLineFromID(tag,*searchPosition);
	return *searchPosition>=0? &lines[*searchPosition]:NULL;
}

/*
SF_SetLineBlocking()

  Sets a line blocking or unblocking
  
	setlineblocking(tag, [1|0]);
*/
void DFraggleThinker::SF_SetLineBlocking()
{
	line_t *line;
	int blocking;
	int searcher = -1;
	int tag;
	static unsigned short blocks[]={0,ML_BLOCKING,ML_BLOCKEVERYTHING};
	
	if (CheckArgs(2))
	{
		blocking=intvalue(t_argv[1]);
		if (blocking>=0 && blocking<=2) 
		{
			blocking=blocks[blocking];
			tag=intvalue(t_argv[0]);
			while((line = P_FindLine(tag, &searcher)) != NULL)
			{
				line->flags = (line->flags & ~(ML_BLOCKING|ML_BLOCKEVERYTHING)) | blocking;
			}
		}
	}
}

//==========================================================================
//
//
//
//==========================================================================

// similar, but monster blocking

void DFraggleThinker::SF_SetLineMonsterBlocking()
{
	line_t *line;
	int blocking;
	int searcher = -1;
	int tag;
	
	if (CheckArgs(2))
	{
		blocking = intvalue(t_argv[1]) ? ML_BLOCKMONSTERS : 0;
		
		tag=intvalue(t_argv[0]);
		while((line = P_FindLine(tag, &searcher)) != NULL)
		{
			line->flags = (line->flags & ~ML_BLOCKMONSTERS) | blocking;
		}
	}
}

/*
SF_SetLineTexture

  #2 in a not-so-long line of ACS-inspired functions
  This one is *much* needed, IMO
  
	Eternity: setlinetexture(tag, side, position, texture)
	Legacy:	  setlinetexture(tag, texture, side, sections)

*/


void DFraggleThinker::SF_SetLineTexture()
{
	line_t *line;
	int tag;
	int side;
	int position;
	const char *texture;
	int texturenum;
	int searcher;
	
	if (CheckArgs(4))
	{
		tag = intvalue(t_argv[0]);

		// the eternity version
		if (t_argv[3].type==svt_string)
		{
			side = intvalue(t_argv[1]);   
			if(side < 0 || side > 1)
			{
				script_error("invalid side number for texture change\n");
				return;
			}
			
			position = intvalue(t_argv[2]);
			if(position < 1 || position > 3)
			{
				script_error("invalid position for texture change\n");
				return;
			}
			position=3-position;
			
			texture = stringvalue(t_argv[3]);
			texturenum = TexMan.GetTexture(texture, FTexture::TEX_Wall, FTextureManager::TEXMAN_Overridable);
			
			searcher = -1;
			
			while((line = P_FindLine(tag, &searcher)) != NULL)
			{
				// bad sidedef, Hexen just SEGV'd here!
				if(line->sidenum[side]!=NO_SIDE)
				{
					side_t * sided=&sides[line->sidenum[side]];
					switch(position)
					{
					case 0:
						sided->toptexture=texturenum;
						break;
					case 1:
						sided->midtexture=texturenum;
						break;
					case 2:
						sided->bottomtexture=texturenum;
						break;
					}
				}
			}
		}
		else // and an improved legacy version
		{ 
			int i = -1; 
			int picnum = TexMan.GetTexture(t_argv[1].value.s, FTexture::TEX_Wall, FTextureManager::TEXMAN_Overridable); 
			side = !!intvalue(t_argv[2]); 
			int sections = intvalue(t_argv[3]); 
			
			// set all sectors with tag 
			while ((i = P_FindLineFromID(tag, i)) >= 0) 
			{ 
				if(lines[i].sidenum[side]!=NO_SIDE)
				{ 
					side_t * sided=&sides[lines[i].sidenum[side]];

					if(sections & 1) sided->toptexture = picnum;
					if(sections & 2) sided->midtexture = picnum;
					if(sections & 4) sided->bottomtexture = picnum;
				} 
			} 
		} 
	}
}


//==========================================================================
//
//
//
//==========================================================================

// SoM: Max, Min, Abs math functions.
void DFraggleThinker::SF_Max()
{
	fixed_t n1, n2;
	
	if (CheckArgs(2))
	{
		n1 = fixedvalue(t_argv[0]);
		n2 = fixedvalue(t_argv[1]);
		
		t_return.type = svt_fixed;
		t_return.value.f = (n1 > n2) ? n1 : n2;
	}
}


//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_Min()
{
	fixed_t   n1, n2;
	
	if (CheckArgs(1))
	{
		n1 = fixedvalue(t_argv[0]);
		n2 = fixedvalue(t_argv[1]);
		
		t_return.type = svt_fixed;
		t_return.value.f = (n1 < n2) ? n1 : n2;
	}
}


//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_Abs()
{
	fixed_t   n1;
	
	if (CheckArgs(1))
	{
		n1 = fixedvalue(t_argv[0]);
		
		t_return.type = svt_fixed;
		t_return.value.f = (n1 < 0) ? -n1 : n1;
	}
}

/* 
SF_Gameskill, SF_Gamemode

  Access functions are more elegant for these than variables, 
  especially for the game mode, which doesn't exist as a numeric 
  variable already.
*/

void DFraggleThinker::SF_Gameskill()
{
	t_return.type = svt_int;
	t_return.value.i = G_SkillProperty(SKILLP_ACSReturn) + 1;  // +1 for the user skill value
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_Gamemode()
{
	t_return.type = svt_int;   
	if(!multiplayer)
	{
		t_return.value.i = 0; // single-player
	}
	else if(!deathmatch)
	{
		t_return.value.i = 1; // cooperative
	}
	else
		t_return.value.i = 2; // deathmatch
}

/*
SF_IsPlayerObj()

  A function suggested by SoM to help the script coder prevent
  exceptions related to calling player functions on non-player
  objects.
*/
void DFraggleThinker::SF_IsPlayerObj()
{
	AActor *mo;
	
	if(!t_argc)
	{
		mo = current_script->trigger;
	}
	else
		mo = MobjForSvalue(t_argv[0]);
	
	t_return.type = svt_int;
	t_return.value.i = (mo && mo->player) ? 1 : 0;
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_PlayerKeys()
{
	// This function is just kept for backwards compatibility and intentionally limited to thr standard keys!
	// Use Give/Take/CheckInventory instead!
	static const char * const DoomKeys[]={"BlueCard", "YellowCard", "RedCard", "BlueSkull", "YellowSkull", "RedSkull"};
	int  playernum, keynum, givetake;
	const char * keyname;
	
	if (CheckArgs(2))
	{
		playernum=T_GetPlayerNum(t_argv[0]);
		if (playernum==-1) return;
		
		keynum = intvalue(t_argv[1]);
		if(keynum < 0 || keynum >= 6)
		{
			script_error("key number out of range: %i\n", keynum);
			return;
		}
		keyname=DoomKeys[keynum];
		
		if(t_argc == 2)
		{
			t_return.type = svt_int;
			t_return.value.i = FS_CheckInventory(players[playernum].mo, keyname);
			return;
		}
		else
		{
			givetake = intvalue(t_argv[2]);
			if(givetake) FS_GiveInventory(players[playernum].mo, keyname, 1);
			else FS_TakeInventory(players[playernum].mo, keyname, 1);
			t_return.type = svt_int;
			t_return.value.i = 0;
		}
	}
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_PlayerAmmo()
{
	// This function is just kept for backwards compatibility and intentionally limited!
	// Use Give/Take/CheckInventory instead!
	int playernum, amount;
	const PClass * ammotype;
	
	if (CheckArgs(2))
	{
		playernum=T_GetPlayerNum(t_argv[0]);
		if (playernum==-1) return;

		ammotype=T_GetAmmo(t_argv[1]);
		if (!ammotype) return;

		if(t_argc >= 3)
		{
			AInventory * iammo = players[playernum].mo->FindInventory(ammotype);
			amount = intvalue(t_argv[2]);
			if(amount < 0) amount = 0;
			if (iammo) iammo->Amount = amount;
			else players[playernum].mo->GiveAmmo(ammotype, amount);
		}

		t_return.type = svt_int;
		AInventory * iammo = players[playernum].mo->FindInventory(ammotype);
		if (iammo) t_return.value.i = iammo->Amount;
		else t_return.value.i = 0;
	}
}


//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_MaxPlayerAmmo()
{
	int playernum, amount;
	const PClass * ammotype;

	if (CheckArgs(2))
	{
		playernum=T_GetPlayerNum(t_argv[0]);
		if (playernum==-1) return;

		ammotype=T_GetAmmo(t_argv[1]);
		if (!ammotype) return;

		if(t_argc == 2)
		{
		}
		else if(t_argc >= 3)
		{
			AAmmo * iammo = (AAmmo*)players[playernum].mo->FindInventory(ammotype);
			amount = intvalue(t_argv[2]);
			if(amount < 0) amount = 0;
			if (!iammo) 
			{
				iammo = static_cast<AAmmo *>(Spawn (ammotype, 0, 0, 0, NO_REPLACE));
				iammo->Amount = 0;
				iammo->AttachToOwner (players[playernum].mo);
			}
			iammo->MaxAmount = amount;


			for (AInventory *item = players[playernum].mo->Inventory; item != NULL; item = item->Inventory)
			{
				if (item->IsKindOf(RUNTIME_CLASS(ABackpackItem)))
				{
					if (t_argc>=4) amount = intvalue(t_argv[3]);
					else amount*=2;
					break;
				}
			}
			iammo->BackpackMaxAmount=amount;
		}

		t_return.type = svt_int;
		AInventory * iammo = players[playernum].mo->FindInventory(ammotype);
		if (iammo) t_return.value.i = iammo->MaxAmount;
		else t_return.value.i = ((AAmmo*)GetDefaultByType(ammotype))->MaxAmount;
	}
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_PlayerWeapon()
{
	static const char * const WeaponNames[]={
		"Fist", "Pistol", "Shotgun", "Chaingun", "RocketLauncher", 
		"PlasmaRifle", "BFG9000", "Chainsaw", "SuperShotgun" };


	// This function is just kept for backwards compatibility and intentionally limited to the standard weapons!
	// Use Give/Take/CheckInventory instead!
    int playernum;
    int weaponnum;
    int newweapon;
	
	if (CheckArgs(2))
	{
		playernum=T_GetPlayerNum(t_argv[0]);
		weaponnum = intvalue(t_argv[1]);
		if (playernum==-1) return;
		if (weaponnum<0 || weaponnum>9)
		{
			script_error("weaponnum out of range! %s\n", weaponnum);
			return;
		}
		const PClass * ti = PClass::FindClass(WeaponNames[weaponnum]);
		if (!ti)
		{
			script_error("incompatibility in playerweapon\n", weaponnum);
			return;
		}
		
		if (t_argc == 2)
		{
			AActor * wp = players[playernum].mo->FindInventory(ti);
			t_return.type = svt_int;
			t_return.value.i = wp!=NULL;;
			return;
		}
		else
		{
			AActor * wp = players[playernum].mo->FindInventory(ti);

			newweapon = !!intvalue(t_argv[2]);
			if (!newweapon)
			{
				if (wp) 
				{
					wp->Destroy();
					// If the weapon is active pick a replacement. Legacy didn't do this!
					if (players[playernum].PendingWeapon==wp) players[playernum].PendingWeapon=WP_NOCHANGE;
					if (players[playernum].ReadyWeapon==wp) 
					{
						players[playernum].ReadyWeapon=NULL;
						players[playernum].mo->PickNewWeapon(NULL);
					}
				}
			}
			else 
			{
				if (!wp) 
				{
					AWeapon * pw=players[playernum].PendingWeapon;
					players[playernum].mo->GiveInventoryType(ti);
					players[playernum].PendingWeapon=pw;
				}
			}
			
			t_return.type = svt_int;
			t_return.value.i = !!newweapon;
			return;
		}
	}
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_PlayerSelectedWeapon()
{
	int playernum;
	int weaponnum;

	// This function is just kept for backwards compatibility and intentionally limited to the standard weapons!

	static const char * const WeaponNames[]={
		"Fist", "Pistol", "Shotgun", "Chaingun", "RocketLauncher", 
		"PlasmaRifle", "BFG9000", "Chainsaw", "SuperShotgun" };


	if (CheckArgs(1))
	{
		playernum=T_GetPlayerNum(t_argv[0]);

		if(t_argc == 2)
		{
			weaponnum = intvalue(t_argv[1]);

			if (weaponnum<0 || weaponnum>=9)
			{
				script_error("weaponnum out of range! %s\n", weaponnum);
				return;
			}
			const PClass * ti = PClass::FindClass(WeaponNames[weaponnum]);
			if (!ti)
			{
				script_error("incompatibility in playerweapon\n", weaponnum);
				return;
			}

			players[playernum].PendingWeapon = (AWeapon*)players[playernum].mo->FindInventory(ti);

		} 
		t_return.type = svt_int;
		for(int i=0;i<9;i++)
		{
			if (players[playernum].ReadyWeapon->GetClass ()->TypeName == FName(WeaponNames[i]))
			{
				t_return.value.i=i;
				break;
			}
		}
	}
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_GiveInventory()
{
	int  playernum, count;
	
	if (CheckArgs(2))
	{
		playernum=T_GetPlayerNum(t_argv[0]);
		if (playernum==-1) return;

		if(t_argc == 2) count=1;
		else count=intvalue(t_argv[2]);
		FS_GiveInventory(players[playernum].mo, stringvalue(t_argv[1]), count);
		t_return.type = svt_int;
		t_return.value.i = 0;
	}
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_TakeInventory()
{
	int  playernum, count;

	if (CheckArgs(2))
	{
		playernum=T_GetPlayerNum(t_argv[0]);
		if (playernum==-1) return;

		if(t_argc == 2) count=32767;
		else count=intvalue(t_argv[2]);
		FS_TakeInventory(players[playernum].mo, stringvalue(t_argv[1]), count);
		t_return.type = svt_int;
		t_return.value.i = 0;
	}
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_CheckInventory()
{
	int  playernum;
	
	if (CheckArgs(2))
	{
		playernum=T_GetPlayerNum(t_argv[0]);
		if (playernum==-1) 
		{
			t_return.value.i = 0;
			return;
		}
		t_return.type = svt_int;
		t_return.value.i = FS_CheckInventory(players[playernum].mo, stringvalue(t_argv[1]));
	}
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_SetWeapon()
{
	if (CheckArgs(2))
	{
		int playernum=T_GetPlayerNum(t_argv[0]);
		if (playernum!=-1) 
		{
			AInventory *item = players[playernum].mo->FindInventory (PClass::FindClass (stringvalue(t_argv[1])));

			if (item == NULL || !item->IsKindOf (RUNTIME_CLASS(AWeapon)))
			{
			}
			else if (players[playernum].ReadyWeapon == item)
			{
				// The weapon is already selected, so setweapon succeeds by default,
				// but make sure the player isn't switching away from it.
				players[playernum].PendingWeapon = WP_NOCHANGE;
				t_return.value.i = 1;
			}
			else
			{
				AWeapon *weap = static_cast<AWeapon *> (item);

				if (weap->CheckAmmo (AWeapon::EitherFire, false))
				{
					// There's enough ammo, so switch to it.
					t_return.value.i = 1;
					players[playernum].PendingWeapon = weap;
				}
			}
		}
		t_return.value.i = 0;
	}
}

// removed SF_PlayerMaxAmmo



//
// movecamera(camera, targetobj, targetheight, movespeed, targetangle, anglespeed)
//

void DFraggleThinker::SF_MoveCamera()
{
	fixed_t    x, y, z;  
	fixed_t    xdist, ydist, zdist, xydist, movespeed;
	fixed_t    xstep, ystep, zstep, targetheight;
	angle_t    anglespeed, anglestep, angledist, targetangle, 
		mobjangle, bigangle, smallangle;
	
	// I have to use floats for the math where angles are divided 
	// by fixed values.  
	double     fangledist, fanglestep, fmovestep;
	int	     angledir;  
	AActor*    target;
	int        moved;
	int        quad1, quad2;
	AActor		* cam;
	
	angledir = moved = 0;

	if (CheckArgs(6))
	{
		cam = MobjForSvalue(t_argv[0]);

		target = MobjForSvalue(t_argv[1]);
		if(!cam || !target) 
		{ 
			script_error("invalid target for camera\n"); return; 
		}
		
		targetheight = fixedvalue(t_argv[2]);
		movespeed    = fixedvalue(t_argv[3]);
		targetangle  = (angle_t)FixedToAngle(fixedvalue(t_argv[4]));
		anglespeed   = (angle_t)FixedToAngle(fixedvalue(t_argv[5]));
		
		// figure out how big one step will be
		xdist = target->x - cam->x;
		ydist = target->y - cam->y;
		zdist = targetheight - cam->z;
		
		// Angle checking...  
		//    90  
		//   Q1|Q0  
		//180--+--0  
		//   Q2|Q3  
		//    270
		quad1 = targetangle / ANG90;
		quad2 = cam->angle / ANG90;
		bigangle = targetangle > cam->angle ? targetangle : cam->angle;
		smallangle = targetangle < cam->angle ? targetangle : cam->angle;
		if((quad1 > quad2 && quad1 - 1 == quad2) || (quad2 > quad1 && quad2 - 1 == quad1) ||
			quad1 == quad2)
		{
			angledist = bigangle - smallangle;
			angledir = targetangle > cam->angle ? 1 : -1;
		}
		else
		{
			angle_t diff180 = (bigangle + ANG180) - (smallangle + ANG180);
			
			if(quad2 == 3 && quad1 == 0)
			{
				angledist = diff180;
				angledir = 1;
			}
			else if(quad1 == 3 && quad2 == 0)
			{
				angledist = diff180;
				angledir = -1;
			}
			else
			{
				angledist = bigangle - smallangle;
				if(angledist > ANG180)
				{
					angledist = diff180;
					angledir = targetangle > cam->angle ? -1 : 1;
				}
				else
					angledir = targetangle > cam->angle ? 1 : -1;
			}
		}
		
		// set step variables based on distance and speed
		mobjangle = R_PointToAngle2(cam->x, cam->y, target->x, target->y);
		xydist = R_PointToDist2(target->x - cam->x, target->y - cam->y);
		
		xstep = FixedMul(finecosine[mobjangle >> ANGLETOFINESHIFT], movespeed);
		ystep = FixedMul(finesine[mobjangle >> ANGLETOFINESHIFT], movespeed);
		
		if(xydist && movespeed)
			zstep = FixedDiv(zdist, FixedDiv(xydist, movespeed));
		else
			zstep = zdist > 0 ? movespeed : -movespeed;
		
		if(xydist && movespeed && !anglespeed)
		{
			fangledist = ((double)angledist / (ANG45/45));
			fmovestep = ((double)FixedDiv(xydist, movespeed) / FRACUNIT);
			if(fmovestep)
				fanglestep = fangledist / fmovestep;
			else
				fanglestep = 360;
			
			anglestep =(angle_t) (fanglestep * (ANG45/45));
		}
		else
			anglestep = anglespeed;
		
		if(abs(xstep) >= (abs(xdist) - 1))
			x = target->x;
		else
		{
			x = cam->x + xstep;
			moved = 1;
		}
		
		if(abs(ystep) >= (abs(ydist) - 1))
			y = target->y;
		else
		{
			y = cam->y + ystep;
			moved = 1;
		}
		
		if(abs(zstep) >= (abs(zdist) - 1))
			z = targetheight;
		else
		{
			z = cam->z + zstep;
			moved = 1;
		}
		
		if(anglestep >= angledist)
			cam->angle = targetangle;
		else
		{
			if(angledir == 1)
			{
				cam->angle += anglestep;
				moved = 1;
			}
			else if(angledir == -1)
			{
				cam->angle -= anglestep;
				moved = 1;
			}
		}

		cam->radius=8;
		cam->height=8;
		if ((x != cam->x || y != cam->y) && !P_TryMove(cam, x, y, true))
		{
			Printf("Illegal camera move to (%f, %f)\n", x/65536.f, y/65536.f);
			return;
		}
		cam->z = z;

		t_return.type = svt_int;
		t_return.value.i = moved;
	}
}



//==========================================================================
//
// SF_ObjAwaken
//
// Implements: void objawaken([mobj mo])
//
//==========================================================================

void DFraggleThinker::SF_ObjAwaken()
{
   AActor *mo;

   if(!t_argc)
      mo = current_script->trigger;
   else
      mo = MobjForSvalue(t_argv[0]);

   if(mo)
   {
	   mo->Activate(current_script->trigger);
   }
}

//==========================================================================
//
// SF_AmbientSound
//
// Implements: void ambientsound(string name)
//
//==========================================================================

void DFraggleThinker::SF_AmbientSound()
{
	if (CheckArgs(1))
	{
		S_SoundID(CHAN_AUTO, T_FindSound(stringvalue(t_argv[0])), 1, ATTN_NORM);
	}
}


//==========================================================================
// 
// SF_ExitSecret
//
// Implements: void exitsecret()
//
//==========================================================================

void DFraggleThinker::SF_ExitSecret()
{
	G_ExitLevel(0, false);
}


//==========================================================================
//
//
//
//==========================================================================

// Type forcing functions -- useful with arrays et al

void DFraggleThinker::SF_MobjValue()
{
	if (CheckArgs(1))
	{
		t_return.type = svt_mobj;
		t_return.value.mobj = MobjForSvalue(t_argv[0]);
	}
}

void DFraggleThinker::SF_StringValue()
{  
	if (CheckArgs(1))
	{
		t_return.type = svt_string;
		t_return.value.s = strdup(stringvalue(t_argv[0]));
		// this cannot be handled in any sensible way - yuck!
		levelpointers.Push(t_return.value.s);
	}
}

void DFraggleThinker::SF_IntValue()
{
	if (CheckArgs(1))
	{
		t_return.type = svt_int;
		t_return.value.i = intvalue(t_argv[0]);
	}
}

void DFraggleThinker::SF_FixedValue()
{
	if (CheckArgs(1))
	{
		t_return.type = svt_fixed;
		t_return.value.f = fixedvalue(t_argv[0]);
	}
}


//==========================================================================
//
// Starting here are functions present in Legacy but not Eternity.
//
//==========================================================================

void DFraggleThinker::SF_SpawnExplosion()
{
	fixed_t   x, y, z;
	AActor*   spawn;
	const PClass * PClass;
	
	if (CheckArgs(3))
	{
		if (!(PClass=T_GetMobjType(t_argv[0]))) return;
		
		x = fixedvalue(t_argv[1]);
		y = fixedvalue(t_argv[2]);
		if(t_argc > 3)
			z = fixedvalue(t_argv[3]);
		else
			z = P_PointInSector(x, y)->floorplane.ZatPoint(x,y);
		
		spawn = Spawn (PClass, x, y, z, ALLOW_REPLACE);
		t_return.type = svt_int;
		t_return.value.i=0;
		if (spawn)
		{
			if (spawn->flags&MF_COUNTKILL) level.total_monsters--;
			if (spawn->flags&MF_COUNTITEM) level.total_items--;
			spawn->flags&=~(MF_COUNTKILL|MF_COUNTITEM);
			t_return.value.i = spawn->SetState(spawn->FindState(NAME_Death));
			if(spawn->DeathSound) S_SoundID (spawn, CHAN_BODY, spawn->DeathSound, 1, ATTN_NORM);
		}
	}
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_RadiusAttack()
{
    AActor *spot;
    AActor *source;
    int damage;

	if (CheckArgs(3))
	{
		spot = MobjForSvalue(t_argv[0]);
		source = MobjForSvalue(t_argv[1]);
		damage = intvalue(t_argv[2]);

		if (spot && source)
		{
			P_RadiusAttack(spot, source, damage, damage, NAME_None, true);
		}
	}
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_SetObjPosition()
{
	AActor* mobj;

	if (CheckArgs(2))
	{
		mobj = MobjForSvalue(t_argv[0]);

		if (!mobj) return;

		mobj->UnlinkFromWorld();

		mobj->x = intvalue(t_argv[1]) << FRACBITS;
		if(t_argc >= 3) mobj->y = intvalue(t_argv[2]) << FRACBITS;
		if(t_argc == 4)	mobj->z = intvalue(t_argv[3]) << FRACBITS;

		mobj->LinkToWorld();
	}
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_TestLocation()
{
    AActor *mo = t_argc ? MobjForSvalue(t_argv[0]) : current_script->trigger;

    if (mo)
	{
       t_return.type = svt_int;
       t_return.value.f = !!P_TestMobjLocation(mo);
    }
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_HealObj()  //no pain sound
{
	AActor *mo = t_argc ? MobjForSvalue(t_argv[0]) : current_script->trigger;

	if(t_argc < 2)
	{
		mo->health = mo->GetDefault()->health;
		if(mo->player) mo->player->health = mo->health;
	}

	else if (t_argc == 2)
	{
		mo->health += intvalue(t_argv[1]);
		if(mo->player) mo->player->health = mo->health;
	}

	else
		script_error("invalid number of arguments for objheal");
}


//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_ObjDead()
{
	AActor *mo = t_argc ? MobjForSvalue(t_argv[0]) : current_script->trigger;
	
	t_return.type = svt_int;
	if(mo && (mo->health <= 0 || mo->flags&MF_CORPSE))
		t_return.value.i = 1;
	else
		t_return.value.i = 0;
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_SpawnMissile()
{
    AActor *mobj;
    AActor *target;
	const PClass * PClass;

	if (CheckArgs(3))
	{
		if (!(PClass=T_GetMobjType(t_argv[2]))) return;

		mobj = MobjForSvalue(t_argv[0]);
		target = MobjForSvalue(t_argv[1]);
		if (mobj && target) P_SpawnMissile(mobj, target, PClass);
	}
}

//==========================================================================
//
//checks to see if a Map Thing Number exists; used to avoid script errors
//
//==========================================================================

void DFraggleThinker::SF_MapThingNumExist()
{

    int intval;

	if (CheckArgs(1))
	{
		intval = intvalue(t_argv[0]);

		if (intval < 0 || intval >= SpawnedThings.Size() || !SpawnedThings[intval]->actor)
		{
			t_return.type = svt_int;
			t_return.value.i = 0;
		}
		else
		{
			t_return.type = svt_int;
			t_return.value.i = 1;
		}
	}
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_MapThings()
{
    t_return.type = svt_int;
    t_return.value.i = SpawnedThings.Size();
}


//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_ObjState()
{
	int state;
	AActor	*mo;

	if (CheckArgs(1))
	{
		if(t_argc == 1)
		{
			mo = current_script->trigger;
			state = intvalue(t_argv[0]);
		}

		else if(t_argc == 2)
		{
			mo = MobjForSvalue(t_argv[0]);
			state = intvalue(t_argv[1]);
		}

		if (mo) 
		{
			static ENamedName statenames[]= {
				NAME_Spawn, NAME_See, NAME_Missile,	NAME_Melee,
				NAME_Pain, NAME_Death, NAME_Raise, NAME_XDeath, NAME_Crash };

			if (state <1 || state > 9)
			{
				script_error("objstate: invalid state");
				return;
			}

			t_return.type = svt_int;
			t_return.value.i = mo->SetState(mo->FindState(statenames[state]));
		}
	}
}


//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_LineFlag()
{
	line_t*  line;
	int      linenum;
	int      flagnum;
	
	if (CheckArgs(2))
	{
		linenum = intvalue(t_argv[0]);
		if(linenum < 0 || linenum > numlines)
		{
			script_error("LineFlag: Invalid line number.\n");
			return;
		}
		
		line = lines + linenum;
		
		flagnum = intvalue(t_argv[1]);
		if(flagnum < 0 || (flagnum > 8 && flagnum!=15))
		{
			script_error("LineFlag: Invalid flag number.\n");
			return;
		}
		
		if(t_argc > 2)
		{
			line->flags &= ~(1 << flagnum);
			if(intvalue(t_argv[2]))
				line->flags |= (1 << flagnum);
		}
		
		t_return.type = svt_int;
		t_return.value.i = line->flags & (1 << flagnum);
	}
}


//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_PlayerAddFrag()
{
	int playernum1;
	int playernum2;

	if (CheckArgs(1))
	{
		if (t_argc == 1)
		{
			playernum1 = T_GetPlayerNum(t_argv[0]);

			players[playernum1].fragcount++;

			t_return.type = svt_int;
			t_return.value.f = players[playernum1].fragcount;
		}

		else
		{
			playernum1 = T_GetPlayerNum(t_argv[0]);
			playernum2 = T_GetPlayerNum(t_argv[1]);

			players[playernum1].frags[playernum2]++;

			t_return.type = svt_int;
			t_return.value.f = players[playernum1].frags[playernum2];
		}
	}
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_SkinColor()
{
	// Ignoring it for now.
}

void DFraggleThinker::SF_PlayDemo()
{ 
	// Ignoring it for now.
}

void DFraggleThinker::SF_CheckCVar()
{
	// can't be done so return 0.
}
//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_Resurrect()
{

	AActor *mo;

	if (CheckArgs(1))
	{
		mo = MobjForSvalue(t_argv[0]);

		FState * state = mo->FindState(NAME_Raise);
		if (!state)  //Don't resurrect things that can't be resurrected
			return;

		mo->SetState(state);
		mo->height = mo->GetDefault()->height;
		mo->radius = mo->GetDefault()->radius;
		mo->flags =  mo->GetDefault()->flags;
		mo->flags2 = mo->GetDefault()->flags2;
		mo->health = mo->GetDefault()->health;
		mo->target = NULL;
	}
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_LineAttack()
{
	AActor	*mo;
	int		damage, angle, slope;

	if (CheckArgs(3))
	{
		mo = MobjForSvalue(t_argv[0]);
		damage = intvalue(t_argv[2]);

		angle = (intvalue(t_argv[1]) * (ANG45 / 45));
		slope = P_AimLineAttack(mo, angle, MISSILERANGE);

		P_LineAttack(mo, angle, MISSILERANGE, slope, damage, NAME_None, NAME_BulletPuff);
	}
}

//==========================================================================
//
// This is a lousy hack. It only works for the standard actors
// and it is quite inefficient.
//
//==========================================================================

void DFraggleThinker::SF_ObjType()
{
	// use trigger object if not specified
	AActor *mo = t_argc ? MobjForSvalue(t_argv[0]) : current_script->trigger;

	for(int i=0;ActorTypes[i];i++) if (mo->GetClass() == ActorTypes[i])
	{
		t_return.type = svt_int;
		t_return.value.i = i;
		return;
	}
	t_return.type = svt_int;
	t_return.value.i = -1;
}

//==========================================================================
//
// some new math functions
//
//==========================================================================

inline fixed_t double2fixed(double t)
{
	return (fixed_t)(t*65536.0);
}



void DFraggleThinker::SF_Sin()
{
	if (CheckArgs(1))
	{
		t_return.type = svt_fixed;
		t_return.value.f = double2fixed(sin(floatvalue(t_argv[0])));
	}
}


void DFraggleThinker::SF_ASin()
{
	if (CheckArgs(1))
	{
		t_return.type = svt_fixed;
		t_return.value.f = double2fixed(asin(floatvalue(t_argv[0])));
	}
}


void DFraggleThinker::SF_Cos()
{
	if (CheckArgs(1))
	{
		t_return.type = svt_fixed;
		t_return.value.f = double2fixed(cos(floatvalue(t_argv[0])));
	}
}


void DFraggleThinker::SF_ACos()
{
	if (CheckArgs(1))
	{
		t_return.type = svt_fixed;
		t_return.value.f = double2fixed(acos(floatvalue(t_argv[0])));
	}
}


void DFraggleThinker::SF_Tan()
{
	if (CheckArgs(1))
	{
		t_return.type = svt_fixed;
		t_return.value.f = double2fixed(tan(floatvalue(t_argv[0])));
	}
}


void DFraggleThinker::SF_ATan()
{
	if (CheckArgs(1))
	{
		t_return.type = svt_fixed;
		t_return.value.f = double2fixed(atan(floatvalue(t_argv[0])));
	}
}


void DFraggleThinker::SF_Exp()
{
	if (CheckArgs(1))
	{
		t_return.type = svt_fixed;
		t_return.value.f = double2fixed(exp(floatvalue(t_argv[0])));
	}
}

void DFraggleThinker::SF_Log()
{
	if (CheckArgs(1))
	{
		t_return.type = svt_fixed;
		t_return.value.f = double2fixed(log(floatvalue(t_argv[0])));
	}
}


void DFraggleThinker::SF_Sqrt()
{
	if (CheckArgs(1))
	{
		t_return.type = svt_fixed;
		t_return.value.f = double2fixed(sqrt(floatvalue(t_argv[0])));
	}
}


void DFraggleThinker::SF_Floor()
{
	if (CheckArgs(1))
	{
		t_return.type = svt_fixed;
		t_return.value.f = fixedvalue(t_argv[0]) & 0xffFF0000;
	}
}


void DFraggleThinker::SF_Pow()
{
	if (CheckArgs(2))
	{
		t_return.type = svt_fixed;
		t_return.value.f = double2fixed(pow(floatvalue(t_argv[0]), floatvalue(t_argv[1])));
	}
}

//==========================================================================
//
// HUD pics (not operational yet!)
//
//==========================================================================


int HU_GetFSPic(int lumpnum, int xpos, int ypos);
int HU_DeleteFSPic(unsigned int handle);
int HU_ModifyFSPic(unsigned int handle, int lumpnum, int xpos, int ypos);
int HU_FSDisplay(unsigned int handle, bool newval);

void DFraggleThinker::SF_NewHUPic()
{
	if (CheckArgs(3))
	{
		t_return.type = svt_int;
		t_return.value.i = HU_GetFSPic(
			TexMan.GetTexture(stringvalue(t_argv[0]), FTexture::TEX_MiscPatch, FTextureManager::TEXMAN_TryAny), 
			intvalue(t_argv[1]), intvalue(t_argv[2]));
	}
}

void DFraggleThinker::SF_DeleteHUPic()
{
	if (CheckArgs(1))
	{
	    if (HU_DeleteFSPic(intvalue(t_argv[0])) == -1)
		    script_error("deletehupic: Invalid sfpic handle: %i\n", intvalue(t_argv[0]));
	}
}

void DFraggleThinker::SF_ModifyHUPic()
{
    if (t_argc != 4)
    {
        script_error("modifyhupic: invalid number of arguments\n");
        return;
    }

    if (HU_ModifyFSPic(intvalue(t_argv[0]), 
			TexMan.GetTexture(stringvalue(t_argv[0]), FTexture::TEX_MiscPatch, FTextureManager::TEXMAN_TryAny),
			intvalue(t_argv[2]), intvalue(t_argv[3])) == -1)
	{
        script_error("modifyhypic: invalid sfpic handle %i\n", intvalue(t_argv[0]));
	}
    return;
}

void DFraggleThinker::SF_SetHUPicDisplay()
{
    if (t_argc != 2)
    {
        script_error("sethupicdisplay: invalud number of arguments\n");
        return;
    }

    if (HU_FSDisplay(intvalue(t_argv[0]), intvalue(t_argv[1]) > 0 ? 1 : 0) == -1)
        script_error("sethupicdisplay: invalid pic handle %i\n", intvalue(t_argv[0]));
}


//==========================================================================
//
// Yet to be made operational.
//
//==========================================================================

void DFraggleThinker::SF_SetCorona()
{
	if(t_argc != 3)
	{
		script_error("incorrect arguments to function\n");
		return;
	}
	
    int num = t_argv[0].value.i;    // which corona we want to modify
    int what = t_argv[1].value.i;   // what we want to modify (type, color, offset,...)
    int ival = t_argv[2].value.i;   // the value of what we modify
    double fval = ((double) t_argv[2].value.f / FRACUNIT);

  	/*
    switch (what)
    {
        case 0:
            lspr[num].type = ival;
            break;
        case 1:
            lspr[num].light_xoffset = fval;
            break;
        case 2:
            lspr[num].light_yoffset = fval;
            break;
        case 3:
            if (t_argv[2].type == svt_string)
                lspr[num].corona_color = String2Hex(t_argv[2].value.s);
            else
                memcpy(&lspr[num].corona_color, &ival, sizeof(int));
            break;
        case 4:
            lspr[num].corona_radius = fval;
            break;
        case 5:
            if (t_argv[2].type == svt_string)
                lspr[num].dynamic_color = String2Hex(t_argv[2].value.s);
            else
                memcpy(&lspr[num].dynamic_color, &ival, sizeof(int));
            break;
        case 6:
            lspr[num].dynamic_radius = fval;
            lspr[num].dynamic_sqrradius = sqrt(lspr[num].dynamic_radius);
            break;
        default:
            CONS_Printf("Error in setcorona\n");
            break;
    }
    */

	// no use for this!
	t_return.type = svt_int;
	t_return.value.i = 0;
}

//==========================================================================
//
// new for GZDoom: Call a Hexen line special
//
//==========================================================================

void DFraggleThinker::SF_Ls()
{
	int args[5]={0,0,0,0,0};
	int spc;

	if (CheckArgs(1))
	{
		spc=intvalue(t_argv[0]);
		for(int i=0;i<5;i++)
		{
			if (t_argc>=i+2) args[i]=intvalue(t_argv[i+1]);
		}
		if (spc>=0 && spc<256)
			LineSpecials[spc](NULL,current_script->trigger,false, args[0],args[1],args[2],args[3],args[4]);
	}
}


//==========================================================================
//
// new for GZDoom: Gets the levelnum
//
//==========================================================================

void DFraggleThinker::SF_LevelNum()
{
	t_return.type = svt_int;
	t_return.value.f = level.levelnum;
}


//==========================================================================
//
// new for GZDoom
//
//==========================================================================

void DFraggleThinker::SF_MobjRadius()
{
	AActor*   mo;
	
	if (CheckArgs(1))
	{
		mo = MobjForSvalue(t_argv[0]);
		if(t_argc > 1)
		{
			if(mo) 
				mo->radius = fixedvalue(t_argv[1]);
		}
		
		t_return.type = svt_fixed;
		t_return.value.f = mo ? mo->radius : 0;
	}
}


//==========================================================================
//
// new for GZDoom
//
//==========================================================================

void DFraggleThinker::SF_MobjHeight()
{
	AActor*   mo;
	
	if (CheckArgs(1))
	{
		mo = MobjForSvalue(t_argv[0]);
		if(t_argc > 1)
		{
			if(mo) 
				mo->height = fixedvalue(t_argv[1]);
		}
		
		t_return.type = svt_fixed;
		t_return.value.f = mo ? mo->height : 0;
	}
}


//==========================================================================
//
// new for GZDoom
//
//==========================================================================

void DFraggleThinker::SF_ThingCount()
{
	const PClass *pClass;
	AActor * mo;
	int count=0;
	bool replacemented = false;

	
	if (CheckArgs(1))
	{
		if (!(pClass=T_GetMobjType(t_argv[0]))) return;
		// If we want to count map items we must consider actor replacement
		pClass = pClass->ActorInfo->GetReplacement()->Class;
		
again:
		TThinkerIterator<AActor> it;

		if (t_argc<2 || intvalue(t_argv[1])==0 || pClass->IsDescendantOf(RUNTIME_CLASS(AInventory)))
		{
			while (mo=it.Next()) 
			{
				if (mo->IsA(pClass))
				{
					if (!mo->IsKindOf (RUNTIME_CLASS(AInventory)) ||
						static_cast<AInventory *>(mo)->Owner == NULL)
					{
						count++;
					}
				}
			}
		}
		else
		{
			while (mo=it.Next())
			{
				if (mo->IsA(pClass) && mo->health>0) count++;
			}
		}
		if (!replacemented)
		{
			// Again, with decorate replacements
			replacemented = true;
			PClass *newkind = pClass->ActorInfo->GetReplacement()->Class;
			if (newkind != pClass)
			{
				pClass = newkind;
				goto again;
			}
		}
		t_return.type = svt_int;
		t_return.value.i = count;
	}	
}

//==========================================================================
//
// new for GZDoom: Sets a sector color
//
//==========================================================================

void DFraggleThinker::SF_SetColor()
{
	int tagnum, secnum;
	int c=2;
	int i = -1;
	PalEntry color=0;
	
	if (CheckArgs(2))
	{
		tagnum = intvalue(t_argv[0]);
		
		secnum = T_FindSectorFromTag(tagnum, -1);
		
		if(secnum < 0)
		{ 
			return;
		}
		
		if(t_argc >1 && t_argc<4)
		{
			color=intvalue(t_argv[1]);
		}
		else if (t_argc>=4)
		{
			color.r=intvalue(t_argv[1]);
			color.g=intvalue(t_argv[2]);
			color.b=intvalue(t_argv[3]);
			color.a=0;
		}
		else return;

		// set all sectors with tag
		while ((i = T_FindSectorFromTag(tagnum, i)) >= 0)
		{
			sectors[i].ColorMap = GetSpecialLights (color, sectors[i].ColorMap->Fade, 0);
		}
	}
}


//==========================================================================
//
// Spawns a projectile at a map spot
//
//==========================================================================

void DFraggleThinker::SF_SpawnShot2()
{
	AActor *source = NULL;
	const PClass * PClass;
	int z=0;
	
	// t_argv[0] = type to spawn
	// t_argv[1] = source mobj, optional, -1 to default
	// shoots at source's angle
	
	if (CheckArgs(2))
	{
		if(t_argv[1].type == svt_int && t_argv[1].value.i < 0)
			source = current_script->trigger;
		else
			source = MobjForSvalue(t_argv[1]);

		if (t_argc>2) z=fixedvalue(t_argv[2]);
		
		if(!source)	return;
		
		if (!(PClass=T_GetMobjType(t_argv[0]))) return;
		
		t_return.type = svt_mobj;

		AActor *mo = Spawn (PClass, source->x, source->y, source->z+z, ALLOW_REPLACE);
		if (mo) 
		{
			S_SoundID (mo, CHAN_VOICE, mo->SeeSound, 1, ATTN_NORM);
			mo->target = source;
			P_ThrustMobj(mo, mo->angle = source->angle, mo->Speed);
			if (!P_CheckMissileSpawn(mo)) mo = NULL;
		}
		t_return.value.mobj = mo;
	}
}



//==========================================================================
//
// new for GZDoom
//
//==========================================================================

void DFraggleThinker::SF_KillInSector()
{
	if (CheckArgs(1))
	{
		TThinkerIterator<AActor> it;
		AActor * mo;
		int tag=intvalue(t_argv[0]);

		while (mo=it.Next())
		{
			if (mo->flags3&MF3_ISMONSTER && mo->Sector->tag==tag) P_DamageMobj(mo, NULL, NULL, 1000000, NAME_Massacre);
		}
	}
}

//==========================================================================
//
// new for GZDoom: Sets a sector's type
// (Sure, this is not particularly useful. But having it made it possible
//  to fix a few annoying bugs in some old maps ;) )
//
//==========================================================================

void DFraggleThinker::SF_SectorType()
{
	int tagnum, secnum;
	sector_t *sector;
	
	if (CheckArgs(1))
	{
		tagnum = intvalue(t_argv[0]);
		
		// argv is sector tag
		secnum = T_FindSectorFromTag(tagnum, -1);
		
		if(secnum < 0)
		{ script_error("sector not found with tagnum %i\n", tagnum); return;}
		
		sector = &sectors[secnum];
		
		if(t_argc > 1)
		{
			int i = -1;
			int spec = intvalue(t_argv[1]);
			
			// set all sectors with tag
			while ((i = T_FindSectorFromTag(tagnum, i)) >= 0)
			{
				sectors[i].special = spec;
			}
		}
		
		t_return.type = svt_int;
		t_return.value.i = sector->special;
	}
}

//==========================================================================
//
// new for GZDoom: Sets a new line trigger type (Doom format!)
// (Sure, this is not particularly useful. But having it made it possible
//  to fix a few annoying bugs in some old maps ;) )
//
//==========================================================================

void DFraggleThinker::SF_SetLineTrigger()
{
	int i,id,spec,tag;

	if (CheckArgs(2))
	{
		id=intvalue(t_argv[0]);
		spec=intvalue(t_argv[1]);
		if (t_argc>2) tag=intvalue(t_argv[2]);
		for (i = -1; (i = P_FindLineFromID (id, i)) >= 0; )
		{
			if (t_argc==2) tag=lines[i].id;
			maplinedef_t mld;
			mld.special=spec;
			mld.tag=tag;
			mld.flags=0;
			int f = lines[i].flags;
			P_TranslateLineDef(&lines[i], &mld);	
			lines[i].id=tag;
			lines[i].flags = (lines[i].flags & (ML_MONSTERSCANACTIVATE|ML_REPEAT_SPECIAL|ML_SPAC_MASK)) |
										(f & ~(ML_MONSTERSCANACTIVATE|ML_REPEAT_SPECIAL|ML_SPAC_MASK));

		}
	}
}


//==========================================================================
//
// new for GZDoom: Changes a sector's tag
// (I only need this because MAP02 in RTC-3057 has some issues with the GL 
// renderer that I can't fix without the scripts. But loading a FS on top on
// ACS still works so I can hack around it with this.)
//
//==========================================================================

void P_InitTagLists();

void DFraggleThinker::SF_ChangeTag()
{
	if (CheckArgs(2))
	{
		for (int secnum = -1; (secnum = P_FindSectorFromTag (t_argv[0].value.i, secnum)) >= 0; ) 
		{
			sectors[secnum].tag=t_argv[1].value.i;
		}

		// Recreate the hash tables
		int i;

		for (i=numsectors; --i>=0; ) sectors[i].firsttag = -1;
		for (i=numsectors; --i>=0; )
		{
			int j = (unsigned) sectors[i].tag % (unsigned) numsectors;
			sectors[i].nexttag = sectors[j].firsttag;
			sectors[j].firsttag = i;
		}
	}
}


void DFraggleThinker::SF_WallGlow()
{
	// Development garbage!
}



//////////////////////////////////////////////////////////////////////////
//
// Init Functions
//
static int zoom=1;	// Dummy - no longer needed!

void DFraggleThinker::init_functions()
{
	// add all the functions
	add_game_int("consoleplayer", &consoleplayer);
	add_game_int("displayplayer", &consoleplayer);
	add_game_int("zoom", &zoom);
	add_game_int("fov", &zoom); // haleyjd: temporary alias (?)
	add_game_mobj("trigger", &trigger_obj);
	
	// important C-emulating stuff
	new_function("break", &DFraggleThinker::SF_Break);
	new_function("continue", &DFraggleThinker::SF_Continue);
	new_function("return", &DFraggleThinker::SF_Return);
	new_function("goto", &DFraggleThinker::SF_Goto);
	new_function("include", &DFraggleThinker::SF_Include);
	
	// standard FraggleScript functions
	new_function("print", &DFraggleThinker::SF_Print);
	new_function("rnd", &DFraggleThinker::SF_Rnd);	// Legacy uses a normal rand() call for this which is extremely dangerous.
	new_function("prnd", &DFraggleThinker::SF_Rnd);	// I am mapping rnd and prnd to the same named RNG which should eliminate any problem
	new_function("input", &DFraggleThinker::SF_Input);
	new_function("beep", &DFraggleThinker::SF_Beep);
	new_function("clock", &DFraggleThinker::SF_Clock);
	new_function("wait", &DFraggleThinker::SF_Wait);
	new_function("tagwait", &DFraggleThinker::SF_TagWait);
	new_function("scriptwait", &DFraggleThinker::SF_ScriptWait);
	new_function("startscript", &DFraggleThinker::SF_StartScript);
	new_function("scriptrunning", &DFraggleThinker::SF_ScriptRunning);
	
	// doom stuff
	new_function("startskill", &DFraggleThinker::SF_StartSkill);
	new_function("exitlevel", &DFraggleThinker::SF_ExitLevel);
	new_function("tip", &DFraggleThinker::SF_Tip);
	new_function("timedtip", &DFraggleThinker::SF_TimedTip);
	new_function("message", &DFraggleThinker::SF_Message);
	new_function("gameskill", &DFraggleThinker::SF_Gameskill);
	new_function("gamemode", &DFraggleThinker::SF_Gamemode);
	
	// player stuff
	new_function("playermsg", &DFraggleThinker::SF_PlayerMsg);
	new_function("playertip", &DFraggleThinker::SF_PlayerTip);
	new_function("playeringame", &DFraggleThinker::SF_PlayerInGame);
	new_function("playername", &DFraggleThinker::SF_PlayerName);
    new_function("playeraddfrag", &DFraggleThinker::SF_PlayerAddFrag);
	new_function("playerobj", &DFraggleThinker::SF_PlayerObj);
	new_function("isplayerobj", &DFraggleThinker::SF_IsPlayerObj);
	new_function("isobjplayer", &DFraggleThinker::SF_IsPlayerObj);
	new_function("skincolor", &DFraggleThinker::SF_SkinColor);
	new_function("playerkeys", &DFraggleThinker::SF_PlayerKeys);
	new_function("playerammo", &DFraggleThinker::SF_PlayerAmmo);
    new_function("maxplayerammo", &DFraggleThinker::SF_MaxPlayerAmmo); 
	new_function("playerweapon", &DFraggleThinker::SF_PlayerWeapon);
	new_function("playerselwep", &DFraggleThinker::SF_PlayerSelectedWeapon);
	
	// mobj stuff
	new_function("spawn", &DFraggleThinker::SF_Spawn);
	new_function("spawnexplosion", &DFraggleThinker::SF_SpawnExplosion);
    new_function("radiusattack", &DFraggleThinker::SF_RadiusAttack);
	new_function("kill", &DFraggleThinker::SF_KillObj);
	new_function("removeobj", &DFraggleThinker::SF_RemoveObj);
	new_function("objx", &DFraggleThinker::SF_ObjX);
	new_function("objy", &DFraggleThinker::SF_ObjY);
	new_function("objz", &DFraggleThinker::SF_ObjZ);
    new_function("testlocation", &DFraggleThinker::SF_TestLocation);
	new_function("teleport", &DFraggleThinker::SF_Teleport);
	new_function("silentteleport", &DFraggleThinker::SF_SilentTeleport);
	new_function("damageobj", &DFraggleThinker::SF_DamageObj);
	new_function("healobj", &DFraggleThinker::SF_HealObj);
	new_function("player", &DFraggleThinker::SF_Player);
	new_function("objsector", &DFraggleThinker::SF_ObjSector);
	new_function("objflag", &DFraggleThinker::SF_ObjFlag);
	new_function("pushobj", &DFraggleThinker::SF_PushThing);
	new_function("pushthing", &DFraggleThinker::SF_PushThing);
	new_function("objangle", &DFraggleThinker::SF_ObjAngle);
	new_function("objhealth", &DFraggleThinker::SF_ObjHealth);
	new_function("objdead", &DFraggleThinker::SF_ObjDead);
	new_function("reactiontime", &DFraggleThinker::SF_ReactionTime);
	new_function("objreactiontime", &DFraggleThinker::SF_ReactionTime);
	new_function("objtarget", &DFraggleThinker::SF_MobjTarget);
	new_function("objmomx", &DFraggleThinker::SF_MobjMomx);
	new_function("objmomy", &DFraggleThinker::SF_MobjMomy);
	new_function("objmomz", &DFraggleThinker::SF_MobjMomz);

    new_function("spawnmissile", &DFraggleThinker::SF_SpawnMissile);
    new_function("mapthings", &DFraggleThinker::SF_MapThings);
    new_function("objtype", &DFraggleThinker::SF_ObjType);
    new_function("mapthingnumexist", &DFraggleThinker::SF_MapThingNumExist);
	new_function("objstate", &DFraggleThinker::SF_ObjState);
	new_function("resurrect", &DFraggleThinker::SF_Resurrect);
	new_function("lineattack", &DFraggleThinker::SF_LineAttack);
	new_function("setobjposition", &DFraggleThinker::SF_SetObjPosition);

	// sector stuff
	new_function("floorheight", &DFraggleThinker::SF_FloorHeight);
	new_function("floortext", &DFraggleThinker::SF_FloorTexture);
	new_function("floortexture", &DFraggleThinker::SF_FloorTexture);   // haleyjd: alias
	new_function("movefloor", &DFraggleThinker::SF_MoveFloor);
	new_function("ceilheight", &DFraggleThinker::SF_CeilingHeight);
	new_function("ceilingheight", &DFraggleThinker::SF_CeilingHeight); // haleyjd: alias
	new_function("moveceil", &DFraggleThinker::SF_MoveCeiling);
	new_function("moveceiling", &DFraggleThinker::SF_MoveCeiling);     // haleyjd: aliases
	new_function("ceilingtexture", &DFraggleThinker::SF_CeilingTexture);
	new_function("ceiltext", &DFraggleThinker::SF_CeilingTexture);  // haleyjd: wrong
	new_function("lightlevel", &DFraggleThinker::SF_LightLevel);    // handler - was
	new_function("fadelight", &DFraggleThinker::SF_FadeLight);      // &DFraggleThinker::SF_FloorTexture!
	new_function("colormap", &DFraggleThinker::SF_SectorColormap);
	
	// cameras!
	new_function("setcamera", &DFraggleThinker::SF_SetCamera);
	new_function("clearcamera", &DFraggleThinker::SF_ClearCamera);
	new_function("movecamera", &DFraggleThinker::SF_MoveCamera);
	
	// trig functions
	new_function("pointtoangle", &DFraggleThinker::SF_PointToAngle);
	new_function("pointtodist", &DFraggleThinker::SF_PointToDist);
	
	// sound functions
	new_function("startsound", &DFraggleThinker::SF_StartSound);
	new_function("startsectorsound", &DFraggleThinker::SF_StartSectorSound);
	new_function("ambientsound", &DFraggleThinker::SF_AmbientSound);
	new_function("startambiantsound", &DFraggleThinker::SF_AmbientSound);	// Legacy's incorrectly spelled name!
	new_function("changemusic", &DFraggleThinker::SF_ChangeMusic);
	
	// hubs!
	new_function("changehublevel", &DFraggleThinker::SF_ChangeHubLevel);
	
	// doors
	new_function("opendoor", &DFraggleThinker::SF_OpenDoor);
	new_function("closedoor", &DFraggleThinker::SF_CloseDoor);

    // HU Graphics
    new_function("newhupic", &DFraggleThinker::SF_NewHUPic);
    new_function("createpic", &DFraggleThinker::SF_NewHUPic);
    new_function("deletehupic", &DFraggleThinker::SF_DeleteHUPic);
    new_function("modifyhupic", &DFraggleThinker::SF_ModifyHUPic);
    new_function("modifypic", &DFraggleThinker::SF_ModifyHUPic);
    new_function("sethupicdisplay", &DFraggleThinker::SF_SetHUPicDisplay);
    new_function("setpicvisible", &DFraggleThinker::SF_SetHUPicDisplay);

	//
    new_function("playdemo", &DFraggleThinker::SF_PlayDemo);
	new_function("runcommand", &DFraggleThinker::SF_RunCommand);
    new_function("checkcvar", &DFraggleThinker::SF_CheckCVar);
	new_function("setlinetexture", &DFraggleThinker::SF_SetLineTexture);
	new_function("linetrigger", &DFraggleThinker::SF_LineTrigger);
	new_function("lineflag", &DFraggleThinker::SF_LineFlag);

	//Hurdler: new math functions
	new_function("max", &DFraggleThinker::SF_Max);
	new_function("min", &DFraggleThinker::SF_Min);
	new_function("abs", &DFraggleThinker::SF_Abs);

	new_function("sin", &DFraggleThinker::SF_Sin);
	new_function("asin", &DFraggleThinker::SF_ASin);
	new_function("cos", &DFraggleThinker::SF_Cos);
	new_function("acos", &DFraggleThinker::SF_ACos);
	new_function("tan", &DFraggleThinker::SF_Tan);
	new_function("atan", &DFraggleThinker::SF_ATan);
	new_function("exp", &DFraggleThinker::SF_Exp);
	new_function("log", &DFraggleThinker::SF_Log);
	new_function("sqrt", &DFraggleThinker::SF_Sqrt);
	new_function("floor", &DFraggleThinker::SF_Floor);
	new_function("pow", &DFraggleThinker::SF_Pow);
	
	// Eternity extensions
	new_function("setlineblocking", &DFraggleThinker::SF_SetLineBlocking);
	new_function("setlinetrigger", &DFraggleThinker::SF_SetLineTrigger);
	new_function("setlinemnblock", &DFraggleThinker::SF_SetLineMonsterBlocking);
	new_function("scriptwaitpre", &DFraggleThinker::SF_ScriptWaitPre);
	new_function("exitsecret", &DFraggleThinker::SF_ExitSecret);
	new_function("objawaken", &DFraggleThinker::SF_ObjAwaken);
	
	// forced coercion functions
	new_function("mobjvalue", &DFraggleThinker::SF_MobjValue);
	new_function("stringvalue", &DFraggleThinker::SF_StringValue);
	new_function("intvalue", &DFraggleThinker::SF_IntValue);
	new_function("fixedvalue", &DFraggleThinker::SF_FixedValue);

	// new for GZDoom
	new_function("spawnshot2", &DFraggleThinker::SF_SpawnShot2);
	new_function("setcolor", &DFraggleThinker::SF_SetColor);
	new_function("sectortype", &DFraggleThinker::SF_SectorType);
	new_function("wallglow", &DFraggleThinker::SF_WallGlow);
	new_function("objradius", &DFraggleThinker::SF_MobjRadius);
	new_function("objheight", &DFraggleThinker::SF_MobjHeight);
	new_function("thingcount", &DFraggleThinker::SF_ThingCount);
	new_function("killinsector", &DFraggleThinker::SF_KillInSector);
	new_function("changetag", &DFraggleThinker::SF_ChangeTag);
	new_function("levelnum", &DFraggleThinker::SF_LevelNum);

	// new inventory
	new_function("giveinventory", &DFraggleThinker::SF_GiveInventory);
	new_function("takeinventory", &DFraggleThinker::SF_TakeInventory);
	new_function("checkinventory", &DFraggleThinker::SF_CheckInventory);
	new_function("setweapon", &DFraggleThinker::SF_SetWeapon);

	new_function("ls", &DFraggleThinker::SF_Ls);	// execute Hexen type line special

	// Dummies - shut up warnings
	new_function("setcorona", &DFraggleThinker::SF_SetCorona);
}

