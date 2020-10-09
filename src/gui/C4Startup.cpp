/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005-2009, RedWolf Design GmbH, http://www.clonk.de/
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
// Startup screen for non-parameterized engine start

#include "C4Include.h"
#include "C4ForbidLibraryCompilation.h"
#include "gui/C4Startup.h"

#include "game/C4Application.h"
#include "game/C4GraphicsSystem.h"
#include "graphics/C4FontLoader.h"
#include "graphics/C4GraphicsResource.h"
#include "gui/C4StartupAboutDlg.h"
#include "gui/C4StartupLegalDlg.h"
#include "gui/C4StartupMainDlg.h"
#include "gui/C4StartupNetDlg.h"
#include "gui/C4StartupOptionsDlg.h"
#include "gui/C4StartupPlrSelDlg.h"
#include "gui/C4StartupScenSelDlg.h"
#include "gui/C4StartupModsDlg.h"

bool C4StartupGraphics::LoadFile(C4FacetID &rToFct, const char *szFilename)
{
	return ::GraphicsResource.LoadFile(rToFct, szFilename, ::GraphicsResource.Files, C4FCT_Full, C4FCT_Full, false, 0);
}

bool C4StartupGraphics::Init()
{
	::GraphicsResource.ProgressStart = 50;
	::GraphicsResource.ProgressIncrement = 8;
	// load startup specific graphics from gfxsys groupset
	Game.SetInitProgress(38.0f);
	if (!LoadFile(fctDlgPaper, "StartupDlgPaper")) return false;
	if (!LoadFile(fctPlrPropBG, "StartupPlrPropBG")) return false;
	if (!LoadFile(fctAboutTitles, "StartupAboutTitles")) return false;
	fctAboutTitles.Set(fctAboutTitles.Surface,0,0,fctAboutTitles.Surface->Wdt,fctAboutTitles.Surface->Hgt/C4StartupAboutTitleCount);
	if (!LoadFile(fctStartupLogo, "StartupLogo")) return false;
	::GraphicsResource.ProgressStart = 92;
	::GraphicsResource.ProgressIncrement = 0.5;
	if (!LoadFile(fctMainButtons, "StartupBigButton")) return false;
	barMainButtons.SetHorizontal(fctMainButtons);
	if (!LoadFile(fctMainButtonsDown, "StartupBigButtonDown")) return false;
	barMainButtonsDown.SetHorizontal(fctMainButtonsDown);
	if (!LoadFile(fctBookScroll, "StartupBookScroll")) return false;
	sfctBookScroll.Set(fctBookScroll);
	sfctBookScrollR.Set(fctBookScroll, 1);
	sfctBookScrollG.Set(fctBookScroll, 2);
	sfctBookScrollB.Set(fctBookScroll, 3);
	if (!LoadFile(fctContext, "StartupContext")) return false;
	fctContext.Set(fctContext.Surface,0,0,fctContext.Hgt,fctContext.Hgt);
	if (!LoadFile(fctScenSelIcons, "StartupScenSelIcons")) return false;
	fctScenSelIcons.Wdt = fctScenSelIcons.Hgt; // icon width is determined by icon height
	if (!LoadFile(fctScenSelTitleOverlay, "StartupScenSelTitleOv")) return false;
	if (!LoadFile(fctOptionsIcons, "StartupOptionIcons")) return false;
	fctOptionsIcons.Set(fctOptionsIcons.Surface, 0,0,fctOptionsIcons.Hgt,fctOptionsIcons.Hgt);
	if (!LoadFile(fctOptionsTabClip, "StartupTabClip")) return false;
	if (!LoadFile(fctNetGetRef, "StartupNetGetRef")) return false;
	fctNetGetRef.Wdt = 40;
#ifndef USE_CONSOLE
	if (!InitFonts()) return false;
#endif
	Game.SetInitProgress(100);
	return true;
}

#ifndef USE_CONSOLE
bool C4StartupGraphics::InitFonts()
{
	const char *szFont = Config.General.RXFontName;
	if (!::FontLoader.InitFont(&BookFontCapt, szFont, C4FontLoader::C4FT_Caption, Config.General.RXFontSize, &::GraphicsResource.Files, false))
		{ LogFatal("Font Error (1)"); return false; }
	Game.SetInitProgress(97);
	if (!::FontLoader.InitFont(&BookFont, szFont, C4FontLoader::C4FT_Main, Config.General.RXFontSize, &::GraphicsResource.Files, false))
		{ LogFatal("Font Error (2)"); return false; }
	Game.SetInitProgress(98);
	if (!::FontLoader.InitFont(&BookFontTitle, szFont, C4FontLoader::C4FT_Title, Config.General.RXFontSize, &::GraphicsResource.Files, false))
		{ LogFatal("Font Error (3)"); return false; }
	Game.SetInitProgress(99);
	if (!::FontLoader.InitFont(&BookSmallFont, szFont, C4FontLoader::C4FT_MainSmall, Config.General.RXFontSize, &::GraphicsResource.Files, false))
		{ LogFatal("Font Error (4)"); return false; }
	return true;
}
#endif

