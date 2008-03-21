#include "doomtype.h"
#include "doomstat.h"
#include "v_font.h"
#include "v_video.h"
#include "sbar.h"
#include "r_defs.h"
#include "w_wad.h"
#include "m_random.h"
#include "d_player.h"
#include "st_stuff.h"
#include "r_local.h"
#include "m_swap.h"
#include "a_keys.h"
#include "templates.h"
#include "i_system.h"
#include "sbarinfo.h"
#include "gi.h"
#include "r_translate.h"
#include "r_main.h"

static FRandom pr_chainwiggle; //use the same method of chain wiggling as heretic.

#define ST_RAMPAGETIME	(TICRATE*2)
#define ARTIFLASH_OFFSET (invBarOffset+6)

EXTERN_CVAR(Int, fraglimit)
EXTERN_CVAR(Int, screenblocks)

enum
{
	imgARTIBOX,
	imgSELECTBOX,
	imgINVLFGEM1,
	imgINVLFGEM2,
	imgINVRTGEM1,
	imgINVRTGEM2,
};


// Custom Mug Shot Stuff (Find mug shot state functions are with the parser).
MugShotFrame::MugShotFrame()
{
}

MugShotFrame::~MugShotFrame()
{
	graphic.Clear();
}

//Assemble a graphic name with the specified prefix and return the FTexture.
FTexture *MugShotFrame::getTexture(FPlayerSkin *skin, int random, int level, int direction, bool usesLevels, bool health2, bool healthspecial, bool directional)
{
	int index = !directional ? random % graphic.Size() : direction;
	if(index > (signed int) (graphic.Size()-1))
		index = graphic.Size()-1;
	char* sprite = new char[9];
	memcpy(sprite, skin->face[0] != 0 ? skin->face : "STF", 3);
	memcpy(sprite+3, graphic[index], strlen(graphic[index]));
	sprite[3+strlen(graphic[index])] = '\0';
	if(usesLevels) //change the last character to the level
	{
		if(!health2 && (!healthspecial || index == 1))
			sprite[2+strlen(graphic[index])] += level;
		else
			sprite[1+strlen(graphic[index])] += level;
	}
	return TexMan[TexMan.CheckForTexture(sprite, 0, true)];
}

MugShotState::MugShotState()
{
}
MugShotState::MugShotState(FString name)
{
	name.ToLower();
	state = FName(name);
	usesLevels = false;
	health2 = false;
	healthspecial = false;
	directional = false;
	random = M_Random();
}

MugShotState::~MugShotState()
{
	frames.Clear();
}

void MugShotState::tick()
{
	if(time == -1) //When the delay is negative 1 stay on this frame indefinitely
		return;
	if(time != 0)
	{
		time--;
	}
	else if(position != frames.Size()-1)
	{
		position++;
		time = frames[position].delay;
		random = M_Random();
	}
	else
	{
		finished = true;
	}
}

void MugShotState::reset()
{
	time = frames[0].delay;
	position = 0;
	finished = false;
	random = M_Random();
}

//Used for shading
FBarShader::FBarShader(bool vertical, bool reverse) //make an alpha map
{
	int i;

	Width = vertical ? 2 : 256;
	Height = vertical ? 256 : 2;
	CalcBitSize();

	// Fill the column/row with shading values.
	// Vertical shaders have have minimum alpha at the top
	// and maximum alpha at the bottom, unless flipped by
	// setting reverse to true. Horizontal shaders are just
	// the opposite.
	if (vertical)
	{
		if (!reverse)
		{
			for (i = 0; i < 256; ++i)
			{
				Pixels[i] = i;
				Pixels[256+i] = i;
			}
		}
		else
		{
			for (i = 0; i < 256; ++i)
			{
				Pixels[i] = 255 - i;
				Pixels[256+i] = 255 -i;
			}
		}
	}
	else
	{
		if (!reverse)
		{
			for (i = 0; i < 256; ++i)
			{
				Pixels[i*2] = 255 - i;
				Pixels[i*2+1] = 255 - i;
			}
		}
		else
		{
			for (i = 0; i < 256; ++i)
			{
				Pixels[i*2] = i;
				Pixels[i*2+1] = i;
			}
		}
	}
	DummySpan[0].TopOffset = 0;
	DummySpan[0].Length = vertical ? 256 : 1;
	DummySpan[1].TopOffset = 0;
	DummySpan[1].Length = 0;
}

const BYTE *FBarShader::GetColumn(unsigned int column, const Span **spans_out)
{
	if (spans_out != NULL)
	{
		*spans_out = DummySpan;
	}
	return Pixels + (column & WidthMask) * 256;
}

const BYTE *FBarShader::GetPixels()
{
	return Pixels;
}

void FBarShader::Unload()
{
}

//SBarInfo Display
DSBarInfo::DSBarInfo () : DBaseStatusBar (SBarInfoScript->height),
	shader_horz_normal(false, false),
	shader_horz_reverse(false, true),
	shader_vert_normal(true, false),
	shader_vert_reverse(true, true)
{
	static const char *InventoryBarLumps[] =
	{
		"ARTIBOX",	"SELECTBO",	"INVGEML1",
		"INVGEML2",	"INVGEMR1",	"INVGEMR2",
		"USEARTIA", "USEARTIB", "USEARTIC", "USEARTID",
	};
	TArray<const char *> patchnames;
	patchnames.Resize(SBarInfoScript->Images.Size()+10);
	unsigned int i = 0;
	for(i = 0;i < SBarInfoScript->Images.Size();i++)
	{
		patchnames[i] = SBarInfoScript->Images[i];
	}
	for(i = 0;i < 10;i++)
	{
		patchnames[i+SBarInfoScript->Images.Size()] = InventoryBarLumps[i];
	}
	invBarOffset = SBarInfoScript->Images.Size();
	Images.Init(&patchnames[0], patchnames.Size());
	drawingFont = V_GetFont("ConFont");
	rampageTimer = 0;
	oldHealth = 0;
	oldArmor = 0;
	mugshotHealth = -1;
	lastPrefix = "";
	weaponGrin = false;
	damageFaceActive = false;
	lastDamageAngle = 1;
	chainWiggle = 0;
	artiflash = 4;
	currentState = NULL;
}

