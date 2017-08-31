/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
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

#include "C4Include.h"
#include "C4ForbidLibraryCompilation.h"
#include "graphics/C4Surface.h"

#include "c4group/CStdFile.h"
#include "graphics/Bitmap256.h"
#include "graphics/C4Draw.h"
#include "graphics/C4DrawGL.h"
#include "graphics/StdPNG.h"
#include "lib/StdColors.h"
#include "platform/C4App.h"
#include "platform/C4Window.h"
#include "platform/StdRegistry.h"

#ifdef HAVE_IO_H
#include <io.h>
#endif

C4Surface::C4Surface()
{
	Default();
}

C4Surface::C4Surface(int iWdt, int iHgt, int iFlags)
{
	Default();
	// create
	Create(iWdt, iHgt, iFlags);
}

C4Surface::C4Surface(C4AbstractApp * pApp, C4Window * pWindow):
		Wdt(0), Hgt(0)
{
	Default();
	fPrimary=true;
	this->pWindow=pWindow;
	// create rendering context
#ifndef USE_CONSOLE
	pCtx = pGL->CreateContext(pWindow, pApp);
#endif
	// reset clipping
	NoClip();
}

C4Surface::~C4Surface()
{
	/*  for (C4ObjectLink *lnk = ::Objects.First; lnk; lnk=lnk->Next)
	    if (lnk->Obj->Menu)
	      lnk->Obj->Menu->AssertSurfaceNotUsed(this);*/
	Clear();
}

void C4Surface::Default()
{
	Wdt=Hgt=0;
	Scale=1;
	PrimarySurfaceLockPitch=0; PrimarySurfaceLockBits=nullptr;
	ClipX=ClipY=ClipX2=ClipY2=0;
	Locked=0;
	Attached=false;
	fPrimary=false;
	pMainSfc=nullptr;
	pNormalSfc=nullptr;
#ifndef USE_CONSOLE
	pCtx=nullptr;
#endif
	pWindow=nullptr;
	ClrByOwnerClr=0;
	iTexSize=0;
	fIsBackground=false;
#ifdef _DEBUG
	dbg_idx = 0;
#endif
}

void C4Surface::MoveFrom(C4Surface *psfcFrom)
{
	// clear own
	Clear();
	// safety
	if (!psfcFrom) return;
	// grab data from other sfc
#ifdef _DEBUG
	dbg_idx = psfcFrom->dbg_idx;
#endif
	Wdt=psfcFrom->Wdt; Hgt=psfcFrom->Hgt;
	PrimarySurfaceLockPitch=psfcFrom->PrimarySurfaceLockPitch;
	PrimarySurfaceLockBits=psfcFrom->PrimarySurfaceLockBits;
	psfcFrom->PrimarySurfaceLockBits=nullptr;
	ClipX=psfcFrom->ClipX; ClipY=psfcFrom->ClipY;
	ClipX2=psfcFrom->ClipX2; ClipY2=psfcFrom->ClipY2;
	Locked=psfcFrom->Locked;
	Attached=psfcFrom->Attached;
	fPrimary=psfcFrom->fPrimary; // shouldn't be true!
	texture = std::move(psfcFrom->texture);
	pMainSfc=psfcFrom->pMainSfc;
	pNormalSfc=psfcFrom->pNormalSfc;
	ClrByOwnerClr=psfcFrom->ClrByOwnerClr;
	iTexSize=psfcFrom->iTexSize;
#ifndef USE_CONSOLE
	Format=psfcFrom->Format;
#endif
	fIsBackground = psfcFrom->fIsBackground;
	// default other sfc
	psfcFrom->Default();
}

void C4Surface::Clear()
{
	// Undo all locks
	while (Locked) Unlock();
	// release surface
#ifndef USE_CONSOLE
	if (pCtx)
	{
		delete pCtx;
		pCtx = nullptr;
	}
#endif
	texture.reset();
#ifdef _DEBUG
	dbg_idx = 0;
#endif
}

bool C4Surface::IsRenderTarget()
{
	// primary is always OK...
	return fPrimary;
}

