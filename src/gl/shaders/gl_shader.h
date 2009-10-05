
#ifndef __GL_SHADERS_H__
#define __GL_SHADERS_H__

#include "gl/renderer/gl_renderstate.h"
#include "name.h"

extern bool gl_shaderactive;

const int VATTR_GLOWDISTANCE = 15;

//==========================================================================
//
// set brightness map and glowstatus
// Change will only take effect when the texture is rebound!
//
//==========================================================================

bool gl_BrightmapsActive();
bool gl_GlowActive();
bool gl_ExtFogActive();

void gl_DisableShader();
void gl_ClearShaders();
void gl_InitShaders();

void gl_EnableShader(bool on);


void gl_SetGlowParams(float *topcolors, float topheight, float *bottomcolors, float bottomheight);
void gl_SetLightRange(int first, int last, int forceadd);


class FShader
{
	friend class GLShader;
	friend class FRenderState;

	unsigned int hShader;
	unsigned int hVertProg;
	unsigned int hFragProg;

	int timer_index;
	int desaturation_index;
	int fogenabled_index;
	int texturemode_index;
	int camerapos_index;
	int lightparms_index;
	int colormapstart_index;
	int colormaprange_index;
	int lightrange_index;

	int glowbottomcolor_index;
	int glowtopcolor_index;

	int currentfogenabled;
	int currenttexturemode;
	float currentlightfactor;
	float currentlightdist;

	FStateVec3 currentcamerapos;

public:
	FShader()
	{
		hShader = hVertProg = hFragProg = NULL;
		currentfogenabled = currenttexturemode = 0;
		currentlightfactor = currentlightdist = 0.0f;
	}

	~FShader();

	bool Load(const char * name, const char * vert_prog_lump, const char * fragprog, const char * fragprog2, const char *defines);

	void SetColormapColor(float r, float g, float b, float r1, float g1, float b1);
	void SetGlowParams(float *topcolors, float topheight, float *bottomcolors, float bottomheight);
	void SetLightRange(int start, int end, int forceadd);

	bool Bind(float Speed);

};

struct FShaderContainer;

class GLShader
{
	FName Name;
	FShaderContainer *container;

public:

	static void Initialize();
	static void Clear();
	static GLShader *Find(const char * shn);
	static GLShader *Find(unsigned int warp);
	FShader *Bind(int cm, bool glowing, float Speed, bool lights);
	static void Unbind();

};



#endif

