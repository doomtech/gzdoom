//
//-----------------------------------------------------------------------------
//
// Copyright (C) 2009 Christoph Oelckers
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// As an exception to the GPL this code may be used in GZDoom
// derivatives under the following conditions:
//
// 1. The license of these files is not changed
// 2. Full source of the derived program is disclosed
//
//
// ----------------------------------------------------------------------------
//
// Hardware texture class
//

#include "gl_translate.h"
#include "m_crc32.h"

namespace GLRenderer
{

TArray<FTranslationPalette::PalData> FTranslationPalette::AllPalettes;


//==========================================================================
//
// 
//
//==========================================================================

FTranslationPalette *FTranslationPalette::CreatePalette(FRemapTable *remap)
{
	FTranslationPalette *p = new FTranslationPalette(remap);
	p->Update();
	return p;
}

//==========================================================================
//
// 
//
//==========================================================================

bool FTranslationPalette::Update()
{
	PalData pd;

	memset(pd.pe, 0, sizeof(pd.pe));
	memcpy(pd.pe, remap->Palette, remap->NumEntries * sizeof(*remap->Palette));
	pd.crc32 = CalcCRC32((BYTE*)pd.pe, sizeof(pd.pe));
	for(unsigned int i=0;i< AllPalettes.Size(); i++)
	{
		if (pd.crc32 == AllPalettes[i].crc32)
		{
			if (!memcmp(pd.pe, AllPalettes[i].pe, sizeof(pd.pe))) 
			{
				Index = 1+i;
				return true;
			}
		}
	}
	Index = 1+AllPalettes.Push(pd);
	return true;
}

//==========================================================================
//
// 
//
//==========================================================================

int FTranslationPalette::GetInternalTranslation(int trans)
{
	if (trans <= 0) return -trans;
	//if (trans == TRANSLATION(TRANSLATION_Standard, 8)) return CM_GRAY;
	if (trans == TRANSLATION(TRANSLATION_Standard, 7)) return CM_ICE;

	FRemapTable *remap = TranslationToTable(trans);
	if (remap == NULL) return 0;

	FTranslationPalette *tpal = CreatePalette(remap);
	if (tpal == NULL) return 0;
	return tpal->GetIndex();
}

}