DSBarInfo::~DSBarInfo ()
{
	Images.Uninit();
}

void DSBarInfo::Draw (EHudState state)
{
	DBaseStatusBar::Draw(state);
	int hud = 2;
	if(state == HUD_StatusBar)
	{
		if(SBarInfoScript->completeBorder) //Fill the statusbar with the border before we draw.
		{
			FTexture *b = TexMan[gameinfo.border->b];
			R_DrawBorder(viewwindowx, viewwindowy + realviewheight + b->GetHeight(), viewwindowx + realviewwidth, SCREENHEIGHT);
			if(screenblocks == 10)
				screen->FlatFill(viewwindowx, viewwindowy + realviewheight, viewwindowx + realviewwidth, viewwindowy + realviewheight + b->GetHeight(), b, true);
		}
		if(SBarInfoScript->automapbar && automapactive)
		{
			hud = 3;
		}
		else
		{
			hud = 2;
		}
	}
	else if(state == HUD_Fullscreen)
	{
		hud = 1;
	}
	else
	{
		hud = 0;
	}
	if(SBarInfoScript->huds[hud].forceScaled) //scale the statusbar
	{
		SetScaled(true);
		setsizeneeded = true;
	}
	doCommands(SBarInfoScript->huds[hud]);
	if(CPlayer->inventorytics > 0 && !(level.flags & LEVEL_NOINVENTORYBAR))
	{
		if(state == HUD_StatusBar)
			doCommands(SBarInfoScript->huds[4]);
		else if(state == HUD_Fullscreen)
			doCommands(SBarInfoScript->huds[5]);
	}
}

void DSBarInfo::NewGame ()
{
	if (CPlayer != NULL)
	{
		AttachToPlayer (CPlayer);
	}
}

void DSBarInfo::AttachToPlayer (player_t *player)
{
	player_t *oldplayer = CPlayer;
	DBaseStatusBar::AttachToPlayer(player);
}

void DSBarInfo::Tick ()
{
	DBaseStatusBar::Tick();
	if(level.time & 1)
		chainWiggle = pr_chainwiggle() & 1;
	if(!SBarInfoScript->interpolateHealth)
	{
		oldHealth = CPlayer->health;
	}
	else
	{
		if(oldHealth > CPlayer->health)
		{
			oldHealth -= clamp((oldHealth - CPlayer->health) >> 2, 1, SBarInfoScript->interpolationSpeed);
		}
		else if(oldHealth < CPlayer->health)
		{
			oldHealth += clamp((CPlayer->health - oldHealth) >> 2, 1, SBarInfoScript->interpolationSpeed);
		}
	}
	AInventory *armor = CPlayer->mo->FindInventory<ABasicArmor>();
	if(armor == NULL)
	{
		oldArmor = 0;
	}
	else
	{
		if(!SBarInfoScript->interpolateArmor)
		{
			oldArmor = armor->Amount;
		}
		else
		{
			if(oldArmor > armor->Amount)
			{
				oldArmor -= clamp((oldArmor - armor->Amount) >> 2, 1, SBarInfoScript->armorInterpolationSpeed);
			}
			else if(oldArmor < armor->Amount)
			{
				oldArmor += clamp((armor->Amount - oldArmor) >> 2, 1, SBarInfoScript->armorInterpolationSpeed);
			}
		}
	}
	if(artiflash)
	{
		artiflash--;
	}

	//Do some stuff related to the mug shot that has to be done at 35fps
	if(currentState != NULL)
	{
		currentState->tick();
		if(currentState->finished)
			currentState = NULL;
	}
	if((CPlayer->cmd.ucmd.buttons & (BT_ATTACK|BT_ALTATTACK)) && !(CPlayer->cheats & (CF_FROZEN | CF_TOTALLYFROZEN)))
	{
		if(rampageTimer != ST_RAMPAGETIME)
		{
			rampageTimer++;
		}
	}
	else
	{
		rampageTimer = 0;
	}
	mugshotHealth = CPlayer->health;
}

void DSBarInfo::ReceivedWeapon (AWeapon *weapon)
{
	weaponGrin = true;
}

void DSBarInfo::FlashItem(const PClass *itemtype)
{
	artiflash = 4;
}

//Public so it can be called by ACS
//Sets the mug shot state and resets it if it is not the state we are already on.
//waitTillDone is basically a priority variable when just to true the state won't change unless the previous state is finished.
void DSBarInfo::SetMugShotState(const char* stateName, bool waitTillDone)
{
	MugShotState *state = (MugShotState *) FindMugShotState(stateName);
	if(state != currentState)
	{
		if(!waitTillDone || currentState == NULL || currentState->finished)
		{
			currentState = state;
			state->reset();
		}
	}
}

