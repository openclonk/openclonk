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
#include "C4Include.h"
#include "gui/C4UpperBoard.h"

#include "game/C4Game.h"
#include "config/C4Config.h"
#include "graphics/C4GraphicsResource.h"
#include "graphics/C4Draw.h"

C4UpperBoard::C4UpperBoard()
{
}

C4UpperBoard::~C4UpperBoard()
{
}

void C4UpperBoard::Execute()
{
	if (!Config.Graphics.UpperBoard) return;
	// Make the time strings
	sprintf(cTimeString,"%02d:%02d:%02d", Game.Time/3600,(Game.Time%3600)/60,Game.Time%60);
	time_t t = time(0); strftime(cTimeString2, sizeof(cTimeString2), "[%H:%M:%S]", localtime(&t));
	Draw(Output);
}

void C4UpperBoard::Draw(C4Facet &cgo)
{
	if (!cgo.Surface) return;
	// Background
	pDraw->BlitSurfaceTile(::GraphicsResource.fctUpperBoard.Surface,Output.Surface,0,0,Output.Wdt,Output.Hgt,0,0,nullptr);
	// Logo
	C4Facet cgo2;
	float fLogoZoom = 1.0f;
	cgo2.Set(cgo.Surface, (int32_t)(cgo.Wdt/2-(::GraphicsResource.fctLogo.Wdt/2)*fLogoZoom), 0,
	         (int32_t)(::GraphicsResource.fctLogo.Wdt*fLogoZoom), (int32_t)(::GraphicsResource.fctLogo.Hgt*fLogoZoom));
	::GraphicsResource.fctLogo.Draw(cgo2);
	// Right text sections
	int32_t iRightOff = 1;
	// Playing time
	pDraw->TextOut(cTimeString, ::GraphicsResource.FontRegular, 1.0, cgo.Surface, C4GUI::GetScreenWdt() - (iRightOff++) * TextWidth - 10, TextYPosition, 0xFFFFFFFF);
	// Clock
	if (Config.Graphics.ShowClock)
		pDraw->TextOut(cTimeString2, ::GraphicsResource.FontRegular, 1.0, cgo.Surface, C4GUI::GetScreenWdt() - (iRightOff++) * TextWidth - 30, TextYPosition, 0xFFFFFFFF);
	// FPS
	if (Config.General.FPS)
	{
		sprintf(cTimeString, "%d FPS", Game.FPS);
		pDraw->TextOut(cTimeString, ::GraphicsResource.FontRegular, 1.0, cgo.Surface, C4GUI::GetScreenWdt() - (iRightOff++) * TextWidth - 30, TextYPosition, 0xFFFFFFFF);
	}
	// Scenario title
	pDraw->TextOut(Game.ScenarioTitle.getData(), ::GraphicsResource.FontRegular, 1.0, cgo.Surface, 10, cgo.Hgt / 2 - ::GraphicsResource.FontRegular.GetLineHeight() / 2, 0xFFFFFFFF);
}

void C4UpperBoard::Init(C4Facet &cgo)
{
	// Save facet
	Output = cgo;
	if (!::GraphicsResource.fctUpperBoard.Surface) return;
	// in newgfx, the upperboard may be larger and overlap the scene
	Output.Hgt = std::max(Output.Hgt, ::GraphicsResource.fctUpperBoard.Hgt);
	// Generate textposition
	sprintf(cTimeString,"%02d:%02d:%02d", Game.Time/3600,(Game.Time%3600)/60,Game.Time%60);
	TextWidth = ::GraphicsResource.FontRegular.GetTextWidth(cTimeString);
	TextYPosition = cgo.Hgt / 2 - ::GraphicsResource.FontRegular.GetLineHeight() / 2;
}
