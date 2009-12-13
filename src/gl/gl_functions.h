#ifndef __GL_FUNCT
#define __GL_FUNCT

#include "v_palette.h"

class AActor;
struct MapData;
struct node_t;
struct subsector_t;

// These are the original nodes that are preserved because
// P_PointInSubsector should use the original data if possible.
extern node_t * 		gamenodes;
extern int 				numgamenodes;

extern subsector_t * 	gamesubsectors;
extern int 				numgamesubsectors;




// External entry points for the GL renderer
void gl_PreprocessLevel();
void gl_CleanLevelData();
void gl_LinkLights();
void gl_SetActorLights(AActor *);
void gl_DeleteAllAttachedLights();
void gl_RecreateAllAttachedLights();
void gl_ParseDefs();
void gl_SetFogParams(int _fogdensity, PalEntry _outsidefogcolor, int _outsidefogdensity, int _skyfog);
void gl_CheckNodes(MapData * map, bool rebuilt, int buildtime);
bool gl_LoadGLNodes(MapData * map);



extern int currentrenderer;

#endif
