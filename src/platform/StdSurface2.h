/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2002, 2004-2005, 2008  Sven Eberhardt
 * Copyright (c) 2004-2005, 2007-2011  Günther Brammer
 * Copyright (c) 2005  Peter Wortmann
 * Copyright (c) 2009  Nicolas Hake
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
// a wrapper class to DirectDraw surfaces

#ifndef INC_StdSurface2
#define INC_StdSurface2

#include <StdColors.h>
#include <C4Rect.h>

#ifdef _WIN32
#include <C4windowswrapper.h>
#endif
#ifdef USE_DIRECTX
#include <d3d9.h>
#undef DrawText
#else
typedef void* IDirect3DSurface9;
#endif

#ifdef USE_GL
#include <GL/glew.h>
#endif

#include <list>

// blitting modes
#define C4GFXBLIT_NORMAL          0 // regular blit
#define C4GFXBLIT_ADDITIVE        1 // all blits additive
#define C4GFXBLIT_MOD2            2 // additive color modulation
#define C4GFXBLIT_CLRSFC_OWNCLR   4 // do not apply global modulation to ColorByOwner-surface
#define C4GFXBLIT_CLRSFC_MOD2     8 // additive color modulation for ClrByOwner-surface

#define C4GFXBLIT_ALL            15 // bist mask covering all blit modes
#define C4GFXBLIT_NOADD          14 // bit mask covering all blit modes except additive

#define C4GFXBLIT_CUSTOM        128 // custom blitting mode - ignored by gfx system
#define C4GFXBLIT_PARENT        256 // blitting mode inherited by parent - ignored by gfx system

// bit depth for emulated surfaces
#define C4GFX_NOGFX_CLRDEPTH      24

const int ALeft=0,ACenter=1,ARight=2;

#ifdef USE_DIRECTX
class CStdD3D;
extern CStdD3D *pD3D;
#endif

#ifdef USE_GL
class CStdGL;
class CStdGLCtx;
extern CStdGL *pGL;
#endif

extern CStdDDraw *lpDDraw;

class CSurface
{
private:
	CSurface(const CSurface &cpy); // do NOT copy
	CSurface &operator = (const CSurface &rCpy);  // do NOT copy

public:
	CSurface();
	~CSurface();
	CSurface(int iWdt, int iHgt); // create new surface and init it
	CSurface(CStdApp * pApp, CStdWindow * pWindow); // create new surface for a window
public:
	int Wdt,Hgt; // size of surface
	int Scale; // scale of image; divide coordinates by this value to get the "original" image size
	int PrimarySurfaceLockPitch; BYTE *PrimarySurfaceLockBits; // lock data if primary surface is locked
	int iTexSize; // size of textures
	int iTexX, iTexY;     // number of textures in x/y-direction
	int ClipX,ClipY,ClipX2,ClipY2;
	bool fIsRenderTarget;              // set for surfaces to be used as offscreen render targets
	bool fIsBackground; // background surfaces fill unused pixels with black, rather than transparency - must be set prior to loading
#ifdef _DEBUG
	int *dbg_idx;
#endif
#if defined(USE_DIRECTX) && defined(USE_GL)
	union
	{
		struct // D3D values
		{
#endif
#ifdef USE_DIRECTX
			IDirect3DSurface9 *pSfc;      // surface (primary sfc)
			D3DFORMAT dwClrFormat;        // used color format in textures
#endif
#if defined(USE_DIRECTX) && defined(USE_GL)

		};
		struct // OpenGL values
		{
#endif
#ifdef USE_GL
			GLenum Format;                // used color format in textures
			CStdGLCtx * pCtx;
#endif
#if defined(USE_DIRECTX) && defined(USE_GL)
		};
	};
#endif
	CTexRef **ppTex;              // textures
	BYTE byBytesPP;               // bytes per pixel (2 or 4)
	CSurface *pMainSfc;           // main surface for simple ColorByOwner-surfaces
	DWORD ClrByOwnerClr;          // current color to be used for ColorByOwner-blits

	void MoveFrom(CSurface *psfcFrom); // grab data from other surface - invalidates other surface
	bool IsRenderTarget();        // surface can be used as a render target?
protected:
	CStdWindow * pWindow;
	int Locked;
	bool Attached;
	bool fPrimary;

