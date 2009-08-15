/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2002-2005  Sven Eberhardt
 * Copyright (c) 2004-2008  Günther Brammer
 * Copyright (c) 2005  Peter Wortmann
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

/* A wrapper class to DirectDraw */

#ifndef INC_STDDDRAW2
#define INC_STDDDRAW2

#include <StdSurface2.h>
#include <StdSurface8.h>
#include <StdFont.h>
#include <StdBuf.h>

// texref-predef
class CStdDDraw;
class CTexRef;
class CSurface;
struct CStdPalette;
class CStdGLCtx;
class CStdApp;
class CStdWindow;

// engines
#define GFXENGN_DIRECTX  0
#define GFXENGN_OPENGL   1
#define GFXENGN_DIRECTXS 2
#define GFXENGN_NOGFX		 3

// Global DDraw access pointer
extern CStdDDraw *lpDDraw;
extern CStdPalette *lpDDrawPal;

extern int iGfxEngine;

/*const DWORD CColors [] =
	{
	0x000000, // CBlack
	0x2A2A2A, // CGray1
	0x545454, // CGray2
	0x7F7F7F, // CGray3
	0xA9A9A9, // CGray4
	0xD3D3D3, // CGray5
	0xFFFFFF, // CWrite
	0x7F0000,	// CDRed
	0x007F00,	// CDGreen
	0x00007F,	// CDBlue
	0xFF0000, // CRed
	0x00FF00, // CGreen
	0x7F7FFF, // CLBlue
	0xFFFF00, // CYellow
	0x0000FF, // CBlue
	};*/

// font def color indices

// rotation info class
class CBltTransform
	{
	public:
		float mat[9]; // transformation matrix
	public:
		CBltTransform() {} // default: don't init fields
		void Set(float fA, float fB, float fC, float fD, float fE, float fF, float fG, float fH, float fI)
			{ mat[0]=fA; mat[1]=fB; mat[2]=fC; mat[3]=fD; mat[4]=fE; mat[5]=fF; mat[6]=fG; mat[7]=fH; mat[8]=fI; }
		void SetRotate(int iAngle, float fOffX, float fOffY); // set by angle and rotation offset
		bool SetAsInv(CBltTransform &rOfTransform);
		void Rotate(int iAngle, float fOffX, float fOffY) // rotate by angle around rotation offset
			{
			// multiply matrix as seen in SetRotate by own matrix
			CBltTransform rot; rot.SetRotate(iAngle, fOffX, fOffY);
			(*this) *= rot;
			}
		void SetMoveScale(float dx, float dy, float sx, float sy)
			{
			mat[0] = sx; mat[1] = 0;  mat[2] = dx;
			mat[3] = 0;  mat[4] = sy; mat[5] = dy;
			mat[6] = 0;  mat[7] = 0;  mat[8] = 1;
			}
		void MoveScale(float dx, float dy, float sx, float sy)
			{
			// multiply matrix by movescale matrix
			CBltTransform move; move.SetMoveScale(dx,dy,sx,sy);
			(*this) *= move;
			}
		void ScaleAt(float sx, float sy, float tx, float ty)
			{
			// scale and move back so tx and ty remain fixpoints
			MoveScale(-tx*(sx-1), -ty*(sy-1), sx, sy);
			}
		CBltTransform &operator *= (CBltTransform &r)
			{
			// transform transformation
			Set(mat[0]*r.mat[0] + mat[3]*r.mat[1] + mat[6]*r.mat[2],
				mat[1]*r.mat[0] + mat[4]*r.mat[1] + mat[7]*r.mat[2],
				mat[2]*r.mat[0] + mat[5]*r.mat[1] + mat[8]*r.mat[2],
				mat[0]*r.mat[3] + mat[3]*r.mat[4] + mat[6]*r.mat[5],
				mat[1]*r.mat[3] + mat[4]*r.mat[4] + mat[7]*r.mat[5],
				mat[2]*r.mat[3] + mat[5]*r.mat[4] + mat[8]*r.mat[5],
				mat[0]*r.mat[6] + mat[3]*r.mat[7] + mat[6]*r.mat[8],
				mat[1]*r.mat[6] + mat[4]*r.mat[7] + mat[7]*r.mat[8],
				mat[2]*r.mat[6] + mat[5]*r.mat[7] + mat[8]*r.mat[8]);
			return *this;
			}
		void TransformPoint(float &rX, float &rY); // rotate point by angle
	};

