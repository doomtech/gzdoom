/*
#include "actor.h"
#include "info.h"
#include "s_sound.h"
#include "m_random.h"
#include "a_pickups.h"
#include "d_player.h"
#include "p_pspr.h"
#include "p_local.h"
#include "gstrings.h"
#include "p_effect.h"
#include "gi.h"
#include "templates.h"
#include "thingdef/thingdef.h"
#include "doomstat.h"
*/

static FRandom pr_punch ("Punch");
static FRandom pr_saw ("Saw");
static FRandom pr_fireshotgun2 ("FireSG2");
static FRandom pr_fireplasma ("FirePlasma");
static FRandom pr_firerail ("FireRail");
static FRandom pr_bfgspray ("BFGSpray");

//
// A_Punch
//
DEFINE_ACTION_FUNCTION(AActor, A_Punch)
{
	angle_t 	angle;
	int 		damage;
	int 		pitch;
	AActor		*linetarget;

	if (self->player != NULL)
	{
		AWeapon *weapon = self->player->ReadyWeapon;
		if (weapon != NULL)
		{
			if (!weapon->DepleteAmmo (weapon->bAltFire))
				return;
		}
	}

	damage = (pr_punch()%10+1)<<1;

	if (self->FindInventory<APowerStrength>())	
		damage *= 10;

	angle = self->angle;

	angle += pr_punch.Random2() << 18;
	pitch = P_AimLineAttack (self, angle, MELEERANGE, &linetarget);
	P_LineAttack (self, angle, MELEERANGE, pitch, damage, NAME_Melee, NAME_BulletPuff, true);

	// turn to face target
	if (linetarget)
	{
		S_Sound (self, CHAN_WEAPON, "*fist", 1, ATTN_NORM);
		self->angle = R_PointToAngle2 (self->x,
										self->y,
										linetarget->x,
										linetarget->y);
	}
}

//
// A_FirePistol
//
DEFINE_ACTION_FUNCTION(AActor, A_FirePistol)
{
	bool accurate;

	if (self->player != NULL)
	{
		AWeapon *weapon = self->player->ReadyWeapon;
		if (weapon != NULL)
		{
			if (!weapon->DepleteAmmo (weapon->bAltFire))
				return;

			P_SetPsprite (self->player, ps_flash, weapon->FindState(NAME_Flash));
		}
		self->player->mo->PlayAttacking2 ();

		accurate = !self->player->refire;
	}
	else
	{
		accurate = true;
	}

	S_Sound (self, CHAN_WEAPON, "weapons/pistol", 1, ATTN_NORM);

	P_GunShot (self, accurate, PClass::FindClass(NAME_BulletPuff), P_BulletSlope (self));
}

//
// A_Saw
//
DEFINE_ACTION_FUNCTION_PARAMS(AActor, A_Saw)
{
	angle_t 	angle;
	player_t *player;
	AActor *linetarget;

	ACTION_PARAM_START(4);
	ACTION_PARAM_SOUND(fullsound, 0);
	ACTION_PARAM_SOUND(hitsound, 1);
	ACTION_PARAM_INT(damage, 2);
	ACTION_PARAM_CLASS(pufftype, 3);

	if (NULL == (player = self->player))
	{
		return;
	}

	AWeapon *weapon = self->player->ReadyWeapon;
	if (weapon != NULL)
	{
		if (!weapon->DepleteAmmo (weapon->bAltFire))
			return;
	}

	if (pufftype == NULL) pufftype = PClass::FindClass(NAME_BulletPuff);
	if (damage == 0) damage = 2;
	
	damage *= (pr_saw()%10+1);
	angle = self->angle;
	angle += pr_saw.Random2() << 18;
	
	// use meleerange + 1 so the puff doesn't skip the flash (i.e. plays all states)
	P_LineAttack (self, angle, MELEERANGE+1,
				  P_AimLineAttack (self, angle, MELEERANGE+1, &linetarget), damage,
				  GetDefaultByType(pufftype)->DamageType, pufftype);

	if (!linetarget)
	{
		S_Sound (self, CHAN_WEAPON, fullsound, 1, ATTN_NORM);
		return;
	}
	S_Sound (self, CHAN_WEAPON, hitsound, 1, ATTN_NORM);
		
	// turn to face target
	angle = R_PointToAngle2 (self->x, self->y,
							 linetarget->x, linetarget->y);
	if (angle - self->angle > ANG180)
	{
		if (angle - self->angle < (angle_t)(-ANG90/20))
			self->angle = angle + ANG90/21;
		else
			self->angle -= ANG90/20;
	}
	else
	{
		if (angle - self->angle > ANG90/20)
			self->angle = angle - ANG90/21;
		else
			self->angle += ANG90/20;
	}
	self->flags |= MF_JUSTATTACKED;
}

