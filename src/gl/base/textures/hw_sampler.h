
#ifndef __HWSAMPLER_H
#define __HWSAMPLER_H

class HWSampler
{
public:
	enum
	{
		WRAPMODE_CLAMP,
		WRAPMODE_REPEAT,
		WRAPMODE_MIRRORED_REPEAT,
		WRAPMODE_MAX
	};

	enum
	{
		WRAPCOORD_U,
		WRAPCOORD_V,
		WRAPCOORD_W,
		WRAPCOORD_MAX
	};

	enum
	{
		TEXFILTER_NEAREST,
		TEXFILTER_NEAREST_MIPMAP_NEAREST,
		TEXFILTER_LINEAR,	
		TEXFILTER_LINEAR_MIPMAP_NEAREST,	
		TEXFILTER_LINEAR_MIPMAP_LINEAR,	
		TEXFILTER_NEAREST_MIPMAP_LINEAR,
		TEXFILTER_MAX
	};

	HWSampler() {}
	virtual ~HWSampler() {}
	virtual void SetTextureFilter(int mode) = 0;
	virtual void SetWrapMode(int coord, int wrapmode) = 0;
	virtual void SetAnisotropy(float factor) = 0;
	virtual void Apply() = 0;
};

#endif