// pattern
class CPattern
	{
	private:
		// pattern surface for new-style patterns
		class CSurface *sfcPattern32;
		// Faster access
		uint32_t * CachedPattern; int Wdt; int Hgt;
		// pattern zoom factor; 0 means no zoom
		int Zoom;
	public:
		CPattern& operator=(const CPattern&);
		const CSurface *getSurface() const { return sfcPattern32; }
		DWORD PatternClr(unsigned int iX, unsigned int iY) const;	// apply pattern to color
		bool Set(class CSurface *sfcSource, int iZoom=0);	// set and enable pattern
		void SetZoom(int iZoom) { Zoom = iZoom; }
		void Clear();											// clear pattern
		CPattern();					// ctor
		~CPattern() { Clear(); }					// dtor
	};

// blit position on screen
// This is the format required by GL_T2F_C4UB_V3F
struct CBltVertex
	{
#ifdef USE_GL
	GLfloat tx, ty; // texture positions
	GLubyte color[4]; // color modulation
#endif
	float ftx,fty,ftz; // blit positions
	};

// blit bounds polygon - note that blitting procedures are not designed for inner angles (>pi)
struct CBltData
	{
	BYTE byNumVertices;  // number of valid vertices
	CBltVertex vtVtx[8]; // vertices for polygon - up to eight vertices may be needed
	CBltTransform TexPos; // texture mapping matrix
	CBltTransform *pTransform; // Vertex transformation

	// clip poly, so that for any point (x,y) is: (fX*x + fY*y <= fMax)
	// assumes a valid poly!
	bool ClipBy(float fX, float fY, float fMax);
	};


// This structure is used by StdGL, too
#ifndef USE_DIRECTX
typedef struct _D3DGAMMARAMP
{
    WORD                red  [256];
    WORD                green[256];
    WORD                blue [256];
} D3DGAMMARAMP;
#endif

// gamma ramp control
class CGammaControl
	{
	private:
		void SetClrChannel(WORD *pBuf, BYTE c1, BYTE c2, int c3); // set color channel ramp

	protected:
		D3DGAMMARAMP ramp;

	public:
		CGammaControl() { Default(); } // ctor
		void Default() { Set(0x000000, 0x808080, 0xffffff); } // set default ramp

		void Set(DWORD dwClr1, DWORD dwClr2, DWORD dwClr3); // set color ramp

		DWORD ApplyTo(DWORD dwClr);		// apply gamma to color value

	friend class CStdDDraw;
	friend class CStdD3D;
	friend class CStdGL;
	};

// helper struct
struct ZoomData
	{
	float Zoom;
	int X, Y;
	};

