
#ifndef __GL3SAMPLER_H
#define __GL3SAMPLER_H

#include "gl/base/textures/hw_sampler.h"

class GL3Sampler : public HWSampler
{
private:
	unsigned int glSamplerID;

public:
	GL3Sampler() throw();
	~GL3Sampler();

	int GetId() const { return glSamplerID; }
	void SetTextureFilter(int mode);
	void SetWrapMode(int coord, int wrapmode);
	void SetAnisotropy(float factor);
	void Apply();
};

#endif
