/*
#include "templates.h"
#include "actor.h"
#include "info.h"
#include "s_sound.h"
#include "m_random.h"
#include "a_pickups.h"
#include "d_player.h"
#include "p_pspr.h"
#include "p_local.h"
#include "gstrings.h"
#include "gi.h"
#include "r_translate.h"
#include "thingdef/thingdef.h"
#include "doomstat.h"
*/

static FRandom pr_sap ("StaffAtkPL1");
static FRandom pr_sap2 ("StaffAtkPL2");
static FRandom pr_fgw ("FireWandPL1");
static FRandom pr_fgw2 ("FireWandPL2");
static FRandom pr_boltspark ("BoltSpark");
static FRandom pr_macerespawn ("MaceRespawn");
static FRandom pr_maceatk ("FireMacePL1");
static FRandom pr_gatk ("GauntletAttack");
static FRandom pr_bfx1 ("BlasterFX1");
static FRandom pr_ripd ("RipperD");
static FRandom pr_fb1 ("FireBlasterPL1");
static FRandom pr_bfx1t ("BlasterFX1Tick");
static FRandom pr_hrfx2 ("HornRodFX2");
static FRandom pr_rp ("RainPillar");
static FRandom pr_fsr1 ("FireSkullRodPL1");
static FRandom pr_storm ("SkullRodStorm");
static FRandom pr_impact ("RainImpact");
static FRandom pr_pfx1 ("PhoenixFX1");
static FRandom pr_pfx2 ("PhoenixFX2");
static FRandom pr_fp2 ("FirePhoenixPL2");

#define FLAME_THROWER_TICS (10*TICRATE)

void P_DSparilTeleport (AActor *actor);

#define USE_BLSR_AMMO_1 1
#define USE_BLSR_AMMO_2 5
#define USE_SKRD_AMMO_1 1
#define USE_SKRD_AMMO_2 5
#define USE_PHRD_AMMO_1 1
#define USE_PHRD_AMMO_2 1
#define USE_MACE_AMMO_1 1
#define USE_MACE_AMMO_2 5

extern bool P_AutoUseChaosDevice (player_t *player);

// --- Staff ----------------------------------------------------------------

//----------------------------------------------------------------------------
//
// PROC A_StaffAttackPL1
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION_PARAMS(AActor, A_StaffAttack)
{
	angle_t angle;
	int slope;
	player_t *player;
	AActor *linetarget;

	if (NULL == (player = self->player))
	{
		return;
	}

	ACTION_PARAM_START(2);
	ACTION_PARAM_INT(damage, 0);
	ACTION_PARAM_CLASS(puff, 1);

	AWeapon *weapon = player->ReadyWeapon;
	if (weapon != NULL)
	{
		if (!weapon->DepleteAmmo (weapon->bAltFire))
			return;
	}
	if (puff == NULL) puff = PClass::FindClass(NAME_BulletPuff);	// just to be sure
	angle = self->angle;
	angle += pr_sap.Random2() << 18;
	slope = P_AimLineAttack (self, angle, MELEERANGE, &linetarget);
	P_LineAttack (self, angle, MELEERANGE, slope, damage, NAME_Melee, puff, true);
	if (linetarget)
	{
		//S_StartSound(player->mo, sfx_stfhit);
		// turn to face target
		self->angle = R_PointToAngle2 (self->x,
			self->y, linetarget->x, linetarget->y);
	}
}


//----------------------------------------------------------------------------
//
// PROC A_FireGoldWandPL1
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_FireGoldWandPL1)
{
	angle_t angle;
	int damage;
	player_t *player;

	if (NULL == (player = self->player))
	{
		return;
	}

	AWeapon *weapon = player->ReadyWeapon;
	if (weapon != NULL)
	{
		if (!weapon->DepleteAmmo (weapon->bAltFire))
			return;
	}
	angle_t pitch = P_BulletSlope(self);
	damage = 7+(pr_fgw()&7);
	angle = self->angle;
	if (player->refire)
	{
		angle += pr_fgw.Random2() << 18;
	}
	P_LineAttack (self, angle, PLAYERMISSILERANGE, pitch, damage, NAME_None, "GoldWandPuff1");
	S_Sound (self, CHAN_WEAPON, "weapons/wandhit", 1, ATTN_NORM);
}

