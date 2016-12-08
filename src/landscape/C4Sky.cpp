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

/* Small member of the landscape class to handle the sky background */

#include "C4Include.h"
#include "landscape/C4Sky.h"

#include "game/C4Game.h"
#include "lib/C4Random.h"
#include "c4group/C4Components.h"
#include "landscape/C4Weather.h"
#include "graphics/C4GraphicsResource.h"
#include "graphics/C4Draw.h"
#include "lib/StdColors.h"

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

bool C4Sky::Init(bool fSavegame, std::string names)
{
	// reset scrolling pos+speed
	// not in savegame, because it will have been loaded from game data there
	if (!fSavegame)
	{
		x=y=xdir=ydir=0; ParX=ParY=10; ParallaxMode=0;
	}

	// Check for sky bitmap in scenario file
	Surface = new C4Surface();
	bool loaded = false;
	if (names.empty())
	{
		loaded = !!Surface->LoadAny(Game.ScenarioFile,C4CFN_Sky,true,true, C4SF_Tileable | C4SF_MipMap);
	}

	// Else, evaluate scenario core landscape sky default list
	if (!loaded)
	{
		if (names.empty()) names = Game.C4S.Landscape.SkyDef;
		static std::regex separator(R"([,;\s]+)");
		std::vector<std::string> parts;
		std::copy(
				std::sregex_token_iterator(names.begin(), names.end(), separator, -1),
				std::sregex_token_iterator(),
				std::back_inserter(parts));

		auto name = parts.at(SeededRandom(Game.RandomSeed, parts.size()));
		// Sky tile specified, try load
		if (name != "Default")
		{
			// Check for sky tile in scenario file
			loaded = !!Surface->LoadAny(Game.ScenarioFile, name.c_str(), true, true, C4SF_Tileable | C4SF_MipMap);
			if (!loaded)
			{
				loaded = !!Surface->LoadAny(::GraphicsResource.Files, name.c_str(), true, false, C4SF_Tileable | C4SF_MipMap);
			}
		}
	}

	if (loaded)
	{
		// surface loaded, store first color index
		FadeClr1=FadeClr2=0xffffffff;

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

	// Load sky shaders: regular sprite shaders with OC_SKY define
	const char* const SkyDefines[] = { "OC_SKY", nullptr };
	if (!pDraw->PrepareSpriteShader(Shader, "Sky", Surface ? C4SSC_BASE : 0, &::GraphicsResource.Files, SkyDefines, nullptr))
		return false;
	if (!pDraw->PrepareSpriteShader(ShaderLight, "SkyLight", (Surface ? C4SSC_BASE : 0) | C4SSC_LIGHT, &::GraphicsResource.Files, SkyDefines, nullptr))
		return false;

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
	Surface=nullptr;
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
	Shader.Clear();
	ShaderLight.Clear();
	delete Surface; Surface=nullptr;
	Modulation=0xffffffff;
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
	if (BackClrEnabled) pDraw->DrawBoxDw(cgo.Surface, cgo.X, cgo.Y, cgo.X+cgo.Wdt, cgo.Y+cgo.Hgt, BackClr);
	// sky surface?
	if (Modulation != 0xffffffff) pDraw->ActivateBlitModulation(Modulation);
	C4ShaderCall call(pDraw->GetFoW() ? &ShaderLight : &Shader); // call is started in C4Draw
	if (Surface)
	{
		// blit parallax sky
		float zoom = cgo.Zoom;
		float targetx = cgo.TargetX; float targety = cgo.TargetY;

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

		pDraw->BlitSurfaceTile(Surface, cgo.Surface, cgo.X, cgo.Y, cgo.Wdt * zoom / resultzoom, cgo.Hgt * zoom / resultzoom, -resultx, -resulty, &call);
	}
	else
	{
		// no sky surface: blit sky fade
		DWORD dwClr1=GetSkyFadeClr(cgo.TargetY);
		DWORD dwClr2=GetSkyFadeClr(cgo.TargetY+cgo.Hgt);
		pDraw->DrawBoxFade(cgo.Surface, cgo.X, cgo.Y, cgo.Wdt, cgo.Hgt, dwClr1, dwClr1, dwClr2, dwClr2, &call);
	}
	if (Modulation != 0xffffffff) pDraw->DeactivateBlitModulation();
	// done
}

DWORD C4Sky::GetSkyFadeClr(int32_t iY)
{
	int32_t iPos2=(iY*256)/::Landscape.GetHeight(); int32_t iPos1=256-iPos2;
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