void C4Surface::NoClip()
{
	ClipX=0; ClipY=0; ClipX2=Wdt-1; ClipY2=Hgt-1;
}

void C4Surface::Clip(int iX, int iY, int iX2, int iY2)
{
	ClipX=Clamp(iX,0,Wdt-1); ClipY=Clamp(iY,0,Hgt-1);
	ClipX2=Clamp(iX2,0,Wdt-1); ClipY2=Clamp(iY2,0,Hgt-1);
}

bool C4Surface::Create(int iWdt, int iHgt, int iFlags)
{
	Clear(); Default();
	// check size
	if (!iWdt || !iHgt) return false;
	Wdt=iWdt; Hgt=iHgt;
	// create texture: check gfx system
	if (!pDraw) return false;
	if (!pDraw->DeviceReady()) return false;

	// store color format that will be used
#ifndef USE_CONSOLE
	Format=pGL->sfcFmt;
#endif
	// create texture
	iTexSize = std::max(iWdt, iHgt);
	texture = std::make_unique<C4TexRef>(iWdt, iHgt, iFlags);
	// update clipping
	NoClip();
	// success
	return true;
}

bool C4Surface::Copy(C4Surface &fromSfc)
{
	// Clear anything old
	Clear();
	// Default to other surface's color depth
	Default();
	// Create surface (TODO: copy flags)
	if (!Create(fromSfc.Wdt, fromSfc.Hgt)) return false;
	// Blit copy
	if (!pDraw->BlitSurface(&fromSfc, this, 0, 0, false))
		{ Clear(); return false; }
	// Success
	return true;
}

#define  RANGE    255
#define  HLSMAX   RANGE
#define  RGBMAX   255

bool ClrByOwner(DWORD &dwClr) // new style, based on Microsoft Knowledge Base Article - 29240
{
	int H,L,S;
	WORD R,G,B;
	BYTE cMax,cMin;
	WORD  Rdelta,Gdelta,Bdelta;
	// get RGB
	R = GetRedValue(dwClr);
	G = GetGreenValue(dwClr);
	B = GetBlueValue(dwClr);
	// calculate lightness
	cMax = std::max<int>(std::max<int>(R,G),B);
	cMin = std::min<int>(std::min<int>(R,G),B);
	L = ( ((cMax+cMin)*HLSMAX) + RGBMAX )/(2*RGBMAX);
	// achromatic case
	if (cMax == cMin)
	{
		S = 0;
		H = (HLSMAX*2/3);
	}
	// chromatic case
	else
	{
		// saturation
		if (L <= (HLSMAX/2))
			S = ( ((cMax-cMin)*HLSMAX) + ((cMax+cMin)/2) ) / (cMax+cMin);
		else
			S = ( ((cMax-cMin)*HLSMAX) + ((2*RGBMAX-cMax-cMin)/2) )
			    / (2*RGBMAX-cMax-cMin);
		// hue
		Rdelta = ( ((cMax-R)*(HLSMAX/6)) + ((cMax-cMin)/2) ) / (cMax-cMin);
		Gdelta = ( ((cMax-G)*(HLSMAX/6)) + ((cMax-cMin)/2) ) / (cMax-cMin);
		Bdelta = ( ((cMax-B)*(HLSMAX/6)) + ((cMax-cMin)/2) ) / (cMax-cMin);
		if (R == cMax)
			H = Bdelta - Gdelta;
		else if (G == cMax)
			H = (HLSMAX/3) + Rdelta - Bdelta;
		else
			H = ((2*HLSMAX)/3) + Gdelta - Rdelta;
		if (H < 0)
			H += HLSMAX;
		if (H > HLSMAX)
			H -= HLSMAX;
	}
	// Not blue
	if (!(Inside(H, 145, 175) && (S > 100))) return false;
	// It's blue: make it gray
	BYTE b = GetBlueValue(dwClr);
	dwClr = RGBA(b, b, b, 0) | (dwClr & 0xff000000);
	return true;
}

