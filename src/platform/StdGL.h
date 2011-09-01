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
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif
#include <StdDDraw2.h>

class CStdWindow;

// one OpenGL context
class CStdGLCtx
{
public:
	CStdGLCtx();  // ctor
	~CStdGLCtx() { Clear(); }; // dtor

	void Clear();               // clear objects

#ifdef _WIN32
	bool Init(CStdWindow * pWindow, CStdApp *pApp, HWND hWindow = NULL);
	std::vector<int> EnumerateMultiSamples() const;
#else
	bool Init(CStdWindow * pWindow, CStdApp *pApp);
#endif

#ifdef USE_COCOA
	/*NSOpenGLContext*/void* GetNativeCtx();
#endif

	bool Select(bool verbose = false);              // select this context
	void Deselect();              // select this context

	bool PageFlip();            // present scene

protected:
	void SelectCommon();
	// this handles are declared as pointers to structs
	CStdWindow * pWindow; // window to draw in
#ifdef _WIN32
	HGLRC hrc;                  // rendering context
	HWND hWindow; // used if pWindow==NULL
	HDC hDC;                    // device context handle
	static bool InitGlew(HINSTANCE hInst);
#elif defined(USE_X11)
	/*GLXContext*/void * ctx;
#elif defined(USE_COCOA)
	/*NSOpenGLContext*/void* ctx;
#endif

	friend class CStdGL;
	friend class CSurface;
};

// OpenGL encapsulation
class CStdGL : public CStdDDraw
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
	void TaskOut(); // user taskswitched the app away
	void TaskIn();  // user tasked back
	virtual bool IsOpenGL() { return true; }
	virtual bool IsShaderific() { return shaders[0] != 0; }
	virtual bool OnResolutionChanged(unsigned int iXRes, unsigned int iYRes); // reinit clipper for new resolution
	// Clipper
	bool UpdateClipper(); // set current clipper to render target
	bool PrepareMaterial(StdMeshMaterial& mat);
	// Surface
	bool PrepareRendering(SURFACE sfcToSurface); // check if/make rendering possible to given surface
	virtual CStdGLCtx *CreateContext(CStdWindow * pWindow, CStdApp *pApp);
#ifdef _WIN32
	virtual CStdGLCtx *CreateContext(HWND hWindow, CStdApp *pApp);
#endif
	// Blit
	void SetupTextureEnv(bool fMod2, bool landscape);
	virtual void PerformBlt(CBltData &rBltData, CTexRef *pTex, DWORD dwModClr, bool fMod2, bool fExact);
	virtual void PerformMesh(StdMeshInstance &instance, float tx, float ty, float twdt, float thgt, DWORD dwPlayerColor, CBltTransform* pTransform);
	virtual void BlitLandscape(SURFACE sfcSource, float fx, float fy,
	                           SURFACE sfcTarget, float tx, float ty, float wdt, float hgt, const SURFACE textures[]);
	void FillBG(DWORD dwClr=0);
	// Drawing
	void DrawQuadDw(SURFACE sfcTarget, float *ipVtx, DWORD dwClr1, DWORD dwClr2, DWORD dwClr3, DWORD dwClr4);
	void PerformLine(SURFACE sfcTarget, float x1, float y1, float x2, float y2, DWORD dwClr);
	void PerformPix(SURFACE sfcDest, float tx, float ty, DWORD dwCol);
	// Gamma
	virtual bool ApplyGammaRamp(D3DGAMMARAMP &ramp, bool fForce);
	virtual bool SaveDefaultGammaRamp(CStdWindow * pWindow);
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
#ifdef USE_X11
	// Size of gamma ramps
	int gammasize;
#endif

	friend class CSurface;
	friend class CTexRef;
	friend class CPattern;
	friend class CStdGLCtx;
	friend class C4StartupOptionsDlg;
	friend class C4FullScreen;
	friend class CStdWindow;
};

// Global access pointer
extern CStdGL *pGL;

#endif // INC_StdGL
