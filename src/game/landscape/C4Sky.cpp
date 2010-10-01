/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2002, 2005-2006  Sven Eberhardt
 * Copyright (c) 2005  Peter Wortmann
 * Copyright (c) 2005-2008  Günther Brammer
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

/* Small member of the landscape class to handle the sky background */

#include <C4Include.h>
#include <C4Sky.h>

#include <C4Game.h>
#include <C4Random.h>
#include <C4SurfaceFile.h>
#include <C4Components.h>
#include <C4Weather.h>
#include <C4GraphicsResource.h>

static bool SurfaceEnsureSize(C4Surface **ppSfc, int iMinWdt, int iMinHgt)
{
	// safety
	if (!ppSfc) return false; if (!*ppSfc) return false;
	// get size
	int iWdt=(*ppSfc)->Wdt, iHgt=(*ppSfc)->Hgt;
	int iDstWdt=iWdt, iDstHgt=iHgt;
	// check if it must be enlarged
	while (iDstWdt<iMinWdt) iDstWdt+=iWdt;
	while (iDstHgt<iMinHgt) iDstHgt+=iHgt;
	// Without shaders, the textures need to be small for the FoW.
	if (iDstWdt==iWdt && iDstHgt==iHgt && lpDDraw->IsShaderific()) return true;
	// create new surface
	C4Surface *pNewSfc=new C4Surface();
	if (!pNewSfc->Create(iDstWdt, iDstHgt, false, false, lpDDraw->IsShaderific() ? 0 : 64))
	{
		delete pNewSfc;
		return false;
	}
	// blit tiled into dest surface
	lpDDraw->BlitSurfaceTile2(*ppSfc, pNewSfc, 0, 0, iDstWdt, iDstHgt, 0, 0, false);
	// destroy old surface, assign new
	delete *ppSfc; *ppSfc=pNewSfc;
	// success
	return true;
}

void C4Sky::SetFadePalette(int32_t *ipColors)
{
	// If colors all zero, use game palette default blue
	if (ipColors[0]+ipColors[1]+ipColors[2]+ipColors[3]+ipColors[4]+ipColors[5]==0)
	{
		FadeClr1=C4RGB(0x1c, 0x40, 0x99);
		FadeClr2=C4RGB(0xc2, 0xc6, 0xff);
	}
	else
	{
		// set colors
		FadeClr1=C4RGB(ipColors[0], ipColors[1], ipColors[2]);
		FadeClr2=C4RGB(ipColors[3], ipColors[4], ipColors[5]);
	}
}

bool C4Sky::Init(bool fSavegame)
{
	int32_t skylistn;

	// reset scrolling pos+speed
	// not in savegame, because it will have been loaded from game data there
	if (!fSavegame)
	{
		x=y=xdir=ydir=0; ParX=ParY=10; ParallaxMode=0;
	}

	// Check for sky bitmap in scenario file
	Surface = new C4Surface();
	bool loaded = !!Surface->LoadAny(Game.ScenarioFile,C4CFN_Sky,true,true);

	// Else, evaluate scenario core landscape sky default list
	if (!loaded)
	{
		// Scan list sections
		SReplaceChar(Game.C4S.Landscape.SkyDef,',',';'); // modifying the C4S here...!
		skylistn=SCharCount(';',Game.C4S.Landscape.SkyDef)+1;
		char str[402];
		SCopySegment(Game.C4S.Landscape.SkyDef,SeededRandom(Game.RandomSeed,skylistn),str,';');
		SClearFrontBack(str);
		// Sky tile specified, try load
		if (*str && !SEqual(str,"Default"))
		{
			// Check for sky tile in scenario file
			loaded = !!Surface->LoadAny(Game.ScenarioFile,str,true,true);
			if (!loaded)
			{
				loaded = !!Surface->LoadAny(::GraphicsResource.Files, str, true);
			}
		}
	}

	if (loaded)
	{
		// surface loaded, store first color index
		FadeClr1=FadeClr2=0xffffffff;
		// enlarge surface to avoid slow 1*1-px-skies
		if (!SurfaceEnsureSize(&Surface, 128, 128)) return false;

		// set parallax scroll mode
		switch (Game.C4S.Landscape.SkyScrollMode)
		{
		case 0: // default: no scrolling
			break;
		case 1: // go with the wind in xdir, and do some parallax scrolling in ydir
			ParallaxMode=C4SkyPM_Wind;
			ParY=20;
			break;
		case 2: // parallax in both directions
			ParX=ParY=20;
			break;
		}

	}


	// Else, try creating default Surface
	if (!loaded)
	{
		SetFadePalette(Game.C4S.Landscape.SkyDefFade);
		delete Surface;
		Surface = 0;
	}

	// no sky - using fade in newgfx
	if (!Surface)
		return true;

	// Store size
	if (Surface)
	{
		int iWdt,iHgt;
		if (Surface->GetSurfaceSize(iWdt, iHgt))
		{
			Width = iWdt; Height = iHgt;
		}
	}

	// Success
	return true;
}