// direct draw encapsulation
class CStdDDraw
	{
	public:
		CStdDDraw(): Saturation(255), MaxTexSize(0) { lpDDrawPal=&Pal; }
		virtual ~CStdDDraw() { lpDDraw=NULL; }
	public:
		CStdApp * pApp; // the application
		SURFACE lpPrimary;	// primary and back surface (emulation...)
		SURFACE lpBack;
		CStdPalette Pal;		// 8bit-pal
		bool Active;										// set if device is ready to render, etc.
		CGammaControl Gamma;						// gamma
		CGammaControl DefRamp;						// default gamma ramp
		int MaxTexSize;
	protected:
		BYTE								byByteCnt;		// bytes per pixel (2 or 4)
		bool fFullscreen;
		float fClipX1,fClipY1,fClipX2,fClipY2; // clipper in unzoomed coordinates
		float fStClipX1,fStClipY1,fStClipX2,fStClipY2; // stored clipper in unzoomed coordinates
		int32_t iClipX1,iClipY1,iClipX2,iClipY2; // clipper in pixel coordinates
		bool ClipAll;	// set if clipper clips everything away
		bool PrimaryLocked;							// set if primary surface is locked (-> can't blit)
		CSurface *RenderTarget;					// current render target
		bool BlitModulated;							// set if blits should be modulated with BlitModulateClr
		DWORD BlitModulateClr;					// modulation color for blitting
		DWORD dwBlitMode;               // extra flags for blit
		CClrModAddMap *pClrModMap;      // map to be used for global color modulation (invalid if !fUseClrModMap)
		bool fUseClrModMap;             // if set, pClrModMap will be checked for color modulations
		unsigned char Saturation;		// if < 255, an extra filter is used to reduce the saturation
		int ZoomX; int ZoomY;
	public:
		float Zoom;
		// General
		bool Init(CStdApp * pApp, bool Fullscreen, bool fUsePageLock, unsigned int iXRes, unsigned int iYRes, int iBitDepth, unsigned int iMonitor);
		virtual void Clear();
		virtual void Default();
		virtual CStdGLCtx *CreateContext(CStdWindow *, CStdApp *) { return NULL; }
#ifdef _WIN32
		virtual CStdGLCtx *CreateContext(HWND, CStdApp *) { return NULL; }
#endif
		virtual bool PageFlip(RECT *pSrcRt=NULL, RECT *pDstRt=NULL, CStdWindow * pWindow = NULL) = 0;
		virtual int GetEngine() = 0;		// get indexed engine
		virtual void TaskOut() = 0; // user taskswitched the app away
		virtual void TaskIn() = 0;  // user tasked back
		virtual bool OnResolutionChanged(unsigned int iXRes, unsigned int iYRes) = 0; // reinit clipper for new resolution
		virtual bool IsOpenGL() { return false; }
		virtual bool IsShaderific() { return false; }
		// Palette
		bool SetPrimaryPalette(BYTE *pBuf, BYTE *pAlphaBuf=NULL);
		bool SetPrimaryPaletteQuad(BYTE *pBuf);
		bool AttachPrimaryPalette(SURFACE sfcSurface);
		// Clipper
		bool GetPrimaryClipper(float &rX1, float &rY1, float &rX2, float &rY2);
		bool SetPrimaryClipper(float iX1, float iY1, float iX2, float iY2);
		bool SubPrimaryClipper(float iX1, float iY1, float iX2, float iY2);
		bool StorePrimaryClipper();
		bool RestorePrimaryClipper();
		bool NoPrimaryClipper();
		bool ApplyPrimaryClipper(SURFACE sfcSurface);
		bool DetachPrimaryClipper(SURFACE sfcSurface);
		virtual bool UpdateClipper() = 0; // set current clipper to render target
		bool ClipPoly(CBltData &rBltData); // clip polygon to clipper; return whether completely clipped out
		// Surface
		bool GetSurfaceSize(SURFACE sfcSurface, int &iWdt, int &iHgt);
		bool WipeSurface(SURFACE sfcSurface);
		void SurfaceAllowColor(SURFACE sfcSfc, DWORD *pdwColors, int iNumColors, bool fAllowZero=false);
		void Grayscale(SURFACE sfcSfc, int32_t iOffset = 0);
		void LockingPrimary() { PrimaryLocked=true; }
		void PrimaryUnlocked() { PrimaryLocked=false; }
		virtual bool PrepareRendering(SURFACE sfcToSurface) = 0; // check if/make rendering possible to given surface
		// Blit
		virtual void BlitLandscape(SURFACE sfcSource, float fx, float fy,
		                           SURFACE sfcTarget, float tx, float ty, float wdt, float hgt, const SURFACE textures[]);
		void Blit8Fast(CSurface8 * sfcSource, int fx, int fy,
		               SURFACE sfcTarget, int tx, int ty, int wdt, int hgt);
		bool Blit(SURFACE sfcSource, float fx, float fy, float fwdt, float fhgt,
							SURFACE sfcTarget, float tx, float ty, float twdt, float thgt,
							bool fSrcColKey=false, CBltTransform *pTransform=NULL);
		virtual void PerformBlt(CBltData &rBltData, CTexRef *pTex, DWORD dwModClr, bool fMod2, bool fExact) = 0;
		bool Blit8(SURFACE sfcSource, int fx, int fy, int fwdt, int fhgt, // force 8bit-blit (inline)
							SURFACE sfcTarget, int tx, int ty, int twdt, int thgt,
							bool fSrcColKey=false, CBltTransform *pTransform=NULL);
		bool BlitRotate(SURFACE sfcSource, int fx, int fy, int fwdt, int fhgt,
										SURFACE sfcTarget, int tx, int ty, int twdt, int thgt,
										int iAngle, bool fTransparency=true);
		bool BlitSurface(SURFACE sfcSurface, SURFACE sfcTarget, int tx, int ty, bool fBlitBase);
		bool BlitSurfaceTile(SURFACE sfcSurface, SURFACE sfcTarget, int iToX, int iToY, int iToWdt, int iToHgt, int iOffsetX=0, int iOffsetY=0, bool fSrcColKey=false);
		bool BlitSurfaceTile2(SURFACE sfcSurface, SURFACE sfcTarget, int iToX, int iToY, int iToWdt, int iToHgt, int iOffsetX=0, int iOffsetY=0, bool fSrcColKey=false);
		virtual void FillBG(DWORD dwClr=0) = 0;
		// Text
		enum { DEFAULT_MESSAGE_COLOR = 0xffffffff };
		bool TextOut(const char *szText, CStdFont &rFont, float fZoom, SURFACE sfcDest, float iTx, float iTy, DWORD dwFCol=0xffffffff, BYTE byForm=ALeft, bool fDoMarkup=true);
		bool StringOut(const char *szText, CStdFont &rFont, float fZoom, SURFACE sfcDest, float iTx, float iTy, DWORD dwFCol=0xffffffff, BYTE byForm=ALeft, bool fDoMarkup=true);
		// Drawing
		virtual void DrawPix(SURFACE sfcDest, float tx, float ty, DWORD dwCol);
		void DrawBox(SURFACE sfcDest, int iX1, int iY1, int iX2, int iY2, BYTE byCol);  // calls DrawBoxDw
		void DrawBoxDw(SURFACE sfcDest, int iX1, int iY1, int iX2, int iY2, DWORD dwClr); // calls DrawBoxFade
		void DrawBoxFade(SURFACE sfcDest, float iX, float iY, float iWdt, float iHgt, DWORD dwClr1, DWORD dwClr2, DWORD dwClr3, DWORD dwClr4, int iBoxOffX, int iBoxOffY); // calls DrawQuadDw
		void DrawPatternedCircle(SURFACE sfcDest, int x, int y, int r, BYTE col, CPattern & Pattern, CStdPalette &rPal);
		void DrawHorizontalLine(SURFACE sfcDest, int x1, int x2, int y, BYTE col);
		void DrawVerticalLine(SURFACE sfcDest, int x, int y1, int y2, BYTE col);
		void DrawFrame(SURFACE sfcDest, int x1, int y1, int x2, int y2, BYTE col);
		void DrawFrameDw(SURFACE sfcDest, int x1, int y1, int x2, int y2, DWORD dwClr);
		virtual void DrawLine(SURFACE sfcTarget, int x1, int y1, int x2, int y2, BYTE byCol)
			{ DrawLineDw(sfcTarget, (float) x1, (float) y1, (float) x2, (float) y2, Pal.GetClr(byCol)); }
		virtual void DrawLineDw(SURFACE sfcTarget, float x1, float y1, float x2, float y2, DWORD dwClr);
		virtual void DrawQuadDw(SURFACE sfcTarget, float *ipVtx, DWORD dwClr1, DWORD dwClr2, DWORD dwClr3, DWORD dwClr4) = 0;
		// gamma
		void SetGamma(DWORD dwClr1, DWORD dwClr2, DWORD dwClr3);	// set gamma ramp
		void DisableGamma();																			// reset gamma ramp to default
		void EnableGamma();																				// set current gamma ramp
		DWORD ApplyGammaTo(DWORD dwClr);													// apply gamma to given color
		virtual bool ApplyGammaRamp(D3DGAMMARAMP &ramp, bool fForce)=0;				// really apply gamma ramp
		virtual bool SaveDefaultGammaRamp(CStdWindow * pWindow)=0;
		// blit states
		void ActivateBlitModulation(DWORD dwWithClr) { BlitModulated=true; BlitModulateClr=dwWithClr; } // modulate following blits with a given color
		void DeactivateBlitModulation() { BlitModulated=false; }	// stop color modulation of blits
		bool GetBlitModulation(DWORD &rdwColor) { rdwColor=BlitModulateClr; return BlitModulated; }
		void SetBlitMode(DWORD dwBlitMode) { this->dwBlitMode=dwBlitMode & DDrawCfg.AllowedBlitModes; } // set blit mode extra flags (additive blits, mod2-modulation, etc.)
		DWORD SetBlitModeGetPrev(DWORD dwNewBlitMode) { DWORD dwTemp=dwBlitMode; SetBlitMode(dwNewBlitMode); return dwTemp; } // set blit mode extra flags, returning old flags
		void ResetBlitMode() { dwBlitMode=0; }
		void ClrByCurrentBlitMod(DWORD &rdwClr)
			{
			// apply modulation if activated
			if (BlitModulated) ModulateClr(rdwClr, BlitModulateClr);
			}
		void SetClrModMap(CClrModAddMap *pClrModMap) { this->pClrModMap = pClrModMap; }
		void SetClrModMapEnabled(bool fToVal) { fUseClrModMap = fToVal; }
		bool GetClrModMapEnabled() const { return fUseClrModMap; }
		unsigned char SetSaturation(unsigned char s) { unsigned char o = Saturation; Saturation = s; return o; }
		void SetZoom(int X, int Y, float Zoom);
		void SetZoom(const ZoomData &zoom) { SetZoom(zoom.X, zoom.Y, zoom.Zoom); }
		void GetZoom(ZoomData *r) { r->Zoom=Zoom; r->X=ZoomX; r->Y=ZoomY; }
		void ApplyZoom(float & X, float & Y);
		void RemoveZoom(float & X, float & Y);
		virtual void SetTexture() = 0;
		virtual void ResetTexture() = 0;

		// device objects
		virtual bool RestoreDeviceObjects() = 0;		// restore device dependant objects
		virtual bool InvalidateDeviceObjects() = 0;	// free device dependant objects
		virtual bool DeviceReady() = 0;							// return whether device exists

		int GetByteCnt() { return byByteCnt; } // return bytes per pixel

	protected:
		bool StringOut(const char *szText, SURFACE sfcDest, float iTx, float iTy, DWORD dwFCol, BYTE byForm, bool fDoMarkup, CMarkup &Markup, CStdFont *pFont, float fZoom);
		virtual void PerformPix(SURFACE sfcDest, float tx, float ty, DWORD dwCol) = 0; // without ClrModMap
		virtual void PerformLine(SURFACE sfcTarget, float x1, float y1, float x2, float y2, DWORD dwClr) = 0;
		bool CreatePrimaryClipper(unsigned int iXRes, unsigned int iYRes);
		virtual bool CreatePrimarySurfaces(bool Fullscreen, unsigned int iXRes, unsigned int iYRes, int iColorDepth, unsigned int iMonitor) = 0;
		bool Error(const char *szMsg);
		void DebugLog(const char *szMsg)
			{
#ifdef _DEBUG
			Log(szMsg);
#endif
			}

	friend class CSurface;
	friend class CTexRef;
	friend class CPattern;
	friend class CStdD3DShader;
	};

bool LockSurfaceGlobal(SURFACE sfcTarget);
bool UnLockSurfaceGlobal(SURFACE sfcTarget);
bool DLineSPix(int32_t x, int32_t y, int32_t col);
bool DLineSPixDw(int32_t x, int32_t y, int32_t dwClr);

CStdDDraw *DDrawInit(CStdApp * pApp, bool Fullscreen, bool fUsePageLock, unsigned int iXRes, unsigned int iYRes, int iBitDepth, int Engine, unsigned int iMonitor);
#endif // INC_STDDDRAW2
