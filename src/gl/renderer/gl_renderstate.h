#ifndef __GL_RENDERSTATE_H
#define __GL_RENDERSTATE_H

#include <string.h>

struct FStateAttr
{
	static int ChangeCounter;
	int mLastChange;

	FStateAttr()
	{
		mLastChange = -1;
	}
};

struct FStateVec3 : public FStateAttr
{
	float vec[3];

	bool Update(FStateVec3 *other)
	{
		if (mLastChange != other->mLastChange)
		{
			*this = *other;
			return true;
		}
		return false;
	}

	void Set(float x, float y, float z)
	{
		vec[0] = x;
		vec[1] = z;
		vec[2] = y;
		mLastChange = ++ChangeCounter;
	}
};


class FRenderState
{
	bool mTextureEnabled;
	bool mFogEnabled;
	bool mGlowEnabled;
	bool mLightEnabled;
	bool mBrightmapEnabled;
	int mTextureMode;
	float mLightParms[2];

	FStateVec3 mCameraPos;


	bool ffTextureEnabled;
	bool ffFogEnabled;
	int ffTextureMode;

public:
	FRenderState()
	{
		mTextureEnabled = mBrightmapEnabled = mFogEnabled = mGlowEnabled = mLightEnabled = false;
		ffTextureEnabled = ffFogEnabled = false;
		mTextureMode = ffTextureMode = -1;
	}

	void Apply(bool forcenoshader = false);

	void SetTextureMode(int mode)
	{
		mTextureMode = mode;
	}

	void EnableTexture(bool on)
	{
		mTextureEnabled = on;
	}

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

	void EnableBrightmap(bool on)
	{
		mBrightmapEnabled = on;
	}

	void SetCameraPos(float x, float y, float z)
	{
		mCameraPos.Set(x,y,z);
	}

	void SetLightParms(float f, float d)
	{
		mLightParms[0] = f;
		mLightParms[1] = d;
	}

	// get state

	int getTextureMode()
	{
		return mTextureMode;
	}

	FStateVec3 *getCameraPos()
	{
		return &mCameraPos;
	}

	float *getLightParms()
	{
		return mLightParms;
	}

	bool isTextureEnabled()
	{
		return mTextureEnabled;
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

	bool isBrightmapEnabled()
	{
		return mBrightmapEnabled;
	}

};

extern FRenderState gl_RenderState;

#endif
