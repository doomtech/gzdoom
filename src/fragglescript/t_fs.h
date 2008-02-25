
#ifndef T_FS_H
#define T_FS_H

// global FS interface

struct MapData;
class AActor;

extern bool HasScripts;

void T_PreprocessScripts();
void T_LoadLevelInfo(MapData * map);
void T_PrepareSpawnThing();
void T_RegisterSpawnThing(AActor * );

#endif