//----------------------------------------------------------------------------
//
// PROC A_FireGoldWandPL2
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_FireGoldWandPL2)
{
	int i;
	angle_t angle;
	int damage;
	fixed_t momz;
	player_t *player;

	if (NULL == (player = self->player))
	{
		return;
	}

	AWeapon *weapon = player->ReadyWeapon;
	if (weapon != NULL)
	{
		if (!weapon->DepleteAmmo (weapon->bAltFire))
			return;
	}
	angle_t pitch = P_BulletSlope(self);
	momz = FixedMul (GetDefaultByName("GoldWandFX2")->Speed,
		finetangent[FINEANGLES/4-((signed)pitch>>ANGLETOFINESHIFT)]);
	P_SpawnMissileAngle (self, PClass::FindClass("GoldWandFX2"), self->angle-(ANG45/8), momz);
	P_SpawnMissileAngle (self, PClass::FindClass("GoldWandFX2"), self->angle+(ANG45/8), momz);
	angle = self->angle-(ANG45/8);
	for(i = 0; i < 5; i++)
	{
		damage = 1+(pr_fgw2()&7);
		P_LineAttack (self, angle, PLAYERMISSILERANGE, pitch, damage, NAME_None, "GoldWandPuff2");
		angle += ((ANG45/8)*2)/4;
	}
	S_Sound (self, CHAN_WEAPON, "weapons/wandhit", 1, ATTN_NORM);
}

//----------------------------------------------------------------------------
//
// PROC A_FireCrossbowPL1
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_FireCrossbowPL1)
{
	player_t *player;

	if (NULL == (player = self->player))
	{
		return;
	}

	AWeapon *weapon = player->ReadyWeapon;
	if (weapon != NULL)
	{
		if (!weapon->DepleteAmmo (weapon->bAltFire))
			return;
	}
	P_SpawnPlayerMissile (self, PClass::FindClass("CrossbowFX1"));
	P_SpawnPlayerMissile (self, PClass::FindClass("CrossbowFX3"), self->angle-(ANG45/10));
	P_SpawnPlayerMissile (self, PClass::FindClass("CrossbowFX3"), self->angle+(ANG45/10));
}

//----------------------------------------------------------------------------
//
// PROC A_FireCrossbowPL2
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_FireCrossbowPL2)
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
	P_SpawnPlayerMissile (self, PClass::FindClass("CrossbowFX2"));
	P_SpawnPlayerMissile (self, PClass::FindClass("CrossbowFX2"), self->angle-(ANG45/10));
	P_SpawnPlayerMissile (self, PClass::FindClass("CrossbowFX2"), self->angle+(ANG45/10));
	P_SpawnPlayerMissile (self, PClass::FindClass("CrossbowFX3"), self->angle-(ANG45/5));
	P_SpawnPlayerMissile (self, PClass::FindClass("CrossbowFX3"), self->angle+(ANG45/5));
}

//---------------------------------------------------------------------------
//
// PROC A_GauntletAttack
//
//---------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION_PARAMS(AActor, A_GauntletAttack)
{
	angle_t angle;
	int damage;
	int slope;
	int randVal;
	fixed_t dist;
	player_t *player;
	const PClass *pufftype;
	AActor *linetarget;

	if (NULL == (player = self->player))
	{
		return;
	}

	ACTION_PARAM_START(1);
	ACTION_PARAM_INT(power, 0);

	AWeapon *weapon = player->ReadyWeapon;
	if (weapon != NULL)
	{
		if (!weapon->DepleteAmmo (weapon->bAltFire))
			return;
	}
	player->psprites[ps_weapon].sx = ((pr_gatk()&3)-2) * FRACUNIT;
	player->psprites[ps_weapon].sy = WEAPONTOP + (pr_gatk()&3) * FRACUNIT;
	angle = self->angle;
	if (power)
	{
		damage = pr_gatk.HitDice (2);
		dist = 4*MELEERANGE;
		angle += pr_gatk.Random2() << 17;
		pufftype = PClass::FindClass("GauntletPuff2");
	}
	else
	{
		damage = pr_gatk.HitDice (2);
		dist = MELEERANGE+1;
		angle += pr_gatk.Random2() << 18;
		pufftype = PClass::FindClass("GauntletPuff1");
	}
	slope = P_AimLineAttack (self, angle, dist, &linetarget);
	P_LineAttack (self, angle, dist, slope, damage, NAME_Melee, pufftype);
	if (!linetarget)
	{
		if (pr_gatk() > 64)
		{
			player->extralight = !player->extralight;
		}
		S_Sound (self, CHAN_AUTO, "weapons/gauntletson", 1, ATTN_NORM);
		return;
	}
	randVal = pr_gatk();
	if (randVal < 64)
	{
		player->extralight = 0;
	}
	else if (randVal < 160)
	{
		player->extralight = 1;
	}
	else
	{
		player->extralight = 2;
	}
	if (power)
	{
		P_GiveBody (self, damage>>1);
		S_Sound (self, CHAN_AUTO, "weapons/gauntletspowhit", 1, ATTN_NORM);
	}
	else
	{
		S_Sound (self, CHAN_AUTO, "weapons/gauntletshit", 1, ATTN_NORM);
	}
	// turn to face target
	angle = R_PointToAngle2 (self->x, self->y,
		linetarget->x, linetarget->y);
	if (angle-self->angle > ANG180)
	{
		if ((int)(angle-self->angle) < -ANG90/20)
			self->angle = angle+ANG90/21;
		else
			self->angle -= ANG90/20;
	}
	else
	{
		if (angle-self->angle > ANG90/20)
			self->angle = angle-ANG90/21;
		else
			self->angle += ANG90/20;
	}
	self->flags |= MF_JUSTATTACKED;
}

