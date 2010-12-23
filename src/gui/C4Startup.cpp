/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005-2007  Sven Eberhardt
 * Copyright (c) 2006-2007, 2009-2010  Günther Brammer
 * Copyright (c) 2007  Matthes Bender
 * Copyright (c) 2010  Benjamin Herr
 * Copyright (c) 2005-2009, RedWolf Design GmbH, http://www.clonk.de
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
// Startup screen for non-parameterized engine start

#include <C4Include.h>
#include <C4Startup.h>

#include <C4StartupMainDlg.h>
#include <C4StartupScenSelDlg.h>
#include <C4StartupNetDlg.h>
#include <C4StartupOptionsDlg.h>
#include <C4StartupAboutDlg.h>
#include <C4StartupPlrSelDlg.h>
#include <C4Game.h>
#include <C4Application.h>
#include <C4Log.h>
#include <C4GraphicsResource.h>
#include <C4GraphicsSystem.h>
#include <C4Fonts.h>

bool C4StartupGraphics::LoadFile(C4FacetID &rToFct, const char *szFilename)
{
	return ::GraphicsResource.LoadFile(rToFct, szFilename, ::GraphicsResource.Files);
}

bool C4StartupGraphics::Init()
{
	::GraphicsResource.ProgressStart = 50;
	::GraphicsResource.ProgressIncrement = 8;
	// load startup specific graphics from gfxsys groupset
	fctScenSelBG.GetFace().SetBackground();
	Game.SetInitProgress(38.0f);
	if (!LoadFile(fctScenSelBG, "StartupScenSelBG")) return false;
	if (!LoadFile(fctPlrSelBG, "StartupPlrSelBG")) return false;
	if (!LoadFile(fctPlrPropBG, "StartupPlrPropBG")) return false;
	if (!LoadFile(fctNetBG, "StartupNetworkBG")) return false;
	if (!LoadFile(fctAboutBG, "StartupAboutBG")) return false;
	if (!LoadFile(fctOptionsDlgPaper, "StartupDlgPaper")) return false;
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
	/*  if (!LoadFile(fctCrew, "StartupCrew")) return false; - currently unused
	  if (fctCrew.idSourceGroup != fctCrewClr.idSourceGroup)
	    {
	    if (!fctCrewClr.CreateClrByOwner(fctCrew.Surface)) { LogFatal("ClrByOwner error! (11)"); return false; }
	    fctCrewClr.Wdt=fctCrew.Wdt;
	    fctCrewClr.Hgt=fctCrew.Hgt;
	    fctCrewClr.idSourceGroup = fctCrew.idSourceGroup;
	    }*/
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
	if (!::FontLoader.InitFont(BookFontCapt, szFont, C4FontLoader::C4FT_Caption, Config.General.RXFontSize, &::GraphicsResource.Files, false))
		{ LogFatal("Font Error (1)"); return false; }
	Game.SetInitProgress(97);
	if (!::FontLoader.InitFont(BookFont, szFont, C4FontLoader::C4FT_Main, Config.General.RXFontSize, &::GraphicsResource.Files, false))
		{ LogFatal("Font Error (2)"); return false; }
	Game.SetInitProgress(98);
	if (!::FontLoader.InitFont(BookFontTitle, szFont, C4FontLoader::C4FT_Title, Config.General.RXFontSize, &::GraphicsResource.Files, false))
		{ LogFatal("Font Error (3)"); return false; }
	Game.SetInitProgress(99);
	if (!::FontLoader.InitFont(BookSmallFont, szFont, C4FontLoader::C4FT_MainSmall, Config.General.RXFontSize, &::GraphicsResource.Files, false))
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
bool C4Startup::fFirstRun = false;

// startup singleton instance
C4Startup *C4Startup::pInstance = NULL;

C4Startup::C4Startup() : fInStartup(false), fAborted(false), pLastDlg(NULL), pCurrDlg(NULL)
{
	// must be single!
	assert(!pInstance);
	pInstance = this;
}

C4Startup::~C4Startup()
{
	pInstance = NULL;
	delete pLastDlg;
	delete pCurrDlg;
}

void C4Startup::Start()
{
	assert(fInStartup);
	// record if desired
	if (Config.General.Record) Game.Record = true;
	// flag game start
	fAborted = false;
	fInStartup = false;
	fLastDlgWasBack = false;
}

void C4Startup::Exit()
{
	assert(fInStartup);
	// flag game start
	fAborted = true;
	fInStartup = false;
}

C4StartupDlg *C4Startup::SwitchDialog(DialogID eToDlg, bool fFade)
{
	// can't go back twice, because dialog is not remembered: Always go back to main in this case
	if (eToDlg == SDID_Back && (fLastDlgWasBack || !pLastDlg)) eToDlg = SDID_Main;
	fLastDlgWasBack = false;
	// create new dialog
	C4StartupDlg *pToDlg = NULL;
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
	case SDID_PlrSel:
		pToDlg = new C4StartupPlrSelDlg();
		break;
	case SDID_Back:
		pToDlg = pLastDlg;
		fLastDlgWasBack = true;
		break;
	};
	assert(pToDlg);
	if (!pToDlg) return NULL;
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
			pLastDlg = NULL;
		}
	}
	// Okay; now using this dialog
	pCurrDlg = pToDlg;
	// fade in new dlg
	if (fFade)
	{
		if (!pToDlg->FadeIn(::pGUI))
		{
			delete pToDlg; pCurrDlg=NULL;
			return NULL;
		}
	}
	else
	{
		if (!pToDlg->Show(::pGUI, true))
		{
			delete pToDlg; pCurrDlg=NULL;
			return NULL;
		}
	}
	return pToDlg;
}