CStdFont &C4StartupGraphics::GetBlackFontByHeight(int32_t iHgt, float *pfZoom)
{
	// get optimal font for given control size
	CStdFont *pUseFont;
	if (iHgt <= BookSmallFont.GetLineHeight()) pUseFont = &BookSmallFont;
	else if (iHgt <= BookFont.GetLineHeight()) pUseFont = &BookFont;
	else if (iHgt <= BookFontCapt.GetLineHeight()) pUseFont = &BookFontCapt;
	else pUseFont = &BookFontTitle;
	// determine zoom
	if (pfZoom)
	{
		int32_t iLineHgt = pUseFont->GetLineHeight();
		if (iLineHgt)
			*pfZoom = (float) iHgt / (float) iLineHgt;
		else
			*pfZoom = 1.0f; // error
	}
	return *pUseFont;
}

// statics
C4Startup::DialogID C4Startup::eLastDlgID = C4Startup::SDID_Main;
StdCopyStrBuf C4Startup::sSubDialog = StdCopyStrBuf();

// startup singleton instance
C4Startup *C4Startup::pInstance = nullptr;

C4Startup::C4Startup()
{
	// must be single!
	assert(!pInstance);
	pInstance = this;
}

C4Startup::~C4Startup()
{
	pInstance = nullptr;
	delete pLastDlg;
	delete pCurrDlg;
}

C4StartupDlg *C4Startup::SwitchDialog(DialogID eToDlg, bool fFade, const char *szSubDialog)
{
	// can't go back twice, because dialog is not remembered: Always go back to main in this case
	if (eToDlg == SDID_Back && (fLastDlgWasBack || !pLastDlg)) eToDlg = SDID_Main;
	fLastDlgWasBack = false;
	// create new dialog
	C4StartupDlg *pToDlg = nullptr;
	switch (eToDlg)
	{
	case SDID_Main:
		pToDlg = new C4StartupMainDlg();
		break;
	case SDID_ScenSel:
		pToDlg = new C4StartupScenSelDlg(false);
		break;
	case SDID_ScenSelNetwork:
		pToDlg = new C4StartupScenSelDlg(true);
		break;
	case SDID_NetJoin:
		pToDlg = new C4StartupNetDlg();
		break;
	case SDID_Options:
		pToDlg = new C4StartupOptionsDlg();
		break;
	case SDID_About:
		pToDlg = new C4StartupAboutDlg();
		break;
	case SDID_Legal:
		pToDlg = new C4StartupLegalDlg();
		break;
	case SDID_PlrSel:
		pToDlg = new C4StartupPlrSelDlg();
		break;
	case SDID_Mods:
		pToDlg = new C4StartupModsDlg();
		break;
	case SDID_Back:
		pToDlg = pLastDlg;
		fLastDlgWasBack = true;
		break;
	};
	assert(pToDlg);
	if (!pToDlg) return nullptr;
	if (pToDlg != pLastDlg)
	{
		// remember current position
		eLastDlgID = eToDlg;
		// kill any old dialog
		if (pLastDlg) delete pLastDlg;
	}
	// retain current dialog as last, so it can fade out and may be used later
	if ((pLastDlg = pCurrDlg))
	{
		if (fFade)
		{
			if (!pLastDlg->IsShown()) pLastDlg->Show(::pGUI, false);
			pLastDlg->FadeOut(true);
		}
		else
		{
			delete pLastDlg;
			pLastDlg = nullptr;
		}
	}
	// Okay; now using this dialog
	pCurrDlg = pToDlg;
	// go to dialog subscreen
	if (szSubDialog) pCurrDlg->SetSubscreen(szSubDialog);
	// fade in new dlg
	if (fFade)
	{
		if (!pToDlg->FadeIn(::pGUI))
		{
			delete pToDlg; pCurrDlg=nullptr;
			return nullptr;
		}
	}
	else
	{
		if (!pToDlg->Show(::pGUI, true))
		{
			delete pToDlg; pCurrDlg=nullptr;
			return nullptr;
		}
	}
	return pToDlg;
}

