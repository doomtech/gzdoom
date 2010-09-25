/*
** r_opengl.cpp
**
** OpenGL system interface
**
**---------------------------------------------------------------------------
** Copyright 2005 Tim Stump
** Copyright 2005 Christoph Oelckers
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
** 4. Full disclosure of the entire project's source code, except for third
**    party libraries is mandatory. (NOTE: This clause is non-negotiable!)
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/
#include "gl/system/gl_system.h"
#include "tarray.h"
#include "doomtype.h"
#include "m_argv.h"
#include "zstring.h"
#include "version.h"
#include "i_system.h"
#include "gl/system/gl_cvars.h"

#ifdef unix
#include <SDL.h>
#define wglGetProcAddress(x) (*SDL_GL_GetProcAddress)(x)
#endif
static void APIENTRY glBlendEquationDummy (GLenum mode);


#ifndef unix
PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB; // = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;
#endif

static TArray<FString>  m_Extensions;

#ifndef unix
HWND m_Window;
HDC m_hDC;
HGLRC m_hRC;
#endif

#define gl pgl

RenderContext * gl;

int occlusion_type=0;


//==========================================================================
//
// 
//
//==========================================================================

#ifndef unix
static HWND InitDummy()
{
	HMODULE g_hInst = GetModuleHandle(NULL);
	HWND dummy;
	//Create a rect structure for the size/position of the window
	RECT windowRect;
	windowRect.left = 0;
	windowRect.right = 64;
	windowRect.top = 0;
	windowRect.bottom = 64;

	//Window class structure
	WNDCLASS wc;

	//Fill in window class struct
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = (WNDPROC) DefWindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = g_hInst;
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "GZDoomOpenGLDummyWindow";

	//Register window class
	if(!RegisterClass(&wc))
	{
		return 0;
	}

	//Set window style & extended style
	DWORD style, exStyle;
	exStyle = WS_EX_CLIENTEDGE;
	style = WS_SYSMENU | WS_BORDER | WS_CAPTION;// | WS_VISIBLE;

	//Adjust the window size so that client area is the size requested
	AdjustWindowRectEx(&windowRect, style, false, exStyle);

	//Create Window
	if(!(dummy = CreateWindowEx(exStyle,
		"GZDoomOpenGLDummyWindow",
		"GZDOOM",
		WS_CLIPSIBLINGS | WS_CLIPCHILDREN | style,
		0, 0,
		windowRect.right-windowRect.left,
		windowRect.bottom-windowRect.top,
		NULL, NULL,
		g_hInst,
		NULL)))
	{
		UnregisterClass("GZDoomOpenGLDummyWindow", g_hInst);
		return 0;
	}
	ShowWindow(dummy, SW_HIDE);

	return dummy;
}

//==========================================================================
//
// 
//
//==========================================================================

static void ShutdownDummy(HWND dummy)
{
	DestroyWindow(dummy);
	UnregisterClass("GZDoomOpenGLDummyWindow", GetModuleHandle(NULL));
}


//==========================================================================
//
// 
//
//==========================================================================

static bool ReadInitExtensions()
{
	HDC hDC;
	HGLRC hRC;
	HWND dummy;

	PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR),
			1,
			PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
			PFD_TYPE_RGBA,
			32, // color depth
			0, 0, 0, 0, 0, 0,
			0,
			0,
			0,
			0, 0, 0, 0,
			16, // z depth
			0, // stencil buffer
			0,
			PFD_MAIN_PLANE,
			0,
			0, 0, 0
	};

	int pixelFormat;

	// we have to create a dummy window to init stuff from or the full init stuff fails
	dummy = InitDummy();

	hDC = GetDC(dummy);
	pixelFormat = ChoosePixelFormat(hDC, &pfd);
	DescribePixelFormat(hDC, pixelFormat, sizeof(pfd), &pfd);

	SetPixelFormat(hDC, pixelFormat, &pfd);

	hRC = wglCreateContext(hDC);
	wglMakeCurrent(hDC, hRC);

	wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
	wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
	// any extra stuff here?

	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(hRC);
	ReleaseDC(dummy, hDC);
	ShutdownDummy(dummy);

	return true;
}
#endif

//==========================================================================
//
// 
//
//==========================================================================
const char *wgl_extensions;

static void CollectExtensions()
{
	const char *supported = NULL;
	char *extensions, *extension;

	supported = (char *)glGetString(GL_EXTENSIONS);

	if (supported)
	{
		extensions = new char[strlen(supported) + 1];
		strcpy(extensions, supported);

		extension = strtok(extensions, " ");
		while(extension)
		{
			m_Extensions.Push(FString(extension));
			extension = strtok(NULL, " ");
		}

		delete [] extensions;
	}
}

//==========================================================================
//
// 
//
//==========================================================================

static bool CheckExtension(const char *ext)
{
	for (unsigned int i = 0; i < m_Extensions.Size(); ++i)
	{
		if (m_Extensions[i].CompareNoCase(ext) == 0) return true;
	}

	return false;
}


//==========================================================================
//
// This experimental build will require GL 3.3 plus GL_EXT_direct_state_access to run
//
//==========================================================================

static void APIENTRY LoadExtensions()
{
	CollectExtensions();

	const char *version = (const char*)glGetString(GL_VERSION);

	// Don't even start if it's lower than 3.3
	if (strcmp(version, "3.3") < 0) 
	{
		I_FatalError("Unsupported OpenGL version.\nAt least GL 3.3 is required to run "GAMENAME".\n");
	}
	if (!CheckExtension("GL_EXT_direct_state_access"))
	{
		// Both AMD and NVidia support this in their latest drivers so don't bother if it isn't there.
		I_FatalError("GL_EXT_direct_state_access extension not found.\n");
	}

	// This loads any function pointers and flags that require a vaild render context to
	// initialize properly

	gl->shadermodel = 4;	// assume no shader support
	gl->vendorstring=(char*)glGetString(GL_VENDOR);

	// First try the regular function
	gl->BlendEquation = (PFNGLBLENDEQUATIONPROC)wglGetProcAddress("glBlendEquation");

	if (CheckExtension("GL_ARB_texture_compression")) gl->flags|=RFL_TEXTURE_COMPRESSION;
	if (CheckExtension("GL_EXT_texture_compression_s3tc")) gl->flags|=RFL_TEXTURE_COMPRESSION_S3TC;
	if (strstr(gl->vendorstring, "NVIDIA")) gl->flags|=RFL_NVIDIA;
	else if (strstr(gl->vendorstring, "ATI Technologies")) gl->flags|=RFL_ATI;

#ifndef unix
	PFNWGLSWAPINTERVALEXTPROC vs = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
	if (vs) gl->SetVSync = vs;
#endif

	glGetIntegerv(GL_MAX_TEXTURE_SIZE,&gl->max_texturesize);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	
	gl->DeleteShader = (PFNGLDELETESHADERPROC)wglGetProcAddress("glDeleteShader");
	gl->DeleteProgram = (PFNGLDELETEPROGRAMPROC)wglGetProcAddress("glDeleteProgram");
	gl->DetachShader = (PFNGLDETACHSHADERPROC)wglGetProcAddress("glDetachShader");
	gl->CreateShader = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
	gl->ShaderSource = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
	gl->CompileShader = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
	gl->CreateProgram = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
	gl->AttachShader = (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
	gl->LinkProgram = (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
	gl->UseProgram = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
	gl->ValidateProgram = (PFNGLVALIDATEPROGRAMPROC)wglGetProcAddress("glValidateProgram");

	gl->VertexAttrib1f = (PFNGLVERTEXATTRIB1FPROC)wglGetProcAddress("glVertexAttrib1f");
	gl->VertexAttrib2f = (PFNGLVERTEXATTRIB2FPROC)wglGetProcAddress("glVertexAttrib2f");
	gl->VertexAttrib4f = (PFNGLVERTEXATTRIB4FPROC)wglGetProcAddress("glVertexAttrib4f");
	gl->VertexAttrib2fv = (PFNGLVERTEXATTRIB4FVPROC)wglGetProcAddress("glVertexAttrib2fv");
	gl->VertexAttrib3fv = (PFNGLVERTEXATTRIB4FVPROC)wglGetProcAddress("glVertexAttrib3fv");
	gl->VertexAttrib4fv = (PFNGLVERTEXATTRIB4FVPROC)wglGetProcAddress("glVertexAttrib4fv");
	gl->VertexAttrib4ubv = (PFNGLVERTEXATTRIB4UBVPROC)wglGetProcAddress("glVertexAttrib4ubv");
	gl->GetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC)wglGetProcAddress("glGetAttribLocation");
	gl->BindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC)wglGetProcAddress("glBindAttribLocation");

	gl->Uniform1f = (PFNGLUNIFORM1FPROC)wglGetProcAddress("glUniform1f");
	gl->Uniform2f = (PFNGLUNIFORM2FPROC)wglGetProcAddress("glUniform2f");
	gl->Uniform3f = (PFNGLUNIFORM3FPROC)wglGetProcAddress("glUniform3f");
	gl->Uniform4f = (PFNGLUNIFORM4FPROC)wglGetProcAddress("glUniform4f");
	gl->Uniform1i = (PFNGLUNIFORM1IPROC)wglGetProcAddress("glUniform1i");
	gl->Uniform2i = (PFNGLUNIFORM2IPROC)wglGetProcAddress("glUniform2i");
	gl->Uniform3i = (PFNGLUNIFORM3IPROC)wglGetProcAddress("glUniform3i");
	gl->Uniform4i = (PFNGLUNIFORM4IPROC)wglGetProcAddress("glUniform4i");
	gl->Uniform1fv = (PFNGLUNIFORM1FVPROC)wglGetProcAddress("glUniform1fv");
	gl->Uniform2fv = (PFNGLUNIFORM2FVPROC)wglGetProcAddress("glUniform2fv");
	gl->Uniform3fv = (PFNGLUNIFORM3FVPROC)wglGetProcAddress("glUniform3fv");
	gl->Uniform4fv = (PFNGLUNIFORM4FVPROC)wglGetProcAddress("glUniform4fv");
	gl->Uniform1iv = (PFNGLUNIFORM1IVPROC)wglGetProcAddress("glUniform1iv");
	gl->Uniform2iv = (PFNGLUNIFORM2IVPROC)wglGetProcAddress("glUniform2iv");
	gl->Uniform3iv = (PFNGLUNIFORM3IVPROC)wglGetProcAddress("glUniform3iv");
	gl->Uniform4iv = (PFNGLUNIFORM4IVPROC)wglGetProcAddress("glUniform4iv");

	gl->UniformMatrix2fv = (PFNGLUNIFORMMATRIX2FVPROC)wglGetProcAddress("glUniformMatrix2fv");
	gl->UniformMatrix3fv = (PFNGLUNIFORMMATRIX3FVPROC)wglGetProcAddress("glUniformMatrix3fv");
	gl->UniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)wglGetProcAddress("glUniformMatrix4fv");

	gl->GetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)wglGetProcAddress("glGetProgramInfoLog");
	gl->GetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");
	gl->GetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation");
	gl->GetActiveUniform = (PFNGLGETACTIVEUNIFORMPROC)wglGetProcAddress("glGetActiveUniform");
	gl->GetUniformfv = (PFNGLGETUNIFORMFVPROC)wglGetProcAddress("glGetUniformfv");
	gl->GetUniformiv = (PFNGLGETUNIFORMIVPROC)wglGetProcAddress("glGetUniformiv");
	gl->GetShaderSource = (PFNGLGETSHADERSOURCEPROC)wglGetProcAddress("glGetShaderSource");

	gl->EnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glEnableVertexAttribArray");
	gl->DisableVertexAttribArray= (PFNGLDISABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glDisableVertexAttribArray");
	gl->VertexAttribPointer		= (PFNGLVERTEXATTRIBPOINTERPROC)wglGetProcAddress("glVertexAttribPointer");

	// what'S the equivalent of this in GL 2.0???
	gl->GetObjectParameteriv = (PFNGLGETOBJECTPARAMETERIVARBPROC)wglGetProcAddress("glGetObjectParameterivARB");

    gl->GenQueries         = (PFNGLGENQUERIESPROC)wglGetProcAddress("glGenQueries");
    gl->DeleteQueries      = (PFNGLDELETEQUERIESPROC)wglGetProcAddress("glDeleteQueries");
    gl->GetQueryObjectuiv  = (PFNGLGETQUERYOBJECTUIVPROC)wglGetProcAddress("glGetQueryObjectuiv");
    gl->BeginQuery         = (PFNGLBEGINQUERYPROC)wglGetProcAddress("glBeginQuery");
    gl->EndQuery           = (PFNGLENDQUERYPROC)wglGetProcAddress("glEndQuery");

	gl->BindBuffer				= (PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer");
	gl->DeleteBuffers			= (PFNGLDELETEBUFFERSPROC)wglGetProcAddress("glDeleteBuffers");
	gl->GenBuffers				= (PFNGLGENBUFFERSPROC)wglGetProcAddress("glGenBuffers");
	gl->BufferData				= (PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferData");
	gl->BufferSubData			= (PFNGLBUFFERSUBDATAPROC)wglGetProcAddress("glBufferSubData");
	gl->MapBuffer				= (PFNGLMAPBUFFERPROC)wglGetProcAddress("glMapBuffer");
	gl->UnmapBuffer				= (PFNGLUNMAPBUFFERPROC)wglGetProcAddress("glUnmapBuffer");

	if (CheckExtension("GL_ARB_map_buffer_range")) 
	{
		gl->MapBufferRange			= (PFNGLMAPBUFFERRANGEPROC)wglGetProcAddress("glMapBufferRange");
		gl->FlushMappedBufferRange	= (PFNGLFLUSHMAPPEDBUFFERRANGEPROC)wglGetProcAddress("glFlushMappedBufferRange");
		gl->flags|=RFL_MAP_BUFFER_RANGE;
	}

	gl->GenFramebuffers			= (PFNGLGENFRAMEBUFFERSPROC)wglGetProcAddress("glGenFramebuffers");
	gl->DeleteFramebuffers		= (PFNGLDELETEFRAMEBUFFERSPROC)wglGetProcAddress("glDeleteFramebuffers");
	gl->BindFramebuffer			= (PFNGLBINDFRAMEBUFFERPROC)wglGetProcAddress("glBindFramebuffer");
	gl->FramebufferTexture2D	= (PFNGLFRAMEBUFFERTEXTURE2DPROC)wglGetProcAddress("glFramebufferTexture2D");
	gl->GenRenderbuffers		= (PFNGLGENRENDERBUFFERSPROC)wglGetProcAddress("glGenRenderbuffers");
	gl->DeleteRenderbuffers		= (PFNGLDELETERENDERBUFFERSPROC)wglGetProcAddress("glDeleteRenderbuffers");
	gl->BindRenderbuffer		= (PFNGLBINDRENDERBUFFERPROC)wglGetProcAddress("glBindRenderbuffer");
	gl->RenderbufferStorage		= (PFNGLRENDERBUFFERSTORAGEPROC)wglGetProcAddress("glRenderbufferStorage");
	gl->NamedRenderbufferStorage= (PFNGLNAMEDRENDERBUFFERSTORAGEEXTPROC)wglGetProcAddress("glNamedRenderbufferStorageEXT");
	gl->FramebufferRenderbuffer	= (PFNGLFRAMEBUFFERRENDERBUFFERPROC)wglGetProcAddress("glFramebufferRenderbuffer");

	gl->GenSamplers = (PFNGLGENSAMPLERSPROC)wglGetProcAddress("GenSamplers");
    gl->DeleteSamplers = (PFNGLDELETESAMPLERSPROC)wglGetProcAddress("DeleteSamplers");
    gl->BindSampler = (PFNGLBINDSAMPLERPROC)wglGetProcAddress("BindSampler");
    gl->SamplerParameteri = (PFNGLSAMPLERPARAMETERIPROC)wglGetProcAddress("SamplerParameteri");
    gl->SamplerParameterf = (PFNGLSAMPLERPARAMETERFPROC)wglGetProcAddress("SamplerParameterf");


	gl->BindMultiTexture		= (PFNGLBINDMULTITEXTUREEXTPROC)wglGetProcAddress("glBindMultiTextureEXT");
	gl->TextureImage2D			= (PFNGLTEXTUREIMAGE2DEXTPROC)wglGetProcAddress("glTextureImage2DEXT");
	gl->GenerateTextureMipmap	= (PFNGLGENERATETEXTUREMIPMAPEXTPROC)wglGetProcAddress("glGenerateTextureMipmapEXT");

#if 0
	if (CheckExtension("GL_ARB_texture_buffer_object") && 
		CheckExtension("GL_ARB_texture_float") && 
		CheckExtension("GL_EXT_GPU_Shader4") && 
		CheckExtension("GL_ARB_texture_rg") && 
		gl->shadermodel == 4)
	{
		gl->TexBufferARB = (PFNGLTEXBUFFERARBPROC)wglGetProcAddress("glTexBufferARB");
		gl->flags|=RFL_TEXTUREBUFFER;
	}
#endif

	gl->ActiveTexture = (PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTextureARB");
	gl->MultiTexCoord2f = (PFNGLMULTITEXCOORD2FPROC) wglGetProcAddress("glMultiTexCoord2fARB");
	gl->MultiTexCoord2fv = (PFNGLMULTITEXCOORD2FVPROC) wglGetProcAddress("glMultiTexCoord2fvARB");
}

//==========================================================================
//
// 
//
//==========================================================================

static void APIENTRY PrintStartupLog()
{
	Printf ("GL_VENDOR: %s\n", glGetString(GL_VENDOR));
	Printf ("GL_RENDERER: %s\n", glGetString(GL_RENDERER));
	Printf ("GL_VERSION: %s\n", glGetString(GL_VERSION));
	Printf ("GL_SHADING_LANGUAGE_VERSION: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
	Printf ("GL_EXTENSIONS: %s\n", glGetString(GL_EXTENSIONS));
	int v;

	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &v);
	Printf ("Max. texture units: %d\n", v);
	glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &v);
	Printf ("Max. fragment uniforms: %d\n", v);
	if (gl->shadermodel == 4) gl->maxuniforms = v;
	glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &v);
	Printf ("Max. vertex uniforms: %d\n", v);
	glGetIntegerv(GL_MAX_VARYING_FLOATS, &v);
	Printf ("Max. varying: %d\n", v);
	glGetIntegerv(GL_MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS, &v);
	Printf ("Max. combined uniforms: %d\n", v);
	glGetIntegerv(GL_MAX_COMBINED_UNIFORM_BLOCKS, &v);
	Printf ("Max. combined uniform blocks: %d\n", v);

}

//==========================================================================
//
// 
//
//==========================================================================

#ifndef unix
static bool SetupPixelFormat(HDC hDC, bool, bool, int multisample)
{
	int colorDepth;
	HDC deskDC;
	int attributes[26];
	int pixelFormat=-1;
	unsigned int numFormats;
	float attribsFloat[] = {0.0f, 0.0f};
	
	deskDC = GetDC(GetDesktopWindow());
	colorDepth = GetDeviceCaps(deskDC, BITSPIXEL);
	ReleaseDC(GetDesktopWindow(), deskDC);

	if (wglChoosePixelFormatARB)
	{
		attributes[0]	=	WGL_RED_BITS_ARB; //bits
		attributes[1]	=	8;
		attributes[2]	=	WGL_GREEN_BITS_ARB; //bits
		attributes[3]	=	8;
		attributes[4]	=	WGL_BLUE_BITS_ARB; //bits
		attributes[5]	=	8;
		attributes[6]	=	WGL_ALPHA_BITS_ARB;
		attributes[7]	=	8;
		attributes[8]	=	WGL_DEPTH_BITS_ARB;
		attributes[9]	=	24;
		attributes[10]	=	WGL_STENCIL_BITS_ARB;
		attributes[11]	=	8;
	
		attributes[12]	=	WGL_DRAW_TO_WINDOW_ARB;	//required to be true
		attributes[13]	=	true;
		attributes[14]	=	WGL_SUPPORT_OPENGL_ARB;
		attributes[15]	=	true;
		attributes[16]	=	WGL_DOUBLE_BUFFER_ARB;
		attributes[17]	=	true;
	
		attributes[18]	=	WGL_ACCELERATION_ARB;	//required to be FULL_ACCELERATION_ARB
		attributes[19]	=	WGL_FULL_ACCELERATION_ARB;
	
		if (multisample > 0)
		{
			attributes[20]	=	WGL_SAMPLE_BUFFERS_ARB;
			attributes[21]	=	true;
			attributes[22]	=	WGL_SAMPLES_ARB;
			attributes[23]	=	multisample;
		}
		else
		{
			attributes[20]	=	0;
			attributes[21]	=	0;
			attributes[22]	=	0;
			attributes[23]	=	0;
		}
	
		attributes[24]	=	0;
		attributes[25]	=	0;
	
		if (!wglChoosePixelFormatARB(hDC, attributes, attribsFloat, 1, &pixelFormat, &numFormats))
		{
			Printf("R_OPENGL: Couldn't choose pixel format. Falling back to software renderer\n");
			return false;
		}
	
		if (numFormats == 0)
		{
			Printf("R_OPENGL: No valid pixel formats found. Falling back to software renderer\n");
			return false;
		}
	}
	if (pixelFormat == -1 || !SetPixelFormat(hDC, pixelFormat, NULL))
	{
		Printf("R_OPENGL: Couldn't set pixel format. Falling back to software renderer\n");
		return false;
	}
	return true;
}
#else

static bool SetupPixelFormat(bool, bool, int multisample)
{
	SDL_GL_SetAttribute( SDL_GL_RED_SIZE,  8 );
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE,  8 );
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE,  8 );
	SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE,  8 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE,  24 );
	SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE,  8 );
//		SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER,  1 );
	if (multisample > 0) 
	{
		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, 1 );
		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, multisample );
	}
	return true;
}
#endif


//==========================================================================
//
// 
//
//==========================================================================

#ifndef unix
CVAR(Bool, gl_debug, false, 0)

static bool APIENTRY InitHardware (HWND Window, bool, bool, int multisample)
{
	m_Window=Window;
	m_hDC = GetDC(Window);

	if (!SetupPixelFormat(m_hDC, false, false, multisample))
	{
		Printf ("R_OPENGL: Reverting to software mode...\n");
		return false;
	}

	m_hRC = 0;
	if (wglCreateContextAttribsARB != NULL)
	{
		int ctxAttribs[] = {
			WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
			WGL_CONTEXT_MINOR_VERSION_ARB, 3,
			WGL_CONTEXT_FLAGS_ARB, gl_debug? WGL_CONTEXT_DEBUG_BIT_ARB : 0,
			WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
			0
		};

		m_hRC = wglCreateContextAttribsARB(m_hDC, 0, ctxAttribs);
	}

	if (m_hRC == NULL)
	{
		Printf ("R_OPENGL: Couldn't create render context. Reverting to software mode...\n");
		return false;
	}

	wglMakeCurrent(m_hDC, m_hRC);
	return true;
}

#else
static bool APIENTRY InitHardware (bool allowsoftware, bool nostencil, int multisample)
{
	if (!SetupPixelFormat(false, false, multisample))
	{
		Printf ("R_OPENGL: Reverting to software mode...\n");
		return false;
	}
	return true;
}
#endif


//==========================================================================
//
// 
//
//==========================================================================

#ifndef unix
static void APIENTRY Shutdown()
{
	if (m_hRC)
	{
		wglMakeCurrent(0, 0);
		wglDeleteContext(m_hRC);
	}
	if (m_hDC) ReleaseDC(m_Window, m_hDC);
}


static bool APIENTRY SetFullscreen(const char *devicename, int w, int h, int bits, int hz)
{
	DEVMODE dm;

	if (w==0)
	{
		ChangeDisplaySettingsEx(devicename, 0, 0, 0, 0);
	}
	else
	{
		dm.dmSize = sizeof(DEVMODE);
		dm.dmSpecVersion = DM_SPECVERSION;//Somebody owes me...
		dm.dmDriverExtra = 0;//...1 hour of my life back
		dm.dmPelsWidth = w;
		dm.dmPelsHeight = h;
		dm.dmBitsPerPel = bits;
		dm.dmDisplayFrequency = hz;
		dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL | DM_DISPLAYFREQUENCY;
		if (DISP_CHANGE_SUCCESSFUL != ChangeDisplaySettingsEx(devicename, &dm, 0, CDS_FULLSCREEN, 0))
		{
			dm.dmFields &= ~DM_DISPLAYFREQUENCY;
			return DISP_CHANGE_SUCCESSFUL == ChangeDisplaySettingsEx(devicename, &dm, 0, CDS_FULLSCREEN, 0);
		}
	}
	return true;
}
#endif
//==========================================================================
//
// 
//
//==========================================================================

static void APIENTRY iSwapBuffers()
{
#ifndef unix
	SwapBuffers(m_hDC);
#else
	SDL_GL_SwapBuffers ();
#endif
}

static BOOL APIENTRY SetVSync(int)
{
	// empty placeholder
	return false;
}

//==========================================================================
//
// 
//
//==========================================================================

static void APIENTRY SetTextureMode(int type)
{
	static float white[] = {1.f,1.f,1.f,1.f};

	if (gl_vid_compatibility)
	{
		type = TM_MODULATE;
	}
	if (type == TM_MASK)
	{
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PRIMARY_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);

		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE); 
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PRIMARY_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_TEXTURE0);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);
	}
	else if (type == TM_OPAQUE)
	{
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE0);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PRIMARY_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);

		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE); 
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PRIMARY_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
	}
	else if (type == TM_INVERT)
	{
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE0);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PRIMARY_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_ONE_MINUS_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);

		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE); 
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PRIMARY_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_TEXTURE0);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);
	}
	else if (type == TM_INVERTOPAQUE)
	{
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE0);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PRIMARY_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_ONE_MINUS_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);

		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE); 
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PRIMARY_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
	}
	else // if (type == TM_MODULATE)
	{
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	}
}

//==========================================================================
//
// 
//
//==========================================================================

/*
extern "C"
{

__declspec(dllexport) 
*/
void APIENTRY GetContext(RenderContext & gl)
{
	::gl=&gl;

	gl.flags=0;

	gl.LoadExtensions = LoadExtensions;
	gl.SetTextureMode = SetTextureMode;
	gl.PrintStartupLog = PrintStartupLog;
	gl.InitHardware = InitHardware;
#ifndef unix
	gl.Shutdown = Shutdown;
#endif
	gl.SwapBuffers = iSwapBuffers;
#ifndef unix
	gl.SetFullscreen = SetFullscreen;
#endif

	gl.Begin = glBegin;
	gl.End = glEnd;
	gl.DrawArrays = glDrawArrays;

	gl.TexCoord2f = glTexCoord2f;
	gl.TexCoord2fv = glTexCoord2fv;

	gl.Vertex2f = glVertex2f;
	gl.Vertex2i = glVertex2i;
	gl.Vertex3f = glVertex3f;
	gl.Vertex3fv = glVertex3fv;
	gl.Vertex3d = glVertex3d;

	gl.Color4f = glColor4f;
	gl.Color4fv = glColor4fv;
	gl.Color3f = glColor3f;
	gl.Color3ub = glColor3ub;
	gl.Color4ub = glColor4ub;

	gl.ColorMask = glColorMask;

	gl.DepthFunc = glDepthFunc;
	gl.DepthMask = glDepthMask;
	gl.DepthRange = glDepthRange;

	gl.StencilFunc = glStencilFunc;
	gl.StencilMask = glStencilMask;
	gl.StencilOp = glStencilOp;

	gl.MatrixMode = glMatrixMode;
	gl.PushMatrix = glPushMatrix;
	gl.PopMatrix = glPopMatrix;
	gl.LoadIdentity = glLoadIdentity;
	gl.MultMatrixd = glMultMatrixd;
	gl.Translatef = glTranslatef;
	gl.Ortho = glOrtho;
	gl.Scalef = glScalef;
	gl.Rotatef = glRotatef;

	gl.Viewport = glViewport;
	gl.Scissor = glScissor;

	gl.Clear = glClear;
	gl.ClearColor = glClearColor;
	gl.ClearDepth = glClearDepth;
	gl.ShadeModel = glShadeModel;
	gl.Hint = glHint;

	gl.DisableClientState = glDisableClientState;
	gl.EnableClientState = glEnableClientState;

	gl.Fogf = glFogf;
	gl.Fogi = glFogi;
	gl.Fogfv = glFogfv;

	gl.Enable = glEnable;
	gl.IsEnabled = glIsEnabled;
	gl.Disable = glDisable;

	gl.TexGeni = glTexGeni;
	gl.DeleteTextures = glDeleteTextures;
	gl.GenTextures = glGenTextures;
	gl.BindTexture = glBindTexture;
	gl.TexImage2D = glTexImage2D;
	gl.TexParameterf = glTexParameterf;
	gl.TexParameteri = glTexParameteri;
	gl.CopyTexSubImage2D = glCopyTexSubImage2D;

	gl.ReadPixels = glReadPixels;
	gl.PolygonOffset = glPolygonOffset;
	gl.ClipPlane = glClipPlane;

	gl.Finish = glFinish;
	gl.Flush = glFlush;

	gl.SetVSync = SetVSync;

#ifndef unix
	ReadInitExtensions();
	//GL is not yet inited in UNIX version, read them later in LoadExtensions
#endif

}



//} // extern "C"
