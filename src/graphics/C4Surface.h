/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2013, The OpenClonk Team and contributors
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
// a wrapper class to DirectDraw surfaces

#ifndef INC_StdSurface2
#define INC_StdSurface2

#include <StdColors.h>
#include <C4Rect.h>

#ifdef _WIN32
#include <C4windowswrapper.h>
#endif

#ifndef USE_CONSOLE
#include <GL/glew.h>
#endif

#include <list>

// blitting modes
#define C4GFXBLIT_NORMAL          0 // regular blit
#define C4GFXBLIT_ADDITIVE        1 // all blits additive
#define C4GFXBLIT_MOD2            2 // additive color modulation
#define C4GFXBLIT_CLRSFC_OWNCLR   4 // do not apply global modulation to ColorByOwner-surface
#define C4GFXBLIT_CLRSFC_MOD2     8 // additive color modulation for ClrByOwner-surface
#define C4GFXBLIT_WIREFRAME      16 // draws a mesh as wireframe

#define C4GFXBLIT_ALL            31 // bist mask covering all blit modes
#define C4GFXBLIT_NOADD          30 // bit mask covering all blit modes except additive

#define C4GFXBLIT_CUSTOM        128 // custom blitting mode - ignored by gfx system
#define C4GFXBLIT_PARENT        256 // blitting mode inherited by parent - ignored by gfx system

// bit depth for emulated surfaces
#define C4GFX_NOGFX_CLRDEPTH      24

const int ALeft=0,ACenter=1,ARight=2;

#ifndef USE_CONSOLE
class CStdGL;
class CStdGLCtx;
extern CStdGL *pGL;
#endif

class C4Surface
{
private:
	C4Surface(const C4Surface &cpy); // do NOT copy
	C4Surface &operator = (const C4Surface &rCpy);  // do NOT copy

public:
	C4Surface();
	~C4Surface();
	C4Surface(int iWdt, int iHgt); // create new surface and init it
	C4Surface(C4AbstractApp * pApp, C4Window * pWindow); // create new surface for a window
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
#ifndef USE_CONSOLE
			GLenum Format;                // used color format in textures
			CStdGLCtx * pCtx;
#endif
	std::vector<C4TexRef> textures;              // textures
	BYTE byBytesPP;               // bytes per pixel (2 or 4)
	C4Surface *pMainSfc;          // main surface for simple ColorByOwner-surfaces
	C4Surface *pNormalSfc;        // normal map; can be NULL
	DWORD ClrByOwnerClr;          // current color to be used for ColorByOwner-blits

	void MoveFrom(C4Surface *psfcFrom); // grab data from other surface - invalidates other surface
	bool IsRenderTarget();        // surface can be used as a render target?
protected:
	C4Window * pWindow;
	int Locked;
	bool Attached;
	bool fPrimary;

	bool IsSingleSurface() const { return iTexX*iTexY==1; } // return whether surface is not split

public:
	void SetBackground() { fIsBackground = true; }
	int IsLocked() const { return Locked; }
	// Note: This uses partial locks, anything but SetPixDw and Unlock is undefined afterwards until unlock.
	void ClearBoxDw(int iX, int iY, int iWdt, int iHgt);
	bool Unlock();
	bool Lock();
	bool GetTexAt(C4TexRef **ppTexRef, int &rX, int &rY) // get texture and adjust x/y
	{
		if (textures.empty()) return false;
		if (rX < 0 || rY < 0 || rX >= Wdt || rY >= Hgt) return false;
		if (IsSingleSurface())
		{
			*ppTexRef = &textures[0];
			return true;
		}
		return GetTexAtImpl(ppTexRef, rX, rY);
	}
	bool GetLockTexAt(C4TexRef **ppTexRef, int &rX, int &rY);  // get texture; ensure it's locked and adjust x/y
	DWORD GetPixDw(int iX, int iY, bool fApplyModulation);  // get 32bit-px
	bool IsPixTransparent(int iX, int iY);  // is pixel's alpha value <= 0x7f?
	bool SetPixDw(int iX, int iY, DWORD dwCol);       // set pix in surface only
	bool SetPixAlpha(int iX, int iY, BYTE byAlpha);   // adjust alpha value of pixel
	bool BltPix(int iX, int iY, C4Surface *sfcSource, int iSrcX, int iSrcY, bool fTransparency); // blit pixel from source to this surface (assumes clipped coordinates!)
	bool Create(int iWdt, int iHgt, bool fOwnPal=false, bool fIsRenderTarget=false, int MaxTextureSize = 0);
	bool Copy(C4Surface &fromSfc);
	bool CreateColorByOwner(C4Surface *pBySurface);  // create ColorByOwner-surface
	bool SetAsClrByOwnerOf(C4Surface *pOfSurface);   // assume that ColorByOwner-surface has been created, and just assign it; fails if the size doesn't match
#ifndef USE_CONSOLE
	bool CreatePrimaryGLTextures();                 // create primary textures from back buffer
#endif
	// Only for surfaces which map to a window
	bool UpdateSize(int wdt, int hgt);
	bool PageFlip(C4Rect *pSrcRt=NULL, C4Rect *pDstRt=NULL);