bool C4Surface::CreateColorByOwner(C4Surface *pBySurface)
{
	// safety
	if (!pBySurface) return false;
	if (!pBySurface->texture) return false;
	// create in same size
	if (!Create(pBySurface->Wdt, pBySurface->Hgt)) return false;
	// copy scale
	Scale = pBySurface->Scale;
	// set main surface
	pMainSfc=pBySurface;
	// lock it
	if (!pMainSfc->Lock()) return false;
	if (!Lock()) { pMainSfc->Unlock(); return false; }
	// set ColorByOwner-pixels
	for (int iY=0; iY<Hgt; ++iY)
		for (int iX=0; iX<Wdt; ++iX)
		{
			// get pixel
			DWORD dwPix=pMainSfc->GetPixDw(iX, iY, false);
			// is it a ClrByOwner-px?
			if (!ClrByOwner(dwPix)) continue;
			// set in this surface
			SetPixDw(iX, iY, dwPix);
			// clear in the other
			pMainSfc->SetPixDw(iX, iY, 0x00ffffff);
		}
	// unlock
	Unlock();
	pMainSfc->Unlock();
	// success
	return true;
}

bool C4Surface::SetAsClrByOwnerOf(C4Surface *pOfSurface)
{
	// safety
	if (!pOfSurface) return false;
	if (Wdt != pOfSurface->Wdt || Hgt != pOfSurface->Hgt)
		return false;
	// set main surface
	pMainSfc=pOfSurface;
	// success
	return true;
}

bool C4Surface::UpdateSize(int wdt, int hgt)
{
	assert(fPrimary);
	if (!fPrimary)
		return false;
	this->Wdt = wdt; this->Hgt = hgt;
	return true;
}

bool C4Surface::PageFlip(C4Rect *pSrcRt, C4Rect *pDstRt)
{
	assert(fPrimary);
	if (!fPrimary)
		return false;
	// call from gfx thread only!
	if (!pDraw->pApp || !pDraw->pApp->AssertMainThread()) return false;
#ifndef USE_CONSOLE
	return pCtx->PageFlip();
#endif
	return true;
}

bool C4Surface::ReadBMP(CStdStream &hGroup, int iFlags)
{
	int lcnt;
	C4BMP256Info BitmapInfo;
	// read bmpinfo-header
	if (!hGroup.Read(&BitmapInfo,sizeof(C4BMPInfo))) return false;
	// is it 8bpp?
	if (BitmapInfo.Info.biBitCount == 8)
	{
		if (!hGroup.Read(((BYTE *) &BitmapInfo)+sizeof(C4BMPInfo),
		                 std::min(sizeof(BitmapInfo)-sizeof(C4BMPInfo),sizeof(BitmapInfo)-sizeof(C4BMPInfo)+BitmapInfo.FileBitsOffset())))
			return false;
		if (!hGroup.Advance(BitmapInfo.FileBitsOffset())) return false;
	}
	else
	{
		// read 24bpp
		if (BitmapInfo.Info.biBitCount != 24) return false;
		if (!hGroup.Advance(((C4BMPInfo) BitmapInfo).FileBitsOffset())) return false;
	}

	// Create and lock surface
	if (!Create(BitmapInfo.Info.biWidth,BitmapInfo.Info.biHeight, iFlags)) return false;
	if (!Lock()) { Clear(); return false; }

	// create line buffer
	int iBufSize=DWordAligned(BitmapInfo.Info.biWidth*BitmapInfo.Info.biBitCount/8);
	BYTE *pBuf = new BYTE[iBufSize];
	// Read lines
	for (lcnt=Hgt-1; lcnt>=0; lcnt--)
	{
		if (!hGroup.Read(pBuf, iBufSize))
			{ Clear(); delete [] pBuf; return false; }
		BYTE *pPix=pBuf;
		for (int x=0; x<BitmapInfo.Info.biWidth; ++x)
			switch (BitmapInfo.Info.biBitCount)
			{
			case 8:
				SetPixDw(x, lcnt, C4RGB(
				         BitmapInfo.Colors[*pPix].rgbRed,
				         BitmapInfo.Colors[*pPix].rgbGreen,
				         BitmapInfo.Colors[*pPix].rgbBlue));
				++pPix;
				break;
			case 24:
				SetPixDw(x, lcnt, C4RGB(pPix[0], pPix[1], pPix[2]));
				pPix+=3;
				break;
			}
	}
	// free buffer again
	delete [] pBuf;

	Unlock();

	return true;
}

