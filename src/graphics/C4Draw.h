/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2016, The OpenClonk Team and contributors
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

/* A wrapper class to OpenGL and Direct3d */

#ifndef INC_STDDDRAW2
#define INC_STDDDRAW2

#include "graphics/C4Surface.h"
#include "graphics/C4BltTransform.h"
#include "lib/StdMeshMaterial.h"

// Global Draw access pointer
extern C4Draw *pDraw;

inline void DwTo4UB(DWORD dwClr, unsigned char (&r)[4])
{
	r[0] = (unsigned char)( (dwClr >> 16) & 0xff);
	r[1] = (unsigned char)( (dwClr >>  8) & 0xff);
	r[2] = (unsigned char)( (dwClr      ) & 0xff);
	r[3] = (unsigned char)( (dwClr >> 24) & 0xff);
}

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

// helper struct
struct ZoomData
{
	float Zoom;
	float X, Y;
};

class C4FoWRegion;

// Shader combinations
static const int C4SSC_MOD2 = 1; // signed addition instead of multiplication for clrMod
static const int C4SSC_BASE = 2; // use a base texture instead of just a single color
static const int C4SSC_OVERLAY = 4; // use a colored overlay on top of base texture
static const int C4SSC_LIGHT = 8; // use dynamic+ambient lighting
static const int C4SSC_NORMAL = 16; // extract normals from normal map instead of (0,0,1)

// direct draw encapsulation
class C4Draw
{
public:
	enum DrawOperation { OP_POINTS, OP_TRIANGLES };

	static constexpr int COLOR_DEPTH = 32;
	static constexpr int COLOR_DEPTH_BYTES = COLOR_DEPTH / 8;