void C4Startup::DoStartup()
{
	assert(!fInStartup);
	assert(::pGUI);
	// now in startup!
	fInStartup = true;
	fLastDlgWasBack = false;

	::GraphicsSystem.EnableLoaderDrawing();

	// clear any previous
	if (pLastDlg) { delete pLastDlg; pLastDlg = nullptr; }
	if (pCurrDlg) { delete pCurrDlg; pCurrDlg = nullptr; }

	// start with the last dlg that was shown - at first startup main dialog
	SwitchDialog(eLastDlgID, true, sSubDialog.getData());

	// show error dlg if restart
	if (Game.fQuitWithError || GetFatalError())
	{
		Game.fQuitWithError = false;
		// preferred: Show fatal error
		const char *szErr = GetFatalError();
		if (szErr)
		{
			::pGUI->ShowMessage(szErr, LoadResStr("IDS_DLG_LOG"), C4GUI::Ico_Error);
		}
		else
		{
			// fallback to showing complete log
			StdStrBuf sLastLog;
			if (GetLogSection(Game.StartupLogPos, Game.QuitLogPos - Game.StartupLogPos, sLastLog))
				if (!sLastLog.isNull())
					::pGUI->ShowRemoveDlg(new C4GUI::InfoDialog(LoadResStr("IDS_DLG_LOG"), 10, sLastLog));
		}
		ResetFatalError();
	}
}

void C4Startup::DontStartup()
{
	// check whether startup was aborted
	delete pLastDlg; pLastDlg = nullptr;
	if (pCurrDlg)
	{
		// deinit last shown dlg
		if (pCurrDlg->IsShown())
		{
			pCurrDlg->Close(true);
		}
		delete pCurrDlg;
		pCurrDlg = nullptr;
	}

	// now no more in startup!
	fInStartup = false;

	// after startup: cleanup
	::pGUI->CloseAllDialogs(true);
}

void C4Startup::CloseStartup()
{
	if (pInstance) pInstance->DontStartup();
}

C4Startup *C4Startup::EnsureLoaded()
{
	// create and load startup data if not done yet
	assert(::pGUI);
	if (!pInstance)
	{
		Game.SetInitProgress(36.0f);
		C4Startup *pStartup = new C4Startup();
		Game.SetInitProgress(37.0f);
		// load startup specific gfx
		if (!pStartup->Graphics.Init())
			{ LogFatal(LoadResStr("IDS_ERR_NOGFXSYS")); delete pStartup; return nullptr; }
	}
	return pInstance;
}

void C4Startup::Unload()
{
	// make sure startup data is destroyed
	if (pInstance) { delete pInstance; pInstance=nullptr; }
}

void C4Startup::InitStartup()
{
	// ensure gfx are loaded
	C4Startup *pStartup = EnsureLoaded();
	if (!pStartup)
	{
		Application.Quit();
		return;
	}
	// exec it
	pStartup->DoStartup();
}

bool C4Startup::SetStartScreen(const char *szScreen, const char *szSubDialog)
{
	sSubDialog.Clear();
	if (szSubDialog != nullptr)
		sSubDialog = szSubDialog;

	// set dialog ID to be shown to specified value
	if (SEqualNoCase(szScreen, "main"))
		eLastDlgID = SDID_Main;
	if (SEqualNoCase(szScreen, "scen"))
		eLastDlgID = SDID_ScenSel;
	if (SEqualNoCase(szScreen, "netscen"))
		eLastDlgID = SDID_ScenSelNetwork;
	else if (SEqualNoCase(szScreen, "net"))
		eLastDlgID = SDID_NetJoin;
	else if (SEqualNoCase(szScreen, "mods"))
		eLastDlgID = SDID_Mods;
	else if (SEqualNoCase(szScreen, "options"))
		eLastDlgID = SDID_Options;
	else if (SEqual2NoCase(szScreen, "options-"))
	{
		eLastDlgID = SDID_Options;
		// subscreen of options
		sSubDialog.Copy(szScreen+8);
	}
	else if (SEqualNoCase(szScreen, "plrsel"))
		eLastDlgID = SDID_PlrSel;
	else if (SEqualNoCase(szScreen, "about"))
		eLastDlgID = SDID_About;
	else return false;
	return true;
}

void C4Startup::OnKeyboardLayoutChanged()
{
	// forward message to current dialog
	if (pCurrDlg) pCurrDlg->OnKeyboardLayoutChanged();
}

void C4Startup::OnLeagueOptionChanged()
{
	// forward message to current dialog
	if (pCurrDlg) pCurrDlg->OnLeagueOptionChanged();
}