bool C4Surface::SavePNG(const char *szFilename, bool fSaveAlpha, bool fSaveOverlayOnly, bool use_background_thread)
{
	// Lock - WARNING - maybe locking primary surface here...
	if (!Lock()) return false;

	// create png file
	std::unique_ptr<CPNGFile> png(new CPNGFile());
	if (!png->Create(Wdt, Hgt, fSaveAlpha)) { Unlock(); return false; }

	// reset overlay if desired
	C4Surface *pMainSfcBackup = nullptr;
	if (fSaveOverlayOnly) { pMainSfcBackup=pMainSfc; pMainSfc=nullptr; }

#ifndef USE_CONSOLE
	if (fPrimary)
	{
		// Take shortcut. FIXME: Check Endian
		for (int y = 0; y < Hgt; ++y)
			glReadPixels(0, Hgt - y - 1, Wdt, 1, fSaveAlpha ? GL_BGRA : GL_BGR, GL_UNSIGNED_BYTE, png->GetRow(y));
	}
	else
#endif
	{
		// write pixel values
		for (int y=0; y<Hgt; ++y)
			for (int x=0; x<Wdt; ++x)
			{
				DWORD dwClr = GetPixDw(x, y, false);
				png->SetPix(x, y, dwClr);
			}
	}

	// reset overlay
	if (fSaveOverlayOnly) pMainSfc=pMainSfcBackup;

	// Unlock
	Unlock();

	// save png - either directly or delayed in a background thread if desired
	if (use_background_thread)
	{
		CPNGFile::ScheduleSaving(png.release(), szFilename);
	}
	else
	{
		if (!png->Save(szFilename)) return false;
	}

	// Success
	return true;
}


bool C4Surface::AttachPalette()
{
	return true;
}

double ColorDistance(BYTE *bpRGB1, BYTE *bpRGB2)
{
	return (double) (Abs(bpRGB1[0]-bpRGB2[0]) + Abs(bpRGB1[1]-bpRGB2[1]) + Abs(bpRGB1[2]-bpRGB2[2])) / 6.0;
}

bool C4Surface::GetSurfaceSize(int &irX, int &irY)
{
	// simply assign stored values
	irX=Wdt;
	irY=Hgt;
	// success
	return true;
}

bool C4Surface::Lock()
{
	// lock main sfc
	if (pMainSfc) if (!pMainSfc->Lock()) return false;
	// lock texture
	if (!Locked && !fPrimary && !texture)
		return false;
	// count lock
	Locked++; return true;
}

bool C4Surface::Unlock()
{
	// unlock main sfc
	if (pMainSfc) pMainSfc->Unlock();
	// locked?
	if (!Locked) return false;
	// decrease lock counter; check if zeroed and unlock then
	Locked--;
	if (!Locked)
	{
		if (fPrimary)
		{
			// emulated primary locks in OpenGL
			delete[] PrimarySurfaceLockBits;
			PrimarySurfaceLockBits = nullptr;
			return true;
		}
		else
		{
			// non-primary unlock: unlock all texture surfaces (if locked)
			if (texture)
				texture->Unlock();
		}
	}
	return true;
}

