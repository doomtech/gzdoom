
#ifndef __HW_TEXTURE_H
#define __HW_TEXTURE_H

#include "tarray.h"

class FTexture;
class HWSystemTexture;

struct HWMakeNull
{
	void Init(HWSystemTexture *&value)
	{
		value = NULL;
	}
};

typedef TMap<int, HWSystemTexture*, THashTraits<int>, HWMakeNull> HWTranslatedTextures;

class HWTexture
{
	FTexture *mTexture;
	FTexture *mHiresTexture;
	char bIsTransparent;
	bool bHasColorkey;		// only for hires
	bool bExpand;
	int mHiresLump;

	HWSystemTexture *mSysTexture[2];
	HWTranslatedTextures mTranslatedTextures;

	unsigned char *LoadHiresTexture(FTexture *mTexture, int *width, int *height);

	HWSystemTexture *HWTexture::CreateTexture(int translation, bool expand, FTexture *hirescheck);

	bool Bind(int texunit, int clamp, int translation, FTexture *hirescheck);
	bool BindPatch(int texunit, int translation);

public:
	HWTexture(FTexture * tx, bool expandpatches);
	~HWTexture();

	unsigned char * CreateTexBuffer(int translation, int & w, int & h, bool expand, FTexture *hirescheck);

	void Clean(bool all);
	int Dump(int i);

};

#endif