// --- Mace -----------------------------------------------------------------

#define MAGIC_JUNK 1234

// Mace FX4 -----------------------------------------------------------------

class AMaceFX4 : public AActor
{
	DECLARE_CLASS (AMaceFX4, AActor)
public:
	int DoSpecialDamage (AActor *target, int damage);
};

IMPLEMENT_CLASS (AMaceFX4)

int AMaceFX4::DoSpecialDamage (AActor *target, int damage)
{
	if ((target->flags2 & MF2_BOSS) || (target->flags3 & MF3_DONTSQUASH) || target->IsTeammate (this->target))
	{ // Don't allow cheap boss kills and don't instagib teammates
		return damage;
	}
	else if (target->player)
	{ // Player specific checks
		if (target->player->mo->flags2 & MF2_INVULNERABLE)
		{ // Can't hurt invulnerable players
			return -1;
		}
		if (P_AutoUseChaosDevice (target->player))
		{ // Player was saved using chaos device
			return -1;
		}
	}
	return 1000000; // Something's gonna die
}

//----------------------------------------------------------------------------
//
// PROC A_FireMacePL1B
//
//----------------------------------------------------------------------------

void FireMacePL1B (AActor *actor)
{
	AActor *ball;
	angle_t angle;
	player_t *player;

	if (NULL == (player = actor->player))
	{
		return;
	}

	AWeapon *weapon = player->ReadyWeapon;
	if (weapon != NULL)
	{
		if (!weapon->DepleteAmmo (weapon->bAltFire))
			return;
	}
	ball = Spawn("MaceFX2", actor->x, actor->y, actor->z + 28*FRACUNIT 
		- actor->floorclip, ALLOW_REPLACE);
	ball->momz = 2*FRACUNIT+/*((player->lookdir)<<(FRACBITS-5))*/
		finetangent[FINEANGLES/4-(actor->pitch>>ANGLETOFINESHIFT)];
	angle = actor->angle;
	ball->target = actor;
	ball->angle = angle;
	ball->z += 2*finetangent[FINEANGLES/4-(actor->pitch>>ANGLETOFINESHIFT)];
	angle >>= ANGLETOFINESHIFT;
	ball->momx = (actor->momx>>1)+FixedMul(ball->Speed, finecosine[angle]);
	ball->momy = (actor->momy>>1)+FixedMul(ball->Speed, finesine[angle]);
	S_Sound (ball, CHAN_BODY, "weapons/maceshoot", 1, ATTN_NORM);
	P_CheckMissileSpawn (ball);
}

//----------------------------------------------------------------------------
//
// PROC A_FireMacePL1
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_FireMacePL1)
{
	AActor *ball;
	player_t *player;

	if (NULL == (player = self->player))
	{
		return;
	}

	if (pr_maceatk() < 28)
	{
		FireMacePL1B (self);
		return;
	}
	AWeapon *weapon = player->ReadyWeapon;
	if (weapon != NULL)
	{
		if (!weapon->DepleteAmmo (weapon->bAltFire))
			return;
	}
	player->psprites[ps_weapon].sx = ((pr_maceatk()&3)-2)*FRACUNIT;
	player->psprites[ps_weapon].sy = WEAPONTOP+(pr_maceatk()&3)*FRACUNIT;
	ball = P_SpawnPlayerMissile (self, PClass::FindClass("MaceFX1"),
		self->angle+(((pr_maceatk()&7)-4)<<24));
	if (ball)
	{
		ball->special1 = 16; // tics till dropoff
	}
}

//----------------------------------------------------------------------------
//
// PROC A_MacePL1Check
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_MacePL1Check)
{
	if (self->special1 == 0)
	{
		return;
	}
	self->special1 -= 4;
	if (self->special1 > 0)
	{
		return;
	}
	self->special1 = 0;
	self->flags &= ~MF_NOGRAVITY;
	self->gravity = FRACUNIT/8;
	// [RH] Avoid some precision loss by scaling the momentum directly
#if 0
	angle_t angle = self->angle>>ANGLETOFINESHIFT;
	self->momx = FixedMul(7*FRACUNIT, finecosine[angle]);
	self->momy = FixedMul(7*FRACUNIT, finesine[angle]);
#else
	float momscale = sqrtf ((float)self->momx * (float)self->momx +
							(float)self->momy * (float)self->momy);
	momscale = 458752.f / momscale;
	self->momx = (int)(self->momx * momscale);
	self->momy = (int)(self->momy * momscale);
#endif
	self->momz -= self->momz>>1;
}