void DSBarInfo::doCommands(SBarInfoBlock &block)
{
	//prepare ammo counts
	AAmmo *ammo1, *ammo2;
	int ammocount1, ammocount2;
	GetCurrentAmmo(ammo1, ammo2, ammocount1, ammocount2);
	ABasicArmor *armor = CPlayer->mo->FindInventory<ABasicArmor>();
	int health = CPlayer->mo->health;
	int armorAmount = armor != NULL ? armor->Amount : 0;
	if(SBarInfoScript->interpolateHealth)
	{
		health = oldHealth;
	}
	if(SBarInfoScript->interpolateArmor)
	{
		armorAmount = oldArmor;
	}
	for(unsigned int i = 0;i < block.commands.Size();i++)
	{
		SBarInfoCommand& cmd = block.commands[i];
		switch(cmd.type) //read and execute all the commands
		{
			case SBARINFO_DRAWSWITCHABLEIMAGE: //draw the alt image if we don't have the item else this is like a normal drawimage
			{
				int drawAlt = 0;
				if((cmd.flags & DRAWIMAGE_WEAPONSLOT)) //weaponslots
				{
					drawAlt = 1; //draw off state until we know we have something.
					for (int i = 0; i < MAX_WEAPONS_PER_SLOT; i++)
					{
						const PClass *weap = LocalWeapons.Slots[cmd.value].GetWeapon(i);
						if(weap == NULL)
						{
							continue;
						}
						else if(CPlayer->mo->FindInventory(weap) != NULL)
						{
							drawAlt = 0;
							break;
						}
					}
				}
				else if((cmd.flags & DRAWIMAGE_INVULNERABILITY))
				{
					if(CPlayer->cheats&CF_GODMODE)
					{
						drawAlt = 1;
					}
				}
				else //check the inventory items and draw selected sprite
				{
					AInventory* item = CPlayer->mo->FindInventory(PClass::FindClass(cmd.string[0]));
					if(item == NULL || item->Amount == 0)
						drawAlt = 1;
					if((cmd.flags & DRAWIMAGE_SWITCHABLE_AND))
					{
						item = CPlayer->mo->FindInventory(PClass::FindClass(cmd.string[1]));
						if((item != NULL && item->Amount != 0) && drawAlt == 0) //both
						{
							drawAlt = 0;
						}
						else if((item != NULL && item->Amount != 0) && drawAlt == 1) //2nd
						{
							drawAlt = 3;
						}
						else if((item == NULL || item->Amount == 0) && drawAlt == 0) //1st
						{
							drawAlt = 2;
						}
					}
				}
				if(drawAlt != 0) //draw 'off' image
				{
					if(cmd.special != -1 && drawAlt == 1)
						DrawGraphic(Images[cmd.special], cmd.x, cmd.y, cmd.flags);
					else if(cmd.special2 != -1 && drawAlt == 2)
						DrawGraphic(Images[cmd.special2], cmd.x, cmd.y, cmd.flags);
					else if(cmd.special3 != -1 && drawAlt == 3)
						DrawGraphic(Images[cmd.special3], cmd.x, cmd.y, cmd.flags);
					break;
				}
			}
			case SBARINFO_DRAWIMAGE:
				if((cmd.flags & DRAWIMAGE_PLAYERICON))
					DrawGraphic(TexMan[CPlayer->mo->ScoreIcon], cmd.x, cmd.y, cmd.flags);
				else if((cmd.flags & DRAWIMAGE_AMMO1))
				{
					if(ammo1 != NULL)
						DrawGraphic(TexMan[ammo1->Icon], cmd.x, cmd.y, cmd.flags);
				}
				else if((cmd.flags & DRAWIMAGE_AMMO2))
				{
					if(ammo2 != NULL)
						DrawGraphic(TexMan[ammo2->Icon], cmd.x, cmd.y, cmd.flags);
				}
				else if((cmd.flags & DRAWIMAGE_ARMOR))
				{
					if(armor != NULL && armor->Amount != 0)
						DrawGraphic(TexMan(armor->Icon), cmd.x, cmd.y, cmd.flags);
				}
				else if((cmd.flags & DRAWIMAGE_WEAPONICON))
				{
					AWeapon *weapon = CPlayer->ReadyWeapon;
					if(weapon != NULL && weapon->Icon > 0)
					{
						DrawGraphic(TexMan[weapon->Icon], cmd.x, cmd.y, cmd.flags);
					}
				}
				else if((cmd.flags & DRAWIMAGE_INVENTORYICON))
				{
					DrawGraphic(TexMan[cmd.sprite], cmd.x, cmd.y, cmd.flags);
				}
				else
				{
					DrawGraphic(Images[cmd.sprite], cmd.x, cmd.y, cmd.flags);
				}
				break;
			case SBARINFO_DRAWNUMBER:
				if(drawingFont != cmd.font)
				{
					drawingFont = cmd.font;
				}
				if(cmd.flags == DRAWNUMBER_HEALTH)
				{
					cmd.value = health;
					if(cmd.value < 0) //health shouldn't display negatives
					{
						cmd.value = 0;
					}
				}
				else if(cmd.flags == DRAWNUMBER_ARMOR)
				{
					cmd.value = armorAmount;
				}
				else if(cmd.flags == DRAWNUMBER_AMMO1)
				{
					cmd.value = ammocount1;
					if(ammo1 == NULL) //no ammo, do not draw
					{
						continue;
					}
				}
				else if(cmd.flags == DRAWNUMBER_AMMO2)
				{
					cmd.value = ammocount2;
					if(ammo2 == NULL) //no ammo, do not draw
					{
						continue;
					}
				}
				else if(cmd.flags == DRAWNUMBER_AMMO)
				{
					const PClass* ammo = PClass::FindClass(cmd.string[0]);
					AInventory* item = CPlayer->mo->FindInventory(ammo);
					if(item != NULL)
					{
						cmd.value = item->Amount;
					}
					else
					{
						cmd.value = 0;
					}
				}
				else if(cmd.flags == DRAWNUMBER_AMMOCAPACITY)
				{
					const PClass* ammo = PClass::FindClass(cmd.string[0]);
					AInventory* item = CPlayer->mo->FindInventory(ammo);
					if(item != NULL)
					{
						cmd.value = item->MaxAmount;
					}
					else
					{
						cmd.value = ((AInventory *)GetDefaultByType(ammo))->MaxAmount;
					}
				}
				else if(cmd.flags == DRAWNUMBER_FRAGS)
					cmd.value = CPlayer->fragcount;
				else if(cmd.flags == DRAWNUMBER_KILLS)
					cmd.value = level.killed_monsters;
				else if(cmd.flags == DRAWNUMBER_MONSTERS)
					cmd.value = level.total_monsters;
				else if(cmd.flags == DRAWNUMBER_ITEMS)
					cmd.value = level.found_items;
				else if(cmd.flags == DRAWNUMBER_TOTALITEMS)
					cmd.value = level.total_items;
				else if(cmd.flags == DRAWNUMBER_SECRETS)
					cmd.value = level.found_secrets;
				else if(cmd.flags == DRAWNUMBER_TOTALSECRETS)
					cmd.value = level.total_secrets;
				else if(cmd.flags == DRAWNUMBER_ARMORCLASS)
				{
					AHexenArmor *harmor = CPlayer->mo->FindInventory<AHexenArmor>();
					if(harmor != NULL)
					{
						cmd.value = harmor->Slots[0] + harmor->Slots[1] + 
							harmor->Slots[2] + harmor->Slots[3] + harmor->Slots[4];
					}
					//Hexen counts basic armor also so we should too.
					if(armor != NULL)
					{
						cmd.value += armor->SavePercent;
					}
					cmd.value /= (5*FRACUNIT);
				}
				else if(cmd.flags == DRAWNUMBER_INVENTORY)
				{
					AInventory* item = CPlayer->mo->FindInventory(PClass::FindClass(cmd.string[0]));
					if(item != NULL)
					{
						cmd.value = item->Amount;
					}
					else
					{
						cmd.value = 0;
					}
				}
				if(cmd.special3 != -1 && cmd.value <= cmd.special3) //low
					DrawNumber(cmd.value, cmd.special, cmd.x, cmd.y, cmd.translation2, cmd.special2);
				else if(cmd.special4 != -1 && cmd.value >= cmd.special4) //high
					DrawNumber(cmd.value, cmd.special, cmd.x, cmd.y, cmd.translation3, cmd.special2);
				else
					DrawNumber(cmd.value, cmd.special, cmd.x, cmd.y, cmd.translation, cmd.special2);
				break;
			case SBARINFO_DRAWMUGSHOT:
			{
				bool xdth = false;
				bool animatedgodmode = false;
				if(cmd.flags & DRAWMUGSHOT_XDEATHFACE)
					xdth = true;
				if(cmd.flags & DRAWMUGSHOT_ANIMATEDGODMODE)
					animatedgodmode = true;
				DrawFace(cmd.special, xdth, animatedgodmode, cmd.x, cmd.y);
				break;
			}
			case SBARINFO_DRAWSELECTEDINVENTORY:
				if(CPlayer->mo->InvSel != NULL && !(level.flags & LEVEL_NOINVENTORYBAR))
				{
					if((cmd.flags & DRAWSELECTEDINVENTORY_ARTIFLASH) && artiflash)
					{
						DrawDimImage(Images[ARTIFLASH_OFFSET+(4-artiflash)], cmd.x, cmd.y, CPlayer->mo->InvSel->Amount <= 0);
					}
					else
					{
						DrawDimImage(TexMan(CPlayer->mo->InvSel->Icon), cmd.x, cmd.y, CPlayer->mo->InvSel->Amount <= 0);
					}
					if((cmd.flags & DRAWSELECTEDINVENTORY_ALWAYSSHOWCOUNTER) || CPlayer->mo->InvSel->Amount != 1)
					{
						if(drawingFont != cmd.font)
						{
							drawingFont = cmd.font;
						}
						DrawNumber(CPlayer->mo->InvSel->Amount, 3, cmd.special2, cmd.special3, cmd.translation, cmd.special4);
					}
				}
				else if((cmd.flags & DRAWSELECTEDINVENTORY_ALTERNATEONEMPTY))
				{
					doCommands(cmd.subBlock);
				}
				break;
			case SBARINFO_DRAWINVENTORYBAR:
			{
				bool alwaysshow = false;
				bool artibox = true;
				bool noarrows = false;
				bool alwaysshowcounter = false;
				if((cmd.flags & DRAWINVENTORYBAR_ALWAYSSHOW))
					alwaysshow = true;
				if((cmd.flags & DRAWINVENTORYBAR_NOARTIBOX))
					artibox = false;
				if((cmd.flags & DRAWINVENTORYBAR_NOARROWS))
					noarrows = true;
				if((cmd.flags & DRAWINVENTORYBAR_ALWAYSSHOWCOUNTER))
					alwaysshowcounter = true;
				if(drawingFont != cmd.font)
				{
					drawingFont = cmd.font;
				}
				DrawInventoryBar(cmd.special, cmd.value, cmd.x, cmd.y, alwaysshow, cmd.special2, cmd.special3, cmd.translation, artibox, noarrows, alwaysshowcounter);
				break;
			}
			case SBARINFO_DRAWBAR:
			{
				if(cmd.sprite == -1 || Images[cmd.sprite] == NULL)
					break; //don't draw anything.
				bool horizontal = !!((cmd.special2 & DRAWBAR_HORIZONTAL));
				bool reverse = !!((cmd.special2 & DRAWBAR_REVERSE));
				fixed_t value = 0;
				int max = 0;
				if(cmd.flags == DRAWNUMBER_HEALTH)
				{
					value = health;
					if(value < 0) //health shouldn't display negatives
					{
						value = 0;
					}
					if(!(cmd.special2 & DRAWBAR_COMPAREDEFAULTS))
					{
						AInventory* item = CPlayer->mo->FindInventory(PClass::FindClass(cmd.string[0])); //max comparer
						if(item != NULL)
						{
							max = item->Amount;
						}
						else
						{
							max = 0;
						}
					}
					else //default to the class's health
					{
						max = CPlayer->mo->GetDefault()->health;
					}
				}
				else if(cmd.flags == DRAWNUMBER_ARMOR)
				{
					value = armorAmount;
					if(!((cmd.special2 & DRAWBAR_COMPAREDEFAULTS) == DRAWBAR_COMPAREDEFAULTS))
					{
						AInventory* item = CPlayer->mo->FindInventory(PClass::FindClass(cmd.string[0])); //max comparer
						if(item != NULL)
						{
							max = item->Amount;
						}
						else
						{
							max = 0;
						}
					}
					else //default to 100
					{
						max = 100;
					}
				}
				else if(cmd.flags == DRAWNUMBER_AMMO1)
				{
					value = ammocount1;
					if(ammo1 == NULL) //no ammo, draw as empty
					{
						value = 0;
						max = 1;
					}
					else
						max = ammo1->MaxAmount;
				}
				else if(cmd.flags == DRAWNUMBER_AMMO2)
				{
					value = ammocount2;
					if(ammo2 == NULL) //no ammo, draw as empty
					{
						value = 0;
						max = 1;
					}
					else
						max = ammo2->MaxAmount;
				}
				else if(cmd.flags == DRAWNUMBER_AMMO)
				{
					const PClass* ammo = PClass::FindClass(cmd.string[0]);
					AInventory* item = CPlayer->mo->FindInventory(ammo);
					if(item != NULL)
					{
						value = item->Amount;
						max = item->MaxAmount;
					}
					else
					{
						value = 0;
					}
				}
				else if(cmd.flags == DRAWNUMBER_FRAGS)
				{
					value = CPlayer->fragcount;
					max = fraglimit;
				}
				else if(cmd.flags == DRAWNUMBER_KILLS)
				{
					value = level.killed_monsters;
					max = level.total_monsters;
				}
				else if(cmd.flags == DRAWNUMBER_ITEMS)
				{
					value = level.found_items;
					max = level.total_items;
				}
				else if(cmd.flags == DRAWNUMBER_SECRETS)
				{
					value = level.found_secrets;
					max = level.total_secrets;
				}
				else if(cmd.flags == DRAWNUMBER_INVENTORY)
				{
					AInventory* item = CPlayer->mo->FindInventory(PClass::FindClass(cmd.string[0]));
					if(item != NULL)
					{
						value = item->Amount;
						max = item->MaxAmount;
					}
					else
					{
						value = 0;
					}
				}
				if(cmd.special3 != 0)
					value = max - value; //invert since the new drawing method requires drawing the bg on the fg.
				if(max != 0 && value > 0)
				{
					value = (value << FRACBITS) / max;
					if(value > FRACUNIT)
						value = FRACUNIT;
				}
				else if(cmd.special3 != 0 && max == 0 && value <= 0)
				{
					value = FRACUNIT;
				}
				else
				{
					value = 0;
				}
				assert(Images[cmd.sprite] != NULL);

				FTexture *fg = Images[cmd.sprite];
				FTexture *bg = (cmd.special != -1) ? Images[cmd.special] : NULL;
				int x, y, w, h;
				int cx, cy, cw, ch, cr, cb;

				// Calc real screen coordinates for bar
				x = cmd.x + ST_X;
				y = cmd.y + ST_Y;
				w = fg->GetScaledWidth();
				h = fg->GetScaledHeight();
				if (Scaled)
				{
					screen->VirtualToRealCoordsInt(x, y, w, h, 320, 200, true);
				}
				if(cmd.special2 & DRAWBAR_KEEPOFFSETS)
				{
					x += fg->LeftOffset;
					y += fg->TopOffset;
				}

				if(cmd.special3 != 0)
				{
					//Draw the whole foreground
					screen->DrawTexture(fg, x, y,
						DTA_DestWidth, w,
						DTA_DestHeight, h,
						TAG_DONE);
				}
				else
				{
					// Draw background
					if (bg != NULL && bg->GetScaledWidth() == fg->GetScaledWidth() && bg->GetScaledHeight() == fg->GetScaledHeight())
					{
						screen->DrawTexture(bg, x, y,
							DTA_DestWidth, w,
							DTA_DestHeight, h,
							TAG_DONE);
					}
					else
					{
						screen->Clear(x, y, x + w, y + h, GPalette.BlackIndex, 0);
					}
				}

				// Calc clipping rect for background
				cx = cmd.x + ST_X + cmd.special3;
				cy = cmd.y + ST_Y + cmd.special3;
				cw = fg->GetScaledWidth() - cmd.special3 * 2;
				ch = fg->GetScaledHeight() - cmd.special3 * 2;
				if (Scaled)
				{
					screen->VirtualToRealCoordsInt(cx, cy, cw, ch, 320, 200, true);
				}
				if(cmd.special2 & DRAWBAR_KEEPOFFSETS)
				{
					cx += fg->LeftOffset;
					cy += fg->TopOffset;
				}
				if (horizontal)
				{
					if ((cmd.special3 != 0 && reverse) || (cmd.special3 == 0 && !reverse))
					{ // left to right
						cr = cx + FixedMul(cw, value);
					}
					else
					{ // right to left
						cr = cx + cw;
						cx += FixedMul(cw, FRACUNIT - value);
					}
					cb = cy + ch;
				}
				else
				{
					if ((cmd.special3 != 0 && reverse) || (cmd.special3 == 0 && !reverse))
					{ // bottom to top
						cb = cy + ch;
						cy += FixedMul(ch, FRACUNIT - value);
					}
					else
					{ // top to bottom
						cb = cy + FixedMul(ch, value);
					}
					cr = cx + cw;
				}

				// Draw background
				if(cmd.special3 != 0)
				{
					if (bg != NULL && bg->GetScaledWidth() == fg->GetScaledWidth() && bg->GetScaledHeight() == fg->GetScaledHeight())
					{
						screen->DrawTexture(bg, x, y,
							DTA_DestWidth, w,
							DTA_DestHeight, h,
							DTA_ClipLeft, cx,
							DTA_ClipTop, cy,
							DTA_ClipRight, cr,
							DTA_ClipBottom, cb,
							TAG_DONE);
					}
					else
					{
						screen->Clear(cx, cy, cr, cb, GPalette.BlackIndex, 0);
					}
				}
				else
				{
					screen->DrawTexture(fg, x, y,
						DTA_DestWidth, w,
						DTA_DestHeight, h,
						DTA_ClipLeft, cx,
						DTA_ClipTop, cy,
						DTA_ClipRight, cr,
						DTA_ClipBottom, cb,
						TAG_DONE);
				}
				break;
			}
			case SBARINFO_DRAWGEM:
			{
				int value = (cmd.flags & DRAWGEM_ARMOR) ? armorAmount : health;
				int max = 100;
				bool wiggle = false;
				bool translate = !!(cmd.flags & DRAWGEM_TRANSLATABLE);
				if(max != 0 || value < 0)
				{
					value = (value*100)/max;
					if(value > 100)
						value = 100;
				}
				else
				{
					value = 0;
				}
				value = (cmd.flags & DRAWGEM_REVERSE) ? 100 - value : value;
				if(health != CPlayer->health)
				{
					wiggle = !!(cmd.flags & DRAWGEM_WIGGLE);
				}
				DrawGem(Images[cmd.special], Images[cmd.sprite], value, cmd.x, cmd.y, cmd.special2, cmd.special3, cmd.special4+1, wiggle, translate);
				break;
			}
			case SBARINFO_DRAWSHADER:
			{
				FBarShader *const shaders[4] =
				{
					&shader_horz_normal, &shader_horz_reverse,
					&shader_vert_normal, &shader_vert_reverse
				};
				bool vertical = !!(cmd.flags & DRAWSHADER_VERTICAL);
				bool reverse = !!(cmd.flags & DRAWSHADER_REVERSE);
				screen->DrawTexture (shaders[(vertical << 1) + reverse], ST_X+cmd.x, ST_Y+cmd.y,
					DTA_DestWidth, cmd.special,
					DTA_DestHeight, cmd.special2,
					DTA_Bottom320x200, Scaled,
					DTA_AlphaChannel, true,
					DTA_FillColor, 0,
					TAG_DONE);
				break;
			}
			case SBARINFO_DRAWSTRING:
				if(drawingFont != cmd.font)
				{
					drawingFont = cmd.font;
				}
				DrawString(cmd.string[0], cmd.x - drawingFont->StringWidth(cmd.string[0]), cmd.y, cmd.translation);
				break;
			case SBARINFO_DRAWKEYBAR:
			{
				bool vertical = !!(cmd.flags & DRAWKEYBAR_VERTICAL);
				AInventory *item = CPlayer->mo->Inventory;
				if(item == NULL)
					break;
				for(int i = 0;i < cmd.value;i++)
				{
					while(item->Icon <= 0 || item->GetClass() == RUNTIME_CLASS(AKey) || !item->IsKindOf(RUNTIME_CLASS(AKey)))
					{
						item = item->Inventory;
						if(item == NULL)
							goto FinishDrawKeyBar;
					}
					if(!vertical)
						DrawImage(TexMan[item->Icon], cmd.x+(cmd.special*i), cmd.y);
					else
						DrawImage(TexMan[item->Icon], cmd.x, cmd.y+(cmd.special*i));
					item = item->Inventory;
					if(item == NULL)
						break;
				}
			FinishDrawKeyBar:
				break;
			}
			case SBARINFO_GAMEMODE:
				if(((cmd.flags & GAMETYPE_SINGLEPLAYER) && !multiplayer) ||
					((cmd.flags & GAMETYPE_DEATHMATCH) && deathmatch) ||
					((cmd.flags & GAMETYPE_COOPERATIVE) && multiplayer && !deathmatch) ||
					((cmd.flags & GAMETYPE_TEAMGAME) && teamplay))
				{
					doCommands(cmd.subBlock);
				}
				break;
			case SBARINFO_PLAYERCLASS:
			{
				int spawnClass = CPlayer->cls->ClassIndex;
				if(cmd.special == spawnClass || cmd.special2 == spawnClass || cmd.special3 == spawnClass)
				{
					doCommands(cmd.subBlock);
				}
				break;
			}
			case SBARINFO_WEAPONAMMO:
				if(CPlayer->ReadyWeapon != NULL)
				{
					const PClass *AmmoType1 = CPlayer->ReadyWeapon->AmmoType1;
					const PClass *AmmoType2 = CPlayer->ReadyWeapon->AmmoType2;
					const PClass *IfAmmo1 = PClass::FindClass(cmd.string[0]);
					const PClass *IfAmmo2 = (cmd.flags & SBARINFOEVENT_OR) || (cmd.flags & SBARINFOEVENT_AND) ?
											PClass::FindClass(cmd.string[1]) : NULL;
					bool usesammo1 = (AmmoType1 != NULL);
					bool usesammo2 = (AmmoType2 != NULL);
					if(!(cmd.flags & SBARINFOEVENT_NOT) && !usesammo1 && !usesammo2) //if the weapon doesn't use ammo don't go though the trouble.
					{
						doCommands(cmd.subBlock);
						break;
					}
					//Or means only 1 ammo type needs to match and means both need to match.
					if((cmd.flags & SBARINFOEVENT_OR) || (cmd.flags & SBARINFOEVENT_AND))
					{
						bool match1 = ((usesammo1 && (AmmoType1 == IfAmmo1 || AmmoType1 == IfAmmo2)) || !usesammo1);
						bool match2 = ((usesammo2 && (AmmoType2 == IfAmmo1 || AmmoType2 == IfAmmo2)) || !usesammo2);
						if(((cmd.flags & SBARINFOEVENT_OR) && (match1 || match2)) || ((cmd.flags & SBARINFOEVENT_AND) && (match1 && match2)))
						{
							if(!(cmd.flags & SBARINFOEVENT_NOT))
								doCommands(cmd.subBlock);
						}
						else if(cmd.flags & SBARINFOEVENT_NOT)
						{
							doCommands(cmd.subBlock);
						}
					}
					else //Every thing here could probably be one long if statement but then it would be more confusing.
					{
						if((usesammo1 && (AmmoType1 == IfAmmo1)) || (usesammo2 && (AmmoType2 == IfAmmo1)))
						{
							if(!(cmd.flags & SBARINFOEVENT_NOT))
								doCommands(cmd.subBlock);
						}
						else if(cmd.flags & SBARINFOEVENT_NOT)
						{
							doCommands(cmd.subBlock);
						}
					}
				}
				break;
		}
	}
}

