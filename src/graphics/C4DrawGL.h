/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2010-2013, The OpenClonk Team and contributors
 *
 * Distributed under the terms of the ISC license; see accompanying file
 * "COPYING" for details.
 *
 * "Clonk" is a registered trademark of Matthes Bender, used with permission.
 * See accompanying file "TRADEMARK" for details.
 *
 * To redistribute this file separately, substitute the full license texts
 * for the above references.
 */

/* OpenGL implementation of NewGfx */


#if !defined(INC_StdGL) && !defined(USE_CONSOLE)
#define INC_StdGL

#ifdef _WIN32
#include <C4windowswrapper.h>
#endif
#include <GL/glew.h>

#if defined(__APPLE__)
#ifdef USE_COCOA
#import "ObjectiveCAssociated.h"
#endif
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif
#include <C4Draw.h>

class C4Window;

class C4DrawGLError: public std::exception
{
public:
	C4DrawGLError(const StdStrBuf& buf): Buf(buf) {}
	virtual ~C4DrawGLError() throw() {}

	virtual const char* what() const throw() { return Buf.getData(); }

private:
	StdCopyStrBuf Buf;
};

// GLSL shaders
class C4DrawGLShader: public StdMeshMaterialShader
{
public:
	C4DrawGLShader(Type shader_type);
	virtual ~C4DrawGLShader();

	void Load(const char* code);

	virtual Type GetType() const;

	GLuint Shader;
};

class C4DrawGLProgram: public StdMeshMaterialProgram
{
public:
	C4DrawGLProgram(const C4DrawGLShader* fragment_shader, const C4DrawGLShader* vertex_shader, const C4DrawGLShader* geometry_shader);
	virtual ~C4DrawGLProgram();

	GLuint Program;
};

// one OpenGL context
class CStdGLCtx
#ifdef USE_COCOA
	: public ObjectiveCAssociated
#endif
{
public:
	CStdGLCtx();  // ctor
	~CStdGLCtx() { Clear(); }; // dtor

	void Clear();               // clear objects

#ifdef USE_WIN32_WINDOWS
	bool Init(C4Window * pWindow, C4AbstractApp *pApp, HWND hWindow = NULL);
	std::vector<int> EnumerateMultiSamples() const;
#else
	bool Init(C4Window * pWindow, C4AbstractApp *pApp);
#endif

	bool Select(bool verbose = false);              // select this context
	void Deselect();              // select this context

	bool PageFlip();            // present scene

protected:
	void SelectCommon();
	// this handles are declared as pointers to structs
	C4Window * pWindow; // window to draw in
#ifdef USE_WIN32_WINDOWS
	HGLRC hrc;                  // rendering context
	HWND hWindow; // used if pWindow==NULL
	HDC hDC;                    // device context handle
	static bool InitGlew(HINSTANCE hInst);
#elif defined(USE_X11)
	/*GLXContext*/void * ctx;
#endif

	friend class CStdGL;
	friend class C4Surface;
};

// OpenGL encapsulation
class CStdGL : public C4Draw
{
public:
	CStdGL();
	~CStdGL();
protected:

	int iPixelFormat;           // used pixel format

	GLenum sfcFmt;              // texture surface format
	CStdGLCtx * pMainCtx;         // main GL context
	CStdGLCtx *pCurrCtx;        // current context (owned if fullscreen)
	int iClrDpt;                // color depth
	// texture for smooth lines
	GLuint lines_tex;
	// programs for drawing points, lines, quads
	std::unique_ptr<C4DrawGLProgram> multi_blt_program;
public:
	// General
	void Clear();
	void Default();
	virtual bool IsOpenGL() { return true; }
	virtual bool IsShaderific() { return true; }
	virtual bool OnResolutionChanged(unsigned int iXRes, unsigned int iYRes); // reinit clipper for new resolution
	// Clipper
	bool UpdateClipper(); // set current clipper to render target
	std::unique_ptr<StdMeshMaterialShader> CompileShader(const char* language, StdMeshMaterialShader::Type type, const char* text);
	bool PrepareMaterial(StdMeshMatManager& mat_manager, StdMeshMaterial& mat);
	// Surface
	bool PrepareRendering(C4Surface * sfcToSurface); // check if/make rendering possible to given surface
	virtual CStdGLCtx *CreateContext(C4Window * pWindow, C4AbstractApp *pApp);
#ifdef USE_WIN32_WINDOWS
	virtual CStdGLCtx *CreateContext(HWND hWindow, C4AbstractApp *pApp);
	void TaskOut();
#endif
	// Blit
	void SetupMultiBlt(const C4BltTransform* pTransform, GLuint baseTex, GLuint overlayTex, DWORD dwOverlayModClr);
	void ResetMultiBlt(GLuint baseTex, GLuint overlayTex);
	virtual void PerformMesh(StdMeshInstance &instance, float tx, float ty, float twdt, float thgt, DWORD dwPlayerColor, C4BltTransform* pTransform);
	void FillBG(DWORD dwClr=0);
	// Drawing
	virtual void PerformMultiPix(C4Surface* sfcTarget, const C4BltVertex* vertices, unsigned int n_vertices);
	virtual void PerformMultiLines(C4Surface* sfcTarget, const C4BltVertex* vertices, unsigned int n_vertices, float width);
	virtual void PerformMultiTris(C4Surface* sfcTarget, const C4BltVertex* vertices, unsigned int n_vertices, const C4BltTransform* pTransform, C4TexRef* pTex, C4TexRef* pOverlay, DWORD dwOverlayClrMod);
	// device objects
	bool RestoreDeviceObjects();    // restore device dependent objects
	bool InvalidateDeviceObjects(); // free device dependent objects
	bool DeviceReady() { return !!pMainCtx; }
	bool EnsureAnyContext();

protected:
	bool CreatePrimarySurfaces(bool Editor, unsigned int iXRes, unsigned int iYRes, int iColorDepth, unsigned int iMonitor);

	bool CheckGLError(const char *szAtOp);
	virtual bool Error(const char *szMsg);

	friend class C4Surface;
	friend class C4TexRef;
	friend class C4Pattern;
	friend class CStdGLCtx;
	friend class C4StartupOptionsDlg;
	friend class C4FullScreen;
	friend class C4Window;
};

// Global access pointer
extern CStdGL *pGL;

#endif // INC_StdGL