	bool IsSingleSurface() { return iTexX*iTexY==1; } // return whether surface is not split

public:
	void SetBackground() { fIsBackground = true; }
	int IsLocked() const { return Locked; }
	// Note: This uses partial locks, anything but SetPixDw and Unlock is undefined afterwards until unlock.
	void ClearBoxDw(int iX, int iY, int iWdt, int iHgt);
	bool Unlock();
	bool Lock();
	bool GetTexAt(CTexRef **ppTexRef, int &rX, int &rY);  // get texture and adjust x/y
	bool GetLockTexAt(CTexRef **ppTexRef, int &rX, int &rY);  // get texture; ensure it's locked and adjust x/y
	DWORD GetPixDw(int iX, int iY, bool fApplyModulation);  // get 32bit-px
	bool IsPixTransparent(int iX, int iY);  // is pixel's alpha value <= 0x7f?
	bool SetPixDw(int iX, int iY, DWORD dwCol);       // set pix in surface only
	bool SetPixAlpha(int iX, int iY, BYTE byAlpha);   // adjust alpha value of pixel
	bool BltPix(int iX, int iY, CSurface *sfcSource, int iSrcX, int iSrcY, bool fTransparency); // blit pixel from source to this surface (assumes clipped coordinates!)
	bool Create(int iWdt, int iHgt, bool fOwnPal=false, bool fIsRenderTarget=false, int MaxTextureSize = 0);
	bool CreateColorByOwner(CSurface *pBySurface);  // create ColorByOwner-surface
	bool SetAsClrByOwnerOf(CSurface *pOfSurface);   // assume that ColorByOwner-surface has been created, and just assign it; fails if the size doesn't match
#ifdef USE_GL
	bool CreatePrimaryGLTextures();                 // create primary textures from back buffer
#endif
	// Only for surfaces which map to a window
	bool UpdateSize(int wdt, int hgt);
	bool PageFlip(C4Rect *pSrcRt=NULL, C4Rect *pDstRt=NULL);

	void Clear();
	void Default();
	void Clip(int iX, int iY, int iX2, int iY2);
	void NoClip();
	bool ReadBMP(class CStdStream &hGroup);
	bool SavePNG(const char *szFilename, bool fSaveAlpha, bool fApplyGamma, bool fSaveOverlayOnly);
	bool AttachPalette();
#ifdef USE_DIRECTX
	IDirect3DSurface9 *GetSurface(); // get internal surface
#endif
	bool GetSurfaceSize(int &irX, int &irY); // get surface size
	void SetClr(DWORD toClr) { ClrByOwnerClr=toClr; }
	DWORD GetClr() { return ClrByOwnerClr; }
	bool CopyBytes(BYTE *pImageData); // assumes an array of wdt*hgt*bitdepth/8 and copies data directly from it
protected:
	void MapBytes(BYTE *bpMap);
	bool ReadBytes(BYTE **lpbpData, void *bpTarget, int iSize);
	bool CreateTextures(int MaxTextureSize = 0);    // create ppTex-array
	void FreeTextures();      // free ppTex-array if existant

	friend class CStdDDraw;
	friend class CPattern;
	friend class CStdD3D;
	friend class CStdGL;
};

typedef CSurface * SURFACE;

#ifndef USE_DIRECTX
typedef struct _D3DLOCKED_RECT
{
	int                 Pitch;
	unsigned char *     pBits;
} D3DLOCKED_RECT;
#endif

// one texture encapsulation
class CTexRef
{
public:
	D3DLOCKED_RECT texLock;   // current lock-data
#if defined(USE_DIRECTX) && defined(USE_GL)
	union
	{
		struct // D3D
		{
#endif
#ifdef USE_DIRECTX
			IDirect3DTexture9 *pTex;  // texture
#endif
#if defined(USE_DIRECTX) && defined(USE_GL)
		};
		struct // OpenGL
		{
#endif
#ifdef USE_GL
			GLuint texName;
#endif
#if defined(USE_DIRECTX) && defined(USE_GL)
		};
	};
#endif
	int iSizeX;
	int iSizeY;
	bool fIntLock;    // if set, texref is locked internally only
	C4Rect LockSize;

	CTexRef(int iSizeX, int iSizeY, bool fAsRenderTarget);   // create texture with given size
	~CTexRef();           // release texture
	bool Lock();          // lock texture
	// Lock a part of the rect, discarding the content
	// Note: Calling Lock afterwards without an Unlock first is undefined
	bool LockForUpdate(C4Rect &rtUpdate);
	void Unlock();        // unlock texture
	bool ClearRect(C4Rect &rtClear); // clear rect in texture to transparent
	bool FillBlack(); // fill complete texture in black
	void SetPix2(int iX, int iY, WORD v)
	{
		*((WORD *) (((BYTE *) texLock.pBits) + (iY - LockSize.y) * texLock.Pitch + (iX - LockSize.x) * 2)) = v;
	}
	void SetPix4(int iX, int iY, DWORD v)
	{
		*((DWORD *) (((BYTE *) texLock.pBits) + (iY - LockSize.y) * texLock.Pitch + (iX - LockSize.x) * 4)) = v;
	}
};

// texture management
class CTexMgr
{
public:
	std::list<CTexRef *> Textures;

public:
	CTexMgr();    // ctor
	~CTexMgr();   // dtor

	void RegTex(CTexRef *pTex);
	void UnregTex(CTexRef *pTex);

	void IntLock();   // do an internal lock
	void IntUnlock(); // undo internal lock
};

extern CTexMgr *pTexMgr;

#define SURFACE CSurface *

#endif
