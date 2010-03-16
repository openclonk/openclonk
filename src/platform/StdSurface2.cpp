/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2002, 2004-2008  Sven Eberhardt
 * Copyright (c) 2003, 2008  Matthes Bender
 * Copyright (c) 2005  Peter Wortmann
 * Copyright (c) 2005, 2007-2008  Günther Brammer
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

#include "C4Include.h"
#include <StdFile.h>
#include <CStdFile.h>
#include <StdGL.h>
#include <StdWindow.h>
#include <StdRegistry.h>
#include <StdResStr.h>
#include <StdConfig.h>
#include <StdSurface2.h>
#include <StdFacet.h>
#include <StdDDraw2.h>
#include <StdD3D.h>
#include <Bitmap256.h>
#include <StdPNG.h>


#ifdef HAVE_IO_H
#include <io.h>
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <list>

CDDrawCfg DDrawCfg; // ddraw config

CSurface::CSurface() : fIsBackground(false)
	{
	Default();
	}

CSurface::CSurface(int iWdt, int iHgt) : fIsBackground(false)
	{
	Default();
	// create
	Create(iWdt, iHgt);
	}

CSurface::CSurface(CStdApp * pApp, CStdWindow * pWindow):
	Wdt(0), Hgt(0)
	{
	Default();
	fPrimary=true;
	this->pWindow=pWindow;
	// create rendering context
#ifdef USE_GL
	if (pGL)
		pCtx = pGL->CreateContext(pWindow, pApp);
#endif
	// reset clipping
	NoClip();
	}

CSurface::~CSurface()
	{
	Clear();
	}

void CSurface::Default()
	{
	Wdt=Hgt=0;
	Scale=1;
	PrimarySurfaceLockPitch=0; PrimarySurfaceLockBits=NULL;
	ClipX=ClipY=ClipX2=ClipY2=0;
	Locked=0;
	Attached=false;
	fPrimary=false;
#ifdef USE_DIRECTX
	pSfc=NULL;
#endif
	ppTex=NULL;
	pMainSfc=NULL;
	pCtx=NULL;
	pWindow=NULL;
	ClrByOwnerClr=0;
	iTexSize=iTexX=iTexY=0;
	fIsRenderTarget=false;
	fIsBackground=false;
#ifdef _DEBUG
	dbg_idx = NULL;
#endif
	}

void CSurface::MoveFrom(CSurface *psfcFrom)
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
	psfcFrom->PrimarySurfaceLockBits=NULL;
	ClipX=psfcFrom->ClipX; ClipY=psfcFrom->ClipY;
	ClipX2=psfcFrom->ClipX2; ClipY2=psfcFrom->ClipY2;
	Locked=psfcFrom->Locked;
	Attached=psfcFrom->Attached;
	fPrimary=psfcFrom->fPrimary; // shouldn't be true!
	ppTex=psfcFrom->ppTex;
	pMainSfc=psfcFrom->pMainSfc;
	ClrByOwnerClr=psfcFrom->ClrByOwnerClr;
	iTexSize=psfcFrom->iTexSize;
	iTexX=psfcFrom->iTexX; iTexY=psfcFrom->iTexY;
	byBytesPP=psfcFrom->byBytesPP;
#ifdef USE_DIRECTX
	dwClrFormat=psfcFrom->dwClrFormat;
	pSfc=psfcFrom->pSfc;
#endif
#ifdef USE_GL
	Format=psfcFrom->Format;
#endif
	fIsBackground = psfcFrom->fIsBackground;
	// default other sfc
	psfcFrom->Default();
	}

void CSurface::Clear()
	{
	// Undo all locks
	while (Locked) Unlock();
	// release surface
#ifdef USE_DIRECTX
	if (pD3D)
		{
		if (pSfc) pSfc->Release();
		}
	pSfc=NULL;
#endif
#ifdef USE_GL
	if (pCtx)
		{
		delete pCtx;
		pCtx = 0;
		}
#endif
	FreeTextures();
	ppTex=NULL;
#ifdef _DEBUG
	delete dbg_idx;
	dbg_idx = NULL;
#endif
	}

bool CSurface::IsRenderTarget()
	{
	// primary is always OK...
	return fPrimary
	// other surfaces may be used as render targets, if offscreen rendertargets are not disabled by config,
	//  or the surface is split (large sfcs) or locked (landscape)
	//  (only D3D for now)
#ifdef USE_DIRECTX
		|| (!Locked && !DDrawCfg.NoOffscreenBlits && pD3D && fIsRenderTarget)
#endif
	;
	}

bool CSurface::Box(int iX, int iY, int iX2, int iY2, int iCol)
	{
	if (!Lock()) return false;
	for (int cy=iY; cy<=iY2; cy++) HLine(iX,iX2,cy,iCol);
	Unlock();
	return true;
	}

void CSurface::NoClip()
	{
	ClipX=0; ClipY=0; ClipX2=Wdt-1; ClipY2=Hgt-1;
	}

void CSurface::Clip(int iX, int iY, int iX2, int iY2)
	{
	ClipX=BoundBy(iX,0,Wdt-1); ClipY=BoundBy(iY,0,Hgt-1);
	ClipX2=BoundBy(iX2,0,Wdt-1); ClipY2=BoundBy(iY2,0,Hgt-1);
	}

bool CSurface::HLine(int iX, int iX2, int iY, int iCol)
	{
	if (!Lock()) return false;
	for (int cx=iX; cx<=iX2; cx++) SetPix(cx,iY,iCol);
	Unlock();
	return true;
	}

bool CSurface::Create(int iWdt, int iHgt, bool fOwnPal, bool fIsRenderTarget, int MaxTextureSize)
	{
	Clear(); Default();
	// check size
	if (!iWdt || !iHgt) return false;
	Wdt=iWdt; Hgt=iHgt;
	// create texture: check gfx system
	if (!lpDDraw) return false;
	if (!lpDDraw->DeviceReady()) return false;

	// store color format that will be used
#ifdef USE_DIRECTX
	if (pD3D)
		dwClrFormat=pD3D->dwSurfaceType;
	else
#endif
#ifdef USE_GL
	if (pGL)
		Format=pGL->sfcFmt;
	else
#endif
		{/* nothing to do */}
	byBytesPP=lpDDraw->byByteCnt;
	this->fIsRenderTarget = fIsRenderTarget;
	// create textures
	if (!CreateTextures(MaxTextureSize)) { Clear(); return false; }
	// update clipping
	NoClip();
	// success
	return true;
	}