//
// A_FireShotgun
//
DEFINE_ACTION_FUNCTION(AActor, A_FireShotgun)
{
	int i;
	player_t *player;

	if (NULL == (player = self->player))
	{
		return;
	}

	S_Sound (self, CHAN_WEAPON,  "weapons/shotgf", 1, ATTN_NORM);
	AWeapon *weapon = self->player->ReadyWeapon;
	if (weapon != NULL)
	{
		if (!weapon->DepleteAmmo (weapon->bAltFire))
			return;
		P_SetPsprite (player, ps_flash, weapon->FindState(NAME_Flash));
	}
	player->mo->PlayAttacking2 ();

	angle_t pitch = P_BulletSlope (self);

	for (i=0 ; i<7 ; i++)
		P_GunShot (self, false, PClass::FindClass(NAME_BulletPuff), pitch);
}

//
// A_FireShotgun2
//
DEFINE_ACTION_FUNCTION(AActor, A_FireShotgun2)
{
	int 		i;
	angle_t 	angle;
	int 		damage;
	player_t *player;

	if (NULL == (player = self->player))
	{
		return;
	}

	S_Sound (self, CHAN_WEAPON, "weapons/sshotf", 1, ATTN_NORM);
	AWeapon *weapon = self->player->ReadyWeapon;
	if (weapon != NULL)
	{
		if (!weapon->DepleteAmmo (weapon->bAltFire))
			return;
		P_SetPsprite (player, ps_flash, weapon->FindState(NAME_Flash));
	}
	player->mo->PlayAttacking2 ();


	angle_t pitch = P_BulletSlope (self);
		
	for (i=0 ; i<20 ; i++)
	{
		damage = 5*(pr_fireshotgun2()%3+1);
		angle = self->angle;
		angle += pr_fireshotgun2.Random2() << 19;

		// Doom adjusts the bullet slope by shifting a random number [-255,255]
		// left 5 places. At 2048 units away, this means the vertical position
		// of the shot can deviate as much as 255 units from nominal. So using
		// some simple trigonometry, that means the vertical angle of the shot
		// can deviate by as many as ~7.097 degrees or ~84676099 BAMs.

		P_LineAttack (self,
					  angle,
					  PLAYERMISSILERANGE,
					  pitch + (pr_fireshotgun2.Random2() * 332063), damage,
					  NAME_None, NAME_BulletPuff);
	}
}

DEFINE_ACTION_FUNCTION(AActor, A_OpenShotgun2)
{
	S_Sound (self, CHAN_WEAPON, "weapons/sshoto", 1, ATTN_NORM);
}

DEFINE_ACTION_FUNCTION(AActor, A_LoadShotgun2)
{
	S_Sound (self, CHAN_WEAPON, "weapons/sshotl", 1, ATTN_NORM);
}

DEFINE_ACTION_FUNCTION(AActor, A_CloseShotgun2)
{
	S_Sound (self, CHAN_WEAPON, "weapons/sshotc", 1, ATTN_NORM);
	CALL_ACTION(A_ReFire, self);
}


