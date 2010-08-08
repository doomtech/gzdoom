/*
** patchtexture.cpp
** Texture class for single Doom patches
**
**---------------------------------------------------------------------------
** Copyright 2004-2006 Randy Heit
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
**
*/

#include "doomtype.h"
#include "files.h"
#include "r_data.h"
#include "w_wad.h"
#include "templates.h"
#include "v_palette.h"


//==========================================================================
//
// A texture that is just a single Doom format patch
//
//==========================================================================
enum
{
	PATCH_NORMAL = 0,
	PATCH_BETA = 1,
	PATCH_ALPHA = 2,
};

class FPatchTexture : public FTexture
{
public:
	FPatchTexture (int lumpnum, patch_t *header, int format = PATCH_NORMAL);
	~FPatchTexture ();

	const BYTE *GetColumn (unsigned int column, const Span **spans_out);
	const BYTE *GetPixels ();
	void Unload ();

protected:
	BYTE *Pixels;
	Span **Spans;
	bool hackflag;
	int Format;

	virtual void MakeTexture ();
	void HackHack (int newheight);
};

//==========================================================================
//
// Checks if the currently open lump can be a Doom patch
//
//==========================================================================

static bool CheckIfPatch(FileReader & file)
{
	if (file.GetLength() < 13) return false;	// minimum length of a valid Doom patch
	
	BYTE *data = new BYTE[file.GetLength()];
	file.Seek(0, SEEK_SET);
	file.Read(data, file.GetLength());
	
	const patch_t *foo = (const patch_t *)data;
	
	int height = LittleShort(foo->height);
	int width = LittleShort(foo->width);
	
	if (height > 0 && height < 2048 && width > 0 && width <= 2048 && width < file.GetLength()/4)
	{
		// The dimensions seem like they might be valid for a patch, so
		// check the column directory for extra security. At least one
		// column must begin exactly at the end of the column directory,
		// and none of them must point past the end of the patch.
		bool gapAtStart = true;
		int x;
	
		for (x = 0; x < width; ++x)
		{
			DWORD ofs = LittleLong(foo->columnofs[x]);
			if (ofs == (DWORD)width * 4 + 8)
			{
				gapAtStart = false;
			}
			else if (ofs >= (DWORD)(file.GetLength()))	// Need one byte for an empty column (but there's patches that don't know that!)
			{
				delete [] data;
				return false;
			}
		}
		delete [] data;
		return !gapAtStart;
	}
	delete [] data;
	return false;
}

static bool CheckIfAlphaPatch(FileReader & file)
{
	if (file.GetLength() < 13) return false;	// minimum length of a valid Doom patch
	
	BYTE *data = new BYTE[file.GetLength()];
	file.Seek(0, SEEK_SET);
	file.Read(data, file.GetLength());
	
	const alphapatch_t *foo = (const alphapatch_t *)data;
	
	int height = foo->height;
	int width = foo->width;
	
	if (width < file.GetLength()/4)
	{
		return true;
		bool gapAtStart = true;
		int x;
	
		for (x = 0; x < width; ++x)
		{
			WORD ofs = LittleShort(foo->columnofs[x]);
			if (ofs == (WORD)width * 2 + 4)
			{
				gapAtStart = false;
			}
			else if (ofs >= (WORD)(file.GetLength()))
			{
				delete [] data;
				return false;
			}
		}
		delete [] data;
		return !gapAtStart;

	}
	delete [] data;
	return false;
}

static bool CheckIfBetaPatch(FileReader & file)
{
	if (file.GetLength() < 13) return false;	// minimum length of a valid Doom patch
	
	BYTE *data = new BYTE[file.GetLength()];
	file.Seek(0, SEEK_SET);
	file.Read(data, file.GetLength());
	
	const betapatch_t *foo = (const betapatch_t *)data;
	
	int height = LittleShort(foo->height);
	int width = LittleShort(foo->width);
	
	if (width < file.GetLength()/4)
	{
		bool gapAtStart = true;
		int x;
	
		for (x = 0; x < width; ++x)
		{
			WORD ofs = LittleShort(foo->columnofs[x]);
			if (ofs == (WORD)width * 2 + 8)
			{
				gapAtStart = false;
			}
			else if (ofs >= (WORD)(file.GetLength()))
			{
				delete [] data;
				return false;
			}
		}
		delete [] data;
		return !gapAtStart;
	}
	delete [] data;
	return false;
}

