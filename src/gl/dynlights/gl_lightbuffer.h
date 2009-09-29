#ifndef __GL_LIGHTBUFFER_H
#define __GL_LIGHTBUFFER_H

class ADynamicLight;

const int MAX_DYNLIGHTS = 20000;	// should hopefully be enough

struct FLightRGB
{
	unsigned char R,G,B,Type;	// Type is 0 for normal, 1 for additive and 2 for subtractive
};

struct FLightPosition
{
	float X,Z,Y,Distance;
};

class FLightBuffer
{
	unsigned int mIDbuf_RGB;
	unsigned int mIDbuf_Position;

	unsigned int mIDtex_RGB;
	unsigned int mIDtex_Position;

	FLightRGB * mp_RGB;
	FLightPosition *mp_Position;

	int mIndex;

public:
	FLightBuffer();
	~FLightBuffer();
	void MapBuffer();
	void UnmapBuffer();
	void BindTextures(int uniloc1, int uniloc2);
	void AddLight(ADynamicLight *light, bool foggy);

	int GetLightIndex() const
	{
		return mIndex;
	}
};


#endif