//----------------------------------------------------------------------------
//
// PROC A_MaceBallImpact
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_MaceBallImpact)
{
	if ((self->health != MAGIC_JUNK) && (self->flags & MF_INBOUNCE))
	{ // Bounce
		self->health = MAGIC_JUNK;
		self->momz = (self->momz * 192) >> 8;
		self->flags2 &= ~MF2_BOUNCETYPE;
		self->SetState (self->SpawnState);
		S_Sound (self, CHAN_BODY, "weapons/macebounce", 1, ATTN_NORM);
	}
	else
	{ // Explode
		self->momx = self->momy = self->momz = 0;
		self->flags |= MF_NOGRAVITY;
		self->gravity = FRACUNIT;
		S_Sound (self, CHAN_BODY, "weapons/macehit", 1, ATTN_NORM);
	}
}

//----------------------------------------------------------------------------
//
// PROC A_MaceBallImpact2
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_MaceBallImpact2)
{
	AActor *tiny;
	angle_t angle;

	if (self->flags & MF_INBOUNCE)
	{
		fixed_t floordist = self->z - self->floorz;
		fixed_t ceildist = self->ceilingz - self->z;
		fixed_t vel;

		if (floordist <= ceildist)
		{
			vel = MulScale32 (self->momz, self->Sector->floorplane.c);
		}
		else
		{
			vel = MulScale32 (self->momz, self->Sector->ceilingplane.c);
		}
		if (vel < 2)
		{
			goto boom;
		}

		// Bounce
		self->momz = (self->momz * 192) >> 8;
		self->SetState (self->SpawnState);

		tiny = Spawn("MaceFX3", self->x, self->y, self->z, ALLOW_REPLACE);
		angle = self->angle+ANG90;
		tiny->target = self->target;
		tiny->angle = angle;
		angle >>= ANGLETOFINESHIFT;
		tiny->momx = (self->momx>>1)+FixedMul(self->momz-FRACUNIT,
			finecosine[angle]);
		tiny->momy = (self->momy>>1)+FixedMul(self->momz-FRACUNIT,
			finesine[angle]);
		tiny->momz = self->momz;
		P_CheckMissileSpawn (tiny);

		tiny = Spawn("MaceFX3", self->x, self->y, self->z, ALLOW_REPLACE);
		angle = self->angle-ANG90;
		tiny->target = self->target;
		tiny->angle = angle;
		angle >>= ANGLETOFINESHIFT;
		tiny->momx = (self->momx>>1)+FixedMul(self->momz-FRACUNIT,
			finecosine[angle]);
		tiny->momy = (self->momy>>1)+FixedMul(self->momz-FRACUNIT,
			finesine[angle]);
		tiny->momz = self->momz;
		P_CheckMissileSpawn (tiny);
	}
	else
	{ // Explode
boom:
		self->momx = self->momy = self->momz = 0;
		self->flags |= MF_NOGRAVITY;
		self->flags2 &= ~MF2_BOUNCETYPE;
		self->gravity = FRACUNIT;
	}
}

//----------------------------------------------------------------------------
//
// PROC A_FireMacePL2
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_FireMacePL2)
{
	AActor *mo;
	player_t *player;
	AActor *linetarget;

	if (NULL == (player = self->player))
	{
		return;
	}

	AWeapon *weapon = player->ReadyWeapon;
	if (weapon != NULL)
	{
		if (!weapon->DepleteAmmo (weapon->bAltFire))
			return;
	}
	mo = P_SpawnPlayerMissile (self, 0,0,0, RUNTIME_CLASS(AMaceFX4), self->angle, &linetarget);
	if (mo)
	{
		mo->momx += self->momx;
		mo->momy += self->momy;
		mo->momz = 2*FRACUNIT+
			clamp<fixed_t>(finetangent[FINEANGLES/4-(self->pitch>>ANGLETOFINESHIFT)], -5*FRACUNIT, 5*FRACUNIT);
		if (linetarget)
		{
			mo->tracer = linetarget;
		}
	}
	S_Sound (self, CHAN_WEAPON, "weapons/maceshoot", 1, ATTN_NORM);
}

