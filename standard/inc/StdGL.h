/*
 * OpenClonk, http://www.openclonk.org
 *
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

#include <GL/glew.h>

#ifdef USE_X11
//  Xmd.h typedefs BOOL to CARD8, but we want int
#define BOOL _BOOL
#include <X11/Xmd.h>
#undef BOOL
#include <GL/glx.h>
#endif

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

		void Clear();								// clear objects
#ifdef _WIN32
		bool Init(CStdWindow * pWindow, CStdApp *pApp, HWND hWindow=NULL);
#else
		bool Init(CStdWindow * pWindow, CStdApp *pApp);
#endif

		bool Select(bool verbose = false);							// select this context
		void Deselect();							// select this context
		bool UpdateSize();					// get new size from hWnd

		bool PageFlip();						// present scene

	protected:
		// this handles are declared as pointers to structs
		CStdWindow * pWindow; // window to draw in
#ifdef _WIN32
		HGLRC hrc;									// rendering context
		HWND hWindow; // used if pWindow==NULL
		HDC hDC;										// device context handle
#elif defined(USE_X11)
		GLXContext ctx;
#endif
		int cx,cy;									// context window size

	friend class CStdGL;
	friend class CSurface;
	};

// OpenGL encapsulation
class CStdGL : public CStdDDraw
	{
	public:
		CStdGL();
		~CStdGL();
		virtual bool PageFlip(RECT *pSrcRt=NULL, RECT *pDstRt=NULL, CStdWindow * pWindow = NULL);
	protected:

		int iPixelFormat;						// used pixel format

		GLenum sfcFmt;							// texture surface format
		CStdGLCtx MainCtx;					// main GL context
		CStdGLCtx *pCurrCtx;				// current context (owned if fullscreen)
		bool fFullscreen;						// fullscreen mode?
		int iClrDpt;								// color depth
		// shaders for the ARB extension
		GLuint shaders[13];
		// vertex buffer object
		GLuint vbo;
	public:
		// General
		void Clear();
		void Default();
		virtual int GetEngine() { return 1; }		// get indexed engine
		void TaskOut(); // user taskswitched the app away
		void TaskIn();  // user tasked back
		virtual bool IsOpenGL() { return true; }
		virtual bool IsShaderific() { return shaders[0] != 0; }
		virtual bool OnResolutionChanged(unsigned int iXRes, unsigned int iYRes); // reinit clipper for new resolution
		// Clipper
		bool UpdateClipper(); // set current clipper to render target
		// Surface
		bool PrepareRendering(SURFACE sfcToSurface); // check if/make rendering possible to given surface
		CStdGLCtx &GetMainCtx() { return MainCtx; }
		virtual CStdGLCtx *CreateContext(CStdWindow * pWindow, CStdApp *pApp);
#ifdef _WIN32
		virtual CStdGLCtx *CreateContext(HWND hWindow, CStdApp *pApp);
#endif
		// Blit
		void PerformBlt(CBltData &rBltData, CTexRef *pTex, DWORD dwModClr, bool fMod2, bool fExact);
		virtual void BlitLandscape(SURFACE sfcSource, SURFACE sfcSource2, SURFACE sfcLiquidAnimation, float fx, float fy,
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
		bool InitDeviceObjects();				// init device dependent objects
		bool RestoreDeviceObjects();		// restore device dependent objects
		bool InvalidateDeviceObjects();	// free device dependent objects
		bool DeleteDeviceObjects();			// free device dependent objects
		bool StoreStateBlock();
		void SetTexture();
		void ResetTexture();
		bool RestoreStateBlock();
#ifdef _WIN32
		bool DeviceReady() { return !!MainCtx.hrc; }
#elif defined(USE_X11)
		bool DeviceReady() { return !!MainCtx.ctx; }
#else
		bool DeviceReady() { return true; } // SDL
#endif

	protected:
		bool CreatePrimarySurfaces(BOOL Fullscreen, unsigned int iXRes, unsigned int iYRes, int iColorDepth, unsigned int iMonitor);
		BOOL CreateDirectDraw();

		bool CheckGLError(const char *szAtOp);
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
	};

// Global access pointer
extern CStdGL *pGL;

#endif // INC_StdGL
