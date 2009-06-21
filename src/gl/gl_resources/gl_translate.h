#ifndef __GL_TRANSLATE__
#define __GL_TRANSLATE__

#include "doomtype.h"
#include "r_translate.h"
#include "v_video.h"

namespace GLRenderer
{

class FTranslationPalette : public FNativePalette
{
	struct PalData
	{
		int crc32;
		PalEntry pe[256];
	};
	static TArray<PalData> AllPalettes;

	int Index;
	FRemapTable *remap;

	FTranslationPalette(FRemapTable *r) { remap=r; Index=-1; }

public:

	enum
	{
		CM_ICE = -1,
	};

	static FTranslationPalette *CreatePalette(FRemapTable *remap);
	static int GetInternalTranslation(int trans);
	static PalEntry *GetPalette(unsigned int index)
	{
		return index > 0 && index <= AllPalettes.Size()? AllPalettes[index-1].pe : NULL;
	}
	bool Update();
	int GetIndex() const { return Index; }
};

}

#endif
