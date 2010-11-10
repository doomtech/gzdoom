/*
** gl_setup.cpp
** Initializes the data structures required by the GL renderer to handle
** a level
**
**---------------------------------------------------------------------------
** Copyright 2005 Christoph Oelckers
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

#include "doomtype.h"
#include "colormatcher.h"
#include "r_translate.h"
#include "i_system.h"
#include "p_local.h"
#include "p_lnspec.h"
#include "c_dispatch.h"
#include "r_sky.h"
#include "sc_man.h"
#include "w_wad.h"
#include "gi.h"
#include "g_level.h"
#include "a_sharedglobal.h"

#include "gl/renderer/gl_renderer.h"
#include "gl/data/gl_data.h"
#include "gl/data/gl_vertexbuffer.h"
#include "gl/scene/gl_clipper.h"
#include "gl/dynlights/gl_dynlight.h"
#include "gl/dynlights/gl_glow.h"
#include "gl/utility/gl_clock.h"
#include "gl/gl_functions.h"

TArray<FPortal> portals;

//==========================================================================
//
//
//
//==========================================================================

int FPortal::PointOnShapeLineSide(fixed_t x, fixed_t y, int shapeindex)
{
	int shapeindex2 = (shapeindex+1)%Shape.Size();

	return DMulScale32(y - Shape[shapeindex]->y, Shape[shapeindex2]->x - Shape[shapeindex]->x,
		Shape[shapeindex]->x - x, Shape[shapeindex2]->y - Shape[shapeindex]->y);
}

//==========================================================================
//
//
//
//==========================================================================

void FPortal::AddVertexToShape(vertex_t *vertex)
{
	for(unsigned i=0;i<Shape.Size(); i++)
	{
		if (vertex->x == Shape[i]->x && vertex->y == Shape[i]->y) return;
	}

	if (Shape.Size() < 2) 
	{
		Shape.Push(vertex);
	}
	else if (Shape.Size() == 2)
	{
		// Special case: We need to check if the vertex is on an extension of the line between the first two vertices.
		int pos = PointOnShapeLineSide(vertex->x, vertex->y, 0);

		if (pos == 0)
		{
			fixed_t distv1 = P_AproxDistance(vertex->x - Shape[0]->x, vertex->y - Shape[0]->y);
			fixed_t distv2 = P_AproxDistance(vertex->x - Shape[1]->x, vertex->y - Shape[1]->y);
			fixed_t distvv = P_AproxDistance(Shape[0]->x - Shape[1]->x, Shape[0]->y - Shape[1]->y);

			if (distv1 > distvv)
			{
				Shape[1] = vertex;
			}
			else if (distv2 > distvv)
			{
				Shape[0] = vertex;
			}
			return;
		}
		else if (pos > 0)
		{
			Shape.Insert(1, vertex);
		}
		else
		{
			Shape.Push(vertex);
		}
	}
	else
	{
		for(unsigned i=0; i<Shape.Size(); i++)
		{
			int startlinepos = PointOnShapeLineSide(vertex->x, vertex->y, i);
			if (startlinepos >= 0)
			{
				int previouslinepos = PointOnShapeLineSide(vertex->x, vertex->y, (i+Shape.Size()-1)%Shape.Size());

				if (previouslinepos < 0)	// we found the first line for which the vertex lies in front
				{
					unsigned int nextpoint = i;

					do
					{
						nextpoint = (nextpoint+1) % Shape.Size();
					}
					while (PointOnShapeLineSide(vertex->x, vertex->y, nextpoint) >= 0);

					int removecount = (nextpoint - i + Shape.Size()) % Shape.Size() - 1;

					if (removecount == 0)
					{
						if (startlinepos > 0) 
						{
							Shape.Insert(i+1, vertex);
						}
					}
					else if (nextpoint > i || nextpoint == 0)
					{
						// The easy case: It doesn't wrap around
						Shape.Delete(i+1, removecount-1);
						Shape[i+1] = vertex;
					}
					else
					{
						// It does wrap around.
						Shape.Delete(i+1, removecount);
						Shape.Delete(1, nextpoint-1);
						Shape[0] = vertex;
					}
					return;
				}
			}
		}
	}
}

//==========================================================================
//
//
//
//==========================================================================

void FPortal::AddSectorToPortal(sector_t *sector)
{
	for(int i=0; i<sector->linecount; i++)
	{
		AddVertexToShape(sector->lines[i]->v1);
		// This is necessary to handle unclosed sectors
		AddVertexToShape(sector->lines[i]->v2);
	}
}

//==========================================================================
//
//
//
//==========================================================================

void FPortal::UpdateClipAngles()
{
	for(unsigned int i=0; i<Shape.Size(); i++)
	{
		ClipAngles[i] = R_PointToPseudoAngle(viewx, viewy, Shape[i]->x, Shape[i]->y);
	}
}



//==========================================================================
//
//
//
//==========================================================================

struct FCoverageVertex
{
	fixed_t x, y;
};

struct FCoverageBuilder
{
	subsector_t *target;
	FPortal *portal;
	TArray<int> collect;
	FCoverageVertex center;

	//==========================================================================
	//
	//
	//
	//==========================================================================

	FCoverageBuilder(subsector_t *sub, FPortal *port)
	{
		target = sub;
		portal = port;
	}

	//==========================================================================
	//
	// GetIntersection
	//
	// adapted from P_InterceptVector
	//
	//==========================================================================

	bool GetIntersection(FCoverageVertex *v1, FCoverageVertex *v2, node_t *bsp, FCoverageVertex *v)
	{
		double frac;
		double num;
		double den;

		double v2x = (double)v1->x;
		double v2y = (double)v1->y;
		double v2dx = (double)(v2->x - v1->x);
		double v2dy = (double)(v2->y - v1->y);
		double v1x = (double)bsp->x;
		double v1y = (double)bsp->y;
		double v1dx = (double)bsp->dx;
		double v1dy = (double)bsp->dy;
			
		den = v1dy*v2dx - v1dx*v2dy;

		if (den == 0)
			return false;		// parallel
		
		num = (v1x - v2x)*v1dy + (v2y - v1y)*v1dx;
		frac = num / den;

		if (frac < 0. || frac > 1.) return false;

		v->x = xs_RoundToInt(v2x + frac * v2dx);
		v->y = xs_RoundToInt(v2y + frac * v2dy);
		return true;
	}

	//==========================================================================
	//
	//
	//
	//==========================================================================

	double PartitionDistance(FCoverageVertex *vt, node_t *node)
	{	
		return fabs(double(-node->dy) * (vt->x - node->x) + double(node->dx) * (vt->y - node->y)) / node->len;
	}

	//==========================================================================
	//
	//
	//
	//==========================================================================

	int PointOnSide(FCoverageVertex *vt, node_t *node)
	{	
		return R_PointOnSide(vt->x, vt->y, node);
	}

	//==========================================================================
	//
	//
	//
	//==========================================================================

	void CollectSubsector(subsector_t *sub, TArray<FCoverageVertex> &shape)
	{
#if 1
		collect.Push(int(sub-subsectors));
#else
		divline_t shapeline;
		divline_t subline;
		double cx, cy;
		fixed_t fcx, fcy;
		unsigned int matches;

		for(unsigned j = 0; j < shape.Size(); j++)
		{
			int jj = (j+1)%shape.Size();
			shapeline.x = shape[j].x;
			shapeline.y = shape[j].y;
			shapeline.dx = shape[jj].x - shape[j].x;
			shapeline.dy = shape[jj].y - shape[j].y;
			for(unsigned i = 0; i < sub->numlines; i++)
			{
				subline.x = sub->firstline[i].v1->x;
				subline.y = sub->firstline[i].v1->y;
				subline.dx = sub->firstline[i].v2->x - subline.x;
				subline.dy = sub->firstline[i].v2->y - subline.y;
			}
			fixed_t inter1 = P_InterceptVector(&subline, &shapeline);
			fixed_t inter2 = P_InterceptVector(&shapeline, &subline);
			if (inter1 > 0 && inter1 < FRACUNIT && inter2 > 0 && inter2 < FRACUNIT)
			{
				collect.Push(int(sub-subsectors));
				return;
			}
		}
		// If we get here the subsector's lines don't intersect with the shape.
		// There's 3 possibilities:
		// 1: The shape is inside the subsector (and therefore the shape's center is inside the subsector)
		cx = cy = 0;
		for(unsigned j = 0; j < shape.Size(); j++)
		{
			cx += shape[j].x;
			cy += shape[j].y;
		}
		fcx = xs_CRoundToInt(cx / shape.Size());
		fcy = xs_CRoundToInt(cy / shape.Size());

		matches = 0;
		for(unsigned i = 0; i < sub->numlines; i++)
		{
			subline.x = sub->firstline[i].v1->x;
			subline.y = sub->firstline[i].v1->y;
			subline.dx = sub->firstline[i].v2->x - subline.x;
			subline.dy = sub->firstline[i].v2->y - subline.y;
			matches += P_PointOnDivlineSide(fcx, fcy, &subline);
		}
		if (matches == sub->numlines)
		{
			collect.Push(int(sub-subsectors));
			return;
		}

		// 2: The subsector is inside the shape (and therefore the subsector's center is inside the shape)
		cx = cy = 0;
		for(unsigned i = 0; i < sub->numlines; i++)
		{
			cx += sub->firstline[i].v1->x;
			cy += sub->firstline[i].v1->y;
		}
		fcx = xs_CRoundToInt(cx / sub->numlines);
		fcy = xs_CRoundToInt(cy / sub->numlines);

		matches = 0;
		for(unsigned j = 0; j < shape.Size(); j++)
		{
			int jj = (j+1)%shape.Size();
			shapeline.x = shape[j].x;
			shapeline.y = shape[j].y;
			shapeline.dx = shape[jj].x - shape[j].x;
			shapeline.dy = shape[jj].y - shape[j].y;
			matches += P_PointOnDivlineSide(fcx, fcy, &shapeline);
		}
		if (matches == (int)shape.Size())
		{
			collect.Push(int(sub-subsectors));
			return;
		}

		// 3: Both are completely separate. No need to check because this isn't part of the coverage.
#endif
	}

	//==========================================================================
	//
	// adapted from polyobject splitter but uses a more precise epsilon
	//
	//==========================================================================

	void CollectNode(void *node, TArray<FCoverageVertex> &shape)
	{
		static TArray<FCoverageVertex> lists[2];
		static const double COVERAGE_EPSILON = 6./FRACUNIT;	// same epsilon as the node builder

		if (!((size_t)node & 1))  // Keep going until found a subsector
		{
			node_t *bsp = (node_t *)node;

			int centerside = R_PointOnSide(centerx, centery, bsp);

			lists[0].Clear();
			lists[1].Clear();
			for(unsigned i=0;i<shape.Size(); i++)
			{
				FCoverageVertex *v1 = &shape[i];
				FCoverageVertex *v2 = &shape[(i+1) % shape.Size()];

				double dist_v1 = PartitionDistance(v1, bsp);
				double dist_v2 = PartitionDistance(v2, bsp);

				if(dist_v1 <= COVERAGE_EPSILON)
				{
					if (dist_v2 <= COVERAGE_EPSILON)
					{
						lists[centerside].Push(*v1);
					}
					else
					{
						int side = PointOnSide(v2, bsp);
						lists[side].Push(*v1);
					}
				}
				else if (dist_v2 <= COVERAGE_EPSILON)
				{
					int side = PointOnSide(v1, bsp);
					lists[side].Push(*v1);
				}
				else 
				{
					int side1 = PointOnSide(v1, bsp);
					int side2 = PointOnSide(v2, bsp);

					if(side1 != side2)
					{
						// if the partition line crosses this seg, we must split it.

						FCoverageVertex vert;

						if (GetIntersection(v1, v2, bsp, &vert))
						{
							lists[side1].Push(*v1);
							lists[side1].Push(vert);
							lists[side2].Push(vert);
						}
						else
						{
							// should never happen
							lists[side1].Push(*v1);
						}
					}
					else 
					{
						// both points on the same side.
						lists[side1].Push(*v1);
					}
				}
			}
			if (lists[1].Size() == 0)
			{
				CollectNode(bsp->children[0], shape);
			}
			else if (lists[0].Size() == 0)
			{
				CollectNode(bsp->children[1], shape);
			}
			else
			{
				// copy the static arrays into local ones
				TArray<FCoverageVertex> locallist0 = lists[0];
				TArray<FCoverageVertex> locallist1 = lists[1];

				CollectNode(bsp->children[0], locallist0);
				CollectNode(bsp->children[1], locallist1);
			}
		}
		else
		{
			// we reached a subsector so we can link the node with this subsector
			subsector_t *sub = (subsector_t *)((BYTE *)node - 1);

			CollectSubsector(sub, shape);
		}
	}
};

//==========================================================================
//
// Calculate portal coverage for a single subsector
//
//==========================================================================

void gl_BuildPortalCoverage(FPortalCoverage *coverage, subsector_t *subsector, FPortal *portal)
{
	TArray<FCoverageVertex> shape;
	double centerx=0, centery=0;

	shape.Resize(subsector->numlines);
	for(unsigned i=0; i<subsector->numlines; i++)
	{
		centerx += (shape[i].x = subsector->firstline[i].v1->x + portal->xDisplacement);
		centery += (shape[i].y = subsector->firstline[i].v1->y + portal->yDisplacement);
	}

	FCoverageBuilder build(subsector, portal);
	build.center.x = xs_CRoundToInt(centerx / subsector->numlines);
	build.center.y = xs_CRoundToInt(centery / subsector->numlines);

	if (numnodes == 0)
	{
		build.CollectSubsector (subsectors, shape);
	}
	else
	{
		build.CollectNode(nodes + numnodes - 1, shape);
	}
	coverage->subsectors = new DWORD[build.collect.Size()]; 
	coverage->sscount = build.collect.Size();
	memcpy(coverage->subsectors, &build.collect[0], build.collect.Size() * sizeof(DWORD));
}

//==========================================================================
//
// portal initialization
//
//==========================================================================

void gl_InitPortals()
{
	glcycle_t tpt;
	tpt.Reset();
	tpt.Clock();

	TThinkerIterator<AStackPoint> it;
	AStackPoint *pt;

	portals.Clear();
	while ((pt = it.Next()))
	{
		FPortal *portal = NULL;
		int plane;
		pt->special1 = -1;
		for(int i=0;i<numsectors;i++)
		{
			if (sectors[i].linecount == 0)
			{
				continue;
			}
			else if (sectors[i].FloorSkyBox == pt)
			{
				plane = 1;
			}
			else if (sectors[i].CeilingSkyBox == pt)
			{
				plane = 2;
			}
			else continue;

			// we only process portals that actually are in use.
			if (portal == NULL) 
			{
				pt->special1 = portals.Size();	// Link portal thing to render data
				portal = &portals[portals.Reserve(1)];
				portal->origin = pt;
				portal->plane = 0;
				portal->xDisplacement = pt->x - pt->Mate->x;
				portal->yDisplacement = pt->y - pt->Mate->y;
			}
			portal->AddSectorToPortal(&sectors[i]);
			portal->plane|=plane;

			for (int j=0;j < sectors[i].subsectorcount; j++)
			{
				subsector_t *sub = sectors[i].subsectors[j];
				gl_BuildPortalCoverage(&sub->portalcoverage[plane-1], sub, portal);
			}
		}
		if (portal != NULL)
		{
			// if the first vertex is duplicated at the end it'll save time in a time critical function
			// because that code does not need to check for wraparounds anymore.
			portal->Shape.Resize(portal->Shape.Size()+1);
			portal->Shape[portal->Shape.Size()-1] = portal->Shape[0];
			portal->Shape.ShrinkToFit();
			portal->ClipAngles.Resize(portal->Shape.Size());
		}
	}
	tpt.Unclock();
	Printf("Portal init took %f ms\n", tpt.TimeMS());
}


CCMD(dumpportals)
{
	for(unsigned i=0;i<portals.Size(); i++)
	{
		double xdisp = portals[i].xDisplacement/65536.;
		double ydisp = portals[i].yDisplacement/65536.;
		Printf(PRINT_LOG, "Portal #%d, plane %d, stackpoint at (%f,%f), displacement = (%f,%f)\nShape:\n", i, portals[i].plane, portals[i].origin->x/65536., portals[i].origin->y/65536.,
			xdisp, ydisp);
		for (unsigned j=0;j<portals[i].Shape.Size(); j++)
		{
			Printf(PRINT_LOG, "\t(%f,%f)\n", portals[i].Shape[j]->x/65536. + xdisp, portals[i].Shape[j]->y/65536. + ydisp);
		}
		Printf(PRINT_LOG, "Coverage:\n");
		for(int j=0;j<numsubsectors;j++)
		{
			subsector_t *sub = &subsectors[j];
			ASkyViewpoint *pt = portals[i].plane == 1? sub->render_sector->FloorSkyBox : sub->render_sector->CeilingSkyBox;
			if (pt != NULL && pt->bAlways && pt->special1 == i)
			{
				Printf(PRINT_LOG, "\tSubsector %d (%d):\n\t\t", j, sub->render_sector->sectornum);
				for(unsigned k = 0;k< sub->numlines; k++)
				{
					Printf(PRINT_LOG, "(%.3f,%.3f), ",	sub->firstline[k].v1->x/65536. + xdisp, sub->firstline[k].v1->y/65536. + ydisp);
				}
				Printf(PRINT_LOG, "\n\t\tCovered by subsectors:\n");
				FPortalCoverage *cov = &sub->portalcoverage[portals[i].plane-1];
				for(int l = 0;l< cov->sscount; l++)
				{
					subsector_t *csub = &subsectors[cov->subsectors[l]];
					Printf(PRINT_LOG, "\t\t\t%5d (%4d): ", cov->subsectors[l], csub->render_sector->sectornum);
					for(unsigned m = 0;m< csub->numlines; m++)
					{
						Printf(PRINT_LOG, "(%.3f,%.3f), ",	csub->firstline[m].v1->x/65536., csub->firstline[m].v1->y/65536.);
					}
					Printf(PRINT_LOG, "\n");
				}
			}
		}
	}
}
