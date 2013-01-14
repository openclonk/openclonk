/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2008  Matthes Bender
 * Copyright (c) 2001-2003, 2005, 2008  Sven Eberhardt
 * Copyright (c) 2005-2007, 2010-2011  Günther Brammer
 * Copyright (c) 2006-2007  Julian Raschke
 * Copyright (c) 2009  Nicolas Hake
 * Copyright (c) 2009-2010  Martin Plicht
 * Copyright (c) 2010  Benjamin Herr
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

/* Main class to execute the game fullscreen mode */

#include <C4Include.h>
#include <C4FullScreen.h>

#include <C4Game.h>
#include <C4Application.h>
#include <C4Viewport.h>
#include <C4League.h>
#include <C4Language.h>
#include <C4Gui.h>
#include <C4Network2.h>
#include <C4GameDialogs.h>
#include <C4GamePadCon.h>
#include <C4Player.h>
#include <C4GameOverDlg.h>
#include <C4GraphicsSystem.h>
#include <C4MouseControl.h>
#include <C4PlayerList.h>

void C4FullScreen::CharIn(const char * c) { ::pGUI->CharIn(c); }

C4FullScreen::C4FullScreen()
{
	pMenu = NULL;
}

C4FullScreen::~C4FullScreen()
{
	if (pMenu) delete pMenu;
	if (pSurface) delete pSurface;
}


C4Window * C4FullScreen::Init(C4AbstractApp * pApp)
{
	C4Rect r(0, 0, Application.GetConfigWidth(), Application.GetConfigHeight());
	return Init(C4Window::W_Fullscreen, pApp, C4ENGINECAPTION, &r);
}

void C4FullScreen::Close()
{
	if (Game.IsRunning)
		ShowAbortDlg();
	else
		Application.Quit();
}

void C4FullScreen::Clear()
{
	if (pSurface) delete pSurface;
	pSurface = 0;
	C4Window::Clear();
}

void C4FullScreen::Execute()
{
	// Execute menu
	if (pMenu) pMenu->Execute();
	// Draw
	RequestUpdate();
}

bool C4FullScreen::ViewportCheck()
{
	int iPlrNum; C4Player *pPlr;
	// Not active
	if (!Active) return false;
	// Determine film mode
	bool fFilm = (Game.C4S.Head.Replay && Game.C4S.Head.Film);
	// Check viewports
	switch (::Viewports.GetViewportCount())
	{
		// No viewports: create no-owner viewport
	case 0:
		iPlrNum = NO_OWNER;
		// Film mode: create viewport for first player (instead of no-owner)
		if (fFilm)
			if ((pPlr = ::Players.First))
				iPlrNum = pPlr->Number;
		// Create viewport
		::Viewports.CreateViewport(iPlrNum, iPlrNum==NO_OWNER);
		// Non-film (observer mode)
		if (!fFilm)
		{
			// Activate mouse control
			::MouseControl.Init(iPlrNum);
			// Display message for how to open observer menu (this message will be cleared if any owned viewport opens)
			StdStrBuf sKey;
			sKey.Format("<c ffff00><%s></c>", Game.KeyboardInput.GetKeyCodeNameByKeyName("FullscreenMenuOpen", false).getData());
			::GraphicsSystem.FlashMessage(FormatString(LoadResStr("IDS_MSG_PRESSORPUSHANYGAMEPADBUTT"), sKey.getData()).getData());
		}
		break;
		// One viewport: do nothing
	case 1:
		break;
		// More than one viewport: remove all no-owner viewports
	default:
		::Viewports.CloseViewport(NO_OWNER, true);
		break;
	}
	// Look for no-owner viewport
	C4Viewport *pNoOwnerVp = ::Viewports.GetViewport(NO_OWNER);
	// No no-owner viewport found
	if (!pNoOwnerVp)
	{
		// Close any open fullscreen menu
		CloseMenu();
	}
	// No-owner viewport present
	else
	{
		// movie mode: player present, and no valid viewport assigned?
		if (Game.C4S.Head.Replay && Game.C4S.Head.Film && (pPlr = ::Players.First))
			// assign viewport to joined player
			pNoOwnerVp->Init(pPlr->Number, true);
	}
	// Done
	return true;
}

bool C4FullScreen::ShowAbortDlg()
{
	// abort dialog already shown
	if (C4AbortGameDialog::IsShown()) return false;
	// not while game over dialog is open
	if (C4GameOverDlg::IsShown()) return false;
	// show abort dialog
	return ::pGUI->ShowRemoveDlg(new C4AbortGameDialog());
}

bool C4FullScreen::ActivateMenuMain()
{
	// Not during game over dialog
	if (C4GameOverDlg::IsShown()) return false;
	// Close previous
	CloseMenu();
	// Open menu
	pMenu = new C4MainMenu();
	return pMenu->ActivateMain(NO_OWNER);
}

void C4FullScreen::CloseMenu()
{
	if (pMenu)
	{
		if (pMenu->IsActive()) pMenu->Close(false);
		delete pMenu;
		pMenu = NULL;
	}
}

void C4FullScreen::PerformUpdate()
{
	GraphicsSystem.Execute();
}

bool C4FullScreen::MenuKeyControl(BYTE byCom)
{
	if (pMenu) return pMenu->KeyControl(byCom);
	return false;
}
