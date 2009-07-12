#ifndef __GL_FUNCT
#define __GL_FUNCT

#include "doomtype.h"
#include "templates.h"
#include "m_fixed.h"
#include "tables.h"
#include "textures/textures.h"
#include "gl/old_renderer/gl1_renderer.h"

class FArchive;

class AActor;
class FTexture;
class FFont;
struct GLSectorPlane;
struct sector_t;
class player_s;
struct GL_IRECT;
class FCanvasTexture;
struct texcoord;
struct MapData;
struct FColormap;
struct particle_t;
struct subsector_t;
struct vertex_t;
class DCanvas;
union FRenderStyle;
struct side_t;


// Light + color

void gl_GetLightColor(int lightlevel, int rellight, const FColormap * cm, float * pred, float * pgreen, float * pblue, bool weapon=false);
void gl_SetColor(int light, int rellight, const FColormap * cm, float alpha, PalEntry ThingColor = 0xffffff, bool weapon=false);
void gl_SetColor(int light, int rellight, const FColormap * cm, float *red, float *green, float *blue, PalEntry ThingColor=0xffffff, bool weapon=false);

void gl_GetSpriteLight(AActor *Self, fixed_t x, fixed_t y, fixed_t z, subsector_t * subsec, int desaturation, float * out);
void gl_SetSpriteLight(AActor * thing, int lightlevel, int rellight, FColormap * cm, float alpha, PalEntry ThingColor = 0xffffff, bool weapon=false);

void gl_GetSpriteLight(AActor * thing, int lightlevel, int rellight, FColormap * cm,
					   float *red, float *green, float *blue,
					   PalEntry ThingColor, bool weapon);

void gl_SetSpriteLighting(FRenderStyle style, AActor *thing, int lightlevel, int rellight, FColormap *cm, 
						  PalEntry ThingColor, float alpha, bool fullbright, bool weapon);


void gl_SetSpriteLight(particle_t * thing, int lightlevel, int rellight, FColormap *cm, float alpha, PalEntry ThingColor = 0xffffff);

void gl_InitFog();
void gl_SetFogParams(int _fogdensity, PalEntry _outsidefogcolor, int _outsidefogdensity, int _skyfog);
float gl_GetFogDensity(int lightlevel, PalEntry fogcolor);
void gl_SetFog(int lightlevel, int rellight, const FColormap *cm, bool isadditive);

inline bool gl_isBlack(PalEntry color)
{
	return color.r + color.g + color.b == 0;
}

inline bool gl_isWhite(PalEntry color)
{
	return color.r + color.g + color.b == 3*0xff;
}

inline bool gl_isFullbright(PalEntry color, int lightlevel)
{
	return GLRendererOld::gl_fixedcolormap || (gl_isWhite(color) && lightlevel==255);
}



// Scene
void gl_LinkLights();
void gl_SetActorLights(AActor *);
void gl_DeleteAllAttachedLights();
void gl_RecreateAllAttachedLights();
void gl_ParseDefs();



inline float Dist2(float x1,float y1,float x2,float y2)
{
	return sqrtf((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2));
}


__forceinline void gl_InverseMap(int gray, BYTE & red, BYTE & green, BYTE & blue)
{
	red = green = blue = clamp<int>(255-gray,0,255);
}

__forceinline void gl_GoldMap(int gray, BYTE & red, BYTE & green, BYTE & blue)
{
	red=clamp<int>(gray+(gray>>1),0,255);
	green=clamp<int>(gray-(gray>>2),0,255);
	blue=0;
}

__forceinline void gl_RedMap(int gray, BYTE & red, BYTE & green, BYTE & blue)
{
	red=clamp<int>(gray+(gray>>1),0,255);
	green=0;
	blue=0;
}

__forceinline void gl_GreenMap(int gray, BYTE & red, BYTE & green, BYTE & blue)
{
	red=clamp<int>(gray+(gray>>1),0,255);
	green=clamp<int>(gray+(gray>>1),0,255);
	blue=gray;
}

__forceinline void gl_Desaturate(int gray, int ired, int igreen, int iblue, BYTE & red, BYTE & green, BYTE & blue, int fac)
{
	red = (ired*(31-fac) + gray*fac)/31;
	green = (igreen*(31-fac) + gray*fac)/31;
	blue = (iblue*(31-fac) + gray*fac)/31;
}

void gl_ModifyColor(BYTE & red, BYTE & green, BYTE & blue, int cm);


extern int currentrenderer;

#endif