//draws an image with the specified flags
void DSBarInfo::DrawGraphic(FTexture* texture, int x, int y, int flags)
{
	if((flags & DRAWIMAGE_OFFSET_CENTER))
	{
		x -= (texture->GetWidth()/2)-texture->LeftOffset;
		y -= (texture->GetHeight()/2)-texture->TopOffset;
	}
	if((flags & DRAWIMAGE_TRANSLATABLE))
		DrawImage(texture, x, y, getTranslation());
	else
		DrawImage(texture, x, y);
}

void DSBarInfo::DrawString(const char* str, int x, int y, EColorRange translation, int spacing)
{
	x += spacing;
	while(*str != '\0')
	{
		if(*str == ' ')
		{
			x += drawingFont->GetSpaceWidth();
			str++;
			continue;
		}
		int width = drawingFont->GetCharWidth((int) *str);
		FTexture* character = drawingFont->GetChar((int) *str, &width);
		if(character == NULL) //missing character.
		{
			str++;
			continue;
		}
		x += (character->LeftOffset+1); //ignore x offsets since we adapt to character size
		DrawImage(character, x, y, drawingFont->GetColorTranslation(translation));
		x += width + spacing - (character->LeftOffset+1);
		str++;
	}
}

//draws the specified number up to len digits
void DSBarInfo::DrawNumber(int num, int len, int x, int y, EColorRange translation, int spacing)
{
	FString value;
	int maxval = (int) ceil(pow(10., len))-1;
	num = clamp(num, -maxval, maxval);
	value.Format("%d", num);
	x -= int(drawingFont->StringWidth(value)+(spacing * value.Len()));
	DrawString(value, x, y, translation, spacing);
}

