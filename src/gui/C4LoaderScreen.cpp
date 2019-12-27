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
#include "c4group/C4GroupSet.h"
#include "graphics/C4Draw.h"
#include "graphics/C4GraphicsResource.h"
#include "lib/C4LogBuf.h"
#include "lib/C4Random.h"


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

bool C4LoaderScreen::Init(std::string loaderSpec)
{
	// Determine loader specification
	if (loaderSpec.empty())
		loaderSpec = "Loader*";

	C4Group *pGroup = nullptr;
	// query groups of equal priority in set
	while ((pGroup=Game.GroupSet.FindGroup(C4GSCnt_Loaders, pGroup, true)))
	{
		SeekLoaderScreens(*pGroup, loaderSpec);
	}
	// nothing found? seek in main gfx grp
	C4Group GfxGrp;
	if (loaders.empty())
	{
		// open it
		GfxGrp.Close();
		if (!Reloc.Open(GfxGrp, C4CFN_Graphics))
		{
			LogFatal(FormatString(LoadResStr("IDS_PRC_NOGFXFILE"),C4CFN_Graphics,GfxGrp.GetError()).getData());
			return false;
		}
		// seek for loaders
		SeekLoaderScreens(GfxGrp, loaderSpec);

		// Still nothing found: fall back to general loader spec in main graphics group
		if (loaders.empty())
		{
			SeekLoaderScreens(GfxGrp, "Loader*");
		}
		// Not even default loaders available? Fail.
		if (loaders.empty())
		{
			LogFatal(FormatString("No loaders found for loader specification: %s", loaderSpec.c_str()).getData());
			return false;
		}
	}

	// choose random loader
	auto entry = loaders.begin();
	std::advance(entry, UnsyncedRandom(loaders.size()));

	// load loader
	fctBackground.GetFace().SetBackground();
	if (!fctBackground.Load(*(entry->first), entry->second.c_str(), C4FCT_Full, C4FCT_Full, true, 0)) return false;

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

void C4LoaderScreen::SeekLoaderScreens(C4Group &rFromGrp, const std::string &wildcard)
{
	// seek for png, jpg, jpeg, bmp
	char filename[_MAX_PATH_LEN];
	for (bool found = rFromGrp.FindEntry(wildcard.c_str(), filename); found; found = rFromGrp.FindNextEntry(wildcard.c_str(), filename))
	{
		// potential candidate - check file extension
		std::string extension{ GetExtension(filename) };
		std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
		if (extension == "png" || extension == "jpg" || extension == "jpeg" || extension == "bmp") {
			loaders.emplace(&rFromGrp, std::string(filename));
		}
	}
}

void C4LoaderScreen::Draw(C4Facet &cgo, Flag options, int iProgress, C4LogBuffer *pLog, int Process)
{
	// simple black screen loader?
	if (fBlackScreen || options == Flag::BLACK)
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

	if (options & Flag::BACKGROUND) {
		// Background (loader)
		fctBackground.DrawFullScreen(cgo);
	}

	if (options & Flag::TITLE) {
		// draw scenario title
		pDraw->StringOut(Game.ScenarioTitle.getData(), TitleFont, 1.0f, cgo.Surface, cgo.Wdt - iHIndent, cgo.Hgt - iVIndent - iLogBoxHgt - iVMargin - iProgressBarHgt - iVMargin - TitleFont.GetLineHeight(), 0xdddddddd, ARight, false);
	}

	if (options & Flag::PROGRESS) {
		// draw progress bar
		pDraw->DrawBoxDw(cgo.Surface, iHIndent, cgo.Hgt - iVIndent - iLogBoxHgt - iVMargin - iProgressBarHgt, cgo.Wdt - iHIndent, cgo.Hgt - iVIndent - iLogBoxHgt - iVMargin, 0xb0000000);
		int iProgressBarWdt = cgo.Wdt - iHIndent * 2 - 2;
		if (::GraphicsResource.fctProgressBar.Surface)
		{
			::GraphicsResource.fctProgressBar.DrawX(cgo.Surface, iHIndent + 1, cgo.Hgt - iVIndent - iLogBoxHgt - iVMargin - iProgressBarHgt + 1, iProgressBarWdt*iProgress / 100, iProgressBarHgt - 2);
		}
		else
		{
			pDraw->DrawBoxDw(cgo.Surface, iHIndent + 1, cgo.Hgt - iVIndent - iLogBoxHgt - iVMargin - iProgressBarHgt + 1, iHIndent + 1 + iProgressBarWdt*iProgress / 100, cgo.Hgt - iVIndent - iLogBoxHgt - iVMargin - 1, 0xb0ff0000);
		}
		pDraw->StringOut(FormatString("%i%%", iProgress).getData(), rProgressBarFont, 1.0f, cgo.Surface,
			cgo.Wdt / 2, cgo.Hgt - iVIndent - iLogBoxHgt - iVMargin - rProgressBarFont.GetLineHeight() / 2 - iProgressBarHgt / 2, 0xffffffff,
			ACenter, true);
	}

	if (options & Flag::LOG) {
		// draw log box
		if (pLog)
		{
			pDraw->DrawBoxDw(cgo.Surface, iHIndent, cgo.Hgt - iVIndent - iLogBoxHgt, cgo.Wdt - iHIndent, cgo.Hgt - iVIndent, 0x7f000000);
			int iLineHgt = int(fLogBoxFontZoom*LogFont.GetLineHeight()); if (!iLineHgt) iLineHgt = 5;
			int iLinesVisible = (iLogBoxHgt - 2 * iLogBoxMargin) / iLineHgt;
			int iX = iHIndent + iLogBoxMargin;
			int iY = cgo.Hgt - iVIndent - iLogBoxHgt + iLogBoxMargin;
			int32_t w, h;
			for (int i = -iLinesVisible; i < 0; ++i)
			{
				const char *szLine = pLog->GetLine(i, nullptr, nullptr, nullptr);
				if (!szLine || !*szLine) continue;
				LogFont.GetTextExtent(szLine, w, h, true);
				pDraw->TextOut(szLine, LogFont, fLogBoxFontZoom, cgo.Surface, iX, iY);
				iY += h;
			}

			if (options & Flag::PROCESS) {
				// append process text
				if (Process)
				{
					iY -= h; iX += w;
					pDraw->TextOut(FormatString("%i%%", (int)Process).getData(), LogFont, fLogBoxFontZoom, cgo.Surface, iX, iY);
				}
			}
		}
	}
}
