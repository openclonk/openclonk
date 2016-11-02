/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2003-2009, RedWolf Design GmbH, http://www.clonk.de/
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
// startup screen

#include "C4Include.h"
#include "gui/C4LoaderScreen.h"

#include "c4group/C4Components.h"
#include "lib/C4LogBuf.h"
#include "lib/C4Log.h"
#include "graphics/C4GraphicsResource.h"
#include "graphics/C4Draw.h"
#include "lib/C4Random.h"
#include "c4group/C4GroupSet.h"
#include "game/C4Game.h"

C4LoaderScreen::C4LoaderScreen()
{
	// zero fields
	szInfo=nullptr;
	fBlackScreen = false;
}

C4LoaderScreen::~C4LoaderScreen()
{
	// clear fields
	if (szInfo) delete [] szInfo;
}

bool C4LoaderScreen::Init(const char *szLoaderSpec)
{
	// Determine loader specification
	if (!szLoaderSpec || !szLoaderSpec[0])
		szLoaderSpec = "Loader*";
	char szLoaderSpecPng[128 + 1 + 4], szLoaderSpecBmp[128 + 1 + 4];
	char szLoaderSpecJpg[128 + 1 + 4], szLoaderSpecJpeg[128 + 1 + 5];
	SCopy(szLoaderSpec, szLoaderSpecPng); DefaultExtension(szLoaderSpecPng, "png");
	SCopy(szLoaderSpec, szLoaderSpecBmp); DefaultExtension(szLoaderSpecBmp, "bmp");
	SCopy(szLoaderSpec, szLoaderSpecJpg); DefaultExtension(szLoaderSpecJpg, "jpg");
	SCopy(szLoaderSpec, szLoaderSpecJpeg); DefaultExtension(szLoaderSpecJpeg, "jpeg");
	int iLoaders=0;
	C4Group *pGroup=nullptr,*pChosenGrp=nullptr;
	char ChosenFilename[_MAX_PATH+1];
	// query groups of equal priority in set
	while ((pGroup=Game.GroupSet.FindGroup(C4GSCnt_Loaders, pGroup, true)))
	{
		iLoaders+=SeekLoaderScreens(*pGroup, szLoaderSpecPng, iLoaders, ChosenFilename, &pChosenGrp);
		iLoaders+=SeekLoaderScreens(*pGroup, szLoaderSpecJpeg, iLoaders, ChosenFilename, &pChosenGrp);
		iLoaders+=SeekLoaderScreens(*pGroup, szLoaderSpecJpg, iLoaders, ChosenFilename, &pChosenGrp);
		// lower the chance for any loader other than png
		iLoaders*=2;
		iLoaders+=SeekLoaderScreens(*pGroup, szLoaderSpecBmp, iLoaders, ChosenFilename, &pChosenGrp);
	}
	// nothing found? seek in main gfx grp
	C4Group GfxGrp;
	if (!iLoaders)
	{
		// open it
		GfxGrp.Close();
		if (!Reloc.Open(GfxGrp, C4CFN_Graphics))
		{
			LogFatal(FormatString(LoadResStr("IDS_PRC_NOGFXFILE"),C4CFN_Graphics,GfxGrp.GetError()).getData());
			return false;
		}
		// seek for png-loaders
		iLoaders=SeekLoaderScreens(GfxGrp, szLoaderSpecPng, iLoaders, ChosenFilename, &pChosenGrp);
		iLoaders+=SeekLoaderScreens(GfxGrp, szLoaderSpecJpg, iLoaders, ChosenFilename, &pChosenGrp);
		iLoaders+=SeekLoaderScreens(GfxGrp, szLoaderSpecJpeg, iLoaders, ChosenFilename, &pChosenGrp);
		iLoaders*=2;
		// seek for bmp-loaders
		iLoaders+=SeekLoaderScreens(GfxGrp, szLoaderSpecBmp, iLoaders, ChosenFilename, &pChosenGrp);
		// Still nothing found: fall back to general loader spec in main graphics group
		if (!iLoaders)
		{
			iLoaders = SeekLoaderScreens(GfxGrp, "Loader*.png", 0, ChosenFilename, &pChosenGrp);
			iLoaders += SeekLoaderScreens(GfxGrp, "Loader*.jpg", iLoaders, ChosenFilename, &pChosenGrp);
			iLoaders += SeekLoaderScreens(GfxGrp, "Loader*.jpeg", iLoaders, ChosenFilename, &pChosenGrp);
		}
		// Not even default loaders available? Fail.
		if (!iLoaders)
		{
			LogFatal(FormatString("No loaders found for loader specification: %s/%s/%s/%s", szLoaderSpecPng, szLoaderSpecBmp, szLoaderSpecJpg, szLoaderSpecJpeg).getData());
			return false;
		}
	}

	// load loader
	fctBackground.GetFace().SetBackground();
	if (!fctBackground.Load(*pChosenGrp,ChosenFilename, C4FCT_Full,C4FCT_Full,true,0)) return false;

	// load info
	if (szInfo) { delete [] szInfo; szInfo=nullptr; }

	// done, success!
	return true;
}

