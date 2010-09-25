
#ifndef __GL3SAMPLER_H
#define __GL3SAMPLER_H

class FGL3Sampler
{
public:
	enum
	{
		MAX_TEXTURE_UNITS = 32
	};

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


private:
	static unsigned int lastbound[MAX_TEXTURE_UNITS];
	unsigned int glSamplerID;

public:
	FGL3Sampler();
	~FGL3Sampler();

	static void Unbind(int texunit);
	static void UnbindAll();

	bool Bind(int texunit);

	//void SetBorderColor(int r, int g, int b, int a);
	void SetTextureFilter(int mode);
	void SetWrapMode(int coord, int wrapmode);
	void SetAnisotropy(float factor);
};

#endif
