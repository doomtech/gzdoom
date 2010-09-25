/*
** gltexture.cpp
** Low level OpenGL texture handling. These classes are also
** containers for the various translations a texture can have.
**
**---------------------------------------------------------------------------
** Copyright 2004-2005 Christoph Oelckers
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
#include "gl/textures/gl3_sampler.h"


//===========================================================================
// 
//	Static texture data
//
//===========================================================================
unsigned int FGL3Sampler::lastbound[FGL3Sampler::MAX_TEXTURE_UNITS];

static int MapWrapMode[] =
{
	GL_CLAMP_TO_EDGE,
	GL_REPEAT,
	GL_MIRRORED_REPEAT
};

static int MapWrapCoord[] = 
{
	GL_TEXTURE_WRAP_S,
	GL_TEXTURE_WRAP_T,
	GL_TEXTURE_WRAP_R,
};

struct TexFilter_t
{
	int minfilter;
	int magfilter;
} ;

static TexFilter_t TexFilter[]=
{
	{GL_NEAREST,					GL_NEAREST},
	{GL_NEAREST_MIPMAP_NEAREST,		GL_NEAREST},
	{GL_LINEAR,						GL_LINEAR},
	{GL_LINEAR_MIPMAP_NEAREST,		GL_LINEAR},
	{GL_LINEAR_MIPMAP_LINEAR,		GL_LINEAR},
	{GL_NEAREST_MIPMAP_LINEAR,		GL_NEAREST},
};

//===========================================================================
// 
//	Creates a texture
//
//===========================================================================
FGL3Sampler::FGL3Sampler() 
{
	gl.GenSamplers(1, &glSamplerID);
}


//===========================================================================
// 
//	Destroys the texture
//
//===========================================================================
FGL3Sampler::~FGL3Sampler() 
{ 
	if (glSamplerID != 0) 
	{
		for(int i = 0; i < MAX_TEXTURE_UNITS; i++)
		{
			if (lastbound[i] == glSamplerID)
			{
				lastbound[i] = 0;
			}
		}
		gl.DeleteSamplers(1, &glSamplerID);
	}
}


//===========================================================================
// 
//	Binds this sampler
//
//===========================================================================

bool FGL3Sampler::Bind(int texunit)
{
	if (lastbound[texunit] != glSamplerID)
	{
		gl.BindSampler(GL_TEXTURE0 + texunit, glSamplerID);
		lastbound[texunit] = glSamplerID;
		return true;
	}
	return false;
}

//===========================================================================
// 
//	Binds this sampler
//
//===========================================================================

void FGL3Sampler::Unbind(int texunit)
{
	if (lastbound[texunit] != 0)
	{
		gl.BindSampler(GL_TEXTURE0 + texunit, 0);
	}
}

void FGL3Sampler::UnbindAll()
{
	for(int texunit = 0; texunit < MAX_TEXTURE_UNITS; texunit++)
	{
		Unbind(texunit);
	}
}

//===========================================================================
// 
//	states
//
//===========================================================================

void FGL3Sampler::SetTextureFilter(int mode)
{
	assert(mode < TEXFILTER_MAX);

	gl.SamplerParameteri(glSamplerID, GL_TEXTURE_MIN_FILTER, TexFilter[mode].minfilter);
	gl.SamplerParameteri(glSamplerID, GL_TEXTURE_MAG_FILTER, TexFilter[mode].magfilter);
}

void FGL3Sampler::SetWrapMode(int coord, int wrapmode)
{
	gl.SamplerParameteri(glSamplerID, MapWrapCoord[coord], MapWrapMode[wrapmode]);
}

void FGL3Sampler::SetAnisotropy(float factor)
{
	gl.SamplerParameterf(glSamplerID, GL_TEXTURE_MAX_ANISOTROPY_EXT, factor);
}
