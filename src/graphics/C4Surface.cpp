/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
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

#include "C4Include.h"
#include <C4Surface.h>

#include <StdFile.h>
#include <CStdFile.h>
#include "C4App.h"
#include <C4DrawGL.h>
#include <C4Window.h>
#include <StdRegistry.h>
#include <C4Draw.h>
#include <Bitmap256.h>
#include <StdPNG.h>
#include <C4Config.h>


#ifdef HAVE_IO_H
#include <io.h>
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <list>

C4Surface::C4Surface() : fIsBackground(false)
{
	Default();
}

C4Surface::C4Surface(int iWdt, int iHgt, int iFlags) : fIsBackground(false)
{
	Default();
	// create
	Create(iWdt, iHgt, false, 0, iFlags);
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
	PrimarySurfaceLockPitch=0; PrimarySurfaceLockBits=NULL;
	ClipX=ClipY=ClipX2=ClipY2=0;
	Locked=0;
	Attached=false;
	fPrimary=false;
	textures.clear();
	pMainSfc=NULL;
	pNormalSfc=NULL;
#ifndef USE_CONSOLE
	pCtx=NULL;
#endif
	pWindow=NULL;
	ClrByOwnerClr=0;
	iTexSize=iTexX=iTexY=0;
	fIsRenderTarget=false;
	fIsBackground=false;
#ifdef _DEBUG
	dbg_idx = NULL;
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
	psfcFrom->PrimarySurfaceLockBits=NULL;
	ClipX=psfcFrom->ClipX; ClipY=psfcFrom->ClipY;
	ClipX2=psfcFrom->ClipX2; ClipY2=psfcFrom->ClipY2;
	Locked=psfcFrom->Locked;
	Attached=psfcFrom->Attached;
	fPrimary=psfcFrom->fPrimary; // shouldn't be true!
	textures.swap(psfcFrom->textures);
	psfcFrom->textures.clear();
	pMainSfc=psfcFrom->pMainSfc;
	pNormalSfc=psfcFrom->pNormalSfc;
	ClrByOwnerClr=psfcFrom->ClrByOwnerClr;
	iTexSize=psfcFrom->iTexSize;
	iTexX=psfcFrom->iTexX; iTexY=psfcFrom->iTexY;
	byBytesPP=psfcFrom->byBytesPP;
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
		pCtx = 0;
	}
#endif
	FreeTextures();
#ifdef _DEBUG
	delete dbg_idx;
	dbg_idx = NULL;
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

bool C4Surface::Create(int iWdt, int iHgt, bool fIsRenderTarget, int MaxTextureSize, int iFlags)
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
	byBytesPP=pDraw->byByteCnt;
	this->fIsRenderTarget = fIsRenderTarget;
	// create textures
	if (!CreateTextures(MaxTextureSize, iFlags)) { Clear(); return false; }
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
	if (!Create(fromSfc.Wdt, fromSfc.Hgt, false, 0, 0)) return false;
	// Blit copy
	if (!pDraw->BlitSurface(&fromSfc, this, 0, 0, false))
		{ Clear(); return false; }
	// Success
	return true;
}

namespace
{
	int GetNeedTexSize(int Size)
	{
		int iNeedSize = Size;

	#ifndef USE_CONSOLE
		if (!pGL || !GLEW_ARB_texture_non_power_of_two)
	#endif
		{
			int n=0;
			while ((1<<++n) < iNeedSize) {}
			iNeedSize = 1<<n;
		}

		return iNeedSize;
	}
}

bool C4Surface::CreateTextures(int MaxTextureSize, int Flags)
{
	// free previous
	FreeTextures();
	iTexSize=std::min(GetNeedTexSize(std::max(Wdt, Hgt)), pDraw->MaxTexSize);
	if (MaxTextureSize)
		iTexSize=std::min(iTexSize, MaxTextureSize);
	// get the number of textures needed for this size
	iTexX=(Wdt-1)/iTexSize +1;
	iTexY=(Hgt-1)/iTexSize +1;
	// get mem for texture array
	textures.reserve(iTexX * iTexY);
	// cvan't be render target if it's not a single surface
	if (!IsSingleSurface()) fIsRenderTarget = false;
	// create textures
	for (int y = 0; y < iTexY; ++y)
	{
		for(int x = 0; x < iTexX; ++x)
		{
			int sizeX = iTexSize;
			int sizeY = iTexSize;
			if(x == iTexX-1) sizeX = GetNeedTexSize( (Wdt - 1) % iTexSize + 1);
			if(y == iTexY-1) sizeY = GetNeedTexSize( (Hgt - 1) % iTexSize + 1);

			textures.emplace_back(sizeX, sizeY, Flags);
			
			if (fIsBackground) textures.rbegin()->FillBlack();
		}
	}

#ifdef _DEBUG
	static int dbg_counter = 0;
	if (dbg_idx) delete dbg_idx;
	dbg_idx = new int;
	*dbg_idx = dbg_counter++;
#endif
	// success
	return true;
}