//==========================================================================
//
//
//
//==========================================================================

FTexture *PatchTexture_TryCreate(FileReader & file, int lumpnum)
{
	int format = PATCH_NORMAL;
	if (!CheckIfPatch(file))
	{
		if (!CheckIfBetaPatch(file))
		{
			if (!CheckIfAlphaPatch(file))
			{
				return NULL;
			}
			else format = PATCH_ALPHA;
		}
		else format = PATCH_BETA;
	}
	file.Seek(0, SEEK_SET);
	patch_t header;
	if (format == PATCH_NORMAL)
		file >> header.width >> header.height >> header.leftoffset >> header.topoffset;
	else if (format == PATCH_BETA)
	{
		betapatch_t tempheader;
		file >> tempheader.width >> tempheader.height >> tempheader.leftoffset >> tempheader.topoffset;
		header.width = tempheader.width; header.height = tempheader.height;
		header.leftoffset = tempheader.leftoffset; header.topoffset = tempheader.topoffset;
	}
	else if (format == PATCH_ALPHA)
	{
		alphapatch_t tempheader;
		file >> tempheader.width >> tempheader.height >> tempheader.leftoffset >> tempheader.topoffset;
		header.width = tempheader.width; header.height = tempheader.height;
		header.leftoffset = tempheader.leftoffset; header.topoffset = tempheader.topoffset;
	}
	else return NULL;
	return new FPatchTexture(lumpnum, &header, format);
}

//==========================================================================
//
//
//
//==========================================================================

FPatchTexture::FPatchTexture (int lumpnum, patch_t * header, int format)
: FTexture(NULL, lumpnum), Pixels(0), Spans(0), hackflag(false)
{
	Format = format;
	Width = header->width;
	Height = header->height;
	LeftOffset = header->leftoffset;
	TopOffset = header->topoffset;
	CalcBitSize ();
}

//==========================================================================
//
//
//
//==========================================================================

FPatchTexture::~FPatchTexture ()
{
	Unload ();
	if (Spans != NULL)
	{
		FreeSpans (Spans);
		Spans = NULL;
	}
}

//==========================================================================
//
//
//
//==========================================================================

void FPatchTexture::Unload ()
{
	if (Pixels != NULL)
	{
		delete[] Pixels;
		Pixels = NULL;
	}
}

//==========================================================================
//
//
//
//==========================================================================

const BYTE *FPatchTexture::GetPixels ()
{
	if (Pixels == NULL)
	{
		MakeTexture ();
	}
	return Pixels;
}

//==========================================================================
//
//
//
//==========================================================================

const BYTE *FPatchTexture::GetColumn (unsigned int column, const Span **spans_out)
{
	if (Pixels == NULL)
	{
		MakeTexture ();
	}
	if ((unsigned)column >= (unsigned)Width)
	{
		if (WidthMask + 1 == Width)
		{
			column &= WidthMask;
		}
		else
		{
			column %= Width;
		}
	}
	if (spans_out != NULL)
	{
		if (Spans == NULL)
		{
			Spans = CreateSpans(Pixels);
		}
		*spans_out = Spans[column];
	}
	return Pixels + column*Height;
}


//==========================================================================
//
//
//
//==========================================================================

