/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001  Michael Käser
 * Copyright (c) 2006  Günther Brammer
 * Copyright (c) 2007  Matthes Bender
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
#include <C4Include.h>
#include <C4UpperBoard.h>

#ifndef BIG_C4INCLUDE
#include <C4Game.h>
#include <C4Config.h>
#include <C4Application.h>
#include <C4GraphicsResource.h>
#endif

C4UpperBoard::C4UpperBoard()
	{
	Default();
	}

C4UpperBoard::~C4UpperBoard()
	{
	Clear();
	}

void C4UpperBoard::Default()
	{
	}

void C4UpperBoard::Clear()
	{
	}

void C4UpperBoard::Execute()
	{
	if(!Config.Graphics.UpperBoard) return;
	// Make the time strings
	sprintf(cTimeString,"%02d:%02d:%02d", Game.Time/3600,(Game.Time%3600)/60,Game.Time%60);
	time_t t = time(0); strftime(cTimeString2, sizeof(cTimeString2), "[%H:%M:%S]", localtime(&t));
	Draw(Output);
	}

void C4UpperBoard::Draw(C4Facet &cgo)
	{
	if(!cgo.Surface) return;
	// Background
	Application.DDraw->BlitSurfaceTile(::GraphicsResource.fctUpperBoard.Surface,Output.Surface,0,0,Output.Wdt,Output.Hgt);
  // Logo
	C4Facet cgo2;
	float fLogoZoom = 0.75f;
	cgo2.Set(cgo.Surface, (int32_t)(cgo.Wdt/2-(::GraphicsResource.fctLogo.Wdt/2)*fLogoZoom), 0,
		                    (int32_t)(::GraphicsResource.fctLogo.Wdt*fLogoZoom), (int32_t)(::GraphicsResource.fctLogo.Hgt*fLogoZoom));
	::GraphicsResource.fctLogo.Draw(cgo2);
	// Right text sections
	int32_t iRightOff = 1;
	// Playing time
	Application.DDraw->TextOut(cTimeString, ::GraphicsResource.FontRegular, 1.0, cgo.Surface, C4GUI::GetScreenWdt() - (iRightOff++) * TextWidth - 10, TextYPosition, 0xFFFFFFFF);
	// Clock
	if (Config.Graphics.ShowClock)
		Application.DDraw->TextOut(cTimeString2, ::GraphicsResource.FontRegular, 1.0, cgo.Surface, C4GUI::GetScreenWdt() - (iRightOff++) * TextWidth - 30, TextYPosition, 0xFFFFFFFF);
	// FPS
	if (Config.General.FPS)
		{
		sprintf(cTimeString, "%d FPS", Game.FPS);
		Application.DDraw->TextOut(cTimeString, ::GraphicsResource.FontRegular, 1.0, cgo.Surface, C4GUI::GetScreenWdt() - (iRightOff++) * TextWidth - 30, TextYPosition, 0xFFFFFFFF);
		}
	// Scenario title
	Application.DDraw->TextOut(Game.ScenarioTitle.getData(), ::GraphicsResource.FontRegular, 1.0, cgo.Surface, 10, cgo.Hgt / 2 - ::GraphicsResource.FontRegular.GetLineHeight() / 2, 0xFFFFFFFF);
	}

void C4UpperBoard::Init(C4Facet &cgo)
	{
	// Save facet
	Output = cgo;
	if (!::GraphicsResource.fctUpperBoard.Surface) return;
	// in newgfx, the upperboard may be larger and overlap the scene
	Output.Hgt = Max(Output.Hgt, ::GraphicsResource.fctUpperBoard.Hgt);
	// surface should not be too small
	::GraphicsResource.fctUpperBoard.EnsureSize(128, Output.Hgt);
	// Generate textposition
	sprintf(cTimeString,"%02d:%02d:%02d", Game.Time/3600,(Game.Time%3600)/60,Game.Time%60);
	TextWidth = ::GraphicsResource.FontRegular.GetTextWidth(cTimeString);
	TextYPosition = cgo.Hgt / 2 - ::GraphicsResource.FontRegular.GetLineHeight() / 2;
}
