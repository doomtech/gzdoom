#ifndef __GL_MATERIAL_H
#define __GL_MATERIAL_H


namespace GLRenderer
{

struct FRect
{
	float left,top;
	float width,height;


	void Offset(float xofs,float yofs)
	{
		left+=xofs;
		top+=yofs;
	}
	void Scale(float xfac,float yfac)
	{
		left*=xfac;
		width*=xfac;
		top*=yfac;
		height*=yfac;
	}
};



class FMaterial
{
	struct ShaderParameter
	{
		int mParamType;
		FName mName;
		int mIndex;
		union
		{
			int mIntParam[4];
			float mFloatParam[4];
		};
	};

	TArray<FGLTexture *> mLayers;
	FShader *mShader;
	TArray<ShaderParameter> mParams;

	int mSizeTexels[2];
	float mSizeUnits[2];

	int mOffsetTexels[2];
	float mOffsetUnits[2];

	float mAlphaThreshold;

	float mDefaultScale[2];
	float mTempScale[2];

	TArray<FRect> mAreas;	// for optimizing mid texture drawing.
	bool mIsTransparent;

public:
	static FMaterial *GetMaterial(FTexture *tex, bool asSprite = false, int translation = 0);
	FMaterial(FTexture *tex, bool asSprite);
	~FMaterial();

	void SetTempScale(float scalex, float scaley);



	int GetWidth() const
	{
		return mSizeTexels[0];
	}

	int GetHeight() const
	{
		return mSizeTexels[1];
	}

	float GetScaledWidth() const
	{
		return mSizeUnits[0];
	}

	float GetScaledHeight() const
	{
		return mSizeUnits[1];
	}

	int GetLeftOffset() const
	{
		return mOffsetTexels[0];
	}

	int GetTopOffset() const
	{
		return mOffsetTexels[1];
	}

	float GetScaledLeftOffset() const
	{
		return mOffsetUnits[0];
	}

	float GetScaledTopOffset() const
	{
		return mOffsetUnits[1];
	}

	float RowOffset(float ofs) const;
	float TextureOffset(float ofs) const;

	bool FindHoles(const unsigned char * buffer, int w, int h);
	void CheckTransparent(const unsigned char * buffer, int w, int h);
	void CreateDefaultBrightmap();


};


}
#endif