void C4Surface::FreeTextures()
{
	textures.clear();
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
	if (pBySurface->textures.empty()) return false;
	// create in same size
	if (!Create(pBySurface->Wdt, pBySurface->Hgt, false, 0, 0)) return false;
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
	if (!Create(BitmapInfo.Info.biWidth,BitmapInfo.Info.biHeight, false, 0, iFlags)) return false;
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
	C4Surface *pMainSfcBackup = NULL;
	if (fSaveOverlayOnly) { pMainSfcBackup=pMainSfc; pMainSfc=NULL; }

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
	if (!Locked && !fPrimary && textures.empty())
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
			PrimarySurfaceLockBits = 0;
			return true;
		}
		else
		{
			// non-primary unlock: unlock all texture surfaces (if locked)
			for (auto &texture : textures)
				texture.Unlock();
		}
	}
	return true;
}

bool C4Surface::GetTexAtImpl(C4TexRef **ppTexRef, int &rX, int &rY)
{
	// get pos
	int iX=rX/iTexSize;
	int iY=rY/iTexSize;
	// get texture by pos
	*ppTexRef = &textures[iY*iTexX + iX];
	// adjust pos
	rX-=iX*iTexSize;
	rY-=iY*iTexSize;
	// success
	return true;
}

bool C4Surface::GetLockTexAt(C4TexRef **ppTexRef, int &rX, int &rY)
{
	// texture present?
	if (!GetTexAt(ppTexRef, rX, rY)) return false;
	// Already partially locked
	if ((*ppTexRef)->texLock.pBits)
	{
		// But not for the requested pixel
		C4Rect & r = (*ppTexRef)->LockSize;
		if (r.x > rX || r.y > rY || (r.x + r.Wdt) < rX || (r.y + r.Hgt) < rY)
			// Unlock, then relock the whole thing
			(*ppTexRef)->Unlock();
		else return true;
	}
	// ensure it's locked
	if (!(*ppTexRef)->Lock()) return false;
	// success
	return true;
}