//----------------------------------------------------------------------------
//
// PROC A_DeathBallImpact
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_DeathBallImpact)
{
	int i;
	AActor *target;
	angle_t angle = 0;
	bool newAngle;
	AActor *linetarget;

	if ((self->z <= self->floorz) && P_HitFloor (self))
	{ // Landed in some sort of liquid
		self->Destroy ();
		return;
	}
	if (self->flags & MF_INBOUNCE)
	{
		fixed_t floordist = self->z - self->floorz;
		fixed_t ceildist = self->ceilingz - self->z;
		fixed_t vel;

		if (floordist <= ceildist)
		{
			vel = MulScale32 (self->momz, self->Sector->floorplane.c);
		}
		else
		{
			vel = MulScale32 (self->momz, self->Sector->ceilingplane.c);
		}
		if (vel < 2)
		{
			goto boom;
		}

		// Bounce
		newAngle = false;
		target = self->tracer;
		if (target)
		{
			if (!(target->flags&MF_SHOOTABLE))
			{ // Target died
				self->tracer = NULL;
			}
			else
			{ // Seek
				angle = R_PointToAngle2(self->x, self->y,
					target->x, target->y);
				newAngle = true;
			}
		}
		else
		{ // Find new target
			angle = 0;
			for (i = 0; i < 16; i++)
			{
				P_AimLineAttack (self, angle, 10*64*FRACUNIT, &linetarget);
				if (linetarget && self->target != linetarget)
				{
					self->tracer = linetarget;
					angle = R_PointToAngle2 (self->x, self->y,
						linetarget->x, linetarget->y);
					newAngle = true;
					break;
				}
				angle += ANGLE_45/2;
			}
		}
		if (newAngle)
		{
			self->angle = angle;
			angle >>= ANGLETOFINESHIFT;
			self->momx = FixedMul (self->Speed, finecosine[angle]);
			self->momy = FixedMul (self->Speed, finesine[angle]);
		}
		self->SetState (self->SpawnState);
		S_Sound (self, CHAN_BODY, "weapons/macestop", 1, ATTN_NORM);
	}
	else
	{ // Explode
boom:
		self->momx = self->momy = self->momz = 0;
		self->flags |= MF_NOGRAVITY;
		self->gravity = FRACUNIT;
		S_Sound (self, CHAN_BODY, "weapons/maceexplode", 1, ATTN_NORM);
	}
}


// Blaster FX 1 -------------------------------------------------------------

class ABlasterFX1 : public AActor
{
	DECLARE_CLASS(ABlasterFX1, AActor)
public:
	void Tick ();
	int DoSpecialDamage (AActor *target, int damage);
};


int ABlasterFX1::DoSpecialDamage (AActor *target, int damage)
{
	if (target->IsKindOf (PClass::FindClass ("Ironlich")))
	{ // Less damage to Ironlich bosses
		damage = pr_bfx1() & 1;
		if (!damage)
		{
			return -1;
		}
	}
	return damage;
}

IMPLEMENT_CLASS(ABlasterFX1)

// Ripper -------------------------------------------------------------------

class ARipper : public AActor
{
	DECLARE_CLASS (ARipper, AActor)
public:
	int DoSpecialDamage (AActor *target, int damage);
};

IMPLEMENT_CLASS(ARipper)

int ARipper::DoSpecialDamage (AActor *target, int damage)
{
	if (target->IsKindOf (PClass::FindClass ("Ironlich")))
	{ // Less damage to Ironlich bosses
		damage = pr_ripd() & 1;
		if (!damage)
		{
			return -1;
		}
	}
	return damage;
}

//----------------------------------------------------------------------------
//
// PROC A_FireBlasterPL1
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_FireBlasterPL1)
{
	angle_t angle;
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
	}
	angle_t pitch = P_BulletSlope(self);
	damage = pr_fb1.HitDice (4);
	angle = self->angle;
	if (player->refire)
	{
		angle += pr_fb1.Random2() << 18;
	}
	P_LineAttack (self, angle, PLAYERMISSILERANGE, pitch, damage, NAME_None, "BlasterPuff");
	S_Sound (self, CHAN_WEAPON, "weapons/blastershoot", 1, ATTN_NORM);
}

//----------------------------------------------------------------------------
//
// PROC A_SpawnRippers
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_SpawnRippers)
{
	int i;
	angle_t angle;
	AActor *ripper;

	for(i = 0; i < 8; i++)
	{
		ripper = Spawn<ARipper> (self->x, self->y, self->z, ALLOW_REPLACE);
		angle = i*ANG45;
		ripper->target = self->target;
		ripper->angle = angle;
		angle >>= ANGLETOFINESHIFT;
		ripper->momx = FixedMul (ripper->Speed, finecosine[angle]);
		ripper->momy = FixedMul (ripper->Speed, finesine[angle]);
		P_CheckMissileSpawn (ripper);
	}
}

//----------------------------------------------------------------------------
//
// PROC P_BlasterMobjThinker
//
// Thinker for the ultra-fast blaster PL2 ripper-spawning missile.
//
//----------------------------------------------------------------------------

