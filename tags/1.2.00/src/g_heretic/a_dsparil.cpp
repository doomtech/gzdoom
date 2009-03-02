/*
#include "actor.h"
#include "info.h"
#include "p_local.h"
#include "p_enemy.h"
#include "a_action.h"
#include "s_sound.h"
#include "m_random.h"
#include "a_sharedglobal.h"
#include "gstrings.h"
#include "a_specialspot.h"
#include "thingdef/thingdef.h"
#include "g_level.h"
*/

static FRandom pr_s2fx1 ("S2FX1");
static FRandom pr_scrc1atk ("Srcr1Attack");
static FRandom pr_dst ("D'SparilTele");
static FRandom pr_s2d ("Srcr2Decide");
static FRandom pr_s2a ("Srcr2Attack");
static FRandom pr_bluespark ("BlueSpark");

//----------------------------------------------------------------------------
//
// PROC A_Sor1Pain
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_Sor1Pain)
{
	self->special1 = 20; // Number of steps to walk fast
	CALL_ACTION(A_Pain, self);
}

//----------------------------------------------------------------------------
//
// PROC A_Sor1Chase
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_Sor1Chase)
{
	if (self->special1)
	{
		self->special1--;
		self->tics -= 3;
	}
	A_Chase(self);
}

//----------------------------------------------------------------------------
//
// PROC A_Srcr1Attack
//
// Sorcerer demon attack.
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_Srcr1Attack)
{
	AActor *mo;
	fixed_t momz;
	angle_t angle;

	if (!self->target)
	{
		return;
	}
	S_Sound (self, CHAN_BODY, self->AttackSound, 1, ATTN_NORM);
	if (self->CheckMeleeRange ())
	{
		int damage = pr_scrc1atk.HitDice (8);
		P_DamageMobj (self->target, self, self, damage, NAME_Melee);
		P_TraceBleed (damage, self->target, self);
		return;
	}

	const PClass *fx = PClass::FindClass("SorcererFX1");
	if (self->health > (self->GetDefault()->health/3)*2)
	{ // Spit one fireball
		P_SpawnMissileZ (self, self->z + 48*FRACUNIT, self->target, fx );
	}
	else
	{ // Spit three fireballs
		mo = P_SpawnMissileZ (self, self->z + 48*FRACUNIT, self->target, fx);
		if (mo != NULL)
		{
			momz = mo->momz;
			angle = mo->angle;
			P_SpawnMissileAngleZ (self, self->z + 48*FRACUNIT, fx, angle-ANGLE_1*3, momz);
			P_SpawnMissileAngleZ (self, self->z + 48*FRACUNIT, fx, angle+ANGLE_1*3, momz);
		}
		if (self->health < self->GetDefault()->health/3)
		{ // Maybe attack again
			if (self->special1)
			{ // Just attacked, so don't attack again
				self->special1 = 0;
			}
			else
			{ // Set state to attack again
				self->special1 = 1;
				self->SetState (self->FindState("Missile2"));
			}
		}
	}
}

//----------------------------------------------------------------------------
//
// PROC A_SorcererRise
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_SorcererRise)
{
	AActor *mo;

	self->flags &= ~MF_SOLID;
	mo = Spawn("Sorcerer2", self->x, self->y, self->z, ALLOW_REPLACE);
	mo->SetState (mo->FindState("Rise"));
	mo->angle = self->angle;
	mo->CopyFriendliness (self, true);
}

//----------------------------------------------------------------------------
//
// PROC P_DSparilTeleport
//
//----------------------------------------------------------------------------

void P_DSparilTeleport (AActor *actor)
{
	fixed_t prevX;
	fixed_t prevY;
	fixed_t prevZ;
	AActor *mo;
	AActor *spot;

	DSpotState *state = DSpotState::GetSpotState();
	if (state == NULL) return;

	spot = state->GetSpotWithMinDistance(PClass::FindClass("BossSpot"), actor->x, actor->y, 128*FRACUNIT);
	if (spot == NULL) return;

	prevX = actor->x;
	prevY = actor->y;
	prevZ = actor->z;
	if (P_TeleportMove (actor, spot->x, spot->y, spot->z, false))
	{
		mo = Spawn("Sorcerer2Telefade", prevX, prevY, prevZ, ALLOW_REPLACE);
		S_Sound (mo, CHAN_BODY, "misc/teleport", 1, ATTN_NORM);
		actor->SetState (actor->FindState("Teleport"));
		S_Sound (actor, CHAN_BODY, "misc/teleport", 1, ATTN_NORM);
		actor->z = actor->floorz;
		actor->angle = spot->angle;
		actor->momx = actor->momy = actor->momz = 0;
	}
}

