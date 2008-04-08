#ifndef __A_SHAREDGLOBAL_H__
#define __A_SHAREDGLOBAL_H__

#include "dobject.h"
#include "info.h"
#include "actor.h"

class FDecalTemplate;
struct vertex_s;
struct side_t;
struct F3DFloor;

extern void P_SpawnDirt (AActor *actor, fixed_t radius);

class DBaseDecal : public DThinker
{
	DECLARE_CLASS (DBaseDecal, DThinker)
	HAS_OBJECT_POINTERS
public:
	DBaseDecal ();
	DBaseDecal (fixed_t z);
	DBaseDecal (int statnum, fixed_t z);
	DBaseDecal (const AActor *actor);
	DBaseDecal (const DBaseDecal *basis);

	void Serialize (FArchive &arc);
	void Destroy ();
	int StickToWall (side_t *wall, fixed_t x, fixed_t y, F3DFloor * ffloor = NULL);
	fixed_t GetRealZ (const side_t *wall) const;
	void SetShade (DWORD rgb);
	void SetShade (int r, int g, int b);
	void Spread (const FDecalTemplate *tpl, side_t *wall, fixed_t x, fixed_t y, fixed_t z, F3DFloor * ffloor = NULL);
	void GetXY (side_t *side, fixed_t &x, fixed_t &y) const;

	static void SerializeChain (FArchive &arc, DBaseDecal **firstptr);

	DBaseDecal *WallNext, **WallPrev;

	fixed_t LeftDistance;
	fixed_t Z;
	fixed_t ScaleX, ScaleY;
	fixed_t Alpha;
	DWORD AlphaColor;
	WORD Translation;
	WORD PicNum;
	DWORD RenderFlags;
	FRenderStyle RenderStyle;
	sector_t * Sector;	// required for 3D floors

protected:
	virtual DBaseDecal *CloneSelf (const FDecalTemplate *tpl, fixed_t x, fixed_t y, fixed_t z, side_t *wall, F3DFloor * ffloor = NULL) const;
	void CalcFracPos (side_t *wall, fixed_t x, fixed_t y);
	void Remove ();

	static void SpreadLeft (fixed_t r, vertex_s *v1, side_t *feelwall, F3DFloor * ffloor = NULL);
	static void SpreadRight (fixed_t r, side_t *feelwall, fixed_t wallsize, F3DFloor * ffloor = NULL);
};

class DImpactDecal : public DBaseDecal
{
	DECLARE_CLASS (DImpactDecal, DBaseDecal)
public:
	DImpactDecal (fixed_t z);
	DImpactDecal (side_t *wall, const FDecalTemplate *templ);

	static DImpactDecal *StaticCreate (const char *name, fixed_t x, fixed_t y, fixed_t z, side_t *wall, F3DFloor * ffloor = NULL, PalEntry color=0);
	static DImpactDecal *StaticCreate (const FDecalTemplate *tpl, fixed_t x, fixed_t y, fixed_t z, side_t *wall, F3DFloor * ffloor = NULL, PalEntry color=0);

	void BeginPlay ();
	void Destroy ();

	void Serialize (FArchive &arc);
	static void SerializeTime (FArchive &arc);

protected:
	DBaseDecal *CloneSelf (const FDecalTemplate *tpl, fixed_t x, fixed_t y, fixed_t z, side_t *wall, F3DFloor * ffloor = NULL) const;
	static void CheckMax ();

private:
	DImpactDecal();
};

class AAmbientSound : public AActor
{
	DECLARE_STATELESS_ACTOR (AAmbientSound, AActor)
public:
	void Serialize (FArchive &arc);

	void BeginPlay ();
	void Tick ();
	void Activate (AActor *activator);
	void Deactivate (AActor *activator);

protected:
	bool bActive;
private:
	void SetTicker (struct AmbientSound *ambient);
	int NextCheck;
};

class ATeleportFog : public AActor
{
	DECLARE_ACTOR (ATeleportFog, AActor)
public:
	void PostBeginPlay ();
};

class ATeleportDest : public AActor
{
	DECLARE_STATELESS_ACTOR (ATeleportDest, AActor)
};

class ASkyViewpoint : public AActor
{
	DECLARE_STATELESS_ACTOR (ASkyViewpoint, AActor)
public:
	void Serialize (FArchive &arc);
	void BeginPlay ();
	void Destroy ();
	bool bInSkybox;
	bool bAlways;
	ASkyViewpoint *Mate;
	fixed_t PlaneAlpha;
};

class DFlashFader : public DThinker
{
	DECLARE_CLASS (DFlashFader, DThinker)
	HAS_OBJECT_POINTERS
public:
	DFlashFader (float r1, float g1, float b1, float a1,
				 float r2, float g2, float b2, float a2,
				 float time, AActor *who);
	~DFlashFader ();
	void Serialize (FArchive &arc);
	void Tick ();
	AActor *WhoFor() { return ForWho; }
	void Cancel ();

protected:
	float Blends[2][4];
	int TotalTics;
	int StartTic;
	TObjPtr<AActor> ForWho;

	void SetBlend (float time);
	DFlashFader ();
};

class DEarthquake : public DThinker
{
	DECLARE_CLASS (DEarthquake, DThinker)
	HAS_OBJECT_POINTERS
public:
	DEarthquake (AActor *center, int intensity, int duration, int damrad, int tremrad);

	void Serialize (FArchive &arc);
	void Tick ();

	TObjPtr<AActor> m_Spot;
	fixed_t m_TremorRadius, m_DamageRadius;
	int m_Intensity;
	int m_Countdown;
	int m_QuakeSFX;

	static int StaticGetQuakeIntensity (AActor *viewer);

private:
	DEarthquake ();
};

class AMorphProjectile : public AActor
{
	DECLARE_ACTOR (AMorphProjectile, AActor)
public:
	int DoSpecialDamage (AActor *target, int damage);
	void Serialize (FArchive &arc);

	FNameNoInit	PlayerClass, MonsterClass, MorphFlash, UnMorphFlash;
	int Duration, MorphStyle;
};

class AMorphedMonster : public AActor
{
	DECLARE_ACTOR (AMorphedMonster, AActor)
	HAS_OBJECT_POINTERS
public:
	void Tick ();
	void Serialize (FArchive &arc);
	void Die (AActor *source, AActor *inflictor);
	void Destroy ();

	TObjPtr<AActor> UnmorphedMe;
	int UnmorphTime, MorphStyle;
	const PClass *MorphExitFlash;
	DWORD FlagsSave;
};

class AMapMarker : public AActor
{
	DECLARE_ACTOR(AMapMarker, AActor)
public:
	void BeginPlay ();
	void Activate (AActor *activator);
	void Deactivate (AActor *activator);
};

#endif //__A_SHAREDGLOBAL_H__