	void Clear();
	void Default();
	void Clip(int iX, int iY, int iX2, int iY2);
	void NoClip();

	// In C4SurfaceLoaders.cpp
	bool LoadAny(C4Group &hGroup, const char *szFilename, bool fOwnPal=false, bool fNoErrIfNotFound=false);
	bool LoadAny(C4GroupSet &hGroupset, const char *szFilename, bool fOwnPal=false, bool fNoErrIfNotFound=false);
	bool Load(C4Group &hGroup, const char *szFilename, bool fOwnPal=false, bool fNoErrIfNotFound=false);
	bool Save(C4Group &hGroup, const char *szFilename);
	bool SavePNG(C4Group &hGroup, const char *szFilename, bool fSaveAlpha=true, bool fApplyGamma=false, bool fSaveOverlayOnly=false);
	bool SavePNG(const char *szFilename, bool fSaveAlpha, bool fApplyGamma, bool fSaveOverlayOnly);
	bool Read(CStdStream &hGroup, const char * extension);
	bool ReadPNG(CStdStream &hGroup);
	bool ReadJPEG(CStdStream &hGroup);
	bool ReadBMP(CStdStream &hGroup);

	bool AttachPalette();
	bool GetSurfaceSize(int &irX, int &irY); // get surface size
	void SetClr(DWORD toClr) { ClrByOwnerClr=toClr; }
	DWORD GetClr() { return ClrByOwnerClr; }
	bool CopyBytes(BYTE *pImageData); // assumes an array of wdt*hgt*bitdepth/8 and copies data directly from it
private:
	void MapBytes(BYTE *bpMap);
	bool ReadBytes(BYTE **lpbpData, void *bpTarget, int iSize);
	bool CreateTextures(int MaxTextureSize = 0);    // create ppTex-array
	void FreeTextures();      // free ppTex-array if existant
	
	bool GetTexAtImpl(C4TexRef **ppTexRef, int &rX, int &rY);

	friend class C4Draw;
	friend class C4Pattern;
	friend class CStdGL;
};

typedef struct _LOCKED_RECT
{
	int                 Pitch;
	unsigned char *     pBits;
} LOCKED_RECT;

// one texture encapsulation
class C4TexRef
{
public:
	LOCKED_RECT texLock;   // current lock-data
#ifndef USE_CONSOLE
	GLuint texName;
#endif
	int iSizeX;
	int iSizeY;
	bool fIntLock;    // if set, texref is locked internally only
	C4Rect LockSize;

	C4TexRef(int iSizeX, int iSizeY, bool fAsRenderTarget);   // create texture with given size
	~C4TexRef();           // release texture
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
class C4TexMgr
{
public:
	std::list<C4TexRef *> Textures;

public:
	C4TexMgr();    // ctor
	~C4TexMgr();   // dtor

	void RegTex(C4TexRef *pTex);
	void UnregTex(C4TexRef *pTex);

	void IntLock();   // do an internal lock
	void IntUnlock(); // undo internal lock
};

extern C4TexMgr *pTexMgr;

#endif