//----------------------------------------------------------------------------
//
// PROC A_Srcr2Decide
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_Srcr2Decide)
{

	static const int chance[] =
	{
		192, 120, 120, 120, 64, 64, 32, 16, 0
	};

	unsigned int chanceindex = self->health / (self->GetDefault()->health/8);
	if (chanceindex >= countof(chance))
	{
		chanceindex = countof(chance) - 1;
	}

	if (pr_s2d() < chance[chanceindex])
	{
		P_DSparilTeleport (self);
	}
}

//----------------------------------------------------------------------------
//
// PROC A_Srcr2Attack
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_Srcr2Attack)
{
	int chance;

	if (!self->target)
	{
		return;
	}
	S_Sound (self, CHAN_BODY, self->AttackSound, 1, ATTN_NONE);
	if (self->CheckMeleeRange())
	{
		int damage = pr_s2a.HitDice (20);
		P_DamageMobj (self->target, self, self, damage, NAME_Melee);
		P_TraceBleed (damage, self->target, self);
		return;
	}
	chance = self->health < self->GetDefault()->health/2 ? 96 : 48;
	if (pr_s2a() < chance)
	{ // Wizard spawners

		const PClass *fx = PClass::FindClass("Sorcerer2FX2");
		if (fx)
		{
			P_SpawnMissileAngle (self, fx, self->angle-ANG45, FRACUNIT/2);
			P_SpawnMissileAngle (self, fx, self->angle+ANG45, FRACUNIT/2);
		}
	}
	else
	{ // Blue bolt
		P_SpawnMissile (self, self->target, PClass::FindClass("Sorcerer2FX1"));
	}
}

//----------------------------------------------------------------------------
//
// PROC A_BlueSpark
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_BlueSpark)
{
	int i;
	AActor *mo;

	for (i = 0; i < 2; i++)
	{
		mo = Spawn("Sorcerer2FXSpark", self->x, self->y, self->z, ALLOW_REPLACE);
		mo->momx = pr_bluespark.Random2() << 9;
		mo->momy = pr_bluespark.Random2() << 9;
		mo->momz = FRACUNIT + (pr_bluespark()<<8);
	}
}

//----------------------------------------------------------------------------
//
// PROC A_GenWizard
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_GenWizard)
{
	AActor *mo;

	mo = Spawn("Wizard", self->x, self->y, self->z, ALLOW_REPLACE);
	if (mo != NULL)
	{
		mo->z -= mo->GetDefault()->height/2;
		if (!P_TestMobjLocation (mo))
		{ // Didn't fit
			mo->Destroy ();
			level.total_monsters--;
		}
		else
		{ // [RH] Make the new wizards inherit D'Sparil's target
			mo->CopyFriendliness (self->target, true);

			self->momx = self->momy = self->momz = 0;
			self->SetState (self->FindState(NAME_Death));
			self->flags &= ~MF_MISSILE;
			mo->master = self->target;
			// Heretic did not offset it by TELEFOGHEIGHT, so I won't either.
			Spawn<ATeleportFog> (self->x, self->y, self->z, ALLOW_REPLACE);
		}
	}
}

//----------------------------------------------------------------------------
//
// PROC A_Sor2DthInit
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_Sor2DthInit)
{
	self->special1 = 7; // Animation loop counter
	P_Massacre (); // Kill monsters early
}

//----------------------------------------------------------------------------
//
// PROC A_Sor2DthLoop
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(AActor, A_Sor2DthLoop)
{
	if (--self->special1)
	{ // Need to loop
		self->SetState (self->FindState("DeathLoop"));
	}
}

