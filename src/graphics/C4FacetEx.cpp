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

/* A facet that can hold its own surface and also target coordinates */

#include <C4Include.h>
#include <C4FacetEx.h>

#include <C4Rect.h>
#include <C4Group.h>


void C4TargetFacet::Set(C4Surface * nsfc, float nx, float ny, float nwdt, float nhgt, float ntx, float nty, float Zoom)
{
	C4Facet::Set(nsfc, nx, ny, nwdt, nhgt);
	TargetX = ntx; TargetY = nty; this->Zoom = Zoom;
}

void C4TargetFacet::Set(C4Surface * nsfc, const C4Rect & r, float ntx, float nty, float Zoom)
{
	Set(nsfc, r.x, r.y, r.Wdt, r.Hgt, ntx, nty, Zoom);
}
void C4TargetFacet::SetRect(C4TargetRect &rSrc)
{
	X=rSrc.x; Y=rSrc.y; Wdt=rSrc.Wdt; Hgt=rSrc.Hgt; TargetX=rSrc.tx; TargetY=rSrc.ty;
}

// ------------------------
// C4FacetSurface

bool C4FacetSurface::Create(int iWdt, int iHgt, int iWdt2, int iHgt2)
{
	Clear();
	// Create surface
	Face.Default();
	if (!Face.Create(iWdt,iHgt)) return false;
	// Set facet
	if (iWdt2==C4FCT_Full) iWdt2=Face.Wdt; if (iWdt2==C4FCT_Height) iWdt2=Face.Hgt; if (iWdt2==C4FCT_Width) iWdt2=Face.Wdt;
	if (iHgt2==C4FCT_Full) iHgt2=Face.Hgt; if (iHgt2==C4FCT_Height) iHgt2=Face.Hgt; if (iHgt2==C4FCT_Width) iHgt2=Face.Wdt;
	Set(&Face,0,0,iWdt2,iHgt2);
	return true;
}

bool C4FacetSurface::CreateClrByOwner(C4Surface *pBySurface)
{
	Clear();
	// create surface
	if (!Face.CreateColorByOwner(pBySurface)) return false;
	// set facet
	Set(&Face,0,0,Face.Wdt,Face.Hgt);
	// success
	return true;
}

bool C4FacetSurface::EnsureSize(int iMinWdt, int iMinHgt)
{
	// safety
	if (!Surface) return false;
	// check size
	int iWdt=Face.Wdt,iHgt=Face.Hgt;
	if (iWdt>=iMinWdt && iHgt>=iMinHgt) return true;
	// create temp surface
	C4Surface *sfcDup=new C4Surface(iWdt,iHgt);
	if (!sfcDup) return false;
	if (!pDraw->BlitSurface(&Face,sfcDup,0,0,false))
		{ delete sfcDup; return false; }
	// calc needed size
	int iDstWdt=Surface->Wdt,iDstHgt=iHgt;
	while (iDstWdt<iMinWdt) iDstWdt+=iWdt;
	while (iDstHgt<iMinHgt) iDstHgt+=iHgt;
	// recreate this one
	if (!Face.Create(iDstWdt, iDstHgt)) { delete sfcDup; Clear(); return false; }
	// blit tiled into it
	bool fSuccess=pDraw->BlitSurfaceTile(sfcDup, &Face, 0, 0, iDstWdt, iDstHgt, 0, 0, false);
	// del temp surface
	delete sfcDup;
	// done
	return fSuccess;
}

/*bool C4FacetSurface::Save(C4Group &hGroup, const char *szName)
  {
  // Empty
  if (!Wdt || !Hgt) return false;
  // Full surface
  if ((Wdt==Face.Wdt) && (Hgt==Face.Hgt))
    {
    if (!Face.Save(hGroup,szName)) return false;
    }
  // Surface section
  else
    {
    C4Surface sfcFacet;
    if (!sfcFacet.Create(Wdt,Hgt)) return false;
    Draw(&sfcFacet,0,0);
    if (!sfcFacet.Save(hGroup,szName)) return false;
    }
  // Success
  return true;
  }*/

