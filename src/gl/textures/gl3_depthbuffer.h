
#ifndef __GL3DEPTHBUF_H
#define __GL3DEPTHBUF_H

class FGL3DepthBuffer
{
private:

	unsigned int glDepthID;

public:
	FGL3DepthBuffer(int w, int h);
	~FGL3DepthBuffer();

	void BindToCurrentFrameBuffer();
};

#endif
