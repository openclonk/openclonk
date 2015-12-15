/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2015, The OpenClonk Team and contributors
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

#include "C4Include.h"
#include "C4LandscapeRender.h"
#include "C4Landscape.h"
#include "C4Texture.h"

const int C4LS_MaxLightDistY = 8;
const int C4LS_MaxLightDistX = 1;

C4LandscapeRenderClassic::C4LandscapeRenderClassic()
	: Surface32(NULL)
{
}

C4LandscapeRenderClassic::~C4LandscapeRenderClassic()
{
	Clear();
}

bool C4LandscapeRenderClassic::ReInit(int32_t iWidth, int32_t iHeight)
{
	// Create surface
	delete Surface32; Surface32 = NULL;
	Surface32 = new C4Surface();
	// without shaders, the FoW is only as detailed as the landscape has tiles.
	if (!Surface32->Create(iWidth, iHeight, false, 0, 0))
		return false;
	// Safe back info
	this->iWidth = iWidth;
	this->iHeight = iHeight;
	return true;
}

bool C4LandscapeRenderClassic::Init(int32_t iWidth, int32_t iHeight, C4TextureMap *pTexs, C4GroupSet *pGraphics)
{
	// Init proc
	if (!ReInit(iWidth, iHeight)) return false;
	this->pTexs = pTexs;
	return true;
}

void C4LandscapeRenderClassic::Clear()
{
	delete Surface32; Surface32 = 0;
	iWidth = iHeight = 0;
	pTexs = NULL;
}

C4Rect C4LandscapeRenderClassic::GetAffectedRect(C4Rect Rect)
{
	Rect.Enlarge(C4LS_MaxLightDistX, C4LS_MaxLightDistY);
	return Rect;
}

void C4LandscapeRenderClassic::Update(C4Rect To, C4Landscape *pSource)
{
	// clip to landscape size
	To.Intersect(C4Rect(0,0,iWidth,iHeight));
	// everything clipped?
	if (To.Wdt<=0 || To.Hgt<=0) return;
	if (!Surface32->Lock()) return;

	// We clear the affected region here because ClearBoxDw allocates the
	// main memory buffer for the box, so that only that box needs to be
	// sent to the gpu, and not the whole texture, or every pixel
	// separately. It's an important optimization.
	Surface32->ClearBoxDw(To.x, To.y, To.Wdt, To.Hgt);

	// do lightning
	for (int32_t iX=To.x; iX<To.x+To.Wdt; ++iX)
	{
		int AboveDensity = 0, BelowDensity = 0;
		for (int i = 1; i <= 8; ++i)
		{
			AboveDensity += pSource->GetPlacement(iX, To.y - i - 1);
			BelowDensity += pSource->GetPlacement(iX, To.y + i - 1);
		}
		for (int32_t iY=To.y; iY<To.y+To.Hgt; ++iY)
		{
			AboveDensity -= pSource->GetPlacement(iX, iY - 9);
			AboveDensity += pSource->GetPlacement(iX, iY - 1);
			BelowDensity -= pSource->GetPlacement(iX, iY);
			BelowDensity += pSource->GetPlacement(iX, iY + 8);
			BYTE pix = pSource->_GetPix(iX, iY);
			// Sky
			if (!pix)
			{
				Surface32->SetPixDw(iX, iY, 0x00ffffff);
				continue;
			}
			// get density
			int iOwnDens = pSource->_GetPlacement(iX, iY);
			if (!iOwnDens) continue;
			iOwnDens *= 2;
			iOwnDens += pSource->GetPlacement(iX + 1, iY) + pSource->GetPlacement(iX - 1, iY);
			iOwnDens /= 4;
			// get texture map entry for pixel
			const C4TexMapEntry *pTex = pTexs->GetEntry(PixCol2Tex(pix));
			assert(pTex);
			// get texture contents
			DWORD dwBackClr = 0u;
			if (pTex) dwBackClr = pTex->GetPattern().PatternClr(iX, iY);
			// get density of surrounding materials
			int iCompareDens = AboveDensity / 8;
			if (iOwnDens > iCompareDens)
			{
				// apply light
				LightenClrBy(dwBackClr, std::min(30, 2 * (iOwnDens - iCompareDens)));
			}
			else if (iOwnDens < iCompareDens && iOwnDens < 30)
			{
				DarkenClrBy(dwBackClr, std::min(30, 2 * (iCompareDens - iOwnDens)));
			}
			iCompareDens = BelowDensity / 8;
			if (iOwnDens > iCompareDens)
			{
				DarkenClrBy(dwBackClr, std::min(30, 2 * (iOwnDens - iCompareDens)));
			}
			Surface32->SetPixDw(iX, iY, dwBackClr);
		}
	}
	Surface32->Unlock();
}

void C4LandscapeRenderClassic::Draw(const C4TargetFacet &cgo, const C4FoWRegion *Light)
{
	// Ignore light for now
	pDraw->BlitLandscape(Surface32, cgo.TargetX, cgo.TargetY, cgo.Surface,
	                                  cgo.X, cgo.Y, cgo.Wdt, cgo.Hgt);
}
