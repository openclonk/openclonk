/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2002-2005  Sven Eberhardt
 * Copyright (c) 2004-2010  Günther Brammer
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

/* A wrapper class to OpenGL and Direct3d */

#ifndef INC_STDDDRAW2
#define INC_STDDDRAW2

#include <C4Surface.h>
#include <CSurface8.h>
#include <StdBuf.h>
#ifdef _WIN32
#include <C4windowswrapper.h>
#endif

// engines
#define GFXENGN_DIRECTX  0
#define GFXENGN_OPENGL   1
#define GFXENGN_DIRECTXS 2
#define GFXENGN_NOGFX    3

// Global Draw access pointer
extern C4Draw *pDraw;

// rotation info class
class C4BltTransform
{
public:
	float mat[9]; // transformation matrix
public:
	C4BltTransform() {} // default: don't init fields
	void Set(float fA, float fB, float fC, float fD, float fE, float fF, float fG, float fH, float fI)
	{ mat[0]=fA; mat[1]=fB; mat[2]=fC; mat[3]=fD; mat[4]=fE; mat[5]=fF; mat[6]=fG; mat[7]=fH; mat[8]=fI; }
	void SetRotate(int iAngle, float fOffX, float fOffY); // set by angle and rotation offset
	bool SetAsInv(C4BltTransform &rOfTransform);
	void Rotate(int iAngle, float fOffX, float fOffY) // rotate by angle around rotation offset
	{
		// multiply matrix as seen in SetRotate by own matrix
		C4BltTransform rot; rot.SetRotate(iAngle, fOffX, fOffY);
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
		C4BltTransform move; move.SetMoveScale(dx,dy,sx,sy);
		(*this) *= move;
	}
	void ScaleAt(float sx, float sy, float tx, float ty)
	{
		// scale and move back so tx and ty remain fixpoints
		MoveScale(-tx*(sx-1), -ty*(sy-1), sx, sy);
	}
	C4BltTransform &operator *= (const C4BltTransform &r)
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
	void TransformPoint(float &rX, float &rY) const; // rotate point by angle
};

// pattern
class C4Pattern
{
private:
	// pattern surface for new-style patterns
	class C4Surface *sfcPattern32;
	// Faster access
	uint32_t * CachedPattern; int Wdt; int Hgt;
	// pattern zoom factor; 0 means no zoom
	int Zoom;
public:
	C4Pattern& operator=(const C4Pattern&);
	const C4Surface *getSurface() const { return sfcPattern32; }
	DWORD PatternClr(unsigned int iX, unsigned int iY) const; // apply pattern to color
	bool Set(class C4Surface *sfcSource, int iZoom=0); // set and enable pattern
	void SetZoom(int iZoom) { Zoom = iZoom; }
	void Clear();                     // clear pattern
	C4Pattern();         // ctor
	~C4Pattern() { Clear(); }          // dtor
};

// blit position on screen
// This is the format required by GL_T2F_C4UB_V3F
struct C4BltVertex
{
	float tx, ty; // texture positions
	unsigned char color[4]; // color modulation
	float ftx,fty,ftz; // blit positions
};

// blit bounds polygon - note that blitting procedures are not designed for inner angles (>pi)
struct C4BltData
{
	BYTE byNumVertices;  // number of valid vertices
	C4BltVertex vtVtx[8]; // vertices for polygon - up to eight vertices may be needed
	const C4BltTransform *pTransform; // Vertex transformation
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
class C4GammaControl
{
private:
	void SetClrChannel(WORD *pBuf, BYTE c1, BYTE c2, int c3); // set color channel ramp

protected:
	D3DGAMMARAMP ramp;

public:
	C4GammaControl() { Default(); } // ctor
	void Default() { Set(0x000000, 0x808080, 0xffffff); } // set default ramp

	void Set(DWORD dwClr1, DWORD dwClr2, DWORD dwClr3); // set color ramp

	DWORD ApplyTo(DWORD dwClr);   // apply gamma to color value

	friend class C4Draw;
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
class C4Draw
{
public:
	static const StdMeshMatrix OgreToClonk;