void C4Sky::Default()
{
	Width=Height=0;
	Surface=NULL;
	x=y=xdir=ydir=0;
	Modulation=0xffffffff;
	ParX=ParY=10;
	ParallaxMode=C4SkyPM_Fixed;
	BackClr=0;
	BackClrEnabled=false;
}

C4Sky::~C4Sky()
{
	Clear();
}

void C4Sky::Clear()
{
	delete Surface; Surface=NULL;
	Modulation=0xffffffff;
}

bool C4Sky::Save(C4Group &hGroup)
{
	// Sky-saving disabled by scenario core
	// (With this option enabled, script-defined changes to sky palette will not be saved!)
	if (Game.C4S.Landscape.NoSky)
	{
		hGroup.Delete(C4CFN_Sky);
		return true;
	}
	// no sky?
	if (!Surface) return true;
	// FIXME?
	// Success
	return true;
}

void C4Sky::Execute()
{
	// surface exists?
	if (!Surface) return;
	// advance pos
	x+=xdir; y+=ydir;
	// clip by bounds
	if (x>=itofix(Width)) x-=itofix(Width);
	if (y>=itofix(Height)) y-=itofix(Height);
	// update speed
	if (ParallaxMode == C4SkyPM_Wind) xdir=C4REAL100(::Weather.Wind);
}

void C4Sky::Draw(C4TargetFacet &cgo)
{
	// background color?
	if (BackClrEnabled) lpDDraw->DrawBoxDw(cgo.Surface, cgo.X, cgo.Y, cgo.X+cgo.Wdt, cgo.Y+cgo.Hgt, BackClr);
	// sky surface?
	if (Modulation != 0xffffffff) lpDDraw->ActivateBlitModulation(Modulation);
	if (Surface)
	{
		// blit parallax sky
		float zoom = cgo.Zoom;
		float targetx = cgo.TargetX; float targety = cgo.TargetY;
		int width = cgo.Wdt; int height = cgo.Hgt;
		float parx = 10.0f / ParX; float pary = 10.0f / ParY;
		float par = parx; //todo: pary?

		// Step 1: project to landscape coordinates
		float resultzoom = 1.0 / (1.0 - (par - par/zoom));

		float rx = ((1 - parx) * targetx) * resultzoom + fixtof(x) / (parx + zoom - parx * zoom);
		float ry = ((1 - pary) * targety) * resultzoom + fixtof(y) / (pary + zoom - pary * zoom);

		// Step 2: convert to screen coordinates
		float resultx = (rx - targetx) * zoom / resultzoom;
		float resulty = (ry - targety) * zoom / resultzoom;

		ZoomDataStackItem zdsi(resultzoom);

		lpDDraw->BlitSurfaceTile2(Surface, cgo.Surface, cgo.X, cgo.Y, cgo.Wdt * zoom / resultzoom, cgo.Hgt * zoom / resultzoom, -resultx, -resulty, false);
	}
	else
	{
		// no sky surface: blit sky fade
		DWORD dwClr1=GetSkyFadeClr(cgo.TargetY);
		DWORD dwClr2=GetSkyFadeClr(cgo.TargetY+cgo.Hgt);
		lpDDraw->DrawBoxFade(cgo.Surface, cgo.X, cgo.Y, cgo.Wdt, cgo.Hgt, dwClr1, dwClr1, dwClr2, dwClr2, cgo.TargetX, cgo.TargetY);
	}
	if (Modulation != 0xffffffff) lpDDraw->DeactivateBlitModulation();
	// done
}

DWORD C4Sky::GetSkyFadeClr(int32_t iY)
{
	int32_t iPos2=(iY*256)/GBackHgt; int32_t iPos1=256-iPos2;
	return (((((FadeClr1&0xff00ff)*iPos1 + (FadeClr2&0xff00ff)*iPos2) & 0xff00ff00)
	         | (((FadeClr1&0x00ff00)*iPos1 + (FadeClr2&0x00ff00)*iPos2) & 0x00ff0000))>>8)
	       | (FadeClr1 & 0xff000000);
}

bool C4Sky::SetModulation(DWORD dwWithClr, DWORD dwBackClr)
{
	Modulation=dwWithClr;
	BackClr=dwBackClr;
	BackClrEnabled=(Modulation>>24 != 0xff) ? true : false;
	return true;
}

void C4Sky::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(mkCastIntAdapt(x),   "X",                     Fix0));
	pComp->Value(mkNamingAdapt(mkCastIntAdapt(y),   "Y",                     Fix0));
	pComp->Value(mkNamingAdapt(mkCastIntAdapt(xdir),"XDir",                  Fix0));
	pComp->Value(mkNamingAdapt(mkCastIntAdapt(ydir),"YDir",                  Fix0));
	pComp->Value(mkNamingAdapt(Modulation,      "Modulation",            0xffffffffU));
	pComp->Value(mkNamingAdapt(ParX,            "ParX",                  10));
	pComp->Value(mkNamingAdapt(ParY,            "ParY",                  10));
	pComp->Value(mkNamingAdapt(ParallaxMode,    "ParMode",               C4SkyPM_Fixed));
	pComp->Value(mkNamingAdapt(BackClr,         "BackClr",               0));
	pComp->Value(mkNamingAdapt(BackClrEnabled,  "BackClrEnabled",        false));
}
