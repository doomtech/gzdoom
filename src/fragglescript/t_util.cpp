/*
** t_load.cpp
** FraggleScript Utilities
**
**---------------------------------------------------------------------------
** Copyright 2002-2008 Christoph Oelckers
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/




#include "t_util.h"
#include "t_script.h"
#include "s_sound.h"
#include "gi.h"
#include "a_keys.h"
#include "p_lnspec.h"
#include "c_dispatch.h"

//==========================================================================
//
//     FraggleScript action special
//
//==========================================================================

static int LS_FS_Execute (line_t *ln, AActor *it, bool backSide,
	int arg0, int arg1, int arg2, int arg3, int arg4)
// FS_Execute(script#,firstsideonly,lock,msgtype)
{
	if (arg1 && ln && backSide) return false;
	if (arg2!=0 && !P_CheckKeys(it, arg2, !!arg3)) return false;

	DFraggleThinker *th = DFraggleThinker::ActiveThinker;
	if (th)
	{
		th->RunScript(arg0,it);
	}
	return true;
}

//==========================================================================
//
// The Doom actors in their original order
//
//==========================================================================

static const char * const ActorNames_init[]=
{
	"DoomPlayer",
	"ZombieMan",
	"ShotgunGuy",
	"Archvile",
	"ArchvileFire",
	"Revenant",
	"RevenantTracer",
	"RevenantTracerSmoke",
	"Fatso",
	"FatShot",
	"ChaingunGuy",
	"DoomImp",
	"Demon",
	"Spectre",
	"Cacodemon",
	"BaronOfHell",
	"BaronBall",
	"HellKnight",
	"LostSoul",
	"SpiderMastermind",
	"Arachnotron",
	"Cyberdemon",
	"PainElemental",
	"WolfensteinSS",
	"CommanderKeen",
	"BossBrain",
	"BossEye",
	"BossTarget",
	"SpawnShot",
	"SpawnFire",
	"ExplosiveBarrel",
	"DoomImpBall",
	"CacodemonBall",
	"Rocket",
	"PlasmaBall",
	"BFGBall",
	"ArachnotronPlasma",
	"BulletPuff",
	"Blood",
	"TeleportFog",
	"ItemFog",
	"TeleportDest",
	"BFGExtra",
	"GreenArmor",
	"BlueArmor",
	"HealthBonus",
	"ArmorBonus",
	"BlueCard",
	"RedCard",
	"YellowCard",
	"YellowSkull",
	"RedSkull",
	"BlueSkull",
	"Stimpack",
	"Medikit",
	"Soulsphere",
	"InvulnerabilitySphere",
	"Berserk",
	"BlurSphere",
	"RadSuit",
	"Allmap",
	"Infrared",
	"Megasphere",
	"Clip",
	"ClipBox",
	"RocketAmmo",
	"RocketBox",
	"Cell",
	"CellBox",
	"Shell",
	"ShellBox",
	"Backpack",
	"BFG9000",
	"Chaingun",
	"Chainsaw",
	"RocketLauncher",
	"PlasmaRifle",
	"Shotgun",
	"SuperShotgun",
	"TechLamp",
	"TechLamp2",
	"Column",
	"TallGreenColumn",
	"ShortGreenColumn",
	"TallRedColumn",
	"ShortRedColumn",
	"SkullColumn",
	"HeartColumn",
	"EvilEye",
	"FloatingSkull",
	"TorchTree",
	"BlueTorch",
	"GreenTorch",
	"RedTorch",
	"ShortBlueTorch",
	"ShortGreenTorch",
	"ShortRedTorch",
	"Slalagtite",
	"TechPillar",
	"CandleStick",
	"Candelabra",
	"BloodyTwitch",
	"Meat2",
	"Meat3",
	"Meat4",
	"Meat5",
	"NonsolidMeat2",
	"NonsolidMeat4",
	"NonsolidMeat3",
	"NonsolidMeat5",
	"NonsolidTwitch",
	"DeadCacodemon",
	"DeadMarine",
	"DeadZombieMan",
	"DeadDemon",
	"DeadLostSoul",
	"DeadDoomImp",
	"DeadShotgunGuy",
	"GibbedMarine",
	"GibbedMarineExtra",
	"HeadsOnAStick",
	"Gibs",
	"HeadOnAStick",
	"HeadCandles",
	"DeadStick",
	"LiveStick",
	"BigTree",
	"BurningBarrel",
	"HangNoGuts",
	"HangBNoBrain",
	"HangTLookingDown",
	"HangTSkull",
	"HangTLookingUp",
	"HangTNoBrain",
	"ColonGibs",
	"SmallBloodPool",
	"BrainStem",
	"PointPusher",
	"PointPuller",
};

const PClass * ActorTypes[countof(ActorNames_init)+1];

// static initialization of FS data
AT_GAME_SET(FraggleScript)
{
	for(int i=0;i<countof(ActorNames_init);i++)
	{
		ActorTypes[i]=PClass::FindClass(ActorNames_init[i]);
	}
	ActorTypes[countof(ActorNames_init)] = NULL;
	LineSpecials[54]=LS_FS_Execute;
}



// [CO] made these real functions to save some memory
//      The compiler generated insane amounts of code for
//      these macros without any real benefit. 
//		Altogether they generated 14(!) KB of code!

//==========================================================================
//
//
//
//==========================================================================
int intvalue(const svalue_s & v)
{
	return (v.type == svt_string ? atoi(v.value.s) :       
	v.type == svt_fixed ? (int)(v.value.f / FRACUNIT) : 
	v.type == svt_mobj ? -1 : v.value.i );
}

//==========================================================================
//
//
//
//==========================================================================
fixed_t fixedvalue(const svalue_s & v)
{
	return (v.type == svt_fixed ? v.value.f :
	v.type == svt_string ? (fixed_t)(atof(v.value.s) * FRACUNIT) :
	v.type == svt_mobj ? -1*FRACUNIT : intvalue(v) * FRACUNIT );
}

//==========================================================================
//
//
//
//==========================================================================
float floatvalue(const svalue_s & v)
{
	return (float)( (v.type == svt_string ? atof(v.value.s) :       
	v.type == svt_fixed ? (int)(v.value.f / (float)FRACUNIT) : 
	v.type == svt_mobj ? -1 : v.value.i ));
}

//==========================================================================
//
// haleyjd: 8-17
//
// sf: string value of an svalue_t
//
//==========================================================================
const char *stringvalue(const svalue_t & v)
{
	static char buffer[256];
	
	switch(v.type)
    {
	case svt_string:
		return v.value.s;
		
	case svt_mobj:
		// return the class name
		return (const char *)v.value.mobj->GetClass()->TypeName;
		
	case svt_fixed:
		{
			double val = ((double)v.value.f) / FRACUNIT;
			sprintf(buffer, "%g", val);
			return buffer;
		}
		
	case svt_int:
	default:
        sprintf(buffer, "%li", v.value.i);  // haleyjd: should be %li, not %i
		return buffer;	
    }
}


//==========================================================================
//
// Some functions that take care of the major differences between the class
// based actor system from ZDoom and Doom's index based one
//
//==========================================================================

//==========================================================================
//
// Gets an actor class
// Input can be either a class name, an actor variable or a Doom index
// Doom index is only supported for the original things up to MBF
//
//==========================================================================
const PClass *T_GetMobjType(svalue_t arg)
{
	const PClass * PClass=NULL;
	
	if (arg.type==svt_string)
	{
		PClass=PClass::FindClass(arg.value.s);
	}
	else if (arg.type==svt_mobj)
	{
		AActor * mo = MobjForSvalue(arg);
		if (mo) PClass = mo->GetClass();
	}
	else
	{
		int objtype = intvalue(arg);
		if (objtype>=0 && objtype<countof(ActorTypes)) PClass=ActorTypes[objtype];
		else PClass=NULL;
	}

	// invalid object to spawn. Print a warning but let it pass
	if(!PClass) Printf("FS: unknown object type: %s\n", arg.value.s); 
	return PClass;
}

//==========================================================================
//
// Gets a player index
// Input can be either an actor variable or an index value
//
//==========================================================================
int T_GetPlayerNum(svalue_t arg)
{
	int playernum;
	if(arg.type == svt_mobj)
	{
		if(!MobjForSvalue(arg) || !arg.value.mobj->player)
		{
			// I prefer not to make this an error.
			// This way a player function used for a non-player
			// object will just do nothing
			//script_error("mobj not a player!\n");
			return -1;
		}
		playernum = int(arg.value.mobj->player - players);
	}
	else
		playernum = intvalue(arg);
	
	if(playernum < 0 || playernum > MAXPLAYERS || !playeringame[playernum])
	{
		return -1;
	}
	return playernum;
}

//==========================================================================
//
// Finds a sector from a tag. This has been extended to allow looking for
// sectors directly by passing a negative value
//
//==========================================================================
int T_FindSectorFromTag(int tagnum,int startsector)
{
	if (tagnum<=0)
	{
		if (startsector<0)
		{
			if (tagnum==-32768) return 0;
			if (-tagnum<numsectors) return -tagnum;
		}
		return -1;
	}
	return P_FindSectorFromTag(tagnum,startsector);
}


//==========================================================================
//
// Get an ammo type
// Input can be either a class name or a Doom index
// Doom index is only supported for the 4 original ammo types
//
//==========================================================================
const PClass *T_GetAmmo(svalue_t t)
{
	const char * p;

	if (t.type==svt_string)
	{
		p=stringvalue(t);
	}
	else	
	{
		// backwards compatibility with Legacy.
		// allow only Doom's standard types here!
		static const char * DefAmmo[]={"Clip","Shell","Cell","RocketAmmo"};
		int ammonum = intvalue(t);
		if(ammonum < 0 || ammonum >= 4)	
		{
			return NULL;
		}
		p=DefAmmo[ammonum];
	}
	const PClass * am=PClass::FindClass(p);
	if (!am->IsDescendantOf(RUNTIME_CLASS(AAmmo)))
	{
		return NULL;
	}
	return am;

}

//==========================================================================
//
// Finds a sound in the sound table and adds a new entry if it isn't defined
// It's too bad that this is necessary but FS doesn't know about this kind
// of sound management.
//
//==========================================================================
int T_FindSound(const char * name)
{
	char buffer[40];
	int so=S_FindSound(name);

	if (so>0) return so;

	// Now it gets dirty!

	if (gameinfo.gametype & GAME_DoomStrife)
	{
		sprintf(buffer, "DS%.35s", name);
		if (Wads.CheckNumForName(buffer, ns_sounds)<0) strcpy(buffer, name);
	}
	else
	{
		strcpy(buffer, name);
		if (Wads.CheckNumForName(buffer, ns_sounds)<0) sprintf(buffer, "DS%.35s", name);
	}
	
	so=S_AddSound(name, buffer);
	S_HashSounds();
	return so;
}


//==========================================================================
//
//
//
//==========================================================================
bool FS_ChangeMusic(const char * string)
{
	char buffer[40];

	if (Wads.CheckNumForName(string, ns_music)<0 || !S_ChangeMusic(string,true))
	{
		// Retry with O_ prepended to the music name, then with D_
		sprintf(buffer, "O_%s", string);
		if (Wads.CheckNumForName(buffer, ns_music)<0 || !S_ChangeMusic(buffer,true))
		{
			sprintf(buffer, "D_%s", string);
			if (Wads.CheckNumForName(buffer, ns_music)<0) 
			{
				S_ChangeMusic(NULL, 0);
				return false;
			}
			else S_ChangeMusic(buffer,true);
		}
	}
	return true;
}

//============================================================================
//
// Since FraggleScript is rather hard coded to the original inventory
// handling of Doom this is a little messy.
//
//============================================================================


//============================================================================
//
// DoGiveInv
//
// Gives an item to a single actor.
//
//============================================================================

void FS_GiveInventory (AActor *actor, const char * type, int amount)
{
	if (amount <= 0)
	{
		return;
	}
	if (strcmp (type, "Armor") == 0)
	{
		type = "BasicArmorPickup";
	}
	const PClass * info = PClass::FindClass (type);
	if (info == NULL || !info->IsDescendantOf (RUNTIME_CLASS(AInventory)))
	{
		Printf ("Unknown inventory item: %s\n", type);
		return;
	}

	AWeapon *savedPendingWeap = actor->player != NULL? actor->player->PendingWeapon : NULL;
	bool hadweap = actor->player != NULL ? actor->player->ReadyWeapon != NULL : true;

	AInventory *item = static_cast<AInventory *>(Spawn (info, 0,0,0, NO_REPLACE));

	// This shouldn't count for the item statistics!
	if (item->flags&MF_COUNTITEM) 
	{
		level.total_items--;
		item->flags&=~MF_COUNTITEM;
	}
	if (info->IsDescendantOf (RUNTIME_CLASS(ABasicArmorPickup)) ||
		info->IsDescendantOf (RUNTIME_CLASS(ABasicArmorBonus)))
	{
		static_cast<ABasicArmorPickup*>(item)->SaveAmount *= amount;
	}
	else
	{
		item->Amount = amount;
	}
	if (!item->TryPickup (actor))
	{
		item->Destroy ();
	}
	// If the item was a weapon, don't bring it up automatically
	// unless the player was not already using a weapon.
	if (savedPendingWeap != NULL && hadweap)
	{
		actor->player->PendingWeapon = savedPendingWeap;
	}
}

//============================================================================
//
// DoTakeInv
//
// Takes an item from a single actor.
//
//============================================================================

void FS_TakeInventory (AActor *actor, const char * type, int amount)
{
	if (strcmp (type, "Armor") == 0)
	{
		type = "BasicArmor";
	}
	if (amount <= 0)
	{
		return;
	}
	const PClass * info = PClass::FindClass (type);
	if (info == NULL)
	{
		return;
	}

	AInventory *item = actor->FindInventory (info);
	if (item != NULL)
	{
		item->Amount -= amount;
		if (item->Amount <= 0)
		{
			// If it's not ammo, destroy it. Ammo needs to stick around, even
			// when it's zero for the benefit of the weapons that use it and 
			// to maintain the maximum ammo amounts a backpack might have given.
			if (item->GetClass()->ParentClass != RUNTIME_CLASS(AAmmo))
			{
				item->Destroy ();
			}
			else
			{
				item->Amount = 0;
			}
		}
	}
}

//============================================================================
//
// CheckInventory
//
// Returns how much of a particular item an actor has.
//
//============================================================================

int FS_CheckInventory (AActor *activator, const char *type)
{
	if (activator == NULL)
		return 0;

	if (strcmp (type, "Armor") == 0)
	{
		type = "BasicArmor";
	}
	else if (strcmp (type, "Health") == 0)
	{
		return activator->health;
	}

	const PClass *info = PClass::FindClass (type);
	AInventory *item = activator->FindInventory (info);
	return item ? item->Amount : 0;
}



//==========================================================================
//
// Simple light fade - locks lightingdata. For SF_FadeLight
//
//==========================================================================


IMPLEMENT_CLASS (DLightLevel)

//==========================================================================
//
//==========================================================================

void DLightLevel::Destroy() 
{ 
	Super::Destroy(); 
	m_Sector->lightingdata=NULL; 
}

//==========================================================================
//
//==========================================================================

void DLightLevel::Serialize (FArchive &arc)
{
	Super::Serialize (arc);
	arc << destlevel << speed;
	if (arc.IsLoading()) m_Sector->lightingdata=this;
}


//==========================================================================
// sf 13/10/99:
//
// T_LightFade()
//
// Just fade the light level in a sector to a new level
//
//==========================================================================

void DLightLevel::Tick()
{
	Super::Tick();
	if(m_Sector->lightlevel < destlevel)
	{
		// increase the lightlevel
		if(m_Sector->lightlevel + speed >= destlevel)
		{
			// stop changing light level
			m_Sector->lightlevel = destlevel;    // set to dest lightlevel
			Destroy();
		}
		else
		{
			m_Sector->lightlevel = m_Sector->lightlevel+speed;
		}
	}
	else
	{
        // decrease lightlevel
		if(m_Sector->lightlevel - speed <= destlevel)
		{
			// stop changing light level
			m_Sector->lightlevel = destlevel;    // set to dest lightlevel
			Destroy();
		}
		else
		{
			m_Sector->lightlevel = m_Sector->lightlevel-speed;
		}
	}
}

//==========================================================================
//
//==========================================================================
DLightLevel::DLightLevel(sector_t * s,int _destlevel,int _speed) : DLighting(s)
{
	destlevel=_destlevel;
	speed=_speed;
	s->lightingdata=this;
}


//==========================================================================
//
//==========================================================================
CCMD(fpuke)
{
	int argc = argv.argc();

	if (argc < 2)
	{
		Printf (" fpuke <script>\n");
	}
	else
	{
		TThinkerIterator<DFraggleThinker> it;
		DFraggleThinker *th = it.Next();
		if (th)
			th->RunScript(atoi (argv[1]), NULL);
	}
}