void C4LoaderScreen::SetBlackScreen(bool fIsBlack)
{
	// enabled/disables drawing of loader screen
	fBlackScreen = fIsBlack;
	// will be updated when drawn next time
}

int C4LoaderScreen::SeekLoaderScreens(C4Group &rFromGrp, const char *szWildcard, int iLoaderCount, char *szDstName, C4Group **ppDestGrp)
{
	bool fFound;
	int iLocalLoaders=0;
	char Filename[_MAX_PATH+1];
	for (fFound=rFromGrp.FindEntry(szWildcard, Filename); fFound; fFound=rFromGrp.FindNextEntry(szWildcard, Filename))
	{
		// loader found; choose it, if Daniel wants it that way
		++iLocalLoaders;
		if (!UnsyncedRandom(++iLoaderCount))
		{
			// copy group and path
			*ppDestGrp=&rFromGrp;
			SCopy(Filename, szDstName, _MAX_PATH);
		}
	}
	return iLocalLoaders;
}

void C4LoaderScreen::Draw(C4Facet &cgo, int iProgress, C4LogBuffer *pLog, int Process)
{
	// simple black screen loader?
	if (fBlackScreen)
	{
		pDraw->FillBG();
		return;
	}
	// cgo.X/Y is assumed 0 here...
	// fixed positions for now
	int iHIndent=20;
	int iVIndent=20;
	int iLogBoxHgt=84;
	int iLogBoxMargin=2;
	int iVMargin=5;
	int iProgressBarHgt=15;
	CStdFont &LogFont=::GraphicsResource.FontTiny, &rProgressBarFont=::GraphicsResource.FontRegular;
	CStdFont &TitleFont = ::GraphicsResource.FontTitle;
	float fLogBoxFontZoom=1.0f;
	// Background (loader)
	fctBackground.DrawFullScreen(cgo);
	// draw scenario title
	pDraw->StringOut(Game.ScenarioTitle.getData(), TitleFont, 1.0f, cgo.Surface, cgo.Wdt-iHIndent, cgo.Hgt-iVIndent-iLogBoxHgt-iVMargin-iProgressBarHgt-iVMargin-TitleFont.GetLineHeight(), 0xdddddddd, ARight, false);
	//
	// draw progress bar
	pDraw->DrawBoxDw(cgo.Surface, iHIndent, cgo.Hgt-iVIndent-iLogBoxHgt-iVMargin-iProgressBarHgt, cgo.Wdt-iHIndent, cgo.Hgt-iVIndent-iLogBoxHgt-iVMargin, 0xb0000000);
	int iProgressBarWdt=cgo.Wdt-iHIndent*2-2;
	if (::GraphicsResource.fctProgressBar.Surface)
	{
		::GraphicsResource.fctProgressBar.DrawX(cgo.Surface, iHIndent+1, cgo.Hgt-iVIndent-iLogBoxHgt-iVMargin-iProgressBarHgt+1, iProgressBarWdt*iProgress/100, iProgressBarHgt-2);
	}
	else
	{
		pDraw->DrawBoxDw(cgo.Surface, iHIndent+1, cgo.Hgt-iVIndent-iLogBoxHgt-iVMargin-iProgressBarHgt+1, iHIndent+1+iProgressBarWdt*iProgress/100, cgo.Hgt-iVIndent-iLogBoxHgt-iVMargin-1, 0xb0ff0000);
	}
	pDraw->StringOut(FormatString("%i%%", iProgress).getData(), rProgressBarFont, 1.0f, cgo.Surface,
	                             cgo.Wdt/2, cgo.Hgt-iVIndent-iLogBoxHgt-iVMargin-rProgressBarFont.GetLineHeight()/2-iProgressBarHgt/2, 0xffffffff,
	                             ACenter, true);
	// draw log box
	if (pLog)
	{
		pDraw->DrawBoxDw(cgo.Surface, iHIndent, cgo.Hgt-iVIndent-iLogBoxHgt, cgo.Wdt-iHIndent, cgo.Hgt-iVIndent, 0x7f000000);
		int iLineHgt=int(fLogBoxFontZoom*LogFont.GetLineHeight()); if (!iLineHgt) iLineHgt=5;
		int iLinesVisible = (iLogBoxHgt-2*iLogBoxMargin)/iLineHgt;
		int iX = iHIndent+iLogBoxMargin;
		int iY = cgo.Hgt-iVIndent-iLogBoxHgt+iLogBoxMargin;
		int32_t w,h;
		for (int i = -iLinesVisible; i < 0; ++i)
		{
			const char *szLine = pLog->GetLine(i, nullptr, nullptr, nullptr);
			if (!szLine || !*szLine) continue;
			LogFont.GetTextExtent(szLine, w,h, true);
			pDraw->TextOut(szLine,LogFont,fLogBoxFontZoom,cgo.Surface,iX,iY);
			iY += h;
		}
		// append process text
		if (Process)
		{
			iY -= h; iX += w;
			pDraw->TextOut(FormatString("%i%%", (int) Process).getData(),LogFont,fLogBoxFontZoom,cgo.Surface,iX,iY);
		}
	}
}