bool CSurface::CreateTextures(int MaxTextureSize)
	{
	// free previous
	FreeTextures();
	// get needed tex size - begin with smaller value of wdt/hgt, so there won't be too much space wasted
	int iNeedSize=Min(Wdt, Hgt);
#ifdef USE_GL
	if (!pGL || !GLEW_ARB_texture_non_power_of_two)
#endif
		{
		int n=0;
		while ((1<<++n) < iNeedSize) {}
		iNeedSize = 1<<n;
		}
	// adjust to available texture size
	iTexSize=Min(iNeedSize, lpDDraw->MaxTexSize);
	if (MaxTextureSize)
		iTexSize=Min(iTexSize, MaxTextureSize);
	// get the number of textures needed for this size
	iTexX=(Wdt-1)/iTexSize +1;
	iTexY=(Hgt-1)/iTexSize +1;
	// get mem for texture array
	ppTex = new CTexRef * [iTexX*iTexY];
	ZeroMemory(ppTex, iTexX*iTexY*sizeof(CTexRef *));
	// cvan't be render target if it's not a single surface
	if (!IsSingleSurface()) fIsRenderTarget = false;
	// create textures
	CTexRef **ppCTex=ppTex;
	for (int i=iTexX*iTexY; i; --i,++ppCTex)
		{
		// regular textures or if last texture fits exactly into the space by Wdt or Hgt
		if (i-1 || !(Wdt%iTexSize) || !(Hgt%iTexSize))
			*ppCTex = new CTexRef(iTexSize, fIsRenderTarget);
		else
			{
			// last texture might be smaller
			iNeedSize=Max(Wdt%iTexSize, Hgt%iTexSize);
#ifdef USE_GL
			if (!pGL || !GLEW_ARB_texture_non_power_of_two)
#endif
				{
				int n=0;
				while ((1<<++n) < iNeedSize) {}
				iNeedSize=1<<n;
				}
			*ppCTex = new CTexRef(iNeedSize, fIsRenderTarget);
			}
		if (fIsBackground && ppCTex) (*ppCTex)->FillBlack();
#ifdef USE_DIRECTX
		if (!(*ppCTex)->pTex && pD3D)
			{
			// error creating texture
			return false;
			}
#endif
		}
#ifdef _DEBUG
	static int dbg_counter = 0;
	dbg_idx = new int;
	*dbg_idx = dbg_counter++;
#endif
	// success
	return true;
	}

void CSurface::FreeTextures()
	{
	if (ppTex)
		{
		// clear all textures
		CTexRef **ppTx=ppTex;
		for (int i=0; i<iTexX*iTexY; ++i,++ppTx)
			if (*ppTx) delete *ppTx;
		// clear texture list
		delete [] ppTex;
		ppTex=NULL;
		}
	}

/*bool ClrByOwner(DWORD &rClr) old style...
	{
	// red value must be approx. same to green
	BYTE byR=GetBValue(rClr), byG=GetGValue(rClr);
	int diff=Abs(byR-byG);
	if (diff>byR/8) return false;
	// get blue value; mustn't be 0 or equal to R/G (grey)
	BYTE byB=GetRValue(rClr); if (!byB || Inside(byB, Min(byR, byG), Max(byR, byG))) return false;
	// medium r/g and blue is very close (additional gray shade check)
	if ((byR > 50) && (Abs(byB - byR) < 20)) return false;
	// if blue is not fully lit, red and green should be very low
	if (byB<240 && byR>15) return false;
	// so, the color seems to be truly blue-ish
	rClr=RGB(byB, byB, byB) | (rClr&0xff000000);
	return true;
	}*/

#define	 RANGE    255
#define  HLSMAX   RANGE
#define  RGBMAX   255

bool ClrByOwner(DWORD &dwClr) // new style, based on Microsoft Knowledge Base Article - 29240
{
  int H,L,S;
  WORD R,G,B;
  BYTE cMax,cMin;
  WORD  Rdelta,Gdelta,Bdelta;
  // get RGB (from BGR...?)
  R = GetBValue(dwClr);
  G = GetGValue(dwClr);
  B = GetRValue(dwClr);
  // calculate lightness
  cMax = Max<int>(Max<int>(R,G),B);
  cMin = Min<int>(Min<int>(R,G),B);
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
	BYTE b = GetRValue(dwClr);
	dwClr = RGB(b, b, b) | (dwClr & 0xff000000);
	return true;
}