//draws the mug shot
void DSBarInfo::DrawFace(int accuracy, bool xdth, bool animatedgodmode, int x, int y)
{
	int angle = updateState(xdth, animatedgodmode);
	int level = 0;
	for(level = 0;CPlayer->health < (accuracy-level-1)*(CPlayer->mo->GetMaxHealth()/accuracy);level++);
	if(currentState != NULL)
	{
		DrawImage(currentState->getCurrentFrameTexture(&skins[CPlayer->userinfo.skin], level, angle), x, y);
	}
}

int DSBarInfo::updateState(bool xdth, bool animatedgodmode)
{
	int 		i;
	angle_t 	badguyangle;
	angle_t 	diffang;

	if(CPlayer->health > 0)
	{
		if(weaponGrin)
		{
			weaponGrin = false;
			if(CPlayer->bonuscount)
			{
				SetMugShotState("grin", false);
				return 0;
			}
		}

		if (CPlayer->damagecount)
		{
			int damageAngle = 1;
			if(CPlayer->attacker && CPlayer->attacker != CPlayer->mo)
			{
				if(CPlayer->mo != NULL)
				{
					//The next 12 lines is from the Doom statusbar code.
					badguyangle = R_PointToAngle2(CPlayer->mo->x, CPlayer->mo->y, CPlayer->attacker->x, CPlayer->attacker->y);
					if(badguyangle > CPlayer->mo->angle)
					{
						// whether right or left
						diffang = badguyangle - CPlayer->mo->angle;
						i = diffang > ANG180; 
					}
					else
					{
						// whether left or right
						diffang = CPlayer->mo->angle - badguyangle;
						i = diffang <= ANG180; 
					} // confusing, aint it?
					if(i && diffang >= ANG45)
					{
						damageAngle = 0;
					}
					else if(!i && diffang >= ANG45)
					{
						damageAngle = 2;
					}
				}
			}
			const char* stateName = new char[5];
			if (mugshotHealth != -1 && CPlayer->health - mugshotHealth > 20)
				stateName = "ouch";
			else
				stateName = "pain";
			char* fullStateName = new char[sizeof(stateName)+sizeof((const char*) CPlayer->LastDamageType) + 1]; 
			sprintf(fullStateName, "%s.%s", stateName, (const char*) CPlayer->LastDamageType);
			if(FindMugShotState(fullStateName) != NULL)
				SetMugShotState(fullStateName);
			else
				SetMugShotState(stateName);
			damageFaceActive = !(currentState == NULL);
			lastDamageAngle = damageAngle;
			return damageAngle;
		}
		if(damageFaceActive)
		{
			if(currentState == NULL)
				damageFaceActive = false;
			else
			{
				const char* stateName = new char[5];
				if (mugshotHealth != -1 && CPlayer->health - mugshotHealth > 20)
					stateName = "ouch";
				else
					stateName = "pain";
				char* fullStateName = new char[sizeof(stateName)+sizeof((const char*) CPlayer->LastDamageType) + 1]; 
				sprintf(fullStateName, "%s.%s", stateName, (const char*) CPlayer->LastDamageType);
				if(FindMugShotState(fullStateName) != NULL)
					SetMugShotState(fullStateName);
				else
					SetMugShotState(stateName);
				return lastDamageAngle;
			}
		}

		if(rampageTimer == ST_RAMPAGETIME)
		{
			SetMugShotState("rampage", true);
			return 0;
		}

		if((CPlayer->cheats & CF_GODMODE) || (CPlayer->mo != NULL && CPlayer->mo->flags2 & MF2_INVULNERABLE))
		{
			if(animatedgodmode)
				SetMugShotState("godanimated", true);
			else
				SetMugShotState("god", true);
		}
		else
			SetMugShotState("normal", true);
	}
	else
	{
		const char* stateName = new char[7];
		if(!xdth || !(CPlayer->cheats & CF_EXTREMELYDEAD))
			stateName = "death";
		else
			stateName = "xdeath";
		//new string the size of stateName and the damage type put together
		char* fullStateName = new char[sizeof(stateName)+sizeof((const char*) CPlayer->LastDamageType) + 1]; 
		sprintf(fullStateName, "%s.%s", stateName, (const char*) CPlayer->LastDamageType);
		if(FindMugShotState(fullStateName) != NULL)
			SetMugShotState(fullStateName);
		else
			SetMugShotState(stateName);
	}
	return 0;
}

