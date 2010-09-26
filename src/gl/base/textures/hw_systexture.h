
#ifndef __HW_SYSTEXTURE_H
#define __HW_SYSTEXTURE_H

class HWSystemTexture
{
public:
	enum
	{
		HWT_RGBA8 = 0,
		HWT_RGB5_A1,
		HWT_RGBA4,
		HWT_RGBA2,
		HWT_RGBA_COMPRESSED,
		HWT_S3TC_DXT1,
		HWT_S3TC_DXT3,
		HWT_S3TC_DXT5,

		HWT_ALPHA8,
		HWT_R8,
		HWT_RG8,
		HWT_R32F,
		HWT_RG32F,
		HWT_RGBA32F,

		HWT_BGRA,
		HWT_ABGR,

		HWT_MAX_TEX_FORMATS
	};

protected:
	bool mMipmapped;

public:
	HWSystemTexture(bool mip) { mMipmapped = mip; }
	virtual ~HWSystemTexture() {}
	bool IsMipmapped() const { return mMipmapped; }
	virtual void CreateTexture(unsigned texformat, unsigned char * buffer, unsigned bufferformat, int w, int h) = 0;
};

#endif