void ABlasterFX1::Tick ()
{
	int i;
	fixed_t xfrac;
	fixed_t yfrac;
	fixed_t zfrac;
	int changexy;

	PrevX = x;
	PrevY = y;
	PrevZ = z;

	// Handle movement
	if (momx || momy || (z != floorz) || momz)
	{
		xfrac = momx>>3;
		yfrac = momy>>3;
		zfrac = momz>>3;
		changexy = xfrac | yfrac;
		for (i = 0; i < 8; i++)
		{
			if (changexy)
			{
				if (!P_TryMove (this, x + xfrac, y + yfrac, true))
				{ // Blocked move
					P_ExplodeMissile (this, BlockingLine, BlockingMobj);
					return;
				}
			}
			z += zfrac;
			if (z <= floorz)
			{ // Hit the floor
				z = floorz;
				P_HitFloor (this);
				P_ExplodeMissile (this, NULL, NULL);
				return;
			}
			if (z + height > ceilingz)
			{ // Hit the ceiling
				z = ceilingz - height;
				P_ExplodeMissile (this, NULL, NULL);
				return;
			}
			if (changexy && (pr_bfx1t() < 64))
			{
				Spawn("BlasterSmoke", x, y, MAX<fixed_t> (z - 8 * FRACUNIT, floorz), ALLOW_REPLACE);
			}
		}
	}
	// Advance the state
	if (tics != -1)
	{
		tics--;
		while (!tics)
		{
			if (!SetState (state->GetNextState ()))
			{ // mobj was removed
				return;
			}
		}
	}
}

// --- Skull rod ------------------------------------------------------------


// Horn Rod FX 2 ------------------------------------------------------------

class AHornRodFX2 : public AActor
{
	DECLARE_CLASS (AHornRodFX2, AActor)
public:
	int DoSpecialDamage (AActor *target, int damage);
};

IMPLEMENT_CLASS (AHornRodFX2)

int AHornRodFX2::DoSpecialDamage (AActor *target, int damage)
{
	if (target->IsKindOf (PClass::FindClass("Sorcerer2")) && pr_hrfx2() < 96)
	{ // D'Sparil teleports away
		P_DSparilTeleport (target);
		return -1;
	}
	return damage;
}

// Rain pillar 1 ------------------------------------------------------------

class ARainPillar : public AActor
{
	DECLARE_CLASS (ARainPillar, AActor)
public:
	int DoSpecialDamage (AActor *target, int damage);
};

IMPLEMENT_CLASS (ARainPillar)

int ARainPillar::DoSpecialDamage (AActor *target, int damage)
{
	if (target->flags2 & MF2_BOSS)
	{ // Decrease damage for bosses
		damage = (pr_rp() & 7) + 1;
	}
	return damage;
}

// Rain tracker "inventory" item --------------------------------------------

class ARainTracker : public AInventory
{
	DECLARE_CLASS (ARainTracker, AInventory)
public:
	void Serialize (FArchive &arc);
	AActor *Rain1, *Rain2;
};

IMPLEMENT_CLASS (ARainTracker)
	
void ARainTracker::Serialize (FArchive &arc)
{
	Super::Serialize (arc);
	arc << Rain1 << Rain2;
}

//----------------------------------------------------------------------------
//
// PROC A_FireSkullRodPL1
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_FireSkullRodPL1)
{
	AActor *mo;
	player_t *player;

	if (NULL == (player = self->player))
	{
		return;
	}

	AWeapon *weapon = player->ReadyWeapon;
	if (weapon != NULL)
	{
		if (!weapon->DepleteAmmo (weapon->bAltFire))
			return;
	}
	mo = P_SpawnPlayerMissile (self, PClass::FindClass("HornRodFX1"));
	// Randomize the first frame
	if (mo && pr_fsr1() > 128)
	{
		mo->SetState (mo->state->GetNextState());
	}
}

//----------------------------------------------------------------------------
//
// PROC A_FireSkullRodPL2
//
// The special2 field holds the player number that shot the rain missile.
// The special1 field holds the id of the rain sound.
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_FireSkullRodPL2)
{
	player_t *player;
	AActor *MissileActor;
	AActor *linetarget;

	if (NULL == (player = self->player))
	{
		return;
	}
	AWeapon *weapon = player->ReadyWeapon;
	if (weapon != NULL)
	{
		if (!weapon->DepleteAmmo (weapon->bAltFire))
			return;
	}
	P_SpawnPlayerMissile (self, 0,0,0, RUNTIME_CLASS(AHornRodFX2), self->angle, &linetarget, &MissileActor);
	// Use MissileActor instead of the return value from
	// P_SpawnPlayerMissile because we need to give info to the mobj
	// even if it exploded immediately.
	if (MissileActor != NULL)
	{
		MissileActor->special2 = (int)(player - players);
		if (linetarget)
		{
			MissileActor->tracer = linetarget;
		}
		S_Sound (MissileActor, CHAN_WEAPON, "weapons/hornrodpowshoot", 1, ATTN_NORM);
	}
}