DWORD C4Surface::GetPixDw(int iX, int iY, bool fApplyModulation)
{
	BYTE *pBuf = NULL; int iPitch = 0; // TODO: are those initialised to something sensible?
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
		if (textures.empty()) return 0;
		C4TexRef *pTexRef;
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
	if (textures.empty()) return false;
	// if color is fully transparent, ensure it's black
	if (dwClr>>24 == 0x00) dwClr=0x00000000;
	C4TexRef *pTexRef;
#ifndef USE_CONSOLE
	// openGL: use glTexSubImage2D
	// This optimization was moved to LockForUpdate, as it only slows down mass updates here
	// Keep this code in case there is a need for fast single pixel updates again
	if (0 && pGL->pCurrCtx)
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
		C4Rect & r = pTexRef->LockSize;
		if (r.x > iX || r.y > iY || (r.x + r.Wdt) < iX || (r.y + r.Hgt) < iY)
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

bool C4Surface::SetPixAlpha(int iX, int iY, BYTE byAlpha)
{
	// clip
	if ((iX<ClipX) || (iX>ClipX2) || (iY<ClipY) || (iY>ClipY2)) return true;
	// get+lock affected texture
	if (textures.empty()) return false;
	C4TexRef *pTexRef;
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

bool C4Surface::BltPix(int iX, int iY, C4Surface *sfcSource, int iSrcX, int iSrcY, bool fTransparency)
{
	// 16- or 32bit-blit. lock target
	C4TexRef *pTexRef;
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
			if (pDraw->dwBlitMode & C4GFXBLIT_ADDITIVE)
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
			if (pDraw->dwBlitMode & C4GFXBLIT_ADDITIVE)
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
	// get textures involved
	int iTexX1=iX/iTexSize;
	int iTexY1=iY/iTexSize;
	int iTexX2=std::min((iX+iWdt-1)/iTexSize +1, iTexX);
	int iTexY2=std::min((iY+iHgt-1)/iTexSize +1, iTexY);
	// clear basesfc?
	bool fBaseSfc=false;
	if (pMainSfc) if (!pMainSfc->textures.empty()) fBaseSfc = true;
	// clear all these textures
	for (int y=iTexY1; y<iTexY2; ++y)
	{
		for (int x=iTexX1; x<iTexX2; ++x)
		{
			C4TexRef *pTex = &textures[y * iTexX + x];
			// get current offset in texture
			int iBlitX=iTexSize*x;
			int iBlitY=iTexSize*y;
			// get clearing bounds in texture
			C4Rect rtClear = C4Rect(0, 0, pTex->iSizeX, pTex->iSizeY);
			rtClear.Intersect(C4Rect(iX - iBlitX, iY - iBlitY, iWdt, iHgt));
			// is there a base-surface to be cleared first?
			if (fBaseSfc)
			{
				// then get this surface as same offset as from other surface
				// assuming this is only valid as long as there's no texture management,
				// organizing partially used textures together!
				C4TexRef *pBaseTex = &pMainSfc->textures[y * iTexX + x];
				pBaseTex->ClearRect(rtClear);
			}
			// clear this texture
			pTex->ClearRect(rtClear);
		}
	}
}

bool C4Surface::CopyBytes(BYTE *pImageData)
{
	// copy image data directly into textures
	auto pTex = textures.begin();
	int iSrcPitch = Wdt * byBytesPP; int iLineTotal = 0;
	for (int iY=0; iY<iTexY; ++iY)
	{
		BYTE *pSource = pImageData + iSrcPitch * iLineTotal;
		int iLastHeight=pTex->iSizeY; int iXImgPos=0;
		for (int iX=0; iX<iTexX; ++iX)
		{
			++pTex;
			if (!pTex->Lock()) return false;
			BYTE *pTarget = (BYTE*)pTex->texLock.pBits;
			int iCpyNum = std::min(pTex->iSizeX, Wdt-iXImgPos)*byBytesPP;
			int iYMax = std::min(pTex->iSizeY, Hgt-iLineTotal);
			for (int iLine = 0; iLine < iYMax; ++iLine)
			{
				memcpy(pTarget, pSource, iCpyNum);
				pSource += iSrcPitch;
				// FIXME: use pTex->texLock.Pitch here?
				pTarget += pTex->iSizeX*byBytesPP;
			}
			pSource += iCpyNum - iSrcPitch*iYMax;
			iXImgPos += pTex->iSizeX;
		}
		iLineTotal += iLastHeight;
	}
	return true;
}

C4TexRef::C4TexRef(int iSizeX, int iSizeY, int iFlags)
{
	// zero fields
#ifndef USE_CONSOLE
	texName = 0;
#endif
	texLock.pBits=NULL; fIntLock=false;
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
		texLock.pBits = new unsigned char[iSizeX*iSizeY*pDraw->byByteCnt];
		texLock.Pitch = iSizeX*pDraw->byByteCnt;
		memset(texLock.pBits, 0x00, texLock.Pitch*iSizeY);
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
	if (pDraw) delete [] static_cast<unsigned char*>(texLock.pBits); texLock.pBits = 0;
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
	if (fMipMap) glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, iSizeX, iSizeY, 0, GL_BGRA, pDraw->byByteCnt == 2 ? GL_UNSIGNED_SHORT_4_4_4_4_REV : GL_UNSIGNED_INT_8_8_8_8_REV, NULL);
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
	texLock.pBits = new unsigned char[rtUpdate.Wdt * rtUpdate.Hgt * pGL->byByteCnt];
	texLock.Pitch = rtUpdate.Wdt * pGL->byByteCnt;
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
				texLock.pBits = new unsigned char[iSizeX*iSizeY*pGL->byByteCnt];
				texLock.Pitch = iSizeX * pGL->byByteCnt;
				glBindTexture(GL_TEXTURE_2D, texName);
				glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, pDraw->byByteCnt == 2 ? GL_UNSIGNED_SHORT_4_4_4_4_REV : GL_UNSIGNED_INT_8_8_8_8_REV, texLock.pBits);
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
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

			// reuse the existing texture
			glBindTexture(GL_TEXTURE_2D, texName);
			glTexSubImage2D(GL_TEXTURE_2D, 0,
			                LockSize.x, LockSize.y, LockSize.Wdt, LockSize.Hgt,
			                GL_BGRA, pDraw->byByteCnt == 2 ? GL_UNSIGNED_SHORT_4_4_4_4_REV : GL_UNSIGNED_INT_8_8_8_8_REV, texLock.pBits);

			delete[] static_cast<unsigned char*>(texLock.pBits); texLock.pBits=NULL;
#endif
}

bool C4TexRef::ClearRect(C4Rect &rtClear)
{
	// ensure locked
	if (!LockForUpdate(rtClear)) return false;
	// clear pixels
	int y;
	switch (pDraw->byByteCnt)
	{
	case 2:
		for (y = rtClear.y; y < rtClear.y + rtClear.Hgt; ++y)
		{
			for (int x = rtClear.x; x < rtClear.x + rtClear.Wdt; ++x)
				SetPix2(x, y, 0x0000);
		}
		break;
	case 4:
		for (y = rtClear.y; y < rtClear.y + rtClear.Hgt; ++y)
		{
			for (int x = rtClear.x; x < rtClear.x + rtClear.Wdt; ++x)
				SetPix4(x, y, 0x00000000);
		}
		break;
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
	switch (pDraw->byByteCnt)
	{
	case 2:
		for (y=0; y<iSizeY; ++y)
		{
			for (int x = 0; x < iSizeX; ++x)
				SetPix2(x, y, 0xf000);
		}
		break;
	case 4:
		for (y=0; y<iSizeY; ++y)
		{
			for (int x = 0; x < iSizeX; ++x)
				SetPix4(x, y, 0xff000000);
		}
		break;
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
	if (Textures.empty()) { delete this; pTexMgr=NULL; }
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
