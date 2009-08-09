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
// Geometry data mainenance
//

#include "gl/gl_include.h"
#include "r_main.h"
#include "r_defs.h"
#include "r_state.h"
#include "v_palette.h"
#include "a_sharedglobal.h"
#include "gl/common/glc_geometric.h"
#include "gl/common/glc_convert.h"
#include "gl/new_renderer/gl2_renderer.h"
#include "gl/new_renderer/gl2_geom.h"
#include "gl/new_renderer/textures/gl2_material.h"

int GetFloorLight (const sector_t *sec);
int GetCeilingLight (const sector_t *sec);

namespace GLRendererNew
{

void MakeTextureMatrix(GLSectorPlane &splane, FMaterial *mat, Matrix3x4 &matx)
{
	matx.MakeIdentity();
	if (mat != NULL)
	{
		float uoffs=TO_GL(splane.xoffs) / mat->GetWidth();
		float voffs=TO_GL(splane.yoffs) / mat->GetHeight();

		float xscale1=TO_GL(splane.xscale);
		float yscale1=TO_GL(splane.yscale);

		float xscale2=64.f / mat->GetWidth();
		float yscale2=64.f / mat->GetHeight();

		float angle=-ANGLE_TO_FLOAT(splane.angle);

		matx.Scale(xscale1, yscale1, 1);
		matx.Translate(uoffs, voffs, 0);
		matx.Scale(xscale1, yscale2, 1);
		matx.Rotate(0, 0, 1, angle);
	}
}

void FGLSectorRenderData::CreatePlane(int in_area, sector_t *sec, GLSectorPlane &splane, 
									  int lightlevel, FDynamicColormap *cm, 
									  float alpha, bool additive, int whichplanes, bool opaque)
{
	TArray<FGLSectorPlaneRenderData> &Plane = mPlanes[in_area];
	FTexture *tex = TexMan[splane.texture];
	FMaterial *mat = GLRenderer2->GetMaterial(tex, false, 0);
	Matrix3x4 matx;

	FPrimitive3D BasicProps;
	FVertex3D BasicVertexProps;

	MakeTextureMatrix(splane, mat, matx);
	memset(&BasicProps, 0, sizeof(BasicProps));
	memset(&BasicVertexProps, 0, sizeof(BasicVertexProps));
	
	
	FRenderStyle style;
	if (additive) 
	{
		style = STYLE_Add;
		BasicProps.mTranslucent = true;
	}
	else if (alpha < 1.0f) 
	{
		style = STYLE_Translucent;
		BasicProps.mTranslucent = true;
	}
	else 
	{
		style = STYLE_Normal;
		BasicProps.mTranslucent = false;
	}

	if (tex != NULL && !tex->gl_info.mIsTransparent)
	{
		BasicProps.mAlphaThreshold = alpha * 0.5f;
	}
	else 
	{
		BasicProps.mAlphaThreshold = 0;
	}

	BasicProps.SetRenderStyle(style, opaque);
	BasicProps.mMaterial = mat;
	BasicProps.mPrimitiveType = GL_TRIANGLE_FAN;	// all subsectors are drawn as triangle fans
	BasicVertexProps.SetLighting(lightlevel, cm, 0/*rellight*/, additive, tex);
	BasicProps.mDesaturation = cm->Desaturate;

}

void FGLSectorRenderData::Init(int sector)
{
	mSector = &sectors[sector];
}

void FGLSectorRenderData::Invalidate()
{
	mPlanes[0].Clear();
	mPlanes[1].Clear();
	mPlanes[2].Clear();
}

void FGLSectorRenderData::Validate(int in_area)
{
	sector_t copy;
	sector_t *sec;

	extsector_t::xfloor &x = mSector->e->XFloor;
	int lightlevel;
	FDynamicColormap Colormap;
	bool stack;
	float alpha; 
	GLSectorPlane splane;

	if (mPlanes[in_area].Size() == 0)
	{
		TArray<FGLSectorPlaneRenderData> &plane = mPlanes[in_area];
		::in_area = area_t(in_area);
		sec = gl_FakeFlat(mSector, &copy, false);

		// create ceiling plane

		lightlevel = GetCeilingLight(sec);
		Colormap = *sec->ColorMap;
		stack = sec->CeilingSkyBox && sec->CeilingSkyBox->bAlways;
		alpha=stack ? sec->CeilingSkyBox->PlaneAlpha/65536.0f : 1.0f-sec->GetCeilingReflect();
		if (alpha != 0.0f)
		{
			if (x.ffloors.Size())
			{
				lightlist_t *light = P_GetPlaneLight(sec, &sec->ceilingplane, true);

				if(!(sec->GetFlags(sector_t::ceiling)&SECF_ABSLIGHTING)) lightlevel = *light->p_lightlevel;
				Colormap.Color = (*light->p_extra_colormap)->Color;
				Colormap.Desaturate = (*light->p_extra_colormap)->Desaturate;
			}
			splane.GetFromSector(sec, false);
			CreatePlane(in_area, sec, splane, lightlevel, &Colormap, alpha, false, 1, !stack);
		}

		// create floor plane
#if 0

			lightlevel = GetFloorLight(sec);
			Colormap=sec->ColorMap;
			stack = sec->FloorSkyBox && sec->FloorSkyBox->bAlways;
			alpha= stack ? sec->FloorSkyBox->PlaneAlpha/65536.0f : 1.0f-sec->floor_reflect;

			ceiling=false;
			renderflags=SSRF_RENDERFLOOR;

			if (x.ffloors.Size())
			{
				light = P_GetPlaneLight(sector, &sec->floorplane, false);
				if (!(sector->GetFlags(sector_t::floor)&SECF_ABSLIGHTING) || light!=&x.lightlist[0])	
					lightlevel = *light->p_lightlevel;

				Colormap.CopyLightColor(*light->p_extra_colormap);
			}
			renderstyle = STYLE_Translucent;
			if (alpha!=0.0f) Process(sec, false, false);


		// create 3d floor planes

			if (x.ffloors.Size())
			{
				player_t * player=players[consoleplayer].camera->player;

				// do the plane setup only once and just mark all subsectors that have to be processed
				gl_drawinfo->ss_renderflags[sub-subsectors]|=SSRF_RENDER3DPLANES;
				renderflags=SSRF_RENDER3DPLANES;
				if (!((*srf)&SSRF_RENDER3DPLANES))
				{
					(*srf) |= SSRF_RENDER3DPLANES;
					// 3d-floors must not overlap!
					fixed_t lastceilingheight=sector->CenterCeiling();	// render only in the range of the
					fixed_t lastfloorheight=sector->CenterFloor();		// current sector part (if applicable)
					F3DFloor * rover;	
					int k;
					
					// floors are ordered now top to bottom so scanning the list for the best match
					// is no longer necessary.

					ceiling=true;
					for(k=0;k<(int)x.ffloors.Size();k++)
					{
						rover=x.ffloors[k];
						
						if ((rover->flags&(FF_EXISTS|FF_RENDERPLANES))==(FF_EXISTS|FF_RENDERPLANES))
						{
							if (rover->flags&FF_FOG && gl_fixedcolormap) continue;
							if (rover->flags&(FF_INVERTPLANES|FF_BOTHPLANES))
							{
								fixed_t ff_top=rover->top.plane->ZatPoint(CenterSpot(sector));
								if (ff_top<lastceilingheight)
								{
									if (TO_GL(viewz) <= rover->top.plane->ZatPoint(TO_GL(viewx), TO_GL(viewy)))
									{
										// FF_FOG requires an inverted logic where to get the light from
										light=P_GetPlaneLight(sector, rover->top.plane,!!(rover->flags&FF_FOG));
										lightlevel=*light->p_lightlevel;
										
										if (rover->flags&FF_FOG) Colormap.LightColor = (*light->p_extra_colormap)->Fade;
										else Colormap.CopyLightColor(*light->p_extra_colormap);

										Colormap.FadeColor=sec->ColorMap->Fade;

										alpha=rover->alpha/255.0f;
										renderstyle = rover->flags&FF_ADDITIVETRANS? STYLE_Add : STYLE_Translucent;
										Process(rover->top.model, rover->top.isceiling, !!(rover->flags&FF_FOG));
									}
									lastceilingheight=ff_top;
								}
							}
							if (!(rover->flags&FF_INVERTPLANES))
							{
								fixed_t ff_bottom=rover->bottom.plane->ZatPoint(CenterSpot(sector));
								if (ff_bottom<lastceilingheight)
								{
									if (TO_GL(viewz)<=rover->bottom.plane->ZatPoint(TO_GL(viewx), TO_GL(viewy)))
									{
										light=P_GetPlaneLight(sector, rover->bottom.plane,!(rover->flags&FF_FOG));
										lightlevel=*light->p_lightlevel;

										if (rover->flags&FF_FOG) Colormap.LightColor = (*light->p_extra_colormap)->Fade;
										else Colormap.CopyLightColor(*light->p_extra_colormap);

										Colormap.FadeColor=sec->ColorMap->Fade;

										alpha=rover->alpha/255.0f;
										renderstyle = rover->flags&FF_ADDITIVETRANS? STYLE_Add : STYLE_Translucent;
										Process(rover->bottom.model, rover->bottom.isceiling, !!(rover->flags&FF_FOG));
									}
									lastceilingheight=ff_bottom;
									if (rover->alpha<255) lastceilingheight++;
								}
							}
						}
					}
						  
					ceiling=false;
					for(k=x.ffloors.Size()-1;k>=0;k--)
					{
						rover=x.ffloors[k];
						
						if ((rover->flags&(FF_EXISTS|FF_RENDERPLANES))==(FF_EXISTS|FF_RENDERPLANES))
						{
							if (rover->flags&FF_FOG && gl_fixedcolormap) continue;
							if (rover->flags&(FF_INVERTPLANES|FF_BOTHPLANES))
							{
								fixed_t ff_bottom=rover->bottom.plane->ZatPoint(CenterSpot(sector));
								if (ff_bottom>lastfloorheight || (rover->flags&FF_FIX))
								{
									if (TO_GL(viewz) >= rover->bottom.plane->ZatPoint(TO_GL(viewx), TO_GL(viewy)))
									{
										if (rover->flags&FF_FIX)
										{
											lightlevel = rover->model->lightlevel;
											Colormap = rover->model->ColorMap;
										}
										else
										{
											light=P_GetPlaneLight(sector, rover->bottom.plane,!(rover->flags&FF_FOG));
											lightlevel=*light->p_lightlevel;

											if (rover->flags&FF_FOG) Colormap.LightColor = (*light->p_extra_colormap)->Fade;
											else Colormap.CopyLightColor(*light->p_extra_colormap);

											Colormap.FadeColor=sec->ColorMap->Fade;
										}

										alpha=rover->alpha/255.0f;
										renderstyle = rover->flags&FF_ADDITIVETRANS? STYLE_Add : STYLE_Translucent;
										Process(rover->bottom.model, rover->bottom.isceiling, !!(rover->flags&FF_FOG));
									}
									lastfloorheight=ff_bottom;
								}
							}
							if (!(rover->flags&FF_INVERTPLANES))
							{
								fixed_t ff_top=rover->top.plane->ZatPoint(CenterSpot(sector));
								if (ff_top>lastfloorheight)
								{
									if (TO_GL(viewz) >= rover->top.plane->ZatPoint(TO_GL(viewx), TO_GL(viewy)))
									{
										light=P_GetPlaneLight(sector, rover->top.plane,!!(rover->flags&FF_FOG));
										lightlevel=*light->p_lightlevel;

										if (rover->flags&FF_FOG) Colormap.LightColor = (*light->p_extra_colormap)->Fade;
										else Colormap.CopyLightColor(*light->p_extra_colormap);

										Colormap.FadeColor=sec->ColorMap->Fade;

										alpha=rover->alpha/255.0f;
										renderstyle = rover->flags&FF_ADDITIVETRANS? STYLE_Add : STYLE_Translucent;
										Process(rover->top.model, rover->top.isceiling, !!(rover->flags&FF_FOG));
									}
									lastfloorheight=ff_top;
									if (rover->alpha<255) lastfloorheight--;
								}
							}
						}
					}
				}
			}
#endif
	}
}



}// namespace


