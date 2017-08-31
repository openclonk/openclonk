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
// a wrapper class to DirectDraw surfaces

#ifndef INC_StdSurface2
#define INC_StdSurface2

#include "C4ForbidLibraryCompilation.h"
#include "lib/C4Rect.h"

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

const int C4SF_Tileable = 1;
const int C4SF_MipMap   = 2;
const int C4SF_Unlocked = 4;

class C4Surface
{
private:
	C4Surface(const C4Surface &cpy) = delete;
	C4Surface &operator = (const C4Surface &rCpy) = delete;

public:
	C4Surface();
	~C4Surface();
	C4Surface(int iWdt, int iHgt, int iFlags); // create new surface and init it
	C4Surface(C4AbstractApp * pApp, C4Window * pWindow); // create new surface for a window
public:
	int Wdt,Hgt; // size of surface
	int Scale; // scale of image; divide coordinates by this value to get the "original" image size
	int PrimarySurfaceLockPitch; BYTE *PrimarySurfaceLockBits; // lock data if primary surface is locked
	int iTexSize; // size of textures
	int ClipX,ClipY,ClipX2,ClipY2;
	bool fIsBackground{false}; // background surfaces fill unused pixels with black, rather than transparency - must be set prior to loading
#ifdef _DEBUG
	unsigned int dbg_idx;
#endif
#ifndef USE_CONSOLE
	unsigned int Format;                // used color format in textures
	CStdGLCtx * pCtx;
#endif
	std::unique_ptr<C4TexRef> texture;
	C4Surface *pMainSfc;          // main surface for simple ColorByOwner-surfaces
	C4Surface *pNormalSfc;        // normal map; can be nullptr
	DWORD ClrByOwnerClr;          // current color to be used for ColorByOwner-blits

	void MoveFrom(C4Surface *psfcFrom); // grab data from other surface - invalidates other surface
	bool IsRenderTarget();        // surface can be used as a render target?
protected:
	C4Window * pWindow;
	int Locked;
	bool Attached;
	bool fPrimary;

public:
	void SetBackground() { fIsBackground = true; }
	int IsLocked() const { return Locked; }
	// Note: This uses partial locks, anything but SetPixDw and Unlock is undefined afterwards until unlock.
	void ClearBoxDw(int iX, int iY, int iWdt, int iHgt);
	bool Unlock();
	bool Lock();
	DWORD GetPixDw(int iX, int iY, bool fApplyModulation);  // get 32bit-px
	bool IsPixTransparent(int iX, int iY);  // is pixel's alpha value <= 0x7f?
	bool SetPixDw(int iX, int iY, DWORD dwCol);       // set pix in surface only
	bool BltPix(int iX, int iY, C4Surface *sfcSource, int iSrcX, int iSrcY, bool fTransparency); // blit pixel from source to this surface (assumes clipped coordinates!)
	bool Create(int iWdt, int iHgt, int iFlags = 0);
	bool Copy(C4Surface &fromSfc);
	bool CreateColorByOwner(C4Surface *pBySurface);  // create ColorByOwner-surface
	bool SetAsClrByOwnerOf(C4Surface *pOfSurface);   // assume that ColorByOwner-surface has been created, and just assign it; fails if the size doesn't match
#ifndef USE_CONSOLE
	bool CreatePrimaryGLTextures();                 // create primary textures from back buffer
#endif
	// Only for surfaces which map to a window
	bool UpdateSize(int wdt, int hgt);
	bool PageFlip(C4Rect *pSrcRt=nullptr, C4Rect *pDstRt=nullptr);

	void Clear();
	void Default();
	void Clip(int iX, int iY, int iX2, int iY2);
	void NoClip();

	// In C4SurfaceLoaders.cpp
	bool LoadAny(C4Group &hGroup, const char *szFilename, bool fOwnPal, bool fNoErrIfNotFound, int iFlags);
	bool LoadAny(C4GroupSet &hGroupset, const char *szFilename, bool fOwnPal, bool fNoErrIfNotFound, int iFlags);
	bool Load(C4Group &hGroup, const char *szFilename, bool fOwnPal, bool fNoErrIfNotFound, int iFlags);
	bool Save(C4Group &hGroup, const char *szFilename);
	bool SavePNG(C4Group &hGroup, const char *szFilename, bool fSaveAlpha=true, bool fSaveOverlayOnly=false);
	bool SavePNG(const char *szFilename, bool fSaveAlpha, bool fSaveOverlayOnly, bool use_background_thread);
	bool Read(CStdStream &hGroup, const char * extension, int iFlags);
	bool ReadPNG(CStdStream &hGroup, int iFlags);
	bool ReadJPEG(CStdStream &hGroup, int iFlags);
	bool ReadBMP(CStdStream &hGroup, int iFlags);

	bool AttachPalette();
	bool GetSurfaceSize(int &irX, int &irY); // get surface size
	void SetClr(DWORD toClr) { ClrByOwnerClr=toClr; }
	DWORD GetClr() { return ClrByOwnerClr; }
private:
	void MapBytes(BYTE *bpMap);
	bool ReadBytes(BYTE **lpbpData, void *bpTarget, int iSize);
	
	friend class C4Draw;
	friend class C4Pattern;
	friend class CStdGL;
};

typedef struct _LOCKED_RECT
{
	int                                  Pitch;
	std::unique_ptr<unsigned char[]>     pBits;
} LOCKED_RECT;

// one texture encapsulation
class C4TexRef
{
public:
	LOCKED_RECT texLock;   // current lock-data
#ifndef USE_CONSOLE
	unsigned int texName;
#endif
	int iSizeX;
	int iSizeY;
	bool fIntLock;    // if set, texref is locked internally only
	int iFlags;
	C4Rect LockSize;

	C4TexRef(int iSizeX, int iSizeY, int iFlags);   // create texture with given size
	~C4TexRef();           // release texture
	bool Lock();          // lock texture
						  // Lock a part of the rect, discarding the content
						  // Note: Calling Lock afterwards without an Unlock first is undefined
	bool LockForUpdate(C4Rect &rtUpdate);
	void Unlock();        // unlock texture
	bool ClearRect(C4Rect &rtClear); // clear rect in texture to transparent
	bool FillBlack(); // fill complete texture in black
	void SetPix(int iX, int iY, DWORD v)
	{
		*((DWORD *)(((BYTE *)texLock.pBits.get()) + (iY - LockSize.y) * texLock.Pitch + (iX - LockSize.x) * 4)) = v;
	}
private:
	void CreateTexture();
	friend class C4TexMgr;
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
