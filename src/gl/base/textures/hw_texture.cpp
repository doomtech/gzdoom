/*
** hw_texture.cpp
** 
**---------------------------------------------------------------------------
** Copyright 2004-2010 Christoph Oelckers
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
** 4. When not used as part of GZDoom or a GZDoom derivative, this code will be
**    covered by the terms of the GNU Lesser General Public License as published
**    by the Free Software Foundation; either version 2.1 of the License, or (at
**    your option) any later version.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/


// Yay! No more OpenGL specific references here! :)

#include "w_wad.h"
#include "m_png.h"
#include "r_draw.h"
#include "sbar.h"
#include "gi.h"
#include "cmdlib.h"
#include "c_dispatch.h"
#include "stats.h"
#include "r_main.h"
#include "templates.h"
#include "sc_man.h"
#include "r_translate.h"
#include "colormatcher.h"

#include "gl/renderer/gl_renderer.h"
#include "gl/textures/gl_texture.h"
#include "gl/textures/gl_translate.h"
#include "gl/textures/gl_bitmap.h"
#include "gl/base/textures/hw_systexture.h"
#include "gl/base/textures/hw_texture.h"

EXTERN_CVAR(Bool, gl_texture_usehires)

//===========================================================================
//
// The hardware texture maintenance class
//
//===========================================================================

//===========================================================================
//
// Constructor
//
//===========================================================================
HWTexture::HWTexture(FTexture * tx, bool expandpatches)
{
	assert(tx->gl_info.SystemTexture3 == NULL);
	mTexture = tx;
	mHiresTexture = NULL;
	mSysTexture[0] = NULL;
	mSysTexture[1] = NULL;

	bIsTransparent = -1;
	bHasColorkey = false;
	bExpand = expandpatches;
	mHiresLump=-1;

	mTexture->gl_info.SystemTexture3 = this;
}

//===========================================================================
//
// Destructor
//
//===========================================================================

HWTexture::~HWTexture()
{
	Clean(true);
	if (mHiresTexture != NULL) delete mHiresTexture;
}

//===========================================================================
// 
//	Deletes all allocated resources
//
//===========================================================================

void HWTexture::Clean(bool all)
{
	HWTranslatedTextures::Iterator it(mTranslatedTextures);
	HWTranslatedTextures::Pair *pair;
	
	while (it.NextPair(pair))
	{
		delete pair->Value;
		pair->Value = NULL;
	}
	if (all)
	{
		if (mSysTexture[0] != NULL)
		{
			delete mSysTexture[0];
			mSysTexture[0] = NULL;
		}
		if (mSysTexture[1] != NULL)
		{
			delete mSysTexture[1];
			mSysTexture[1] = NULL;
		}
	}
}

//==========================================================================
//
// Checks for the presence of a hires texture replacement and loads it
//
//==========================================================================

unsigned char *HWTexture::LoadHiresTexture(FTexture *mTexture, int *width, int *height)
{
	if (mHiresLump==-1) 
	{
		bHasColorkey = false;
		mHiresLump = CheckDDPK3(mTexture);
		if (mHiresLump < 0) mHiresLump = CheckExternalFile(mTexture, bHasColorkey);

		if (mHiresLump >=0) 
		{
			mHiresTexture = FTexture::CreateTexture(mHiresLump, FTexture::TEX_Any);
		}
	}
	if (mHiresTexture != NULL)
	{
		int w=mHiresTexture->GetWidth();
		int h=mHiresTexture->GetHeight();

		unsigned char * buffer=new unsigned char[w*(h+1)*4];
		memset(buffer, 0, w * (h+1) * 4);
		FGLBitmap bmp(buffer, w*4, w, h);
		
		int trans = mHiresTexture->CopyTrueColorPixels(&bmp, 0, 0);
		mHiresTexture->CheckTrans(buffer, w*h, trans);
		bIsTransparent = mHiresTexture->gl_info.mIsTransparent;

		if (bHasColorkey)
		{
			// This is a crappy Doomsday color keyed image
			// We have to remove the key manually. :(
			DWORD * dwdata=(DWORD*)buffer;
			for (int i=(w*h);i>0;i--)
			{
				if (dwdata[i]==0xffffff00 || dwdata[i]==0xffff00ff) dwdata[i]=0;
			}
		}
		*width = w;
		*height = h;
		return buffer;
	}
	return NULL;
}

//===========================================================================
// 
//	Initializes the buffer for the texture data
//
//===========================================================================

unsigned char * HWTexture::CreateTexBuffer(int translation, int &w, int &h, bool expand, FTexture *hirescheck)
{
	unsigned char * buffer;
	int W, H;


	// Textures that are already scaled in the texture lump will not get replaced
	// by hires textures
	if (gl_texture_usehires && hirescheck != NULL)
	{
		buffer = LoadHiresTexture (hirescheck, &w, &h);
		if (buffer)
		{
			return buffer;
		}
	}

	W = w = mTexture->GetWidth() + expand*2;
	H = h = mTexture->GetHeight() + expand*2;


	buffer=new unsigned char[W*(H+1)*4];
	memset(buffer, 0, W * (H+1) * 4);

	FGLBitmap bmp(buffer, W*4, W, H);
	bmp.SetTranslationInfo(0, translation);

	if (mTexture->bComplex)
	{
		FBitmap imgCreate;

		// The texture contains special processing so it must be composited using the
		// base bitmap class and then be converted as a whole.
		if (imgCreate.Create(W, H))
		{
			memset(imgCreate.GetPixels(), 0, W * H * 4);
			int trans = mTexture->CopyTrueColorPixels(&imgCreate, expand, expand);
			bmp.CopyPixelDataRGB(0, 0, imgCreate.GetPixels(), W, H, 4, W * 4, 0, CF_BGRA);
			mTexture->CheckTrans(buffer, W*H, trans);
			bIsTransparent = mTexture->gl_info.mIsTransparent;
		}
	}
	else if (translation <= 0)
	{
		int trans = mTexture->CopyTrueColorPixels(&bmp, expand, expand);
		mTexture->CheckTrans(buffer, W*H, trans);
		bIsTransparent = mTexture->gl_info.mIsTransparent;
	}
	else
	{
		// When using translations everything must be mapped to the base palette.
		// Since FTexture's method is doing exactly that by calling GetPixels let's use that here
		// to do all the dirty work for us. ;)
		mTexture->FTexture::CopyTrueColorPixels(&bmp, expand, expand);
		bIsTransparent = 0;
	}

	// [BB] Potentially upsample the buffer.
	return gl_CreateUpsampledTextureBuffer ( mTexture, buffer, W, H, w, h, bIsTransparent || translation == CM_SHADE);
}


//===========================================================================
// 
//	Creates a hardware texture for this system texture
//
//===========================================================================

HWSystemTexture *HWTexture::CreateTexture(int translation, bool expand, FTexture *hirescheck)
{
	int w, h;

	// Create this texture
	int texformat = translation == CM_SHADE? HWSystemTexture::HWT_ALPHA8 : HWSystemTexture::HWT_RGBA8;
	unsigned char * buffer = CreateTexBuffer(translation, w, h, expand, hirescheck);
	mTexture->ProcessData(buffer, w, h, true);

	HWSystemTexture *ttex = HWRenderer->CreateSystemTexture(true);
	ttex->CreateTexture(texformat, buffer, HWSystemTexture::HWT_RGBA8, w, h);
	delete[] buffer;
	return ttex;
}

//===========================================================================
// 
//	Binds a texture to the renderer
//
//===========================================================================

bool HWTexture::Bind(int texunit, int clampmode, int translation, FTexture *hirescheck)
{
	if (translation <= 0) translation = -translation;
	else if (translation == TRANSLATION(TRANSLATION_Standard, 8)) translation = CM_GRAY;
	else if (translation == TRANSLATION(TRANSLATION_Standard, 7)) translation = CM_ICE;
	else translation = GLTranslationPalette::GetInternalTranslation(translation);
	
	// Texture has become invalid
	if (!mTexture->bHasCanvas && mTexture->CheckModified())
	{
		Clean(true);
	}
	HWSystemTexture *&tex = translation == 0? mSysTexture[0] : mTranslatedTextures[2*translation];

	if (tex == NULL)
	{
		tex = CreateTexture(translation, false, hirescheck);
	}
	bool res = HWRenderer->SetSampler(texunit, tex->IsMipmapped(), clampmode);
	res |= HWRenderer->SetTexture(texunit, tex);
	return res;
}

//===========================================================================
// 
//	Binds a sprite to the renderer
//
//===========================================================================
bool HWTexture::BindPatch(int texunit, int translation)
{
	if (translation <= 0) translation = -translation;
	else if (translation == TRANSLATION(TRANSLATION_Standard, 8)) translation = CM_GRAY;
	else if (translation == TRANSLATION(TRANSLATION_Standard, 7)) translation = CM_ICE;
	else translation = GLTranslationPalette::GetInternalTranslation(translation);

	// Texture has become invalid so delete all instances
	if (!mTexture->bHasCanvas && mTexture->CheckModified())
	{
		Clean(true);
	}

	HWSystemTexture *&tex = translation == 0? mSysTexture[1] : mTranslatedTextures[2*translation+1];

	if (tex == NULL)
	{
		tex = CreateTexture(translation, bExpand, NULL);
	}
	bool res = HWRenderer->SetSampler(texunit, false, 3);
	res |= HWRenderer->SetTexture(texunit, tex);
	return res;
}