bool CSurface::CreateColorByOwner(CSurface *pBySurface)
	{
	// safety
	if (!pBySurface) return false;
	if (!pBySurface->ppTex) return false;
	// create in same size
	if (!Create(pBySurface->Wdt, pBySurface->Hgt, false)) return false;
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

bool CSurface::SetAsClrByOwnerOf(CSurface *pOfSurface)
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

#ifdef USE_GL
/*bool CSurface::CreatePrimaryGLTextures()
	{
	if (!pGL) return false;
	// primary OpenGL-surface: ensure context is selected
	if (!pGL->pCurrCtx) if (!pGL->MainCtx.Select()) return false;
	// create texture array
	CreateTextures();
	// get from framebuffer
	CTexRef **ppTexRef = ppTex;
	for (int iY=0; iY<Hgt; iY+=iTexSize)
		for (int iX=0; iX<Wdt; iX+=iTexSize)
			{
			// get tex size
			int txWdt=Min(Wdt-iX, iTexSize), txHgt=Min(Hgt-iY, iTexSize);
			// copy surface into texture
			glBindTexture(GL_TEXTURE_2D, (*ppTexRef)->texName);
			glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, iX, iY, txWdt, txHgt, 0);
			// next texture reference
			++ppTexRef;
			}
	// done, success
	return true;
	}*/
#endif

bool CSurface::UpdateSize(int wdt, int hgt)
	{
	assert(fPrimary);
	if(!fPrimary)
		return false;
	this->Wdt = wdt; this->Hgt = hgt;
	return true;
	}

bool CSurface::PageFlip(RECT *pSrcRt, RECT *pDstRt)
	{
	assert(fPrimary);
	if(!fPrimary)
		return false;
	// call from gfx thread only!
	if (!lpDDraw->pApp || !lpDDraw->pApp->AssertMainThread()) return false;
#ifdef USE_GL
	if(pGL)
		return pCtx->PageFlip();
#endif
#ifdef USE_DIRECTX
	if (pD3D)
		return pD3D->PageFlip(pSrcRt, pDstRt);
#endif
	return true;
	}

#ifdef USE_DIRECTX
IDirect3DSurface9 *CSurface::GetSurface()
	{
	// direct surface?
	if (pSfc)
		{
		pSfc->AddRef();
		return pSfc;
		}
	// surface by texture?
	if (fIsRenderTarget && ppTex)
		{
		IDirect3DTexture9 *pTex = (*ppTex)->pTex;
		IDirect3DSurface9 *pSfcResult=NULL;
		if (pTex) pTex->GetSurfaceLevel(0, &pSfcResult);
		return pSfcResult;
		}
	// split surfaces: Won't work; we're no render target anyway
	return NULL;
	}
#endif //USE_DIRECTX

bool CSurface::ReadBMP(CStdStream &hGroup, bool fOwnPal)
	{
	int lcnt,iLineRest;
	CBitmap256Info BitmapInfo;
	// read bmpinfo-header
	if (!hGroup.Read(&BitmapInfo,sizeof(CBitmapInfo))) return false;
	// is it 8bpp?
	if (BitmapInfo.Info.biBitCount == 8)
		{
		if (!hGroup.Read(((BYTE *) &BitmapInfo)+sizeof(CBitmapInfo),
			Min(sizeof(BitmapInfo)-sizeof(CBitmapInfo),sizeof(BitmapInfo)-sizeof(CBitmapInfo)+BitmapInfo.FileBitsOffset())))
			return false;
		if (!hGroup.Advance(BitmapInfo.FileBitsOffset())) return false;
		}
	else
		{
		// read 24bpp
		if (BitmapInfo.Info.biBitCount != 24) return false;
		if (!hGroup.Advance(((CBitmapInfo) BitmapInfo).FileBitsOffset())) return false;
		}

	// Create and lock surface
	if (!Create(BitmapInfo.Info.biWidth,BitmapInfo.Info.biHeight, fOwnPal)) return false;
	if (!Lock()) { Clear(); return false; }

	// create line buffer
	int iBufSize=DWordAligned(BitmapInfo.Info.biWidth*BitmapInfo.Info.biBitCount/8);
	BYTE *pBuf = new BYTE[iBufSize];
	// Read lines
	iLineRest = DWordAligned(BitmapInfo.Info.biWidth) - BitmapInfo.Info.biWidth;
	for (lcnt=Hgt-1; lcnt>=0; lcnt--)
		{
		if (!hGroup.Read(pBuf, iBufSize))
			{ Clear(); delete [] pBuf; return false; }
		BYTE *pPix=pBuf;
		for (int x=0; x<BitmapInfo.Info.biWidth; ++x)
			switch (BitmapInfo.Info.biBitCount)
			{
			case 8:
				if (fOwnPal)
					SetPixDw(x, lcnt, C4RGB(
						BitmapInfo.Colors[*pPix].rgbRed,
						BitmapInfo.Colors[*pPix].rgbGreen,
						BitmapInfo.Colors[*pPix].rgbBlue));
				else
					SetPix(x, lcnt, *pPix);
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

/*bool CSurface::Save(const char *szFilename)
	{
	CBitmapInfo BitmapInfo2;
	CBitmap256Info BitmapInfo;
	// Set bitmap info
	if (fPrimary)
		{
		if (byBytesPP==4)
			BitmapInfo2.Set(Wdt,Hgt,32);
		else
			BitmapInfo2.Set(Wdt,Hgt,16);
		}
	else
		BitmapInfo.Set(Wdt,Hgt,pPal->Colors);

	// Lock - WARNING - maybe locking primary surface here...
	if (!Lock()) return false;

	// Create file & write info
	CStdFile hFile;

	if(fPrimary)
		{
		if ( !hFile.Create(szFilename)
		|| !hFile.Write(&BitmapInfo2,sizeof(BitmapInfo2)) )
			{ Unlock(); return false; }

		// write lines
		char bpEmpty[4]; int iEmpty = DWordAligned(Wdt*byBytesPP)-Wdt*byBytesPP;
		for (int cnt=Hgt-1; cnt>=0; cnt--)
			{
			if (!hFile.Write(Bits+(Pitch*cnt),Wdt*byBytesPP))
				{ Unlock(); return false; }
			if (iEmpty)
				if (!hFile.Write(bpEmpty,iEmpty))
					{ Unlock(); return false; }
			}

		}
	else
		{
		if ( !hFile.Create(szFilename)
		|| !hFile.Write(&BitmapInfo,sizeof(BitmapInfo)) )
			{ Unlock(); return false; }

		// Write lines
		char bpEmpty[4]; int iEmpty = DWordAligned(Wdt)-Wdt;
		for (int cnt=Hgt-1; cnt>=0; cnt--)
			{
			if (!hFile.Write(Bits+(Pitch*cnt),Wdt))
				{ Unlock(); return false; }
			if (iEmpty)
				if (!hFile.Write(bpEmpty,iEmpty))
					{ Unlock(); return false; }
			}
		}

	// Close file
  hFile.Close();

	// Unlock
	Unlock();

	// Success
  return true;
	}
*/
bool CSurface::SavePNG(const char *szFilename, bool fSaveAlpha, bool fApplyGamma, bool fSaveOverlayOnly)
	{
	// Lock - WARNING - maybe locking primary surface here...
	if (!Lock()) return false;

	// create png file
	CPNGFile png;
	if (!png.Create(Wdt, Hgt, fSaveAlpha)) { Unlock(); return false; }

	// reset overlay if desired
	CSurface *pMainSfcBackup = NULL;
	if (fSaveOverlayOnly) { pMainSfcBackup=pMainSfc; pMainSfc=NULL; }

#ifdef USE_GL
	if (fPrimary && pGL)
		{
		// Take shortcut. FIXME: Check Endian
		for (int y = 0; y < Hgt; ++y)
			glReadPixels(0, Hgt - y, Wdt, 1, fSaveAlpha ? GL_BGRA : GL_BGR, GL_UNSIGNED_BYTE, png.GetImageData() + y * Wdt * (3 + fSaveAlpha));
		}
	else
#endif
		{
		// write pixel values
		for (int y=0; y<Hgt; ++y)
			for (int x=0; x<Wdt; ++x)
				{
				DWORD dwClr = GetPixDw(x, y, false);
				if (fApplyGamma) dwClr = lpDDraw->Gamma.ApplyTo(dwClr);
				png.SetPix(x, y, dwClr);
				}
		}

	// reset overlay
	if (fSaveOverlayOnly) pMainSfc=pMainSfcBackup;

	// save png
	if (!png.Save(szFilename)) { Unlock(); return false; }

	// Unlock
	Unlock();

	// Success
  return true;
	}


bool CSurface::AttachPalette()
	{
	return true;
	}

double ColorDistance(BYTE *bpRGB1, BYTE *bpRGB2)
	{
	return (double) (Abs(bpRGB1[0]-bpRGB2[0]) + Abs(bpRGB1[1]-bpRGB2[1]) + Abs(bpRGB1[2]-bpRGB2[2])) / 6.0;
	}

bool CSurface::Wipe()
	{
	if (!ppTex) return false;
	// simply clear it (currently slow...)
	if (!Lock()) return false;
	for (int i=0; i<Wdt*Hgt; ++i)
		if (!fIsBackground)
			SetPix(i%Wdt, i/Wdt, 0);
		else
			SetPixDw(i%Wdt, i/Wdt, 0xff000000);
	Unlock();
	// success
	return true;
	}

bool CSurface::GetSurfaceSize(int &irX, int &irY)
	{
	// simply assign stored values
	irX=Wdt;
	irY=Hgt;
	// success
	return true;
	}

bool CSurface::Lock()
	{
	// lock main sfc
	if (pMainSfc) if (!pMainSfc->Lock()) return false;
	// not yet locked?
	if (!Locked)
		{
		if (fPrimary)
			{
#ifdef USE_DIRECTX
			if (pD3D)
				{
				D3DLOCKED_RECT lock;
				// locking primary
				if (!pSfc) return false;
				// lock it
				if (pSfc->LockRect(&lock, NULL, 0) != D3D_OK)
					return false;
				lpDDraw->LockingPrimary();
				// store pitch and pointer
				PrimarySurfaceLockPitch=lock.Pitch;
				PrimarySurfaceLockBits=(BYTE*) lock.pBits;
				}
#endif //USE_DIRECTX

			// OpenGL:
			// cannot really lock primary surface, but Get/SetPix will emulate it
			}
		else
			{
			if (!ppTex) return false;
			// lock texture
			// textures will be locked when needed
			}
		}
	// count lock
	Locked++;	return true;
	}

bool CSurface::Unlock()
	{
	// unlock main sfc
	if (pMainSfc) pMainSfc->Unlock();
	// locked?
	if (!Locked) return false;
	// decrease lock counter; check if zeroed
	Locked--;
	if (!Locked)
		{
		// zeroed: unlock
		if (fPrimary)
			{
#ifdef USE_DIRECTX
			if (pD3D)
				{
				if (!pSfc) return false;
				// unlocking primary?
				if (pSfc->UnlockRect() != D3D_OK)
					return false;
				lpDDraw->PrimaryUnlocked();
				}
			else
#endif
				{
				// if tex refs exist, free them
				/*FreeTextures();*/
				// otherwise, emulated primary locks in OpenGL
				delete PrimarySurfaceLockBits;
				PrimarySurfaceLockBits = 0;
				return true;
				}
			}
		else
			{
			// non-primary unlock: unlock all texture surfaces (if locked)
			CTexRef **ppTx=ppTex;
			for (int i=0; i<iTexX*iTexY; ++i,++ppTx)
				(*ppTx)->Unlock();
			}
		}
	return true;
	}

bool CSurface::GetTexAt(CTexRef **ppTexRef, int &rX, int &rY)
	{
	// texture present?
	if (!ppTex) return false;
	// get pos
	int iX=rX/iTexSize;
	int iY=rY/iTexSize;
	// clip
	if (iX<0 || iY<0 || iX>=iTexX || iY>=iTexY) return false;
	// get texture by pos
	*ppTexRef=*(ppTex+iY*iTexX+iX);
	// adjust pos
	rX-=iX*iTexSize;
	rY-=iY*iTexSize;
	// success
	return true;
	}

bool CSurface::GetLockTexAt(CTexRef **ppTexRef, int &rX, int &rY)
	{
	// texture present?
	if (!GetTexAt(ppTexRef, rX, rY)) return false;
	// Already partially locked
	if ((*ppTexRef)->texLock.pBits)
		{
		// But not for the requested pixel
		RECT & r = (*ppTexRef)->LockSize;
		if (r.left > rX || r.top > rY || r.right < rX || r.bottom < rY)
			// Unlock, then relock the whole thing
			(*ppTexRef)->Unlock();
		else return true;
		}
	// ensure it's locked
	if (!(*ppTexRef)->Lock()) return false;
	// success
	return true;
	}

bool CSurface::SetPix(int iX, int iY, BYTE byCol)
	{
	return SetPixDw(iX, iY, lpDDrawPal->GetClr(byCol));
	}

DWORD CSurface::GetPixDw(int iX, int iY, bool fApplyModulation)
	{
	BYTE *pBuf = NULL; int iPitch = 0; // TODO: are those initialised to something sensible?
	// backup pos
	int iX2=iX; int iY2=iY;
	// primary?
	if (fPrimary)
		{
#ifdef USE_GL
		// OpenGL?
		if (pGL)
			{
			if (!PrimarySurfaceLockBits)
				{
				PrimarySurfaceLockBits = new unsigned char[Wdt*Hgt*3 + 1];
				glReadPixels( 0, 0, Wdt, Hgt, GL_BGR, GL_UNSIGNED_BYTE, PrimarySurfaceLockBits);
				PrimarySurfaceLockPitch = Wdt*3;
				}
			return * (DWORD *) (PrimarySurfaceLockBits+(Hgt-iY-1)*PrimarySurfaceLockPitch+iX*3);

			// copy content into textures
			/*if (!ppTex) if (!CreatePrimaryGLTextures()) return 0;
			// get+lock affected texture - inverse Y as primary is locked upside down!
			iY = Hgt-iY-1;
			CTexRef *pTexRef;
			if (!GetLockTexAt(&pTexRef, iX, iY)) return 0;
			pBuf=(BYTE *) pTexRef->texLock.pBits;
			iPitch=pTexRef->texLock.Pitch;
			// get pixel
			return *(DWORD *)(pBuf+iY*iPitch+iX*4);*/
			}
#endif
#ifdef USE_DIRECTX
		if (!PrimarySurfaceLockBits)
			{
			return 0;
			}
		else
			{
			// clip
			if (iX<0 || iY<0 || iX>=Wdt || iY>=Hgt) return 0;
			// get pixel from primary surface
			WORD pix16;
			switch (dwClrFormat)
				{
				case D3DFMT_X1R5G5B5:
					// 16 bit 5-5-5
					pix16= * (WORD *) (((BYTE *) PrimarySurfaceLockBits)+iY*PrimarySurfaceLockPitch+iX*2);
					return ((pix16 & 0x001f) << 3)
						   | ((pix16 & 0x03e0) << 6)
							 | ((pix16 & 0x7c00) << 9);

				case D3DFMT_R5G6B5:
					// 16 bit 5-6-5
					pix16= * (WORD *) (((BYTE *) PrimarySurfaceLockBits)+iY*PrimarySurfaceLockPitch+iX*2);
					return ((pix16 & 0x001f) <<  3)
						   | ((pix16 & 0x07e0) <<  5)
							 | ((pix16 & 0xf800) <<  8);
					break;

				case D3DFMT_X8R8G8B8:
					// 32 bit
					return * (DWORD *) (((BYTE *) PrimarySurfaceLockBits)+iY*PrimarySurfaceLockPitch+iX*4);
				}
			}
#endif
		}
	else
		{
		// get+lock affected texture
		if (!ppTex) return 0;
		CTexRef *pTexRef;
		if (!GetLockTexAt(&pTexRef, iX, iY)) return 0;
		pBuf=(BYTE *) pTexRef->texLock.pBits;
		iPitch=pTexRef->texLock.Pitch;
		}
	// get pix of surface
	DWORD dwPix;
	if (byBytesPP == 4)
		{
		// 32 bit
		DWORD *pPix=(DWORD *) (pBuf+iY*iPitch+iX*4);
		dwPix = *pPix;
		}
	else
		{
		// 16 bit
		WORD *pPix=(WORD *) (pBuf+iY*iPitch+iX*2);
		dwPix = ClrW2Dw(*pPix);
		}
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
				if (lpDDraw->dwBlitMode & C4GFXBLIT_CLRSFC_MOD2)
					ModulateClrMOD2(dwPix, ClrByOwnerClr);
				else
					ModulateClr(dwPix, ClrByOwnerClr);
				if (lpDDraw->BlitModulated && !(lpDDraw->dwBlitMode & C4GFXBLIT_CLRSFC_OWNCLR))
					ModulateClr(dwPix, lpDDraw->BlitModulateClr);
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
		if (fApplyModulation && lpDDraw->BlitModulated)
			{
			if (lpDDraw->dwBlitMode & C4GFXBLIT_MOD2)
				ModulateClrMOD2(dwPix, lpDDraw->BlitModulateClr);
			else
				ModulateClr(dwPix, lpDDraw->BlitModulateClr);
			}
		}
	// return pixel value
	return dwPix;
	}

bool CSurface::IsPixTransparent(int iX, int iY)
	{
	// get pixel value
	DWORD dwPix=GetPixDw(iX, iY, false);
	// get alpha value
	return (dwPix>>24) >= 128;
	}

/*bool CSurface::SetPixEx(int iX, int iY, BYTE byCol, DWORD dwClr)
	{
	// clip
	if ((iX<ClipX) || (iX>ClipX2) || (iY<ClipY) || (iY>ClipY2)) return true;
	// primary?
	if (fPrimary)
		{
#ifdef USE_GL
		// OpenGL: Use OpenGL API
		if (pGL)
			{
			pGL->DrawPixInt(this, iX, iY, dwClr);
			}
		else
#endif
			{
#ifdef USE_DIRECTX
			// must be locked!
			if (!Bits) return false;
			// set according to pixel format
			DWORD *pPix32; WORD *pPix16;
			switch (dwClrFormat)
				{
				case D3DFMT_X1R5G5B5:
					// 16 bit 5-5-5
					pPix16=(WORD *) (((BYTE *) Bits)+iY*Pitch+iX*2);
					*pPix16=WORD((dwClr & 0x000000f8) >> 3)
								| WORD((dwClr & 0x0000f800) >> 6)
								| WORD((dwClr & 0x00f80000) >> 9);
					break;

				case D3DFMT_R5G6B5:
					// 16 bit 5-6-5
					pPix16=(WORD *) (((BYTE *) Bits)+iY*Pitch+iX*2);
					*pPix16=WORD((dwClr & 0x000000f8) >> 3)
								| WORD((dwClr & 0x0000fc00) >> 5)
								| WORD((dwClr & 0x00f80000) >> 8);
					break;

				case D3DFMT_X8R8G8B8:
					// 32 bit
					pPix32=(DWORD *) (((BYTE *) Bits)+iY*Pitch+iX*4);
					*pPix32=dwClr;
					break;
				}
#endif
			}
		return true;
		}
	else
		{
		SetPixDw(iX, iY, dwClr);
		}
	return true;
	}*/

bool CSurface::SetPixDw(int iX, int iY, DWORD dwClr)
	{
	// clip
	if ((iX<ClipX) || (iX>ClipX2) || (iY<ClipY) || (iY>ClipY2)) return true;
	// get+lock affected texture
	if (!ppTex) return false;
	// if color is fully transparent, ensure it's black
	if (dwClr>>24 == 0x00) dwClr=0x00000000;
	CTexRef *pTexRef;
#ifdef USE_GL
	// openGL: use glTexSubImage2D
	// This optimization was moved to LockForUpdate, as it only slows down mass updates here
	// Keep this code in case there is a need for fast single pixel updates again
	if (0 && pGL && pGL->pCurrCtx)
		{
		if (!GetTexAt(&pTexRef, iX, iY))
			return false;
		// If the texture is not copied into system memory, modify it directly in the video memory
		if (!pTexRef->texLock.pBits)
			{
			glBindTexture(GL_TEXTURE_2D, pTexRef->texName);
			if (byBytesPP == 4)
				{
				// 32 Bit
				glTexSubImage2D(GL_TEXTURE_2D, 0, iX, iY, 1, 1, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, &dwClr);
				}
			else
				{
				// 16 bit
				uint16_t wClr=ClrDw2W(dwClr);
				glTexSubImage2D(GL_TEXTURE_2D, 0, iX, iY, 1, 1, GL_BGRA, GL_UNSIGNED_SHORT_4_4_4_4_REV, &wClr);
				}
			return true;
			}
		// Otherwise, make sure that the texlock covers the new pixel
		RECT & r = pTexRef->LockSize;
		if (r.left > iX || r.top > iY || r.right < iX || r.bottom < iY)
			{
			// Unlock, then relock the whole thing
			pTexRef->Unlock();
			if (!pTexRef->Lock()) return false;
			}
		}
	else
#endif
		{
		if (!GetLockTexAt(&pTexRef, iX, iY)) return false;
		}
	// ...and set in actual surface
	if (byBytesPP == 4)
		{
		// 32 bit
		pTexRef->SetPix4(iX, iY, dwClr);
		}
	else
		{
		// 16 bit
		pTexRef->SetPix2(iX, iY, ClrDw2W(dwClr));
		}
	// success
	return true;
	}

bool CSurface::SetPixAlpha(int iX, int iY, BYTE byAlpha)
	{
	// clip
	if ((iX<ClipX) || (iX>ClipX2) || (iY<ClipY) || (iY>ClipY2)) return true;
	// get+lock affected texture
	if (!ppTex) return false;
	CTexRef *pTexRef;
	if (!GetLockTexAt(&pTexRef, iX, iY)) return false;
	// set alpha value of pix in surface
	if (byBytesPP == 4)
		// 32 bit
		*(((BYTE *) pTexRef->texLock.pBits)+iY*pTexRef->texLock.Pitch+iX*4+3)=byAlpha;
	else
		{
		// 16 bit
		BYTE *pPix=((BYTE *) pTexRef->texLock.pBits)+iY*pTexRef->texLock.Pitch+iX*2+1;
		*pPix = (*pPix & 0x0f) | (byAlpha & 0xf0);
		}
	// success
	return true;
	}

bool CSurface::BltPix(int iX, int iY, CSurface *sfcSource, int iSrcX, int iSrcY, bool fTransparency)
	{
	// 16- or 32bit-blit. lock target
	CTexRef *pTexRef;
	if (!GetLockTexAt(&pTexRef, iX, iY)) return false;
	if (byBytesPP == 4)
		{
		// 32 bit
		DWORD *pPix32=(DWORD *) (((BYTE *) pTexRef->texLock.pBits)+iY*pTexRef->texLock.Pitch+iX*4);
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
			if (lpDDraw->dwBlitMode & C4GFXBLIT_ADDITIVE)
				BltAlphaAdd(*pPix32, srcPix);
			else
				BltAlpha(*pPix32, srcPix);
			}
		}
	else
		{
		// 16 bit
		WORD *pPix16=(WORD *) (((BYTE *) pTexRef->texLock.pBits)+iY*pTexRef->texLock.Pitch+iX*2);
		// get source pix as dword
		DWORD srcPix=sfcSource->GetPixDw(iSrcX, iSrcY, true);
		if (!fTransparency)
			{
			// set it
			*pPix16=ClrDw2W(srcPix);
			}
		else
			{
			// merge in 32 bit
			DWORD dwDst=ClrW2Dw(*pPix16);
			if (lpDDraw->dwBlitMode & C4GFXBLIT_ADDITIVE)
				BltAlphaAdd(dwDst, srcPix);
			else
				BltAlpha(dwDst, srcPix);
			// set
			*pPix16=ClrDw2W(dwDst);
			}
		}
	// done
	return true;
	}

void CSurface::ClearBoxDw(int iX, int iY, int iWdt, int iHgt)
	{
	// lock
	if (!Locked) return;
	// clip to target size
	if (iX<0) { iWdt+=iX; iX=0; }
	if (iY<0) { iHgt+=iY; iY=0; }
	int iOver;
	iOver=Wdt-(iX+iWdt); if (iOver<0) iWdt+=iOver;
	iOver=Hgt-(iY+iHgt); if (iOver<0) iHgt+=iOver;
	// get textures involved
	int iTexX1=iX/iTexSize;
	int iTexY1=iY/iTexSize;
	int iTexX2=Min((iX+iWdt-1)/iTexSize +1, iTexX);
	int iTexY2=Min((iY+iHgt-1)/iTexSize +1, iTexY);
	// clear basesfc?
	bool fBaseSfc=false;
	if (pMainSfc) if (pMainSfc->ppTex) fBaseSfc=true;
	// clear all these textures
	for (int y=iTexY1; y<iTexY2; ++y)
		{
		for (int x=iTexX1; x<iTexX2; ++x)
			{
			CTexRef *pTex = *(ppTex + y * iTexX + x);
			// get current offset in texture
			int iBlitX=iTexSize*x;
			int iBlitY=iTexSize*y;
			// get clearing bounds in texture
			RECT rtClear;
			rtClear.left  = Max(iX-iBlitX, 0);
			rtClear.top   = Max(iY-iBlitY, 0);
			rtClear.right = Min(iX+iWdt-iBlitX, iTexSize);
			rtClear.bottom= Min(iY+iHgt-iBlitY, iTexSize);
			// is there a base-surface to be cleared first?
			if (fBaseSfc)
				{
				// then get this surface as same offset as from other surface
				// assuming this is only valid as long as there's no texture management,
				// organizing partially used textures together!
				CTexRef *pBaseTex = *(pMainSfc->ppTex + y * iTexX + x);
				pBaseTex->ClearRect(rtClear);
				}
			// clear this texture
			pTex->ClearRect(rtClear);
			}
		}
	}

bool CSurface::CopyBytes(BYTE *pImageData)
	{
	// copy image data directly into textures
	CTexRef **ppCurrTex = ppTex, *pTex = *ppTex;
	int iSrcPitch = Wdt * byBytesPP; int iLineTotal = 0;
	for (int iY=0; iY<iTexY; ++iY)
		{
		BYTE *pSource = pImageData + iSrcPitch * iLineTotal;
		int iLastHeight=pTex->iSize; int iXImgPos=0;
		for (int iX=0; iX<iTexX; ++iX)
			{
			pTex = *ppCurrTex++;
			if (!pTex->Lock()) return false;
			BYTE *pTarget = (BYTE*)pTex->texLock.pBits;
			int iCpyNum = Min(pTex->iSize, Wdt-iXImgPos)*byBytesPP;
			int iYMax = Min(pTex->iSize, Hgt-iLineTotal);
			for (int iLine = 0; iLine < iYMax; ++iLine)
				{
				memcpy(pTarget, pSource, iCpyNum);
				pSource += iSrcPitch;
				// FIXME: use pTex->texLock.Pitch here?
				pTarget += pTex->iSize*byBytesPP;
				}
			pSource += iCpyNum - iSrcPitch*iYMax;
			iXImgPos += pTex->iSize;
			}
		iLineTotal += iLastHeight;
		}
	return true;
	}

CTexRef::CTexRef(int iSize, bool fSingle)
	{
	// zero fields
#ifdef USE_DIRECTX
	pTex=NULL;
#endif
#ifdef USE_GL
	texName=0;
#endif
	texLock.pBits=NULL; fIntLock=false;
	// store size
	this->iSize=iSize;
	// add to texture manager
	if (!pTexMgr) pTexMgr = new CTexMgr();
	pTexMgr->RegTex(this);
	// create texture: check ddraw
	if (!lpDDraw) return;
	if (!lpDDraw->DeviceReady()) return;
	// create it!
#ifdef USE_DIRECTX
	if (pD3D)
		{
		// Direct3D
		bool fRenderTarget = fSingle && !DDrawCfg.NoOffscreenBlits;
		if (pD3D->lpDevice->CreateTexture(iSize, iSize, 1, fRenderTarget ? D3DUSAGE_RENDERTARGET : 0, pD3D->dwSurfaceType, fRenderTarget ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED, &pTex, NULL) != D3D_OK)
			{
			lpDDraw->Error("Error creating surface");
			return;
			}
		// empty texture
		if (!Lock()) return;
		FillMemory(texLock.pBits, texLock.Pitch*iSize, 0x00);
		Unlock();
		}
	else
#endif
#ifdef USE_GL
	if (pGL)
		{
		// OpenGL
		// create mem array for texture creation
		texLock.pBits = new unsigned char[iSize*iSize*pGL->byByteCnt];
		texLock.Pitch = iSize*pGL->byByteCnt;
		memset(texLock.pBits, 0x00, texLock.Pitch*iSize);
		// turn mem array into texture
		Unlock();
		}
	else
#endif
	if (lpDDraw) {
		texLock.pBits = new unsigned char[iSize*iSize*lpDDraw->byByteCnt];
		texLock.Pitch = iSize*lpDDraw->byByteCnt;
		memset(texLock.pBits, 0x00, texLock.Pitch*iSize);
		// Always locked
		LockSize.left = LockSize.top = 0;
		LockSize.right = LockSize.bottom = iSize;
		}
	}

CTexRef::~CTexRef()
	{
	fIntLock=false;
	// free texture
#ifdef USE_DIRECTX
	if (pD3D)
		{
		if (texLock.pBits) Unlock();
		if (pTex) pTex->Release();
		}
#endif
#ifdef USE_GL
	if (pGL)
		{
		if(texName && pGL->pCurrCtx) glDeleteTextures(1, &texName);
		}
#endif
	if (lpDDraw) delete [] texLock.pBits; texLock.pBits = 0;
	// remove from texture manager
	pTexMgr->UnregTex(this);
	}

bool CTexRef::LockForUpdate(RECT & rtUpdate)
	{
	// already locked?
	if (texLock.pBits)
		{
		// fully locked
		if (LockSize.left == 0 && LockSize.right == iSize && LockSize.top == 0 && LockSize.bottom == iSize)
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
#ifdef USE_DIRECTX
	if (pD3D)
		{
		if (pTex)
			if (pTex->LockRect(0, &texLock, &rtUpdate, D3DLOCK_DISCARD) == D3D_OK)
				{
				LockSize = rtUpdate;
				return true;
				}
		}
	else
#endif
#ifdef USE_GL
	if (pGL)
		{
		if (texName)
			{
			// prepare texture data
			texLock.pBits = new unsigned char[
				(rtUpdate.right - rtUpdate.left) * (rtUpdate.bottom - rtUpdate.top) * pGL->byByteCnt];
			texLock.Pitch = (rtUpdate.right - rtUpdate.left) * pGL->byByteCnt;
			LockSize = rtUpdate;
			return true;
			}
		}
	else
#endif
		{
		// nothing to do
		}
	// failure
	return false;
	}

bool CTexRef::Lock()
	{
	// already locked?
	if (texLock.pBits) return true;
	LockSize.right = LockSize.bottom = iSize;
	LockSize.top = LockSize.left = 0;
	// lock
#ifdef USE_DIRECTX
	if (pD3D)
		{
		if (pTex)
			if (pTex->LockRect(0, &texLock, NULL, 0) == D3D_OK) return true;
		}
	else
#endif
#ifdef USE_GL
	if (pGL)
		{
		if (texName)
			{
			if (!pGL->pCurrCtx) return false;
			// get texture
			texLock.pBits = new unsigned char[iSize*iSize*pGL->byByteCnt];
			texLock.Pitch = iSize * pGL->byByteCnt;
			glBindTexture(GL_TEXTURE_2D, texName);
			glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, lpDDraw->byByteCnt == 2 ? GL_UNSIGNED_SHORT_4_4_4_4_REV : GL_UNSIGNED_INT_8_8_8_8_REV, texLock.pBits);
			return true;
			}
		}
	else
#endif
		{
		// nothing to do
		}
	// failure
	return false;
	}

void CTexRef::Unlock()
	{
	// locked?
	if (!texLock.pBits || fIntLock) return;
#ifdef USE_DIRECTX
	if (pD3D)
		{
		// unlock
		if (pTex) pTex->UnlockRect(0);
		texLock.pBits=NULL;
		}
	else
#endif
#ifdef USE_GL
	if (pGL)
		{
		if (!pGL->pCurrCtx)
			{
//			BREAKPOINT_HERE;
			assert(pGL->pMainCtx);
			pGL->pMainCtx->Select();
			}
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		if (!texName)
			{
			// create a new texture
			glGenTextures(1, &texName);
			glBindTexture(GL_TEXTURE_2D, texName);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			// Default, changed in PerformBlt if necessary
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexImage2D(GL_TEXTURE_2D, 0, 4, iSize, iSize, 0, GL_BGRA, lpDDraw->byByteCnt == 2 ? GL_UNSIGNED_SHORT_4_4_4_4_REV : GL_UNSIGNED_INT_8_8_8_8_REV, texLock.pBits);
			}
		else
			{
			// reuse the existing texture
			glBindTexture(GL_TEXTURE_2D, texName);
			glTexSubImage2D(GL_TEXTURE_2D, 0,
				LockSize.left, LockSize.top, LockSize.right - LockSize.left, LockSize.bottom - LockSize.top,
				GL_BGRA, lpDDraw->byByteCnt == 2 ? GL_UNSIGNED_SHORT_4_4_4_4_REV : GL_UNSIGNED_INT_8_8_8_8_REV, texLock.pBits);
			}
		delete[] texLock.pBits; texLock.pBits=NULL;
		// switch back to original context
		}
	else
#endif
		{
		// nothing to do
		}
	}

bool CTexRef::ClearRect(RECT &rtClear)
	{
	// ensure locked
	if (!LockForUpdate(rtClear)) return false;
	// clear pixels
	int y;
	switch (lpDDraw->byByteCnt)
		{
		case 2:
			for (y=rtClear.top; y<rtClear.bottom; ++y)
				{
				for (int x = rtClear.left; x < rtClear.right; ++x)
					SetPix2(x, y, 0x0000);
				}
			break;
		case 4:
			for (y=rtClear.top; y<rtClear.bottom; ++y)
				{
				for (int x = rtClear.left; x < rtClear.right; ++x)
					SetPix4(x, y, 0x00000000);
				}
			break;
		}
	// success
	return true;
	}

bool CTexRef::FillBlack()
	{
	// ensure locked
	if (!Lock()) return false;
	// clear pixels
	int y;
	switch (lpDDraw->byByteCnt)
		{
		case 2:
			for (y=0; y<iSize; ++y)
				{
				for (int x = 0; x < iSize; ++x)
					SetPix2(x, y, 0xf000);
				}
			break;
		case 4:
			for (y=0; y<iSize; ++y)
				{
				for (int x = 0; x < iSize; ++x)
					SetPix4(x, y, 0xff000000);
				}
			break;
		}
	// success
	return true;
	}

// texture manager

CTexMgr::CTexMgr()
	{
	// clear textures
	Textures.clear();
	}

CTexMgr::~CTexMgr()
	{
	// unlock all textures
	IntUnlock();
	}

void CTexMgr::RegTex(CTexRef *pTex)
	{
	// add texture to list
	Textures.push_front(pTex);
	}

void CTexMgr::UnregTex(CTexRef *pTex)
	{
	// remove texture from list
	Textures.remove(pTex);
	// if list is empty, remove self
	if (Textures.empty()) { delete this; pTexMgr=NULL; }
	}

void CTexMgr::IntLock()
	{
	// lock all textures
	int j=Textures.size();
	for (std::list<CTexRef *>::iterator i=Textures.begin(); j--; ++i)
		{
		CTexRef *pRef = *i;
		if (pRef->Lock() && !pRef->texLock.pBits) pRef->fIntLock = true;
		}
	}

void CTexMgr::IntUnlock()
	{
	// unlock all internally locked textures
	int j=Textures.size();
	for (std::list<CTexRef *>::iterator i=Textures.begin(); j--; ++i)
		{
		CTexRef *pRef = *i;
		if (pRef->fIntLock) { pRef->fIntLock = false; pRef->Unlock(); }
		}
	}

CTexMgr *pTexMgr;
const BYTE FColors [] = {31,16,39,47,55,63,71,79,87,95,23,30,99,103};