void FPatchTexture::MakeTexture ()
{
	BYTE *remap, remaptable[256];
	int numspans;
	const column_t *maxcol;
	int x;

	FMemLump lump = Wads.ReadLump (SourceLump);
	const patch_t *patch = (const patch_t *)lump.GetMem();
	const alphapatch_t *apatch = (const alphapatch_t *)lump.GetMem();
	const betapatch_t *bpatch = (const betapatch_t *)lump.GetMem();

	maxcol = (const column_t *)((const BYTE *)patch + Wads.LumpLength (SourceLump) - 3);

	// Check for badly-sized patches
#if 0	// Such textures won't be created so there's no need to check here
	if (LittleShort(patch->width) <= 0 || LittleShort(patch->height) <= 0)
	{
		lump = Wads.ReadLump ("-BADPATC");
		patch = (const patch_t *)lump.GetMem();
		Printf (PRINT_BOLD, "Patch %s has a non-positive size.\n", Name);
	}
	else if (LittleShort(patch->width) > 2048 || LittleShort(patch->height) > 2048)
	{
		lump = Wads.ReadLump ("-BADPATC");
		patch = (const patch_t *)lump.GetMem();
		Printf (PRINT_BOLD, "Patch %s is too big.\n", Name);
	}
#endif

	if (bNoRemap0)
	{
		memcpy (remaptable, GPalette.Remap, 256);
		remaptable[0] = 0;
		remap = remaptable;
	}
	else
	{
		remap = GPalette.Remap;
	}


	if (hackflag)
	{
		Pixels = new BYTE[Width * Height];
		BYTE *out;

		// Draw the image to the buffer
		for (x = 0, out = Pixels; x < Width; ++x)
		{
			const BYTE *in = (const BYTE *)patch + LittleLong(patch->columnofs[x]) + 3;

			for (int y = Height; y > 0; --y)
			{
				*out = remap[*in];
				out++, in++;
			}
		}
		return;
	}

	// Add a little extra space at the end if the texture's height is not
	// a power of 2, in case somebody accidentally makes it repeat vertically.
	int numpix = Width * Height + (1 << HeightBits) - Height;

	numspans = Width;

	Pixels = new BYTE[numpix];
	memset (Pixels, 0, numpix);

	// Draw the image to the buffer
	for (x = 0; x < Width; ++x)
	{
		BYTE *outtop = Pixels + x*Height;
		const column_t *column;
		switch(Format) 
		{
		case PATCH_NORMAL: 
			column = (const column_t *)((const BYTE *)patch + LittleLong(patch->columnofs[x]));
			break;
		case PATCH_BETA: 
			column = (const column_t *)((const BYTE *)bpatch + LittleShort(bpatch->columnofs[x]));
			break;
		case PATCH_ALPHA: 
			column = (const column_t *)((const BYTE *)apatch + LittleShort(apatch->columnofs[x]));
			break;
		}
			
		int top = -1;

		while (column < maxcol && column->topdelta != 0xFF)
		{
			if (column->topdelta <= top)
			{
				top += column->topdelta;
			}
			else
			{
				top = column->topdelta;
			}

			int len = column->length;
			BYTE *out = outtop + top;

			if (len != 0)
			{
				if (top + len > Height)	// Clip posts that extend past the bottom
				{
					len = Height - top;
				}
				if (len > 0)
				{
					numspans++;

					const BYTE *in = (const BYTE *)column + (Format ? 2 : 3);
					for (int i = 0; i < len; ++i)
					{
						out[i] = remap[in[i]];
					}
				}
			}
			column = (const column_t *)((const BYTE *)column + column->length + (Format ? 2 : 4));
		}
	}
}


//==========================================================================
//
// Fix for certain special patches on single-patch textures.
//
//==========================================================================

void FPatchTexture::HackHack (int newheight)
{
	// Check if this patch is likely to be a problem.
	// It must be 256 pixels tall, and all its columns must have exactly
	// one post, where each post has a supposed length of 0.
	FMemLump lump = Wads.ReadLump (SourceLump);
	const patch_t *realpatch = (patch_t *)lump.GetMem();
	const DWORD *cofs = realpatch->columnofs;
	int x, x2 = LittleShort(realpatch->width);

	if (LittleShort(realpatch->height) == 256)
	{
		for (x = 0; x < x2; ++x)
		{
			const column_t *col = (column_t*)((BYTE*)realpatch+LittleLong(cofs[x]));
			if (col->topdelta != 0 || col->length != 0)
			{
				break;	// It's not bad!
			}
			col = (column_t *)((BYTE *)col + 256 + 4);
			if (col->topdelta != 0xFF)
			{
				break;	// More than one post in a column!
			}
		}
		if (x == x2)
		{ 
			// If all the columns were checked, it needs fixing.
			Unload ();
			if (Spans != NULL)
			{
				FreeSpans (Spans);
			}

			Height = newheight;
			LeftOffset = 0;
			TopOffset = 0;

			hackflag = true;
			bMasked = false;	// Hacked textures don't have transparent parts.
		}
	}
}