void DSBarInfo::DrawInventoryBar(int type, int num, int x, int y, bool alwaysshow, 
	int counterx, int countery, EColorRange translation, bool drawArtiboxes, bool noArrows, bool alwaysshowcounter)
{ //yes, there is some Copy & Paste here too
	AInventory *item;
	int i;

	// If the player has no artifacts, don't draw the bar
	CPlayer->mo->InvFirst = ValidateInvFirst(num);
	if(CPlayer->mo->InvFirst != NULL || alwaysshow)
	{
		for(item = CPlayer->mo->InvFirst, i = 0; item != NULL && i < num; item = item->NextInv(), ++i)
		{
			if(drawArtiboxes)
			{
				DrawImage (Images[invBarOffset + imgARTIBOX], x+i*31, y);
			}
			DrawDimImage (TexMan(item->Icon), x+i*31, y, item->Amount <= 0);
			if(alwaysshowcounter || item->Amount != 1)
			{
				DrawNumber(item->Amount, 3, counterx+i*31, countery, translation);
			}
			if(item == CPlayer->mo->InvSel)
			{
				if(type == GAME_Heretic)
				{
					DrawImage(Images[invBarOffset + imgSELECTBOX], x+i*31, y+29);
				}
				else
				{
					DrawImage(Images[invBarOffset + imgSELECTBOX], x+i*31, y);
				}
			}
		}
		for (; i < num && drawArtiboxes; ++i)
		{
			DrawImage (Images[invBarOffset + imgARTIBOX], x+i*31, y);
		}
		// Is there something to the left?
		if (!noArrows && CPlayer->mo->FirstInv() != CPlayer->mo->InvFirst)
		{
			DrawImage (Images[!(gametic & 4) ?
				invBarOffset + imgINVLFGEM1 : invBarOffset + imgINVLFGEM2], x-12, y);
		}
		// Is there something to the right?
		if (!noArrows && item != NULL)
		{
			DrawImage (Images[!(gametic & 4) ?
				invBarOffset + imgINVRTGEM1 : invBarOffset + imgINVRTGEM2], x+num*31+2, y);
		}
	}
}

//draws heretic/hexen style life gems
void DSBarInfo::DrawGem(FTexture* chain, FTexture* gem, int value, int x, int y, int padleft, int padright, int chainsize, 
	bool wiggle, bool translate)
{
	if(value > 100)
		value = 100;
	else if(value < 0)
		value = 0;
	if(wiggle)
		y += chainWiggle;
	int gemWidth = gem->GetWidth();
	int chainWidth = chain->GetWidth();
	int offset = (int) (((double) (chainWidth-padleft-padright)/100)*value);
	if(chain != NULL)
	{
		DrawImage(chain, x+(offset%chainsize), y);
	}
	if(gem != NULL)
		DrawImage(gem, x+padleft+offset, y, translate ? getTranslation() : NULL);
}

FRemapTable* DSBarInfo::getTranslation()
{
	if(gameinfo.gametype & GAME_Raven)
		return translationtables[TRANSLATION_PlayersExtra][int(CPlayer - players)];
	return translationtables[TRANSLATION_Players][int(CPlayer - players)];
}

IMPLEMENT_CLASS(DSBarInfo);

DBaseStatusBar *CreateCustomStatusBar ()
{
	return new DSBarInfo;
}