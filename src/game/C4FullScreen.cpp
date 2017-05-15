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

/* Main class to execute the game fullscreen mode */

#include "C4Include.h"
#include "game/C4FullScreen.h"

#include "C4Version.h"
#include "game/C4Application.h"
#include "game/C4GraphicsSystem.h"
#include "game/C4Viewport.h"
#include "gui/C4GameDialogs.h"
#include "gui/C4GameOverDlg.h"
#include "gui/C4Gui.h"
#include "gui/C4MouseControl.h"
#include "player/C4Player.h"
#include "player/C4PlayerList.h"

void C4FullScreen::CharIn(const char * c) { ::pGUI->CharIn(c); }

C4FullScreen::C4FullScreen()
{
	pMenu = nullptr;
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
	pSurface = nullptr;
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
		pMenu = nullptr;
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
