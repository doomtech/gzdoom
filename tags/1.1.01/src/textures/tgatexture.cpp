/*
** tgatexture.cpp
** Texture class for TGA images
**
**---------------------------------------------------------------------------
** Copyright 2006 Christoph Oelckers
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
#include "r_local.h"
#include "w_wad.h"
#include "templates.h"


bool FTGATexture::Check(FileReader & data)
{
	TGAHeader hdr;
	
	if (data.GetLength() < (long)sizeof(hdr)) return false;
	
	data.Seek(0, SEEK_SET);
	data.Read(&hdr, sizeof(hdr));
#ifdef WORDS_BIGENDIAN
	hdr.width = LittleShort(hdr.width);
	hdr.height = LittleShort(hdr.height);
#endif
	
	// Not much that can be done here because TGA does not have a proper
	// header to be identified with.
	if (hdr.has_cm != 0 && hdr.has_cm != 1) return false;
	if (hdr.width <=0 || hdr.height <=0 || hdr.width > 2048 || hdr.height > 2048) return false;
	if (hdr.bpp != 8 && hdr.bpp != 15 && hdr.bpp != 16 && hdr.bpp !=24 && hdr.bpp !=32) return false;
	if (hdr.img_type <= 0 || hdr.img_type > 11) return false;
	if (hdr.img_type >=4  && hdr.img_type <= 8) return false;
	if ((hdr.img_desc & 16) != 0) return false;
	return true;
}

FTexture *FTGATexture::Create(FileReader & file, int lumpnum)
{
	TGAHeader hdr;
	file.Seek(0, SEEK_SET);
	file.Read(&hdr, sizeof(hdr));
#ifdef WORDS_BIGENDIAN
	hdr.width = LittleShort(hdr.width);
	hdr.height = LittleShort(hdr.height);
#endif

	return new FTGATexture(lumpnum, &hdr);
}


FTGATexture::FTGATexture (int lumpnum, TGAHeader * hdr)
: SourceLump(lumpnum), Pixels(0), Spans(0)
{
	Wads.GetLumpName (Name, lumpnum);
	Name[8] = 0;
	Width = hdr->width;
	Height = hdr->height;
	// Alpha channel is used only for 32 bit RGBA and paletted images with RGBA palettes.
	bMasked = (hdr->img_desc&15)==8 && (hdr->bpp==32 || (hdr->img_type==1 && hdr->cm_size==32));
	CalcBitSize();
}

FTGATexture::~FTGATexture ()
{
	Unload ();
	if (Spans != NULL)
	{
		FreeSpans (Spans);
		Spans = NULL;
	}
}


void FTGATexture::Unload ()
{
	if (Pixels != NULL)
	{
		delete[] Pixels;
		Pixels = NULL;
	}
}

FTextureFormat FTGATexture::GetFormat()
{
	return TEX_RGB;
}

const BYTE *FTGATexture::GetColumn (unsigned int column, const Span **spans_out)
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
		*spans_out = Spans[column];
	}
	return Pixels + column*Height;
}

const BYTE *FTGATexture::GetPixels ()
{
	if (Pixels == NULL)
	{
		MakeTexture ();
	}
	return Pixels;
}

void FTGATexture::ReadCompressed(FileReader &lump, BYTE * buffer, int bytesperpixel)
{
	BYTE b;
	BYTE data[4];
	int Size = Width * Height;
	
	while (Size > 0) 
	{
		lump >> b;
		if (b & 128)
		{
			b&=~128;
			lump.Read(data, bytesperpixel);
			for (int i=MIN<int>(Size, (b+1)); i>0; i--)
			{
				buffer[0] = data[0];
				if (bytesperpixel>=2) buffer[1] = data[1];
				if (bytesperpixel>=3) buffer[2] = data[2];
				if (bytesperpixel==4) buffer[3] = data[3];
				buffer+=bytesperpixel;
			}
		}
		else 
		{
			lump.Read(buffer, MIN<int>(Size, (b+1))*bytesperpixel);
			buffer += (b+1)*bytesperpixel;
		}
		Size -= b+1;
	}
}

void FTGATexture::MakeTexture ()
{
	BYTE PaletteMap[256];
	FWadLump lump = Wads.OpenLumpNum (SourceLump);
	TGAHeader hdr;
	WORD w;
	BYTE r,g,b,a;
	BYTE * buffer;

	Pixels = new BYTE[Width*Height];
	lump.Read(&hdr, sizeof(hdr));
	lump.Seek(hdr.id_len, SEEK_CUR);
	
#ifdef WORDS_BIGENDIAN
	hdr.width = LittleShort(hdr.width);
	hdr.height = LittleShort(hdr.height);
	hdr.cm_first = LittleShort(hdr.cm_first);
	hdr.cm_length = LittleShort(hdr.cm_length);
#endif

	if (hdr.has_cm)
	{
		memset(PaletteMap, 0, 256);
		for (int i = hdr.cm_first; i < hdr.cm_first + hdr.cm_length && i < 256; i++)
		{
			switch (hdr.cm_size)
			{
			case 15:
			case 16:
				lump >> w;
				r = (w & 0x001F) << 3;
				g = (w & 0x03E0) >> 2;
				b = (w & 0x7C00) >> 7;
				a = 255;
				break;
				
			case 24:
				lump >> b >> g >> r;
				a=255;
				break;
				
			case 32:
				lump >> b >> g >> r >> a;
				if ((hdr.img_desc&15)!=8) a=255;
				break;
				
			default:	// should never happen
				r=g=b=a=0;
				break;
			}
			PaletteMap[i] = a>=128? ColorMatcher.Pick(r, g, b) : 0;
		}
    }
    
    int Size = Width * Height * (hdr.bpp>>3);
   	buffer = new BYTE[Size];
   	
    if (hdr.img_type < 4)	// uncompressed
    {
    	lump.Read(buffer, Size);
    }
    else				// compressed
    {
    	ReadCompressed(lump, buffer, hdr.bpp>>3);
    }
    
	BYTE * ptr = buffer;
	int step_x = (hdr.bpp>>3);
	int Pitch = Width * step_x;

	if (hdr.img_desc&32)
	{
		ptr += (Width-1) * step_x;
		step_x =- step_x;
	}
	if (!(hdr.img_desc&64))
	{
		ptr += (Height-1) * Pitch;
		Pitch = -Pitch;
	}

    switch (hdr.img_type & 7)
    {
	case 1:	// paletted
		for(int y=0;y<Height;y++)
		{
			BYTE * p = ptr + y * Pitch;
			for(int x=0;x<Width;x++)
			{
				Pixels[x*Height+y] = PaletteMap[*p];
				p+=step_x;
			}
		}
		break;

	case 2:	// RGB
		switch (hdr.bpp)
		{
		case 15:
		case 16:
			step_x>>=1;
			for(int y=0;y<Height;y++)
			{
				WORD * p = (WORD*)(ptr + y * Pitch);
				for(int x=0;x<Width;x++)
				{
					int v = LittleLong(*p);
					Pixels[x*Height+y] = RGB32k[(v>>10) & 0x1f][(v>>5) & 0x1f][v & 0x1f];
					p+=step_x;
				}
			}
			break;
		
		case 24:
			for(int y=0;y<Height;y++)
			{
				BYTE * p = ptr + y * Pitch;
				for(int x=0;x<Width;x++)
				{
					Pixels[x*Height+y] = RGB32k[p[2]>>3][p[1]>>3][p[0]>>3];
					p+=step_x;
				}
			}
			break;
		
		case 32:
			if ((hdr.img_desc&15)!=8)	// 32 bits without a valid alpha channel
			{
				for(int y=0;y<Height;y++)
				{
					BYTE * p = ptr + y * Pitch;
					for(int x=0;x<Width;x++)
					{
						Pixels[x*Height+y] = RGB32k[p[2]>>3][p[1]>>3][p[0]>>3];
						p+=step_x;
					}
				}
			}
			else
			{
				for(int y=0;y<Height;y++)
				{
					BYTE * p = ptr + y * Pitch;
					for(int x=0;x<Width;x++)
					{
						Pixels[x*Height+y] = p[3] >= 128? RGB32k[p[2]>>3][p[1]>>3][p[0]>>3] : 0;
						p+=step_x;
					}
				}
			}
			break;
		
		default:
			break;
		}
		break;
	
	case 3:	// Grayscale
		switch (hdr.bpp)
		{
		case 8:
			for(int y=0;y<Height;y++)
			{
				BYTE * p = ptr + y * Pitch;
				for(int x=0;x<Width;x++)
				{
					Pixels[x*Height+y] = GrayMap[*p];
					p+=step_x;
				}
			}
			break;
		
		case 16:
			for(int y=0;y<Height;y++)
			{
				BYTE * p = ptr + y * Pitch;
				for(int x=0;x<Width;x++)
				{
					Pixels[x*Height+y] = GrayMap[p[1]];	// only use the high byte
					p+=step_x;
				}
			}
			break;
		
		default:
			break;
		}
		break;

	default:
		break;
    }
	delete [] buffer;
	if (Spans == NULL)
	{
		Spans = CreateSpans (Pixels);
	}
}	

//===========================================================================
//
// FTGATexture::CopyTrueColorPixels
//
//===========================================================================

int FTGATexture::CopyTrueColorPixels(BYTE *buffer, int buf_pitch, int buf_height, int x, int y)
{
	PalEntry pe[256];
	FWadLump lump = Wads.OpenLumpNum (SourceLump);
	TGAHeader hdr;
	WORD w;
	BYTE r,g,b,a;
	BYTE * sbuffer;
	int transval = 0;

	lump.Read(&hdr, sizeof(hdr));
	lump.Seek(hdr.id_len, SEEK_CUR);
	
#ifdef WORDS_BIGENDIAN
	hdr.width = LittleShort(hdr.width);
	hdr.height = LittleShort(hdr.height);
	hdr.cm_first = LittleShort(hdr.cm_first);
	hdr.cm_length = LittleShort(hdr.cm_length);
#endif

	if (hdr.has_cm)
	{
		memset(pe, 0, 256*sizeof(PalEntry));
		for (int i = hdr.cm_first; i < hdr.cm_first + hdr.cm_length && i < 256; i++)
		{
			switch (hdr.cm_size)
			{
			case 15:
			case 16:
				lump >> w;
				r = (w & 0x001F) << 3;
				g = (w & 0x03E0) >> 2;
				b = (w & 0x7C00) >> 7;
				a = 255;
				break;
				
			case 24:
				lump >> b >> g >> r;
				a=255;
				break;
				
			case 32:
				lump >> b >> g >> r >> a;
				if ((hdr.img_desc&15)!=8) a=255;
				else if (a!=0 && a!=255) transval = true;
				break;
				
			default:	// should never happen
				r=g=b=a=0;
				break;
			}
			pe[i] = PalEntry(255-a, r, g, b);
		}
    }
    
    int Size = Width * Height * (hdr.bpp>>3);
   	sbuffer = new BYTE[Size];
   	
    if (hdr.img_type < 4)	// uncompressed
    {
    	lump.Read(sbuffer, Size);
    }
    else				// compressed
    {
    	ReadCompressed(lump, sbuffer, hdr.bpp>>3);
    }
    
	BYTE * ptr = sbuffer;
	int step_x = (hdr.bpp>>3);
	int Pitch = Width * step_x;

	if (hdr.img_desc&32)
	{
		ptr += (Width-1) * step_x;
		step_x =- step_x;
	}
	if (!(hdr.img_desc&64))
	{
		ptr += (Height-1) * Pitch;
		Pitch = -Pitch;
	}

    switch (hdr.img_type & 7)
    {
	case 1:	// paletted
		screen->CopyPixelData(buffer, buf_pitch, buf_height, x, y, ptr, Width, Height, step_x, Pitch, pe);
		break;

	case 2:	// RGB
		switch (hdr.bpp)
		{
		case 15:
		case 16:
			screen->CopyPixelDataRGB(buffer, buf_pitch, buf_height, x, y, ptr, Width, Height, step_x, Pitch, CF_RGB555);
			break;
		
		case 24:
			screen->CopyPixelDataRGB(buffer, buf_pitch, buf_height, x, y, ptr, Width, Height, step_x, Pitch, CF_BGR);
			break;
		
		case 32:
			if ((hdr.img_desc&15)!=8)	// 32 bits without a valid alpha channel
			{
				screen->CopyPixelDataRGB(buffer, buf_pitch, buf_height, x, y, ptr, Width, Height, step_x, Pitch, CF_BGR);
			}
			else
			{
				screen->CopyPixelDataRGB(buffer, buf_pitch, buf_height, x, y, ptr, Width, Height, step_x, Pitch, CF_BGRA);
				transval = -1;
			}
			break;
		
		default:
			break;
		}
		break;
	
	case 3:	// Grayscale
		switch (hdr.bpp)
		{
		case 8:
			for(int i=0;i<256;i++) pe[i]=PalEntry(0,i,i,i);	// gray map
			screen->CopyPixelData(buffer, buf_pitch, buf_height, x, y, ptr, Width, Height, step_x, Pitch, pe);
			break;
		
		case 16:
			screen->CopyPixelDataRGB(buffer, buf_pitch, buf_height, x, y, ptr, Width, Height, step_x, Pitch, CF_I16);
			break;
		
		default:
			break;
		}
		break;

	default:
		break;
    }
	delete [] sbuffer;
	return transval;
}	

//===========================================================================
//
//
//===========================================================================

bool FTGATexture::UseBasePalette() 
{ 
	return false; 
}