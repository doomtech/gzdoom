/*
** gl1_renderer.cpp
** Renderer interface
**
**---------------------------------------------------------------------------
** Copyright 2008 Christoph Oelckers
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

#include "gl/gl_include.h"
#include "gl/common/glc_clock.h"
#include "gl/common/glc_texture.h"
#include "files.h"
#include "m_swap.h"
#include "r_draw.h"
#include "v_video.h"
#include "r_main.h"
#include "m_png.h"
#include "m_crc32.h"
#include "gl/common/glc_templates.h"
#include "gl/common/glc_data.h"
#include "gl/gl_struct.h"
#include "gl/gl_intern.h"
#include "gl/old_renderer/gl1_renderer.h"
#include "gl/old_renderer/gl1_texture.h"
#include "gl/gl_functions.h"
#include "gl/old_renderer/gl1_shader.h"
#include "gl/gl_framebuffer.h"
#include "gl/common/glc_translate.h"
#include "vectors.h"
#include "gl/old_renderer/gl1_drawinfo.h"


//===========================================================================
// 
// Renderer interface
//
//===========================================================================

using namespace GLRendererOld;
EXTERN_CVAR(Bool, gl_render_segs)


GL1Renderer::~GL1Renderer()
{
	FGLTexture::DeleteAll();
	gl_ClearShaders();
}

void GL1Renderer::Initialize()
{
	gl_InitShaders();
	gl_InitFog();
}

void GL1Renderer::SetPaused()
{
	gl_DisableShader();
	gl_SetTextureMode(-1);
}

void GL1Renderer::UnsetPaused()
{
	gl_SetTextureMode(TM_MODULATE);
}

void GL1Renderer::Begin2D()
{
	gl_EnableFog(false);
}

//===========================================================================
// 
//
//
//===========================================================================

void GL1Renderer::ProcessWall(seg_t *seg, sector_t *sector, sector_t *backsector, subsector_t *polysub)
{
	GLRendererOld::GLWall wall;
	wall.Process(seg, sector, backsector, polysub, gl_render_segs);
	rendered_lines++;
}

//===========================================================================
// 
//
//
//===========================================================================

void GL1Renderer::ProcessLowerMiniseg(seg_t *seg, sector_t * frontsector, sector_t * backsector)
{
	GLRendererOld::GLWall wall;
	wall.ProcessLowerMiniseg(seg, frontsector, backsector);
	rendered_lines++;
}

//===========================================================================
// 
//
//
//===========================================================================

void GL1Renderer::ProcessSprite(AActor *thing, sector_t *sector)
{
	GLSprite glsprite;
	glsprite.Process(thing, sector);
}

//===========================================================================
// 
//
//
//===========================================================================

void GL1Renderer::ProcessParticle(particle_t *part, sector_t *sector)
{
	GLSprite glsprite;
	glsprite.ProcessParticle(part, sector);//, 0, 0);
}

//===========================================================================
// 
//
//
//===========================================================================

void GL1Renderer::ProcessSector(sector_t *fakesector, subsector_t *sub)
{
	GLFlat glflat;
	glflat.ProcessSector(fakesector, sub);
}

//===========================================================================
// 
//
//
//===========================================================================

void GL1Renderer::FlushTextures()
{
	FGLTexture::FlushAll();
}

//===========================================================================
// 
//
//
//===========================================================================

void GL1Renderer::RenderTextureView (FCanvasTexture *self, AActor *viewpoint, int fov)
{
	gl_RenderTextureView(self, viewpoint, fov);
}

//===========================================================================
// 
//
//
//===========================================================================

void GL1Renderer::PrecacheTexture(FTexture *tex)
{
	FGLTexture * gltex = FGLTexture::ValidateTexture(tex);
	if (gltex) 
	{
		if (tex->UseType==FTexture::TEX_Sprite) 
		{
			gltex->BindPatch(CM_DEFAULT, 0);
		}
		else 
		{
			gltex->Bind (CM_DEFAULT, 0, 0);
		}
	}
}

//===========================================================================
// 
//
//
//===========================================================================

void GL1Renderer::UncacheTexture(FTexture *tex)
{
	FGLTexture * gltex = FGLTexture::ValidateTexture(tex);
	if (gltex) gltex->Clean(true); 
}

//===========================================================================
// 
//
//
//===========================================================================

unsigned char *GL1Renderer::GetTextureBuffer(FTexture *tex, int &w, int &h)
{
	FGLTexture * gltex = FGLTexture::ValidateTexture(tex);
	if (gltex)
	{
		return gltex->CreateTexBuffer(FGLTexture::GLUSE_TEXTURE, CM_DEFAULT, 0, w, h);
	}
	return NULL;
}

//===========================================================================
// 
//
//
//===========================================================================

TArray<GLVertex> gl_vertices(1024);

void GL1Renderer::SetupLevel()
{
	int i,j;

	gl_vertices.Resize(100);	
	gl_vertices.Clear();	

	// Create the flat vertex array
	for (i=0; i<numsubsectors; i++)
	{
		subsector_t * ssector = &subsectors[i];

		if (ssector->numlines<=2) continue;
			
		ssector->numvertices = ssector->numlines;
		ssector->firstvertex = gl_vertices.Size();

		for(j = 0;  j < ssector->numlines; j++)
		{
			seg_t * seg = &segs[ssector->firstline + j];
			vertex_t * vtx = seg->v1;
			GLVertex * vt=&gl_vertices[gl_vertices.Reserve(1)];

			vt->u =  TO_GL(vtx->x)/64.0f;
			vt->v = -TO_GL(vtx->y)/64.0f;
			vt->x =  TO_GL(vtx->x);
			vt->y =  TO_GL(vtx->y);
			vt->z = 0.0f;
			vt->vt = vtx;
		}
	}
	gl_InitVertexData();
	pitch=0.0f;
}

//===========================================================================
// 
//
//
//===========================================================================

void GL1Renderer::CleanLevelData()
{
	gl_CleanVertexData();
}

//===========================================================================
// 
//
//
//===========================================================================

void GL1Renderer::ClearBorders()
{
	OpenGLFrameBuffer *glscreen = static_cast<OpenGLFrameBuffer*>(screen);

	// Letterbox time! Draw black top and bottom borders.
	int width = glscreen->GetWidth();
	int height = glscreen->GetHeight();
	int trueHeight = glscreen->GetTrueHeight();

	int borderHeight = (trueHeight - height) / 2;

	gl.Viewport(0, 0, width, trueHeight);
	gl.MatrixMode(GL_PROJECTION);
	gl.LoadIdentity();
	gl.Ortho(0.0, width * 1.0, 0.0, trueHeight, -1.0, 1.0);
	gl.MatrixMode(GL_MODELVIEW);
	gl.Color3f(0.f, 0.f, 0.f);
	gl_EnableTexture(false);
	gl_DisableShader();

	gl.Begin(GL_QUADS);
	// upper quad
	gl.Vertex2i(0, borderHeight);
	gl.Vertex2i(0, 0);
	gl.Vertex2i(width, 0);
	gl.Vertex2i(width, borderHeight);
	gl.End();

	gl.Begin(GL_QUADS);
	// lower quad
	gl.Vertex2i(0, trueHeight);
	gl.Vertex2i(0, trueHeight - borderHeight);
	gl.Vertex2i(width, trueHeight - borderHeight);
	gl.Vertex2i(width, trueHeight);
	gl.End();

	gl_EnableTexture(true);

	gl.Viewport(0, (trueHeight - height) / 2, width, height); 
}

//==========================================================================
//
// Draws a texture
//
//==========================================================================

void GL1Renderer::DrawTexture(FTexture *img, DCanvas::DrawParms &parms)
{
	float x = FIXED2FLOAT(parms.x - Scale (parms.left, parms.destwidth, parms.texwidth));
	float y = FIXED2FLOAT(parms.y - Scale (parms.top, parms.destheight, parms.texheight));
	float w = FIXED2FLOAT(parms.destwidth);
	float h = FIXED2FLOAT(parms.destheight);
	float ox, oy, cx, cy, r, g, b;
	float light = 1.f;

	FGLTexture * gltex = FGLTexture::ValidateTexture(img);

	const PatchTextureInfo * pti;

	if (parms.colorOverlay)
	{
		// Right now there's only black. Should be implemented properly later
		light = 1.f - APART(parms.colorOverlay)/255.f;
	}

	if (!img->bHasCanvas)
	{
		if (!parms.alphaChannel) 
		{
			int translation = 0;
			if (parms.remap != NULL)
			{
				GLTranslationPalette * pal = static_cast<GLTranslationPalette*>(parms.remap->GetNative());
				if (pal) translation = -pal->GetIndex();
			}
			pti = gltex->BindPatch(CM_DEFAULT, translation);
		}
		else 
		{
			// This is an alpha texture
			pti = gltex->BindPatch(CM_SHADE, 0);
		}

		if (!pti) return;

		ox = pti->GetUL();
		oy = pti->GetVT();
		cx = pti->GetUR();
		cy = pti->GetVB();
	}
	else
	{
		gltex->Bind(CM_DEFAULT, 0, 0);
		cx=1.f;
		cy=-1.f;
		ox = oy = 0.f;
	}
	
	if (parms.flipX)
	{
		float temp = ox;
		ox = cx;
		cx = temp;
	}
	
	// also take into account texInfo->windowLeft and texInfo->windowRight
	// just ignore for now...
	if (parms.windowleft || parms.windowright != img->GetScaledWidth()) return;
	
	if (parms.style.Flags & STYLEF_ColorIsFixed)
	{
		r = RPART(parms.fillcolor)/255.0f;
		g = GPART(parms.fillcolor)/255.0f;
		b = BPART(parms.fillcolor)/255.0f;
	}
	else
	{
		r = g = b = light;
	}
	
	// scissor test doesn't use the current viewport for the coordinates, so use real screen coordinates
	int btm = (SCREENHEIGHT - screen->GetHeight()) / 2;
	btm = SCREENHEIGHT - btm;

	gl.Enable(GL_SCISSOR_TEST);
	int space = (static_cast<OpenGLFrameBuffer*>(screen)->GetTrueHeight()-screen->GetHeight())/2;
	gl.Scissor(parms.lclip, btm - parms.dclip + space, parms.rclip - parms.lclip, parms.dclip - parms.uclip);
	
	gl_SetRenderStyle(parms.style, !parms.masked, false);

	gl.Color4f(r, g, b, FIXED2FLOAT(parms.alpha));
	
	gl.Disable(GL_ALPHA_TEST);
	gl_ApplyShader();
	gl.Begin(GL_TRIANGLE_STRIP);
	gl.TexCoord2f(ox, oy);
	gl.Vertex2i(x, y);
	gl.TexCoord2f(ox, cy);
	gl.Vertex2i(x, y + h);
	gl.TexCoord2f(cx, oy);
	gl.Vertex2i(x + w, y);
	gl.TexCoord2f(cx, cy);
	gl.Vertex2i(x + w, y + h);
	gl.End();
	gl.Enable(GL_ALPHA_TEST);
	
	gl.Scissor(0, 0, screen->GetWidth(), screen->GetHeight());
	gl.Disable(GL_SCISSOR_TEST);
	gl_SetTextureMode(TM_MODULATE);
	gl.BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	gl.BlendEquation(GL_FUNC_ADD);
}

//==========================================================================
//
//
//
//==========================================================================
void GL1Renderer::DrawLine(int x1, int y1, int x2, int y2, int palcolor, uint32 color)
{
	PalEntry p = color? (PalEntry)color : GPalette.BaseColors[color];
	gl_EnableTexture(false);
	gl_DisableShader();
	gl.Color3ub(p.r, p.g, p.b);
	gl.Begin(GL_LINES);
	gl.Vertex2i(x1, y1);
	gl.Vertex2i(x2, y2);
	gl.End();
	gl_EnableTexture(true);
}

//==========================================================================
//
//
//
//==========================================================================
void GL1Renderer::DrawPixel(int x1, int y1, int palcolor, uint32 color)
{
	PalEntry p = color? (PalEntry)color : GPalette.BaseColors[color];
	gl_EnableTexture(false);
	gl_DisableShader();
	gl.Color3ub(p.r, p.g, p.b);
	gl.Begin(GL_POINTS);
	gl.Vertex2i(x1, y1);
	gl.End();
	gl_EnableTexture(true);
}

//===========================================================================
// 
//
//
//===========================================================================

void GL1Renderer::Dim(PalEntry color, float damount, int x1, int y1, int w, int h)
{
	float r, g, b;
	
	gl_EnableTexture(false);
	gl_DisableShader();
	gl.BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	gl.AlphaFunc(GL_GREATER,0);
	
	r = color.r/255.0f;
	g = color.g/255.0f;
	b = color.b/255.0f;
	
	gl.Begin(GL_TRIANGLE_FAN);
	gl.Color4f(r, g, b, damount);
	gl.Vertex2i(x1, y1);
	gl.Vertex2i(x1, y1 + h);
	gl.Vertex2i(x1 + w, y1 + h);
	gl.Vertex2i(x1 + w, y1);
	gl.End();
	
	gl_EnableTexture(true);
}

//==========================================================================
//
//
//
//==========================================================================
void GL1Renderer::FlatFill (int left, int top, int right, int bottom, FTexture *src, bool local_origin)
{
	float fU1,fU2,fV1,fV2;

	FGLTexture *gltexture=FGLTexture::ValidateTexture(src);
	
	if (!gltexture) return;

	const WorldTextureInfo * wti = gltexture->Bind(CM_DEFAULT, 0, 0);
	if (!wti) return;
	
	if (!local_origin)
	{
		fU1=wti->GetU(left);
		fV1=wti->GetV(top);
		fU2=wti->GetU(right);
		fV2=wti->GetV(bottom);
	}
	else
	{		fU1=wti->GetU(0);
		fV1=wti->GetV(0);
		fU2=wti->GetU(right-left);
		fV2=wti->GetV(bottom-top);
	}
	gl_ApplyShader();
	gl.Begin(GL_TRIANGLE_STRIP);
	gl.TexCoord2f(fU1, fV1); gl.Vertex2f(left, top);
	gl.TexCoord2f(fU1, fV2); gl.Vertex2f(left, bottom);
	gl.TexCoord2f(fU2, fV1); gl.Vertex2f(right, top);
	gl.TexCoord2f(fU2, fV2); gl.Vertex2f(right, bottom);
	gl.End();
}

//==========================================================================
//
//
//
//==========================================================================
void GL1Renderer::Clear(int left, int top, int right, int bottom, int palcolor, uint32 color)
{
	int rt;
	int offY = 0;
	PalEntry p = palcolor==-1? (PalEntry)color : GPalette.BaseColors[palcolor];
	int width = right-left;
	int height= bottom-top;
	
	
	rt = screen->GetHeight() - top;
	
	int space = (static_cast<OpenGLFrameBuffer*>(screen)->GetTrueHeight()-screen->GetHeight())/2;	// ugh...
	rt += space;
	/*
	if (!m_windowed && (m_trueHeight != m_height))
	{
		offY = (m_trueHeight - m_height) / 2;
		rt += offY;
	}
	*/
	
	gl_DisableShader();

	gl.Enable(GL_SCISSOR_TEST);
	gl.Scissor(left, rt - height, width, height);
	
	gl.ClearColor(p.r/255.0f, p.g/255.0f, p.b/255.0f, 0.f);
	gl.Clear(GL_COLOR_BUFFER_BIT);
	gl.ClearColor(0.f, 0.f, 0.f, 0.f);
	
	gl.Disable(GL_SCISSOR_TEST);
}
