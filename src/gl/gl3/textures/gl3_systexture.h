
#ifndef __GL3TEXTURE_H
#define __GL3TEXTURE_H

#include "gl/base/textures/hw_systexture.h"

class GL3SystemTexture : public HWSystemTexture
{
private:
	unsigned int mGlTexId;

public:
	GL3SystemTexture(bool mip) throw();
	~GL3SystemTexture();

	int GetId() { return mGlTexId; }
	void CreateTexture(unsigned texformat, unsigned char * buffer,unsigned bufferformat, int w, int h);
};

#endif
