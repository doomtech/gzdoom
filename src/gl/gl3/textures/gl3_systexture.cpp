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
#include "gl/gl3/textures/gl3_systexture.h"

//===========================================================================
// 
//	Map engine identifiers to OpenGL
//
//===========================================================================

static int TexFormatMap[] = {
	GL_RGBA8,
	GL_RGB5_A1,
	GL_RGBA4,
	GL_RGBA2,
	GL_COMPRESSED_RGBA_ARB,
	GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,
	GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,
	GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,

	GL_ALPHA8,
	GL_R8,
	GL_RG8,
	GL_R32F,
	GL_RG32F,
	GL_RGBA32F,

	GL_BGRA,
	GL_ABGR_EXT,
};

//===========================================================================
// 
//	Creates a texture
//
//===========================================================================

GL3SystemTexture::GL3SystemTexture(bool mip) 
: HWSystemTexture(mip)
{
	gl.GenTextures(1,&mGlTexId);
}

//===========================================================================
// 
//	Destroys the texture
//
//===========================================================================

GL3SystemTexture::~GL3SystemTexture() 
{ 
	HWRenderer->TextureDeleted(this);
	if (mGlTexId != 0) 
	{
		gl.DeleteTextures(1,&mGlTexId);
	}
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

void GL3SystemTexture::CreateTexture(unsigned texformat, unsigned char *buffer,unsigned bufferformat, int w, int h)
{
	bool deletebuffer = false;

	assert(texformat < HWT_MAX_TEX_FORMATS);
	assert(bufferformat < HWT_MAX_TEX_FORMATS);
	if (texformat >= HWT_MAX_TEX_FORMATS || bufferformat >= HWT_MAX_TEX_FORMATS) return;

	texformat = TexFormatMap[texformat];
	bufferformat = TexFormatMap[bufferformat];

	if (buffer == NULL)
	{
		// The texture must at least be initialized if no data is present.
		mMipmapped = false;
		buffer = (unsigned char *)calloc(4,w * (h+1));
		deletebuffer=true;
		bufferformat = GL_RGBA;
	}
	gl.TextureImage2D(mGlTexId, GL_TEXTURE_2D, 0, texformat, w, h, 0, bufferformat, GL_UNSIGNED_BYTE, buffer);
	if (deletebuffer) free(buffer);

	if (mMipmapped)
	{
		gl.GenerateTextureMipmap(mGlTexId, GL_TEXTURE_2D);
	}
}

