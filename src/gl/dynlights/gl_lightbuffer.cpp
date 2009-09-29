/*
** gl_dynlight1.cpp
** dynamic light buffer for shader rendering
**
**---------------------------------------------------------------------------
** Copyright 2009 Christoph Oelckers
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
#include "c_dispatch.h"
#include "p_local.h"
#include "vectors.h"
#include "g_level.h"

#include "gl/system/gl_cvars.h"
#include "gl/renderer/gl_renderer.h"
#include "gl/renderer/gl_lightdata.h"
#include "gl/data/gl_data.h"
#include "gl/dynlights/gl_dynlight.h"
#include "gl/dynlights/gl_lightbuffer.h"
#include "gl/scene/gl_drawinfo.h"
#include "gl/scene/gl_portal.h"
#include "gl/shaders/gl_shader.h"
#include "gl/textures/gl_material.h"
#include "gl/utility/gl_convert.h"


//==========================================================================
//
//
//
//==========================================================================

FLightBuffer::FLightBuffer()
{
	gl.GenBuffers(1, &mIDbuf_RGB);
	gl.BindBuffer(GL_TEXTURE_BUFFER, mIDbuf_RGB);
	gl.BufferData(GL_TEXTURE_BUFFER, sizeof(FLightRGB) * MAX_DYNLIGHTS, NULL, GL_DYNAMIC_DRAW);

	gl.GenBuffers(1, &mIDbuf_Position);
	gl.BindBuffer(GL_TEXTURE_BUFFER, mIDbuf_Position);
	gl.BufferData(GL_TEXTURE_BUFFER, sizeof(FLightPosition) * MAX_DYNLIGHTS, NULL, GL_DYNAMIC_DRAW);

	gl.GenTextures(1, &mIDtex_RGB);
	gl.BindTexture(GL_TEXTURE_BUFFER, mIDtex_RGB);
	gl.TexBufferARB(GL_TEXTURE_BUFFER, GL_RGBA8, mIDbuf_RGB);

	gl.GenTextures(1, &mIDtex_Position);
	gl.BindTexture(GL_TEXTURE_BUFFER, mIDtex_Position);
	gl.TexBufferARB(GL_TEXTURE_BUFFER, GL_RGBA32F, mIDbuf_Position);
}


//==========================================================================
//
//
//
//==========================================================================

FLightBuffer::~FLightBuffer()
{
	gl.BindBuffer(GL_TEXTURE_BUFFER, 0);
	gl.DeleteBuffers(1, &mIDbuf_RGB);
	gl.DeleteBuffers(1, &mIDbuf_Position);

	gl.BindTexture(GL_TEXTURE_BUFFER, 0);
	gl.DeleteTextures(1, &mIDtex_RGB);
	gl.DeleteTextures(1, &mIDtex_Position);

}


//==========================================================================
//
//
//
//==========================================================================

void FLightBuffer::MapBuffer()
{
	gl.BindBuffer(GL_TEXTURE_BUFFER, mIDbuf_RGB);
	mp_RGB = (FLightRGB*)gl.MapBufferRange(GL_TEXTURE_BUFFER, 0, sizeof(FLightRGB) * MAX_DYNLIGHTS, 
			GL_MAP_WRITE_BIT|GL_MAP_FLUSH_EXPLICIT_BIT|GL_MAP_UNSYNCHRONIZED_BIT);

	gl.BindBuffer(GL_TEXTURE_BUFFER, mIDbuf_Position);
	mp_Position = (FLightPosition*)gl.MapBufferRange(GL_TEXTURE_BUFFER, 0, sizeof(FLightPosition) * MAX_DYNLIGHTS, 
			GL_MAP_WRITE_BIT|GL_MAP_FLUSH_EXPLICIT_BIT|GL_MAP_UNSYNCHRONIZED_BIT);

	mIndex = 0;
}

//==========================================================================
//
//
//
//==========================================================================

void FLightBuffer::UnmapBuffer()
{
	gl.BindBuffer(GL_TEXTURE_BUFFER, mIDbuf_RGB);
	gl.FlushMappedBufferRange(GL_TEXTURE_BUFFER, 0, mIndex * sizeof(FLightRGB));
	gl.UnmapBuffer(GL_TEXTURE_BUFFER);
	mp_RGB = NULL;

	gl.BindBuffer(GL_TEXTURE_BUFFER, mIDbuf_Position);
	gl.FlushMappedBufferRange(GL_TEXTURE_BUFFER, 0, mIndex * sizeof(FLightPosition));
	gl.UnmapBuffer(GL_TEXTURE_BUFFER);
	mp_Position = NULL;
}


//==========================================================================
//
//
//
//==========================================================================

void FLightBuffer::BindTextures(int texunit1, int texunit2)
{
	gl.ActiveTexture(GL_TEXTURE14);
	gl.BindTexture(GL_TEXTURE_BUFFER, mIDtex_RGB);
	gl.ActiveTexture(GL_TEXTURE15);
	gl.BindTexture(GL_TEXTURE_BUFFER, mIDtex_Position);
	gl.ActiveTexture(GL_TEXTURE0);
}


//==========================================================================
//
//
//
//==========================================================================

void FLightBuffer::AddLight(ADynamicLight *light, bool foggy)
{
	if (mIndex >= MAX_DYNLIGHTS) 
	{
		return;
	}
	assert(mp_RGB != NULL && mp_Position != NULL);
	mp_RGB[mIndex].R = light->GetRed();
	mp_RGB[mIndex].G = light->GetGreen();
	mp_RGB[mIndex].B = light->GetBlue();
	mp_RGB[mIndex].Type = (light->flags4 & MF4_SUBTRACTIVE)? 128 : (light->flags4 & MF4_ADDITIVE || foggy)? 255:0;
	mp_Position[mIndex].X = TO_GL(light->x);
	mp_Position[mIndex].Y = TO_GL(light->y); 
	mp_Position[mIndex].Z =  TO_GL(light->z);
	mp_Position[mIndex].Distance = (light->GetRadius() * gl_lights_size);
	mIndex++; 
}