bool C4FacetSurface::Load(C4Group &hGroup, const char *szName, int iWdt, int iHgt, bool fOwnPal, bool fNoErrIfNotFound)
{
	Clear();
	// Entry name
	char szFilename[_MAX_FNAME+1];
	SCopy(szName,szFilename,_MAX_FNAME);
	char *szExt = GetExtension(szFilename);
	if (!*szExt)
	{
		// no extension: Default to extension that is found as file in group
		const char * const extensions[] = { "png", "bmp", "jpeg", "jpg", NULL };
		int i = 0; const char *szExt;
		while ((szExt = extensions[i++]))
		{
			EnforceExtension(szFilename, szExt);
			if (hGroup.FindEntry(szFilename)) break;
		}
	}
	// Load surface
	if (!Face.Load(hGroup,szFilename,fOwnPal,fNoErrIfNotFound)) return false;
	// Set facet
	if (iWdt==C4FCT_Full) iWdt=Face.Wdt; if (iWdt==C4FCT_Height) iWdt=Face.Hgt; if (iWdt==C4FCT_Width) iWdt=Face.Wdt;
	if (iHgt==C4FCT_Full) iHgt=Face.Hgt; if (iHgt==C4FCT_Height) iHgt=Face.Hgt; if (iHgt==C4FCT_Width) iHgt=Face.Wdt;
	Set(&Face,0,0,iWdt,iHgt);
	return true;
}

bool C4FacetSurface::CopyFromSfcMaxSize(C4Surface &srcSfc, int32_t iMaxSize, uint32_t dwColor)
{
	// safety
	if (!srcSfc.Wdt || !srcSfc.Hgt) return false;
	Clear();
	// no scale?
	bool fNeedsScale = !(srcSfc.Wdt <= iMaxSize && srcSfc.Hgt <= iMaxSize);
	if (!fNeedsScale && !dwColor)
	{
		// no change necessary; just copy then
		Face.Copy(srcSfc);
	}
	else
	{
		// must scale down or colorize. Just blit.
		C4Facet fctSource;
		fctSource.Set(&srcSfc, 0,0,srcSfc.Wdt,srcSfc.Hgt);
		int32_t iTargetWdt, iTargetHgt;
		if (fNeedsScale)
		{
			if (fctSource.Wdt > fctSource.Hgt)
			{
				iTargetWdt = iMaxSize;
				iTargetHgt = fctSource.Hgt * iTargetWdt / fctSource.Wdt;
			}
			else
			{
				iTargetHgt = iMaxSize;
				iTargetWdt = fctSource.Wdt * iTargetHgt / fctSource.Hgt;
			}
		}
		else
		{
			iTargetWdt = fctSource.Wdt;
			iTargetHgt = fctSource.Hgt;
		}
		if (dwColor) srcSfc.SetClr(dwColor);
		Create(iTargetWdt, iTargetHgt);
		pDraw->Blit(&srcSfc, 0.0f,0.0f,float(fctSource.Wdt),float(fctSource.Hgt),
		              &Face, 0,0,iTargetWdt,iTargetHgt);
	}
	Set(&Face, 0,0, Face.Wdt, Face.Hgt);
	return true;
}

void C4FacetSurface::Grayscale(int32_t iOffset)
{
	if (!pDraw || !Surface || !Wdt || !Hgt) return;
	pDraw->Grayscale(Surface, iOffset);
}

bool C4FacetSurface::EnsureOwnSurface()
{
	// is it a link?
	if (Surface != &Face)
	{
		// then recreate in same size
		C4Facet fctOld = *this;
		if (!Create(fctOld.Wdt, fctOld.Hgt)) return false;
		fctOld.Draw(*this);
	}
	return true;
}