DWORD C4Surface::GetPixDw(int iX, int iY, bool fApplyModulation)
{
	BYTE *pBuf = nullptr; int iPitch = 0; // TODO: are those initialised to something sensible?
	// backup pos
	int iX2=iX; int iY2=iY;
	// primary?
	if (fPrimary)
	{
#ifndef USE_CONSOLE
		if (!PrimarySurfaceLockBits)
		{
			PrimarySurfaceLockBits = new unsigned char[Wdt*Hgt*3];
			glPixelStorei(GL_PACK_ALIGNMENT, 1);
			glReadPixels( 0, 0, Wdt, Hgt, GL_BGR, GL_UNSIGNED_BYTE, PrimarySurfaceLockBits);
			PrimarySurfaceLockPitch = Wdt*3;
		}
		return * (DWORD *) (PrimarySurfaceLockBits+(Hgt-iY-1)*PrimarySurfaceLockPitch+iX*3);
#endif
	}
	else
	{
		// get+lock affected texture
		if (!texture) return 0;
		texture->Lock();
		pBuf=(BYTE *) texture->texLock.pBits.get();
		iPitch=texture->texLock.Pitch;
	}
	// get pix of surface
	DWORD dwPix;
	DWORD *pPix=(DWORD *) (pBuf+iY*iPitch+iX*4);
	dwPix = *pPix;
	// this is a ColorByOwner-surface?
	if (pMainSfc)
	{
		BYTE byAlpha=BYTE(dwPix>>24);
		// pix is fully transparent?
		if (byAlpha==0x00)
			// then get the main surfaces's pixel
			dwPix = pMainSfc->GetPixDw(iX2, iY2, fApplyModulation);
		else
		{
			// otherwise, it's a ColorByOwner-pixel: adjust the color
			if (fApplyModulation)
			{
				if (pDraw->dwBlitMode & C4GFXBLIT_CLRSFC_MOD2)
					ModulateClrMOD2(dwPix, ClrByOwnerClr);
				else
					ModulateClr(dwPix, ClrByOwnerClr);
				if (pDraw->BlitModulated && !(pDraw->dwBlitMode & C4GFXBLIT_CLRSFC_OWNCLR))
					ModulateClr(dwPix, pDraw->BlitModulateClr);
			}
			else
				ModulateClr(dwPix, ClrByOwnerClr);
			// does it contain transparency? then blit on main sfc
			if (byAlpha)
			{
				DWORD dwMainPix = pMainSfc->GetPixDw(iX2, iY2, fApplyModulation);
				BltAlpha(dwMainPix, dwPix); dwPix=dwMainPix;
			}
		}
	}
	else
	{
		// single main surface
		// apply color modulation if desired
		if (fApplyModulation && pDraw->BlitModulated)
		{
			if (pDraw->dwBlitMode & C4GFXBLIT_MOD2)
				ModulateClrMOD2(dwPix, pDraw->BlitModulateClr);
			else
				ModulateClr(dwPix, pDraw->BlitModulateClr);
		}
	}
	// return pixel value
	return dwPix;
}

bool C4Surface::IsPixTransparent(int iX, int iY)
{
	// get pixel value
	DWORD dwPix=GetPixDw(iX, iY, false);
	// get alpha value
	return (dwPix>>24) < 128;
}

bool C4Surface::SetPixDw(int iX, int iY, DWORD dwClr)
{
	// clip
	if ((iX<ClipX) || (iX>ClipX2) || (iY<ClipY) || (iY>ClipY2)) return true;
	// get+lock affected texture
	if (!texture) return false;
	texture->Lock();
	// if color is fully transparent, ensure it's black
	if (dwClr>>24 == 0x00) dwClr=0x00000000;
	// ...and set in actual surface
	texture->SetPix(iX, iY, dwClr);
	// success
	return true;
}

bool C4Surface::BltPix(int iX, int iY, C4Surface *sfcSource, int iSrcX, int iSrcY, bool fTransparency)
{
	// 32bit-blit. lock target
	if (!texture) return false;
	texture->Lock();
	DWORD *pPix32 = (DWORD *)(((BYTE *)texture->texLock.pBits.get()) + iY*texture->texLock.Pitch + iX * 4);
	// get source pix as dword
	DWORD srcPix=sfcSource->GetPixDw(iSrcX, iSrcY, true);
	// merge
	if (!fTransparency)
	{
		// set it
		*pPix32=srcPix;
	}
	else
	{
		if (pDraw->dwBlitMode & C4GFXBLIT_ADDITIVE)
			BltAlphaAdd(*pPix32, srcPix);
		else
			BltAlpha(*pPix32, srcPix);
	}
	// done
	return true;
}

