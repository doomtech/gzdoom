
#ifndef __FS_T_UTIL_H

#define __FS_T_UTIL_H

#include "zstring.h"
#include "doomdef.h"
#include "r_defs.h"
#include "r_state.h"
#include "t_parse.h"
#include "p_spec.h"
#include "farchive.h"

#define FIXED_TO_FLOAT(f) ((f)/(float)FRACUNIT)
#define CenterSpot(sec) (vertex_t*)&(sec)->soundorg[0]


extern const PClass *ActorTypes[];


const PClass *T_GetMobjType(svalue_t arg);
int T_GetPlayerNum(svalue_t arg);
int T_FindSectorFromTag(int tagnum,int startsector);
const PClass *T_GetAmmo(svalue_t t);
int T_FindSound(const char * name);
FString T_GetFormatString(int startarg);
bool T_CheckArgs(int cnt);
bool FS_ChangeMusic(const char * string);
void FS_GiveInventory (AActor *actor, const char * type, int amount);
void FS_TakeInventory (AActor *actor, const char * type, int amount);
int FS_CheckInventory (AActor *activator, const char *type);


class DLightLevel : public DLighting
{
	DECLARE_CLASS (DLightLevel, DLighting)

	unsigned char destlevel;
	unsigned char speed;

	DLightLevel() {}

public:

	DLightLevel(sector_t * s,int destlevel,int speed);
	void Serialize (FArchive &arc);
	void Tick ();
	void Destroy();
};


#endif