//----------------------------------------------------------------------------
//
// PROC A_AddPlayerRain
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_AddPlayerRain)
{
	ARainTracker *tracker;

	if (self->target == NULL || self->target->health <= 0)
	{ // Shooter is dead or nonexistant
		return;
	}

	tracker = self->target->FindInventory<ARainTracker> ();

	// They player is only allowed two rainstorms at a time. Shooting more
	// than that will cause the oldest one to terminate.
	if (tracker != NULL)
	{
		if (tracker->Rain1 && tracker->Rain2)
		{ // Terminate an active rain
			if (tracker->Rain1->health < tracker->Rain2->health)
			{
				if (tracker->Rain1->health > 16)
				{
					tracker->Rain1->health = 16;
				}
				tracker->Rain1 = NULL;
			}
			else
			{
				if (tracker->Rain2->health > 16)
				{
					tracker->Rain2->health = 16;
				}
				tracker->Rain2 = NULL;
			}
		}
	}
	else
	{
		tracker = static_cast<ARainTracker *> (self->target->GiveInventoryType (RUNTIME_CLASS(ARainTracker)));
	}
	// Add rain mobj to list
	if (tracker->Rain1)
	{
		tracker->Rain2 = self;
	}
	else
	{
		tracker->Rain1 = self;
	}
	self->special1 = S_FindSound ("misc/rain");
}

//----------------------------------------------------------------------------
//
// PROC A_SkullRodStorm
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_SkullRodStorm)
{
	fixed_t x;
	fixed_t y;
	AActor *mo;
	ARainTracker *tracker;

	if (self->health-- == 0)
	{
		S_StopSound (self, CHAN_BODY);
		if (self->target == NULL)
		{ // Player left the game
			self->Destroy ();
			return;
		}
		tracker = self->target->FindInventory<ARainTracker> ();
		if (tracker != NULL)
		{
			if (tracker->Rain1 == self)
			{
				tracker->Rain1 = NULL;
			}
			else if (tracker->Rain2 == self)
			{
				tracker->Rain2 = NULL;
			}
		}
		self->Destroy ();
		return;
	}
	if (pr_storm() < 25)
	{ // Fudge rain frequency
		return;
	}
	x = self->x + ((pr_storm()&127) - 64) * FRACUNIT;
	y = self->y + ((pr_storm()&127) - 64) * FRACUNIT;
	mo = Spawn<ARainPillar> (x, y, ONCEILINGZ, ALLOW_REPLACE);
	mo->Translation = multiplayer ?
		TRANSLATION(TRANSLATION_PlayersExtra,self->special2) : 0;
	mo->target = self->target;
	mo->momx = 1; // Force collision detection
	mo->momz = -mo->Speed;
	mo->special2 = self->special2; // Transfer player number
	P_CheckMissileSpawn (mo);
	if (self->special1 != -1 && !S_IsActorPlayingSomething (self, CHAN_BODY, -1))
	{
		S_Sound (self, CHAN_BODY|CHAN_LOOP, self->special1, 1, ATTN_NORM);
	}
}

//----------------------------------------------------------------------------
//
// PROC A_RainImpact
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_RainImpact)
{
	if (self->z > self->floorz)
	{
		self->SetState (self->FindState("NotFloor"));
	}
	else if (pr_impact() < 40)
	{
		P_HitFloor (self);
	}
}

//----------------------------------------------------------------------------
//
// PROC A_HideInCeiling
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_HideInCeiling)
{
	self->z = self->ceilingz + 4*FRACUNIT;
}

// --- Phoenix Rod ----------------------------------------------------------

class APhoenixRod : public AWeapon
{
	DECLARE_CLASS (APhoenixRod, AWeapon)
public:
	void Serialize (FArchive &arc)
	{
		Super::Serialize (arc);
		arc << FlameCount;
	}
	int FlameCount;		// for flamethrower duration
};

class APhoenixRodPowered : public APhoenixRod
{
	DECLARE_CLASS (APhoenixRodPowered, APhoenixRod)
public:
	void EndPowerup ();
};

IMPLEMENT_CLASS (APhoenixRod)
IMPLEMENT_CLASS (APhoenixRodPowered)

void APhoenixRodPowered::EndPowerup ()
{
	P_SetPsprite (Owner->player, ps_weapon, SisterWeapon->GetReadyState());
	DepleteAmmo (bAltFire);
	Owner->player->refire = 0;
	S_StopSound (Owner, CHAN_WEAPON);
	Owner->player->ReadyWeapon = SisterWeapon;
}

class APhoenixFX1 : public AActor
{
	DECLARE_CLASS (APhoenixFX1, AActor)
public:
	int DoSpecialDamage (AActor *target, int damage);
};


IMPLEMENT_CLASS (APhoenixFX1)

int APhoenixFX1::DoSpecialDamage (AActor *target, int damage)
{
	if (target->IsKindOf (PClass::FindClass("Sorcerer2")) && pr_hrfx2() < 96)
	{ // D'Sparil teleports away
		P_DSparilTeleport (target);
		return -1;
	}
	return damage;
}

// Phoenix FX 2 -------------------------------------------------------------

class APhoenixFX2 : public AActor
{
	DECLARE_CLASS (APhoenixFX2, AActor)
public:
	int DoSpecialDamage (AActor *target, int damage);
};

IMPLEMENT_CLASS (APhoenixFX2)

int APhoenixFX2::DoSpecialDamage (AActor *target, int damage)
{
	if (target->player && pr_pfx2 () < 128)
	{ // Freeze player for a bit
		target->reactiontime += 4;
	}
	return damage;
}