	C4Draw(): MaxTexSize(0), Saturation(255) { }
	virtual ~C4Draw() { pDraw=NULL; }
public:
	C4AbstractApp * pApp; // the application
	bool Active;                    // set if device is ready to render, etc.
	C4GammaControl Gamma;            // gamma
	C4GammaControl DefRamp;            // default gamma ramp
	uint32_t dwGamma[C4MaxGammaRamps*3];    // gamma ramps
	int MaxTexSize;
protected:
	bool fSetGamma;     // must gamma ramp be reassigned?
	BYTE                byByteCnt;    // bytes per pixel (2 or 4)
	bool Editor;
	float fClipX1,fClipY1,fClipX2,fClipY2; // clipper in unzoomed coordinates
	float fStClipX1,fStClipY1,fStClipX2,fStClipY2; // stored clipper in unzoomed coordinates
	int32_t iClipX1,iClipY1,iClipX2,iClipY2; // clipper in pixel coordinates
	bool ClipAll; // set if clipper clips everything away
	bool PrimaryLocked;             // set if primary surface is locked (-> can't blit)
	C4Surface *RenderTarget;         // current render target
	bool BlitModulated;             // set if blits should be modulated with BlitModulateClr
	DWORD BlitModulateClr;          // modulation color for blitting
	DWORD dwBlitMode;               // extra flags for blit
	C4FogOfWar *pClrModMap;      // map to be used for global color modulation (invalid if !fUseClrModMap)
	bool fUseClrModMap;             // if set, pClrModMap will be checked for color modulations
	unsigned char Saturation;   // if < 255, an extra filter is used to reduce the saturation
	int ZoomX; int ZoomY;
	const StdMeshMatrix* MeshTransform; // Transformation to apply to mesh before rendering
	bool fUsePerspective;
public:
	float Zoom;
	// General
	bool Init(C4AbstractApp * pApp, bool Editor, bool fUsePageLock, unsigned int iXRes, unsigned int iYRes, int iBitDepth, unsigned int iMonitor);
	virtual void Clear();
	virtual void Default();
	virtual CStdGLCtx *CreateContext(C4Window *, C4AbstractApp *) { return NULL; }
#ifdef _WIN32
	virtual CStdGLCtx *CreateContext(HWND, C4AbstractApp *) { return NULL; }
#endif
	virtual int GetEngine() = 0;    // get indexed engine
	virtual bool OnResolutionChanged(unsigned int iXRes, unsigned int iYRes) = 0; // reinit clipper for new resolution
	virtual bool IsOpenGL() { return false; }
	virtual bool IsShaderific() { return false; }
	// Clipper
	bool GetPrimaryClipper(int &rX1, int &rY1, int &rX2, int &rY2);
	bool SetPrimaryClipper(int iX1, int iY1, int iX2, int iY2);
	bool SubPrimaryClipper(int iX1, int iY1, int iX2, int iY2);
	bool StorePrimaryClipper();
	bool RestorePrimaryClipper();
	bool NoPrimaryClipper();
	bool ApplyPrimaryClipper(C4Surface * sfcSurface);
	bool DetachPrimaryClipper(C4Surface * sfcSurface);
	virtual bool UpdateClipper() = 0; // set current clipper to render target
	// Surface
	bool GetSurfaceSize(C4Surface * sfcSurface, int &iWdt, int &iHgt);
	void Grayscale(C4Surface * sfcSfc, int32_t iOffset = 0);
	void LockingPrimary() { PrimaryLocked=true; }
	void PrimaryUnlocked() { PrimaryLocked=false; }
	virtual bool PrepareMaterial(StdMeshMaterial &mat) = 0; // Find best technique, fail if there is none
	virtual bool PrepareRendering(C4Surface * sfcToSurface) = 0; // check if/make rendering possible to given surface
	// Blit
	virtual void BlitLandscape(C4Surface * sfcSource, float fx, float fy,
	                           C4Surface * sfcTarget, float tx, float ty, float wdt, float hgt, const C4Surface * textures[]);
	void Blit8Fast(CSurface8 * sfcSource, int fx, int fy,
	               C4Surface * sfcTarget, int tx, int ty, int wdt, int hgt);
	bool Blit(C4Surface * sfcSource, float fx, float fy, float fwdt, float fhgt,
	          C4Surface * sfcTarget, float tx, float ty, float twdt, float thgt,
	          bool fSrcColKey=false, const C4BltTransform *pTransform=NULL);
	bool BlitUnscaled(C4Surface * sfcSource, float fx, float fy, float fwdt, float fhgt,
	                  C4Surface * sfcTarget, float tx, float ty, float twdt, float thgt,
	                  bool fSrcColKey=false, const C4BltTransform *pTransform=NULL);
	bool RenderMesh(StdMeshInstance &instance, C4Surface * sfcTarget, float tx, float ty, float twdt, float thgt, DWORD dwPlayerColor, C4BltTransform* pTransform); // Call PrepareMaterial with Mesh's material before
	virtual void PerformBlt(C4BltData &rBltData, C4TexRef *pTex, DWORD dwModClr, bool fMod2, bool fExact) = 0;
	virtual void PerformMesh(StdMeshInstance &instance, float tx, float ty, float twdt, float thgt, DWORD dwPlayerColor, C4BltTransform* pTransform) = 0;
	bool Blit8(C4Surface * sfcSource, int fx, int fy, int fwdt, int fhgt, // force 8bit-blit (inline)
	           C4Surface * sfcTarget, int tx, int ty, int twdt, int thgt,
	           bool fSrcColKey=false, const C4BltTransform *pTransform=NULL);
	bool BlitRotate(C4Surface * sfcSource, int fx, int fy, int fwdt, int fhgt,
	                C4Surface * sfcTarget, int tx, int ty, int twdt, int thgt,
	                int iAngle, bool fTransparency=true);
	bool BlitSurface(C4Surface * sfcSurface, C4Surface * sfcTarget, int tx, int ty, bool fBlitBase);
	bool BlitSurfaceTile(C4Surface * sfcSurface, C4Surface * sfcTarget, int iToX, int iToY, int iToWdt, int iToHgt, int iOffsetX=0, int iOffsetY=0, bool fSrcColKey=false);
	bool BlitSurfaceTile2(C4Surface * sfcSurface, C4Surface * sfcTarget, int iToX, int iToY, int iToWdt, int iToHgt, int iOffsetX=0, int iOffsetY=0, bool fSrcColKey=false);
	virtual void FillBG(DWORD dwClr=0) = 0;
	// Text
	enum { DEFAULT_MESSAGE_COLOR = 0xffffffff };
	bool TextOut(const char *szText, CStdFont &rFont, float fZoom, C4Surface * sfcDest, float iTx, float iTy, DWORD dwFCol=0xffffffff, BYTE byForm=ALeft, bool fDoMarkup=true);
	bool StringOut(const char *szText, CStdFont &rFont, float fZoom, C4Surface * sfcDest, float iTx, float iTy, DWORD dwFCol=0xffffffff, BYTE byForm=ALeft, bool fDoMarkup=true);
	// Drawing
	virtual void DrawPix(C4Surface * sfcDest, float tx, float ty, DWORD dwCol);
	void DrawBoxDw(C4Surface * sfcDest, int iX1, int iY1, int iX2, int iY2, DWORD dwClr); // calls DrawBoxFade
	void DrawBoxFade(C4Surface * sfcDest, float iX, float iY, float iWdt, float iHgt, DWORD dwClr1, DWORD dwClr2, DWORD dwClr3, DWORD dwClr4, int iBoxOffX, int iBoxOffY); // calls DrawQuadDw
	void DrawPatternedCircle(C4Surface * sfcDest, int x, int y, int r, BYTE col, C4Pattern & Pattern, CStdPalette &rPal);
	void DrawFrameDw(C4Surface * sfcDest, int x1, int y1, int x2, int y2, DWORD dwClr);
	virtual void DrawLineDw(C4Surface * sfcTarget, float x1, float y1, float x2, float y2, DWORD dwClr, float width = 1.0f);
	virtual void DrawQuadDw(C4Surface * sfcTarget, float *ipVtx, DWORD dwClr1, DWORD dwClr2, DWORD dwClr3, DWORD dwClr4) = 0;
	// gamma
	void SetGamma(DWORD dwClr1, DWORD dwClr2, DWORD dwClr3, int32_t iRampIndex);  // set gamma ramp
	void ApplyGamma();                                        // apply gamma ramp to ddraw
	void DisableGamma();                                      // reset gamma ramp to default
	void EnableGamma();                                       // set current gamma ramp
	DWORD ApplyGammaTo(DWORD dwClr);                          // apply gamma to given color
	// blit states
	void ActivateBlitModulation(DWORD dwWithClr) { BlitModulated=true; BlitModulateClr=dwWithClr; } // modulate following blits with a given color
	void DeactivateBlitModulation() { BlitModulated=false; }  // stop color modulation of blits
	bool GetBlitModulation(DWORD &rdwColor) { rdwColor=BlitModulateClr; return BlitModulated; }
	void SetBlitMode(DWORD dwBlitMode) { this->dwBlitMode=dwBlitMode & C4GFXBLIT_ALL; } // set blit mode extra flags (additive blits, mod2-modulation, etc.)
	DWORD SetBlitModeGetPrev(DWORD dwNewBlitMode) { DWORD dwTemp=dwBlitMode; SetBlitMode(dwNewBlitMode); return dwTemp; } // set blit mode extra flags, returning old flags
	void ResetBlitMode() { dwBlitMode=0; }
	void ClrByCurrentBlitMod(DWORD &rdwClr)
	{
		// apply modulation if activated
		if (BlitModulated) ModulateClr(rdwClr, BlitModulateClr);
	}
	void SetClrModMap(C4FogOfWar *pClrModMap) { this->pClrModMap = pClrModMap; }
	void SetClrModMapEnabled(bool fToVal) { fUseClrModMap = fToVal; }
	bool GetClrModMapEnabled() const { return fUseClrModMap; }
	unsigned char SetSaturation(unsigned char s) { unsigned char o = Saturation; Saturation = s; return o; }
	void SetZoom(int X, int Y, float Zoom);
	void SetZoom(const ZoomData &zoom) { SetZoom(zoom.X, zoom.Y, zoom.Zoom); }
	void GetZoom(ZoomData *r) { r->Zoom=Zoom; r->X=ZoomX; r->Y=ZoomY; }
	void ApplyZoom(float & X, float & Y);
	void RemoveZoom(float & X, float & Y);
	void SetMeshTransform(const StdMeshMatrix* Transform) { MeshTransform = Transform; } // if non-NULL make sure to keep matrix valid
	void SetPerspective(bool fSet) { fUsePerspective = fSet; }
	virtual void SetTexture() = 0;
	virtual void ResetTexture() = 0;

