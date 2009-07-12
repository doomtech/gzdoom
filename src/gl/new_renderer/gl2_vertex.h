#ifndef __GL2_VERTEX_H
#define __GL2_VERTEX_H

#include "tarray.h"

namespace GLRendererNew
{
class FMaterial;

struct FVertex3D
{
	float x,y,z;				// coordinates
	float u,v;					// texture coordinates
	unsigned char r,g,b,a;		// light color
	unsigned char fr,fg,fb,fd;	// fog color
	unsigned char tr,tg,tb,td;	// ceiling glow
	unsigned char br,bg,bb,bd;	// floor glow
	float lighton;
	float lightfogdensity;
	float lightfactor;
	float lightdist;
	float glowdisttop;
	float glowdistbottom;
};

struct FVertex2D
{
	float x,y;
	float u,v;
	unsigned char r,g,b,a;
};

struct FPrimitive2D
{
	int mPrimitiveType;
	int mTextureMode;
	int mSrcBlend;
	int mDstBlend;
	int mBlendEquation;
	FMaterial *mMaterial;
	bool mUseScissor;
	int mScissor[4];
	int mVertexStart;
};

class FVertexBuffer
{
	static unsigned int mLastBound;	// Crappy GL state machine. :(
protected:
	unsigned int mBufferId;
	unsigned int mMaxSize;

	FVertexBuffer(int size);
	void BindBuffer();
	void *Map();
	bool Unmap();
public:
	~FVertexBuffer();
	virtual bool Bind() = 0;
};

}


#endif