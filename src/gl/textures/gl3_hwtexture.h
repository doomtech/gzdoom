
#ifndef __GL3TEXTURE_H
#define __GL3TEXTURE_H

class FGL3HardwareTexture
{
public:
	enum
	{
		MAX_TEXTURE_UNITS = 32
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

	void CreateTexture(unsigned char * buffer,int w, int h, int texformat);
};

#endif