void C4Surface::ClearBoxDw(int iX, int iY, int iWdt, int iHgt)
{
	// lock
	if (!Locked) return;
	// clip to target size
	if (iX<0) { iWdt+=iX; iX=0; }
	if (iY<0) { iHgt+=iY; iY=0; }
	int iOver;
	iOver=Wdt-(iX+iWdt); if (iOver<0) iWdt+=iOver;
	iOver=Hgt-(iY+iHgt); if (iOver<0) iHgt+=iOver;
	C4Rect rtClear{ iX, iY, iWdt, iHgt };
	if (pMainSfc && pMainSfc->texture)
	{
		// assuming this is only valid as long as there's no texture management,
		// organizing partially used textures together!
		pMainSfc->texture->ClearRect(rtClear);
	}
	// clear this texture
	texture->ClearRect(rtClear);
}

C4TexRef::C4TexRef(int iSizeX, int iSizeY, int iFlags)
{
	// zero fields
#ifndef USE_CONSOLE
	texName = 0;
#endif
	texLock.pBits.reset(); fIntLock=false;
	// store size
	this->iSizeX=iSizeX;
	this->iSizeY=iSizeY;
	this->iFlags=iFlags;
	// add to texture manager
	if (!pTexMgr) pTexMgr = new C4TexMgr();
	pTexMgr->RegTex(this);
	// create texture: check ddraw
	if (!pDraw) return;
	if (!pDraw->DeviceReady()) return;
	// create it!
	// Reserve video memory
	CreateTexture();

	if ((iFlags & C4SF_Unlocked) == 0 && pDraw)
	{
		texLock.pBits = std::make_unique<unsigned char[]>(iSizeX * iSizeY * C4Draw::COLOR_DEPTH_BYTES);
		texLock.Pitch = iSizeX * C4Draw::COLOR_DEPTH_BYTES;
		memset(texLock.pBits.get(), 0x00, texLock.Pitch*iSizeY);
		// Always locked
		LockSize.x = LockSize.y = 0;
		LockSize.Wdt = iSizeX; LockSize.Hgt = iSizeY;
	}
}

C4TexRef::~C4TexRef()
{
	fIntLock=false;
	// free texture
#ifndef USE_CONSOLE
	if (pGL && pGL->pCurrCtx) glDeleteTextures(1, &texName);
#endif
	if (pDraw) texLock.pBits = nullptr;
	// remove from texture manager
	pTexMgr->UnregTex(this);
}

void C4TexRef::CreateTexture()
{
#ifndef USE_CONSOLE
	assert(texName == 0);

	const bool fTileable = (iFlags & C4SF_Tileable) != 0;
	const bool fMipMap = (iFlags & C4SF_MipMap) != 0;

	glGenTextures(1, &texName);
	glBindTexture(GL_TEXTURE_2D, texName);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, fTileable ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, fTileable ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, fMipMap ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, iSizeX, iSizeY, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, nullptr);
	if (fMipMap) glGenerateMipmap(GL_TEXTURE_2D);
#endif
}

bool C4TexRef::LockForUpdate(C4Rect & rtUpdate)
{
	// already locked?
	if (texLock.pBits)
	{
		// fully locked
		if (LockSize.x == 0 && LockSize.Wdt == iSizeX && LockSize.y == 0 && LockSize.Hgt == iSizeY)
		{
			return true;
		}
		else
		{
			// Commit previous changes to the texture
			Unlock();
		}
	}
	// lock
#ifndef USE_CONSOLE
	// prepare texture data
	texLock.pBits = std::make_unique<unsigned char[]>(rtUpdate.Wdt * rtUpdate.Hgt * C4Draw::COLOR_DEPTH_BYTES);
	texLock.Pitch = rtUpdate.Wdt * C4Draw::COLOR_DEPTH_BYTES;
	LockSize = rtUpdate;
	return true;
#endif
	// failure
	return false;
}