//----------------------------------------------------------------------------
//
// PROC A_FirePhoenixPL1
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_FirePhoenixPL1)
{
	angle_t angle;
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
	P_SpawnPlayerMissile (self, RUNTIME_CLASS(APhoenixFX1));
	angle = self->angle + ANG180;
	angle >>= ANGLETOFINESHIFT;
	self->momx += FixedMul (4*FRACUNIT, finecosine[angle]);
	self->momy += FixedMul (4*FRACUNIT, finesine[angle]);
}

//----------------------------------------------------------------------------
//
// PROC A_PhoenixPuff
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_PhoenixPuff)
{
	AActor *puff;
	angle_t angle;

	//[RH] Heretic never sets the target for seeking
	//P_SeekerMissile (self, ANGLE_1*5, ANGLE_1*10);
	puff = Spawn("PhoenixPuff", self->x, self->y, self->z, ALLOW_REPLACE);
	angle = self->angle + ANG90;
	angle >>= ANGLETOFINESHIFT;
	puff->momx = FixedMul (FRACUNIT*13/10, finecosine[angle]);
	puff->momy = FixedMul (FRACUNIT*13/10, finesine[angle]);
	puff->momz = 0;
	puff = Spawn("PhoenixPuff", self->x, self->y, self->z, ALLOW_REPLACE);
	angle = self->angle - ANG90;
	angle >>= ANGLETOFINESHIFT;
	puff->momx = FixedMul (FRACUNIT*13/10, finecosine[angle]);
	puff->momy = FixedMul (FRACUNIT*13/10, finesine[angle]);
	puff->momz = 0;
}

//----------------------------------------------------------------------------
//
// PROC A_InitPhoenixPL2
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_InitPhoenixPL2)
{
	if (self->player != NULL)
	{
		APhoenixRod *flamethrower = static_cast<APhoenixRod *> (self->player->ReadyWeapon);
		if (flamethrower != NULL)
		{
			flamethrower->FlameCount = FLAME_THROWER_TICS;
		}
	}
}

//----------------------------------------------------------------------------
//
// PROC A_FirePhoenixPL2
//
// Flame thrower effect.
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_FirePhoenixPL2)
{
	AActor *mo;
	angle_t angle;
	fixed_t x, y, z;
	fixed_t slope;
	FSoundID soundid;
	player_t *player;
	APhoenixRod *flamethrower;

	if (NULL == (player = self->player))
	{
		return;
	}

	soundid = "weapons/phoenixpowshoot";

	flamethrower = static_cast<APhoenixRod *> (player->ReadyWeapon);
	if (flamethrower == NULL || --flamethrower->FlameCount == 0)
	{ // Out of flame
		P_SetPsprite (player, ps_weapon, flamethrower->FindState("Powerdown"));
		player->refire = 0;
		S_StopSound (self, CHAN_WEAPON);
		return;
	}
	angle = self->angle;
	x = self->x + (pr_fp2.Random2() << 9);
	y = self->y + (pr_fp2.Random2() << 9);
	z = self->z + 26*FRACUNIT + finetangent[FINEANGLES/4-(self->pitch>>ANGLETOFINESHIFT)];
	z -= self->floorclip;
	slope = finetangent[FINEANGLES/4-(self->pitch>>ANGLETOFINESHIFT)] + (FRACUNIT/10);
	mo = Spawn("PhoenixFX2", x, y, z, ALLOW_REPLACE);
	mo->target = self;
	mo->angle = angle;
	mo->momx = self->momx + FixedMul (mo->Speed, finecosine[angle>>ANGLETOFINESHIFT]);
	mo->momy = self->momy + FixedMul (mo->Speed, finesine[angle>>ANGLETOFINESHIFT]);
	mo->momz = FixedMul (mo->Speed, slope);
	if (!player->refire || !S_IsActorPlayingSomething (self, CHAN_WEAPON, -1))
	{
		S_Sound (self, CHAN_WEAPON|CHAN_LOOP, soundid, 1, ATTN_NORM);
	}	
	P_CheckMissileSpawn (mo);
}

//----------------------------------------------------------------------------
//
// PROC A_ShutdownPhoenixPL2
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_ShutdownPhoenixPL2)
{
	player_t *player;

	if (NULL == (player = self->player))
	{
		return;
	}
	S_StopSound (self, CHAN_WEAPON);
	AWeapon *weapon = player->ReadyWeapon;
	if (weapon != NULL)
	{
		if (!weapon->DepleteAmmo (weapon->bAltFire))
			return;
	}
}

//----------------------------------------------------------------------------
//
// PROC A_FlameEnd
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_FlameEnd)
{
	self->momz += FRACUNIT*3/2;
}

//----------------------------------------------------------------------------
//
// PROC A_FloatPuff
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_FloatPuff)
{
	self->momz += FRACUNIT*18/10;
}