	C4Draw(): MaxTexSize(0) { }
	virtual ~C4Draw() { pDraw=nullptr; }
public:
	C4AbstractApp * pApp; // the application
	bool Active;                    // set if device is ready to render, etc.
	float gamma[C4MaxGammaRamps][3]; // input gammas
	float gammaOut[3]; // combined gamma
	int MaxTexSize;
	C4ScriptUniform scriptUniform; // uniforms added to all draw calls
protected:
	float fClipX1,fClipY1,fClipX2,fClipY2; // clipper in unzoomed coordinates
	float fStClipX1,fStClipY1,fStClipX2,fStClipY2; // stored clipper in unzoomed coordinates
	int32_t iClipX1,iClipY1,iClipX2,iClipY2; // clipper in pixel coordinates
	bool ClipAll; // set if clipper clips everything away
	bool PrimaryLocked;             // set if primary surface is locked (-> can't blit)
	C4Surface *RenderTarget;         // current render target
	bool BlitModulated;             // set if blits should be modulated with BlitModulateClr
	DWORD BlitModulateClr;          // modulation color for blitting
	DWORD dwBlitMode;               // extra flags for blit
	const C4FoWRegion* pFoW;     // new-style FoW
	float ZoomX; float ZoomY;
	const StdMeshMatrix* MeshTransform; // Transformation to apply to mesh before rendering
	bool fUsePerspective;
public:
	float Zoom;
	// General
	bool Init(C4AbstractApp * pApp, unsigned int iXRes, unsigned int iYRes, unsigned int iMonitor);
	virtual void Clear();
	virtual void Default();
	virtual CStdGLCtx *CreateContext(C4Window *, C4AbstractApp *) { return nullptr; }
	virtual bool OnResolutionChanged(unsigned int iXRes, unsigned int iYRes) = 0; // reinit clipper for new resolution
	// Clipper
	bool GetPrimaryClipper(int &rX1, int &rY1, int &rX2, int &rY2);
	bool SetPrimaryClipper(int iX1, int iY1, int iX2, int iY2);
	bool SubPrimaryClipper(int iX1, int iY1, int iX2, int iY2);
	C4Rect GetClipRect() const;
	C4Rect GetOutRect() const;
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
	virtual bool PrepareMaterial(StdMeshMatManager& mat_manager, StdMeshMaterialLoader& loader, StdMeshMaterial& mat) = 0; // Find best technique, fail if there is none
	virtual bool PrepareRendering(C4Surface * sfcToSurface) = 0; // check if/make rendering possible to given surface
	virtual bool EnsureMainContextSelected() = 0;
	virtual bool PrepareSpriteShader(C4Shader& shader, const char* name, int ssc, C4GroupSet* pGroups, const char* const* additionalDefines, const char* const* additionalSlices) = 0; // create sprite shader
	// Blit
	virtual void BlitLandscape(C4Surface * sfcSource, float fx, float fy,
	                           C4Surface * sfcTarget, float tx, float ty, float wdt, float hgt);
	void Blit8Fast(CSurface8 * sfcSource, int fx, int fy,
	               C4Surface * sfcTarget, int tx, int ty, int wdt, int hgt);
	bool Blit(C4Surface * sfcSource, float fx, float fy, float fwdt, float fhgt,
	          C4Surface * sfcTarget, float tx, float ty, float twdt, float thgt,
	          bool fSrcColKey=false, const C4BltTransform *pTransform=nullptr);
	bool BlitUnscaled(C4Surface * sfcSource, float fx, float fy, float fwdt, float fhgt,
	                  C4Surface * sfcTarget, float tx, float ty, float twdt, float thgt,
	                  bool fSrcColKey=false, const C4BltTransform *pTransform=nullptr);
	bool RenderMesh(StdMeshInstance &instance, C4Surface * sfcTarget, float tx, float ty, float twdt, float thgt, DWORD dwPlayerColor, C4BltTransform* pTransform); // Call PrepareMaterial with Mesh's material before
	virtual void PerformMesh(StdMeshInstance &instance, float tx, float ty, float twdt, float thgt, DWORD dwPlayerColor, C4BltTransform* pTransform) = 0;
	bool Blit8(C4Surface * sfcSource, int fx, int fy, int fwdt, int fhgt, // force 8bit-blit (inline)
	           C4Surface * sfcTarget, int tx, int ty, int twdt, int thgt,
	           bool fSrcColKey=false, const C4BltTransform *pTransform=nullptr);
	bool BlitSimple(C4Surface * sfcSource, int fx, int fy, int fwdt, int fhgt,
	                C4Surface * sfcTarget, int tx, int ty, int twdt, int thgt,
	                bool fTransparency=true);
	bool BlitSurface(C4Surface * sfcSurface, C4Surface * sfcTarget, int tx, int ty, bool fBlitBase);
	bool BlitSurfaceTile(C4Surface * sfcSurface, C4Surface * sfcTarget, float iToX, float iToY, float iToWdt, float iToHgt, float iOffsetX, float iOffsetY, C4ShaderCall* shader_call);
	virtual void FillBG(DWORD dwClr=0) = 0;
	// Text
	enum { DEFAULT_MESSAGE_COLOR = 0xffffffff };
	bool TextOut(const char *szText, CStdFont &rFont, float fZoom, C4Surface * sfcDest, float iTx, float iTy, DWORD dwFCol=0xffffffff, BYTE byForm=ALeft, bool fDoMarkup=true);
	bool StringOut(const char *szText, CStdFont &rFont, float fZoom, C4Surface * sfcDest, float iTx, float iTy, DWORD dwFCol=0xffffffff, BYTE byForm=ALeft, bool fDoMarkup=true);
	// Drawing
	virtual void PerformMultiPix(C4Surface* sfcTarget, const C4BltVertex* vertices, unsigned int n_vertices, C4ShaderCall* shader_call) = 0;
	virtual void PerformMultiLines(C4Surface* sfcTarget, const C4BltVertex* vertices, unsigned int n_vertices, float width, C4ShaderCall* shader_call) = 0;
	virtual void PerformMultiTris(C4Surface* sfcTarget, const C4BltVertex* vertices, unsigned int n_vertices, const C4BltTransform* pTransform, C4TexRef* pTex, C4TexRef* pOverlay, C4TexRef* pNormal, DWORD dwOverlayClrMod, C4ShaderCall* shader_call) = 0; // blit the same texture many times
	// Convenience drawing functions
	void DrawBoxDw(C4Surface * sfcDest, int iX1, int iY1, int iX2, int iY2, DWORD dwClr); // calls DrawBoxFade
	void DrawBoxFade(C4Surface * sfcDest, float iX, float iY, float iWdt, float iHgt, DWORD dwClr1, DWORD dwClr2, DWORD dwClr3, DWORD dwClr4, C4ShaderCall* shader_call); // calls DrawQuadDw
	void DrawPatternedCircle(C4Surface * sfcDest, int x, int y, int r, BYTE col, C4Pattern & Pattern, CStdPalette &rPal);
	void DrawFrameDw(C4Surface * sfcDest, int x1, int y1, int x2, int y2, DWORD dwClr, float width=1.0f);
	void DrawQuadDw(C4Surface * sfcTarget, float *ipVtx, DWORD dwClr1, DWORD dwClr2, DWORD dwClr3, DWORD dwClr4, C4ShaderCall* shader_call);
	void DrawPix(C4Surface * sfcDest, float tx, float ty, DWORD dwCol); // Consider using PerformMultiPix if you draw more than one pixel
	void DrawLineDw(C4Surface * sfcTarget, float x1, float y1, float x2, float y2, DWORD dwClr, float width = 1.0f); // Consider using PerformMultiLines if you draw more than one line
	void DrawCircleDw(C4Surface * sfcTarget, float cx, float cy, float r, DWORD dwClr, float width = 1.0f);
	// gamma
	void SetGamma(float r, float g, float b, int32_t iRampIndex);  // set gamma
	void ResetGamma(); // reset gamma to default
	DWORD ApplyGammaTo(DWORD dwClr); // apply gamma to given color
	// blit states
	void ActivateBlitModulation(DWORD dwWithClr) { BlitModulated=true; BlitModulateClr=dwWithClr; } // modulate following blits with a given color
	void DeactivateBlitModulation() { BlitModulated=false; }  // stop color modulation of blits
	bool GetBlitModulation(DWORD &rdwColor) { rdwColor=BlitModulateClr; return BlitModulated; }
	void SetBlitMode(DWORD dwBlitMode) { this->dwBlitMode=dwBlitMode & C4GFXBLIT_ALL; } // set blit mode extra flags (additive blits, mod2-modulation, etc.)
	void ResetBlitMode() { dwBlitMode=0; }
	void SetFoW(const C4FoWRegion* fow) { pFoW = fow; }
	const C4FoWRegion* GetFoW() const { return pFoW; }
	void SetZoom(float X, float Y, float Zoom);
	void SetZoom(const ZoomData &zoom) { SetZoom(zoom.X, zoom.Y, zoom.Zoom); }
	void GetZoom(ZoomData *r) { r->Zoom=Zoom; r->X=ZoomX; r->Y=ZoomY; }
	void ApplyZoom(float & X, float & Y);
	void RemoveZoom(float & X, float & Y);
	void SetMeshTransform(const StdMeshMatrix* Transform) { MeshTransform = Transform; } // if non-nullptr make sure to keep matrix valid
	void SetPerspective(bool fSet) { fUsePerspective = fSet; }

