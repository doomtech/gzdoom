#ifndef GLSL_STATE_H
#define GLSL_STATE_H

#include "doomtype.h"

struct FColormap;

class GLSLRenderState
{


public:

	void SetFixedColormap(int cm);
	void SetLight(int light, int rellight, FColormap *cm, float alpha, bool additive);
	void SetLight(int light, int rellight, FColormap *cm, float alpha, bool additive, PalEntry ThingColor, bool weapon);
	void SetWarp(int mode, float speed);
	void EnableBrightmap(bool on);
	void SetBrightmap(bool on);
	void SetFloorGlow(PalEntry color, float height);
	void SetCeilingGlow(PalEntry color, float height);
	void ClearDynLights();
	void SetDynLight(int index, float x, float y, float z, float size, PalEntry color, int mode);
	void SetLightingMode(int mode);
	void Apply();
	void Enable(bool on);
	void Reset();
};

extern GLSLRenderState *glsl;


#endif