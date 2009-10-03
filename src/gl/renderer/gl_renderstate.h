#ifndef __GL_RENDERSTATE_H
#define __GL_RENDERSTATE_H

#include <string.h>

class FRenderState
{
	bool mFogEnabled;
	bool mGlowEnabled;
	bool mLightEnabled;

	float glowtopparms[4], glowbottomparms[4];

public:
	FRenderState()
	{
		mFogEnabled = mGlowEnabled = mLightEnabled = false;
	}

	void Begin(int primtype, bool forcenoshader = false);
	void End();

	void EnableFog(bool on)
	{
		mFogEnabled = on;
	}

	void EnableGlow(bool on)
	{
		mGlowEnabled = on;
	}

	void EnableLight(bool on)
	{
		mLightEnabled = on;
	}

	bool isFogEnabled()
	{
		return mFogEnabled;
	}

	bool isGlowEnabled()
	{
		return mGlowEnabled;
	}

	bool isLightEnabled()
	{
		return mLightEnabled;
	}

	void SetGlowParams(float *top, float *bottom)
	{
		memcpy(glowtopparms, top, sizeof(glowtopparms));
		memcpy(glowbottomparms, bottom, sizeof(glowbottomparms));
	}


};

extern FRenderState gl_RenderState;

#endif