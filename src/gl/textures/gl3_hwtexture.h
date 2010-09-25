
#ifndef __GL3TEXTURE_H
#define __GL3TEXTURE_H

class FGL3HardwareTexture
{
public:
	enum
	{
		MAX_TEXTURE_UNITS = 32
	};

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

private:
	static unsigned int lastbound[MAX_TEXTURE_UNITS];

	unsigned int mGlTexId;
	bool mMipmapped;

public:
	FGL3HardwareTexture(bool mip);
	~FGL3HardwareTexture();

	bool Bind(int texunit);
	bool IsMipmapped() const { return mMipmapped; }
	void BindToCurrentFrameBuffer();

	static void Unbind(int texunit);
	static void UnbindAll();

	void CreateTexture(unsigned texformat, unsigned char * buffer,unsigned bufferformat, int w, int h);
};

#endif