//------------------------------------------------------------------------------------
//
// Setting a random flash like some of Doom's weapons can easily crash when the
// definition is overridden incorrectly so let's check that the state actually exists.
// Be aware though that this will not catch all DEHACKED related problems. But it will
// find all DECORATE related ones.
//
//------------------------------------------------------------------------------------

void P_SetSafeFlash(AWeapon * weapon, player_t * player, FState * flashstate, int index)
{

	const PClass * cls = weapon->GetClass();
	while (cls != RUNTIME_CLASS(AWeapon))
	{
		FActorInfo * info = cls->ActorInfo;
		if (flashstate >= info->OwnedStates && flashstate < info->OwnedStates + info->NumOwnedStates)
		{
			// The flash state belongs to this class.
			// Now let's check if the actually wanted state does also
			if (flashstate+index < info->OwnedStates + info->NumOwnedStates)
			{
				// we're ok so set the state
				P_SetPsprite (player, ps_flash, flashstate + index);
				return;
			}
			else
			{
				// oh, no! The state is beyond the end of the state table so use the original flash state.
				P_SetPsprite (player, ps_flash, flashstate);
				return;
			}
		}
		// try again with parent class
		cls = cls->ParentClass;
	}
	// if we get here the state doesn't seem to belong to any class in the inheritance chain
	// This can happen with Dehacked if the flash states are remapped. 
	// The only way to check this would be to go through all Dehacked modifiable actors and
	// find the correct one.
	// For now let's assume that it will work.
	P_SetPsprite (player, ps_flash, flashstate + index);
}

//
// A_FireCGun
//
DEFINE_ACTION_FUNCTION(AActor, A_FireCGun)
{
	player_t *player;

	if (self == NULL || NULL == (player = self->player))
	{
		return;
	}

	AWeapon *weapon = player->ReadyWeapon;
	if (weapon != NULL)
	{
		if (!weapon->DepleteAmmo (weapon->bAltFire))
			return;
		
		S_Sound (self, CHAN_WEAPON, "weapons/chngun", 1, ATTN_NORM);

		FState *flash = weapon->FindState(NAME_Flash);
		if (flash != NULL)
		{
			// [RH] Fix for Sparky's messed-up Dehacked patch! Blargh!
			FState * atk = weapon->FindState(NAME_Fire);

			int theflash = clamp (int(player->psprites[ps_weapon].state - atk), 0, 1);

			if (flash[theflash].sprite != flash->sprite)
			{
				theflash = 0;
			}

			P_SetSafeFlash (weapon, player, flash, theflash);
		}

	}
	player->mo->PlayAttacking2 ();

	P_GunShot (self, !player->refire, PClass::FindClass(NAME_BulletPuff), P_BulletSlope (self));
}

//
// A_FireMissile
//
DEFINE_ACTION_FUNCTION(AActor, A_FireMissile)
{
	player_t *player;

	if (NULL == (player = self->player))
	{
		return;
	}
	AWeapon *weapon = self->player->ReadyWeapon;
	if (weapon != NULL)
	{
		if (!weapon->DepleteAmmo (weapon->bAltFire))
			return;
	}
	P_SpawnPlayerMissile (self, PClass::FindClass("Rocket"));
}

//
// A_FirePlasma
//
DEFINE_ACTION_FUNCTION(AActor, A_FirePlasma)
{
	player_t *player;

	if (NULL == (player = self->player))
	{
		return;
	}
	AWeapon *weapon = self->player->ReadyWeapon;
	if (weapon != NULL)
	{
		if (!weapon->DepleteAmmo (weapon->bAltFire))
			return;

		FState *flash = weapon->FindState(NAME_Flash);
		if (flash != NULL)
		{
			P_SetSafeFlash(weapon, player, flash, (pr_fireplasma()&1));
		}
	}

	P_SpawnPlayerMissile (self, PClass::FindClass("PlasmaBall"));
}

