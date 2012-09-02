/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2002, 2006  Sven Eberhardt
 * Copyright (c) 2004-2007  Günther Brammer
 * Copyright (c) 2010  Martin Plicht
 * Copyright (c) 2010  Armin Burgmeier
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de
 *
 * Portions might be copyrighted by other authors who have contributed
 * to OpenClonk.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * See isc_license.txt for full license and disclaimer.
 *
 * "Clonk" is a registered trademark of Matthes Bender.
 * See clonk_trademark_license.txt for full license.
 */

/* OpenGL implementation of NewGfx */

#if !defined(INC_StdGL) && defined(USE_GL)
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
	// shaders for the ARB extension
	GLuint shaders[12];
	// vertex buffer object
	GLuint vbo;
	// texture for smooth lines
	GLuint lines_tex;
public:
	// General
	void Clear();
	void Default();
	virtual int GetEngine() { return 1; }   // get indexed engine
	virtual bool IsOpenGL() { return true; }
	virtual bool IsShaderific() { return shaders[0] != 0; }
	virtual bool OnResolutionChanged(unsigned int iXRes, unsigned int iYRes); // reinit clipper for new resolution
	// Clipper
	bool UpdateClipper(); // set current clipper to render target
	bool PrepareMaterial(StdMeshMaterial& mat);
	// Surface
	bool PrepareRendering(C4Surface * sfcToSurface); // check if/make rendering possible to given surface
	virtual CStdGLCtx *CreateContext(C4Window * pWindow, C4AbstractApp *pApp);
#ifdef USE_WIN32_WINDOWS
	virtual CStdGLCtx *CreateContext(HWND hWindow, C4AbstractApp *pApp);
	void TaskOut();
#endif
	// Blit
	void SetupTextureEnv(bool fMod2, bool landscape);
	virtual void PerformBlt(C4BltData &rBltData, C4TexRef *pTex, DWORD dwModClr, bool fMod2, bool fExact);
	virtual void PerformMesh(StdMeshInstance &instance, float tx, float ty, float twdt, float thgt, DWORD dwPlayerColor, C4BltTransform* pTransform);
	virtual void BlitLandscape(C4Surface * sfcSource, float fx, float fy,
	                           C4Surface * sfcTarget, float tx, float ty, float wdt, float hgt, const C4Surface * textures[]);
	void FillBG(DWORD dwClr=0);
	// Drawing
	void DrawQuadDw(C4Surface * sfcTarget, float *ipVtx, DWORD dwClr1, DWORD dwClr2, DWORD dwClr3, DWORD dwClr4);
	void PerformLine(C4Surface * sfcTarget, float x1, float y1, float x2, float y2, DWORD dwClr, float width);
	void PerformPix(C4Surface * sfcDest, float tx, float ty, DWORD dwCol);
	// device objects
	bool RestoreDeviceObjects();    // restore device dependent objects
	bool InvalidateDeviceObjects(); // free device dependent objects
	void SetTexture();
	void ResetTexture();
	bool DeviceReady() { return !!pMainCtx; }

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
