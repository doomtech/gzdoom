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
// Vertex and vertex buffer classes
//

#include "gl/gl_include.h"
#include "gl/r_render/r_render.h"
#include "gl/new_renderer/gl2_vertex.h"

namespace GLRendererNew
{
unsigned int FVertexBuffer::mLastBound;

FVertexBuffer::FVertexBuffer(int size)
{
	gl.GenBuffers(1, &mBufferId);
	mMaxSize = size;
}

FVertexBuffer::~FVertexBuffer()
{
	gl.DeleteBuffers(1, &mBufferId);
}

void FVertexBuffer::BindBuffer()
{
	if (mLastBound != mBufferId)
	{
		mLastBound = mBufferId;
		gl.BindBuffer(GL_ARRAY_BUFFER, mBufferId);
	}
}

void *FVertexBuffer::Map()
{
	BindBuffer();
	return gl.MapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
}

bool FVertexBuffer::Unmap()
{
	BindBuffer();
	return !!gl.UnmapBuffer(GL_ARRAY_BUFFER);
}

}
