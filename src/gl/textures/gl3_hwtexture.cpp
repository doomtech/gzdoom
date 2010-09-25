/*
** gl3_hwtexture.cpp
**
** Encapsulates the GL object describing one hardware texture
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
** 5. Full disclosure of the entire project's source code, except for third
**    party libraries is mandatory. (NOTE: This clause is non-negotiable!)
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

#include "gl/system/gl_system.h"

#include "gl/renderer/gl_renderer.h"
#include "gl/textures/gl3_hwtexture.h"

//===========================================================================
// 
//	Static texture data
//
//===========================================================================
unsigned int FGL3HardwareTexture::lastbound[FGL3HardwareTexture::MAX_TEXTURE_UNITS];


//===========================================================================
// 
//	Creates a texture
//
//===========================================================================

FGL3HardwareTexture::FGL3HardwareTexture(bool mip) 
{
	mMipmapped = mip;
	gl.GenTextures(1,&mGlTexId);
}

//===========================================================================
// 
//	Destroys the texture
//
//===========================================================================

FGL3HardwareTexture::~FGL3HardwareTexture() 
{ 
	if (mGlTexId != 0) 
	{
		for(int i = 0; i < MAX_TEXTURE_UNITS; i++)
		{
			if (lastbound[i] == mGlTexId)
			{
				lastbound[i] = 0;
			}
		}
	}
}

//===========================================================================
// 
//	Binds this texture to a texture unit
//
//===========================================================================

bool FGL3HardwareTexture::Bind(int texunit)
{
	if (mGlTexId!=0)
	{
		if (lastbound[texunit] != mGlTexId)
		{
			lastbound[texunit] = mGlTexId;
			gl.BindMultiTexture(GL_TEXTURE0 + texunit, GL_TEXTURE_2D, mGlTexId);
		}
		return true;
	}
	return false;
}

//===========================================================================
// 
//	(static) unbinds texture from given unit
//
//===========================================================================

void FGL3HardwareTexture::Unbind(int texunit)
{
	if (lastbound[texunit] != 0)
	{
		gl.BindMultiTexture(GL_TEXTURE0 + texunit, GL_TEXTURE_2D, 0);
		lastbound[texunit] = 0;
	}
}

//===========================================================================
// 
//	(static) unbinds textures from all texture units
//
//===========================================================================

void FGL3HardwareTexture::UnbindAll()
{
	for(int texunit = 0; texunit < MAX_TEXTURE_UNITS; texunit++)
	{
		Unbind(texunit);
	}
}

//===========================================================================
// 
//	Binds this texture's surface to the current framebuffer
//
//===========================================================================

void FGL3HardwareTexture::BindToCurrentFrameBuffer()
{
	gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mGlTexId, 0);
}


//===========================================================================
// 
//	Loads the texture image into the hardware
//
// NOTE: For some strange reason I was unable to find the source buffer
// should be one line higher than the actual texture. I got extremely
// strange crashes deep inside the GL driver when I didn't do it!
//
//===========================================================================

void FGL3HardwareTexture::CreateTexture(unsigned char * buffer,int w, int h, int texformat)
{
	bool deletebuffer = false;
	if (buffer == NULL)
	{
		// The texture must at least be initialized if no data is present.
		mMipmapped = false;
		buffer = (unsigned char *)calloc(4,w * (h+1));
		deletebuffer=true;
	}
	gl.TextureImage2D(mGlTexId, GL_TEXTURE_2D, 0, texformat, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	if (deletebuffer) free(buffer);

	if (mMipmapped)
	{
		gl.GenerateTextureMipmap(mGlTexId, GL_TEXTURE_2D);
	}
}