bool C4TexRef::Lock()
{
	// already locked?
	if (texLock.pBits) return true;
	LockSize.Wdt = iSizeX; LockSize.Hgt = iSizeY;
	LockSize.x = LockSize.y = 0;
	// lock
#ifndef USE_CONSOLE
			if (texName)
			{
				if (!pGL->pCurrCtx) return false;
				// get texture
				texLock.pBits = std::make_unique<unsigned char[]>(iSizeX * iSizeY * C4Draw::COLOR_DEPTH_BYTES);
				texLock.Pitch = iSizeX * C4Draw::COLOR_DEPTH_BYTES;
				glBindTexture(GL_TEXTURE_2D, texName);
				glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, texLock.pBits.get());
				return true;
			}
#endif
		{
			// nothing to do
		}
	// failure
	return false;
}

void C4TexRef::Unlock()
{
	// locked?
	if (!texLock.pBits || fIntLock) return;
#ifndef USE_CONSOLE
			if (!pGL->pCurrCtx)
			{
//      BREAKPOINT_HERE;
				assert(pGL->pMainCtx);
				pGL->pMainCtx->Select();
			}

			const bool fTileable = (iFlags & C4SF_Tileable) != 0;
			const bool fMipMap = (iFlags & C4SF_MipMap) != 0;

			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

			// reuse the existing texture
			glBindTexture(GL_TEXTURE_2D, texName);
			glTexSubImage2D(GL_TEXTURE_2D, 0,
			                LockSize.x, LockSize.y, LockSize.Wdt, LockSize.Hgt,
			                GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, texLock.pBits.get());

			texLock.pBits.reset();
			if (fMipMap) glGenerateMipmap(GL_TEXTURE_2D);
#endif
}

bool C4TexRef::ClearRect(C4Rect &rtClear)
{
	// ensure locked
	if (!LockForUpdate(rtClear)) return false;
	// clear pixels
	int y;
	for (y = rtClear.y; y < rtClear.y + rtClear.Hgt; ++y)
	{
		for (int x = rtClear.x; x < rtClear.x + rtClear.Wdt; ++x)
			SetPix(x, y, 0x00000000);
	}
	// success
	return true;
}

bool C4TexRef::FillBlack()
{
	// ensure locked
	if (!Lock()) return false;
	// clear pixels
	int y;
	for (y=0; y<iSizeY; ++y)
	{
		for (int x = 0; x < iSizeX; ++x)
			SetPix(x, y, 0xff000000);
	}
	// success
	return true;
}

// texture manager

C4TexMgr::C4TexMgr()
{
	// clear textures
	Textures.clear();
}

C4TexMgr::~C4TexMgr()
{
	// unlock all textures
	IntUnlock();
}

void C4TexMgr::RegTex(C4TexRef *pTex)
{
	// add texture to list
	Textures.push_front(pTex);
}

void C4TexMgr::UnregTex(C4TexRef *pTex)
{
	// remove texture from list
	Textures.remove(pTex);
	// if list is empty, remove self
	if (Textures.empty()) { delete this; pTexMgr=nullptr; }
}

void C4TexMgr::IntLock()
{
	// lock all textures
	int j=Textures.size();
	for (std::list<C4TexRef *>::iterator i=Textures.begin(); j--; ++i)
	{
		C4TexRef *pRef = *i;
		if (pRef->Lock() && pRef->texLock.pBits)
		{
			pRef->fIntLock = true;
#ifndef USE_CONSOLE
			// Release the underlying texture with GL and recreate
			// it on unlock, so that the texture survives
			// context recreation.
			if(pGL)
			{
				glDeleteTextures(1, &pRef->texName);
				pRef->texName = 0;
			}
#endif
		}
	}
}

void C4TexMgr::IntUnlock()
{
	// unlock all internally locked textures
	int j=Textures.size();
	for (std::list<C4TexRef *>::iterator i=Textures.begin(); j--; ++i)
	{
		C4TexRef *pRef = *i;
		if (pRef->fIntLock)
		{
			pRef->fIntLock = false;
			pRef->CreateTexture();
			pRef->Unlock();
		}
	}
}

C4TexMgr *pTexMgr;