	// device objects
	virtual bool RestoreDeviceObjects() = 0;    // restore device dependant objects
	virtual bool InvalidateDeviceObjects() = 0; // free device dependant objects
	virtual bool DeviceReady() = 0;             // return whether device exists

	int GetByteCnt() { return byByteCnt; } // return bytes per pixel

protected:
	bool StringOut(const char *szText, C4Surface * sfcDest, float iTx, float iTy, DWORD dwFCol, BYTE byForm, bool fDoMarkup, C4Markup &Markup, CStdFont *pFont, float fZoom);
	virtual void PerformPix(C4Surface * sfcDest, float tx, float ty, DWORD dwCol) = 0; // without ClrModMap
	virtual void PerformLine(C4Surface * sfcTarget, float x1, float y1, float x2, float y2, DWORD dwClr, float width) = 0;
	bool CreatePrimaryClipper(unsigned int iXRes, unsigned int iYRes);
	virtual bool CreatePrimarySurfaces(bool Editor, unsigned int iXRes, unsigned int iYRes, int iColorDepth, unsigned int iMonitor) = 0;
	virtual bool Error(const char *szMsg);
	void DebugLog(const char *szMsg)
	{
#ifdef _DEBUG
		Log(szMsg);
#endif
	}

	friend class C4Surface;
	friend class C4TexRef;
	friend class C4Pattern;
	friend class CStdD3DShader;
};

struct ZoomDataStackItem: public ZoomData
{
	ZoomDataStackItem(float newzoom) { pDraw->GetZoom(this); pDraw->SetZoom(X, Y, newzoom); }
	ZoomDataStackItem(int X, int Y, float newzoom) { pDraw->GetZoom(this); pDraw->SetZoom(X, Y, newzoom); }
	~ZoomDataStackItem() { pDraw->SetZoom(*this); }
};

bool LockSurfaceGlobal(C4Surface * sfcTarget);
bool UnLockSurfaceGlobal(C4Surface * sfcTarget);
bool DLineSPix(int32_t x, int32_t y, int32_t col);
bool DLineSPixDw(int32_t x, int32_t y, int32_t dwClr);

bool DDrawInit(C4AbstractApp * pApp, bool Editor, bool fUsePageLock, unsigned int iXRes, unsigned int iYRes, int iBitDepth, int Engine, unsigned int iMonitor);
#endif // INC_STDDDRAW2