bool C4Startup::DoStartup()
{
	assert(!fInStartup);
	assert(::pGUI);
	// now in startup!
	fInStartup = true;
	fLastDlgWasBack = false;

	// first run: Splash video
#ifndef USE_CONSOLE
	if (!fFirstRun)
	{
		fFirstRun = true;
		if (!Config.Startup.NoSplash && !Application.NoSplash)
		{
			Game.VideoPlayer.PlayVideo(C4CFN_Splash);
		}
	}
#endif

	// make sure loader is drawn after splash
	::GraphicsSystem.EnableLoaderDrawing();

	// Play some music!
	if (Config.Sound.FEMusic)
		Application.MusicSystem.Play();

	// clear any previous
	if (pLastDlg) { delete pLastDlg; pLastDlg = NULL; }
	if (pCurrDlg) { delete pCurrDlg; pCurrDlg = NULL; }

	// start with the last dlg that was shown - at first startup main dialog
	if (!SwitchDialog(eLastDlgID)) return false;

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

	// while state startup: keep looping
	while (fInStartup && !pCurrDlg->IsAborted())
		if (!Application.ScheduleProcs()) return false;

	// check whether startup was aborted
	if (pLastDlg) { delete pLastDlg; pLastDlg = NULL; }
	if (pCurrDlg)
	{
		// deinit last shown dlg
		if (pCurrDlg->IsAborted())
		{
			// force abort flag if dlg abort done by user
			fAborted = true;
		}
		else if (pCurrDlg->IsShown())
		{
			pCurrDlg->Close(true);
		}
		delete pCurrDlg;
		pCurrDlg = NULL;
	}

	// now no more in startup!
	fInStartup = false;

	// after startup: cleanup
	::pGUI->CloseAllDialogs(true);

	// reinit keyboard to reflect any config changes that might have been done
	// this is a good time to do it, because no GUI dialogs are opened
	if (!Game.InitKeyboard()) LogFatal(LoadResStr("IDS_ERR_NOKEYBOARD"));

	// all okay; return whether startup finished with a game start selection
	return !fAborted;
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
			{ LogFatal(LoadResStr("IDS_ERR_NOGFXSYS")); delete pStartup; return NULL; }
	}
	return pInstance;
}

void C4Startup::Unload()
{
	// make sure startup data is destroyed
	if (pInstance) { delete pInstance; pInstance=NULL; }
}

bool C4Startup::Execute()
{
	// ensure gfx are loaded
	C4Startup *pStartup = EnsureLoaded();
	if (!pStartup) return false;
	// exec it
	bool fResult = pStartup->DoStartup();
	return fResult;
}

bool C4Startup::SetStartScreen(const char *szScreen)
{
	// set dialog ID to be shown to specified value
	if (SEqualNoCase(szScreen, "main"))
		eLastDlgID = SDID_Main;
	if (SEqualNoCase(szScreen, "scen"))
		eLastDlgID = SDID_ScenSel;
	if (SEqualNoCase(szScreen, "netscen"))
		eLastDlgID = SDID_ScenSelNetwork;
	else if (SEqualNoCase(szScreen, "net"))
		eLastDlgID = SDID_NetJoin;
	else if (SEqualNoCase(szScreen, "options"))
		eLastDlgID = SDID_Options;
	else if (SEqualNoCase(szScreen, "plrsel"))
		eLastDlgID = SDID_PlrSel;
	else if (SEqualNoCase(szScreen, "about"))
		eLastDlgID = SDID_About;
	else return false;
	return true;
}
