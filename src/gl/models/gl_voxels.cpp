/*
** gl_voxels.cpp
**
** Voxel management
**
**---------------------------------------------------------------------------
** Copyright 2010 Christoph Oelckers
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

#include "gl/system/gl_system.h"
#include "w_wad.h"
#include "cmdlib.h"
#include "sc_man.h"
#include "m_crc32.h"
#include "c_console.h"
#include "g_game.h"
#include "doomstat.h"
#include "g_level.h"
//#include "gl/gl_intern.h"

#include "gl/renderer/gl_renderer.h"
#include "gl/scene/gl_drawinfo.h"
#include "gl/models/gl_models.h"
#include "gl/textures/gl_material.h"
#include "gl/utility/gl_geometric.h"
#include "gl/utility/gl_convert.h"



//===========================================================================
//
// 
//
//===========================================================================

FVoxelModel::FVoxelModel(FVoxel *voxel, bool owned)
{
	mVoxel = voxel;
	mOwningVoxel = owned;
	mVBO = NULL;
	Initialize();
}

//===========================================================================
//
// 
//
//===========================================================================

FVoxelModel::~FVoxelModel()
{
	CleanGLData();
	if (mOwningVoxel) delete mVoxel;
}

//===========================================================================
//
// 
//
//===========================================================================

void FVoxelModel::Initialize()
{
}

//===========================================================================
//
// 
//
//===========================================================================

bool FVoxelModel::Load(const char * fn, int lumpnum, const char * buffer, int length)
{
	return false;	// not needed
}

//===========================================================================
//
// 
//
//===========================================================================

void FVoxelModel::MakeGLData()
{
	// todo: create the vertex buffer
}

//===========================================================================
//
// 
//
//===========================================================================

void FVoxelModel::CleanGLData()
{
	if (mVBO != NULL)
	{
		// todo: delete the vertex buffer
		mVBO = NULL;
	}
}

//===========================================================================
//
// Voxels don't have frames so always return 0
//
//===========================================================================

int FVoxelModel::FindFrame(const char * name)
{
	return 0;
}

//===========================================================================
//
// 
//
//===========================================================================

void FVoxelModel::RenderFrame(FTexture * skin, int frame, int cm, Matrix3x4 *m2v, int translation)
{
}

//===========================================================================
//
// Voxels never interpolate between frames
//
//===========================================================================

void FVoxelModel::RenderFrameInterpolated(FTexture * skin, int frame, int frame2, double inter, int cm, Matrix3x4 *m2v, int translation)
{
	RenderFrame(skin, frame, cm, m2v, translation);
}