	// device objects
	virtual bool RestoreDeviceObjects() = 0;    // restore device dependant objects
	virtual bool InvalidateDeviceObjects() = 0; // free device dependant objects
	virtual bool DeviceReady() = 0;             // return whether device exists

protected:
	bool StringOut(const char *szText, C4Surface * sfcDest, float iTx, float iTy, DWORD dwFCol, BYTE byForm, bool fDoMarkup, C4Markup &Markup, CStdFont *pFont, float fZoom);
	bool CreatePrimaryClipper(unsigned int iXRes, unsigned int iYRes);
	virtual bool Error(const char *szMsg);

	friend class C4Surface;
	friend class C4TexRef;
	friend class C4Pattern;
};

struct ZoomDataStackItem: public ZoomData
{
	ZoomDataStackItem(float newzoom) { pDraw->GetZoom(this); pDraw->SetZoom(X, Y, newzoom); }
	ZoomDataStackItem(float X, float Y, float newzoom) { pDraw->GetZoom(this); pDraw->SetZoom(X, Y, newzoom); }
	~ZoomDataStackItem() { pDraw->SetZoom(*this); }
};

bool DDrawInit(C4AbstractApp * pApp, unsigned int iXRes, unsigned int iYRes, unsigned int iMonitor);
#endif // INC_STDDDRAW2