//
// [RH] A_FireRailgun
//
static void FireRailgun(AActor *self, int RailOffset)
{
	int damage;
	player_t *player;

	if (NULL == (player = self->player))
	{
		return;
	}

	AWeapon *weapon = self->player->ReadyWeapon;
	if (weapon != NULL)
	{
		if (!weapon->DepleteAmmo (weapon->bAltFire))
			return;

		FState *flash = weapon->FindState(NAME_Flash);
		if (flash != NULL)
		{
			P_SetSafeFlash(weapon, player, flash, (pr_firerail()&1));
		}
	}

	damage = deathmatch ? 100 : 150;

	P_RailAttack (self, damage, RailOffset);
}


DEFINE_ACTION_FUNCTION(AActor, A_FireRailgun)
{
	FireRailgun(self, 0);
}

DEFINE_ACTION_FUNCTION(AActor, A_FireRailgunRight)
{
	FireRailgun(self, 10);
}

DEFINE_ACTION_FUNCTION(AActor, A_FireRailgunLeft)
{
	FireRailgun(self, -10);
}

DEFINE_ACTION_FUNCTION(AActor, A_RailWait)
{
	// Okay, this was stupid. Just use a NULL function instead of this.
}

//
// A_FireBFG
//

DEFINE_ACTION_FUNCTION(AActor, A_FireBFG)
{
	player_t *player;

	if (NULL == (player = self->player))
	{
		return;
	}

	AWeapon *weapon = self->player->ReadyWeapon;
	if (weapon != NULL)
	{
		if (!weapon->DepleteAmmo (weapon->bAltFire))
			return;
	}

	P_SpawnPlayerMissile (self,  0, 0, 0, PClass::FindClass("BFGBall"), self->angle, NULL, NULL, !!(dmflags2 & DF2_NO_FREEAIMBFG));
}

//
// A_BFGSpray
// Spawn a BFG explosion on every monster in view
//
DEFINE_ACTION_FUNCTION_PARAMS(AActor, A_BFGSpray)
{
	int 				i;
	int 				j;
	int 				damage;
	angle_t 			an;
	AActor				*thingToHit;
	AActor				*linetarget;

	ACTION_PARAM_START(3);
	ACTION_PARAM_CLASS(spraytype, 0);
	ACTION_PARAM_INT(numrays, 1);
	ACTION_PARAM_INT(damagecnt, 2);

	if (spraytype == NULL) spraytype = PClass::FindClass("BFGExtra");
	if (numrays <= 0) numrays = 40;
	if (damagecnt <= 0) damagecnt = 15;

	// [RH] Don't crash if no target
	if (!self->target)
		return;

	// offset angles from its attack angle
	for (i = 0; i < numrays; i++)
	{
		an = self->angle - ANG90/2 + ANG90/numrays*i;

		// self->target is the originator (player) of the missile
		P_AimLineAttack (self->target, an, 16*64*FRACUNIT, &linetarget, ANGLE_1*32);

		if (!linetarget)
			continue;

		AActor *spray = Spawn (spraytype, linetarget->x, linetarget->y,
			linetarget->z + (linetarget->height>>2), ALLOW_REPLACE);

		if (spray && (spray->flags5 & MF5_PUFFGETSOWNER))
			spray->target = self->target;

		
		damage = 0;
		for (j = 0; j < damagecnt; ++j)
			damage += (pr_bfgspray() & 7) + 1;

		thingToHit = linetarget;
		P_DamageMobj (thingToHit, self->target, self->target, damage, NAME_BFGSplash);
		P_TraceBleed (damage, thingToHit, self->target);
	}
}

//
// A_BFGsound
//
DEFINE_ACTION_FUNCTION(AActor, A_BFGsound)
{
	S_Sound (self, CHAN_WEAPON, "weapons/bfgf", 1, ATTN_NORM);
}

