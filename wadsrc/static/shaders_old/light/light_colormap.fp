uniform vec4 colormapcolor;

vec4 lightpixel(vec4 texel)
{
	float gray = (texel.r * 0.3 + texel.g * 0.56 + texel.b * 0.14);
	if (colormapcolor.a != 0.0) gray = 1.0 - gray;
	
	return vec4(clamp(gray * colormapcolor.rgb, 0.0, 1.0), texel.a) * gl_Color;
}

