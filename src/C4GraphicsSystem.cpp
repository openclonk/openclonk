/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2004, 2008  Matthes Bender
 * Copyright (c) 2001-2003, 2005-2009  Sven Eberhardt
 * Copyright (c) 2001  Michael Käser
 * Copyright (c) 2005-2006, 2008  Günther Brammer
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

/* Operates viewports, message board and draws the game */

#include <C4Include.h>
#include <C4GraphicsSystem.h>

#include <C4Viewport.h>
#include <C4ViewportWindow.h>
#include <C4Application.h>
#include <C4Console.h>
#include <C4Random.h>
#include <C4SurfaceFile.h>
#include <C4FullScreen.h>
#include <C4Gui.h>
#include <C4LoaderScreen.h>
#include <C4Player.h>
#include <C4SoundSystem.h>
#include <C4MouseControl.h>
#include <C4GraphicsResource.h>
#include <C4Landscape.h>
#include <C4Network2.h>
#include <C4Game.h>
#include <C4PlayerList.h>
#include <C4GameObjects.h>

#include <StdPNG.h>

C4GraphicsSystem::C4GraphicsSystem()
{
	Default();
}

C4GraphicsSystem::~C4GraphicsSystem()
{
	Clear();
}

bool C4GraphicsSystem::Init()
{
#ifdef _WIN32
	// Register viewport class
	if (!C4ViewportWindow::RegisterViewportClass(Application.GetInstance()))
		return false;
#endif
	// Init video module
	if (Config.Graphics.VideoModule)
		Video.Init(FullScreen.pSurface);
	// Success
	return true;
}

void C4GraphicsSystem::Clear()
{
	// Clear message board
	MessageBoard.Clear();
	// Clear upper board
	UpperBoard.Clear();
	// clear loader
	if (pLoaderScreen) { delete pLoaderScreen; pLoaderScreen=NULL; }
	// Close viewports
	::Viewports.Clear();
	// Clear video system
	Video.Clear();
	// No debug stuff
	DeactivateDebugOutput();
}

bool C4GraphicsSystem::StartDrawing()
{
	// only if ddraw is ready
	if (!lpDDraw) return false;
	if (!lpDDraw->Active) return false;

	// only if application is active or windowed (if config allows)
	if (!Application.Active && (!Application.isEditor || !Config.Graphics.RenderInactiveEM)) return false;

	// drawing OK
	return true;
}

void C4GraphicsSystem::FinishDrawing()
{
	if (!Application.isEditor) FullScreen.pSurface->PageFlip();
}

void C4GraphicsSystem::Execute()
{
	// activity check
	if (!StartDrawing()) return;

	bool fBGDrawn = false;

	// If lobby running, message board only (page flip done by startup message board)
	if (!::pGUI || !::pGUI->HasFullscreenDialog(true)) // allow for message board behind GUI
		if (::Network.isLobbyActive() || !Game.IsRunning)
			if (!Application.isEditor)
			{
				// Message board
				if (iRedrawBackground) ClearFullscreenBackground();
				MessageBoard.Execute();
				if (!::pGUI || !C4GUI::IsActive())
					{ FinishDrawing(); return; }
				fBGDrawn = true;
			}

	// fullscreen GUI?
	if (!Application.isEditor && ::pGUI && C4GUI::IsActive() && (::pGUI->HasFullscreenDialog(false) || !Game.IsRunning))
	{
		if (!fBGDrawn && iRedrawBackground) ClearFullscreenBackground();
		::pGUI->Render(!fBGDrawn);
		FinishDrawing();
		return;
	}

	
	// Reset object audibility
	::Objects.ResetAudibility();

	// some hack to ensure the mouse is drawn after a dialog close and before any
	// movement messages
	if (::pGUI && !C4GUI::IsActive())
		::pGUI->SetMouseInGUI(false, false);

	// Viewports
	::Viewports.Execute(!Application.isEditor && iRedrawBackground);
	if (iRedrawBackground) --iRedrawBackground;

	if (!Application.isEditor)
	{
		// Upper board
		UpperBoard.Execute();

		// Message board
		MessageBoard.Execute();

		// Help & Messages
		DrawHelp();
		DrawHoldMessages();
		DrawFlashMessage();
	}

	// InGame-GUI
	if (::pGUI && C4GUI::IsActive())
	{
		::pGUI->Render(false);
	}

	// gamma update
	if (fSetGamma)
	{
		ApplyGamma();
		fSetGamma=false;
	}

	// Video record & status (fullsrceen)
	if (!Application.isEditor)
		Video.Execute();

	// done
	FinishDrawing();
}

void C4GraphicsSystem::Default()
{
	UpperBoard.Default();
	MessageBoard.Default();
	InvalidateBg();
	ShowVertices=false;
	ShowAction=false;
	ShowCommand=false;
	ShowEntrance=false;
	ShowPathfinder=false;
	ShowNetstatus=false;
	ShowSolidMask=false;
	ShowHelp=false;
	FlashMessageText[0]=0;
	FlashMessageTime=0; FlashMessageX=FlashMessageY=0;
	Video.Default();
	for (int32_t iRamp=0; iRamp<3*C4MaxGammaRamps; iRamp+=3)
		{ dwGamma[iRamp+0]=0; dwGamma[iRamp+1]=0x808080; dwGamma[iRamp+2]=0xffffff; }
	fSetGamma=false;
	pLoaderScreen=NULL;
}

void C4GraphicsSystem::ClearFullscreenBackground()
{
	lpDDraw->FillBG(0);
	--iRedrawBackground;
}

bool C4GraphicsSystem::InitLoaderScreen(const char *szLoaderSpec, bool fDrawBlackScreenFirst)
{
	// create new loader; overwrite current only if successful
	C4LoaderScreen *pNewLoader = new C4LoaderScreen();
	pNewLoader->SetBlackScreen(fDrawBlackScreenFirst);
	if (!pNewLoader->Init(szLoaderSpec)) { delete pNewLoader; return false; }
	if (pLoaderScreen) delete pLoaderScreen;
	pLoaderScreen = pNewLoader;
	// apply user gamma for loader
	ApplyGamma();
	// done, success
	return true;
}

void C4GraphicsSystem::EnableLoaderDrawing()
{
	// reset black screen loader flag
	if (pLoaderScreen) pLoaderScreen->SetBlackScreen(false);
}

bool C4GraphicsSystem::SaveScreenshot(bool fSaveAll)
{
	// Filename
	char szFilename[_MAX_PATH+1];
	int32_t iScreenshotIndex=1;
	const char *strFilePath = NULL;
	do
		sprintf(szFilename,"Screenshot%03i.png",iScreenshotIndex++);
	while (FileExists(strFilePath = Config.AtScreenshotPath(szFilename)));
	bool fSuccess=DoSaveScreenshot(fSaveAll, strFilePath);
	// log if successful/where it has been stored
	if (!fSuccess)
		LogF(LoadResStr("IDS_PRC_SCREENSHOTERROR"), Config.AtUserDataRelativePath(Config.AtScreenshotPath(szFilename)));
	else
		LogF(LoadResStr("IDS_PRC_SCREENSHOT"), Config.AtUserDataRelativePath(Config.AtScreenshotPath(szFilename)));
	// return success
	return !!fSuccess;
}

bool C4GraphicsSystem::DoSaveScreenshot(bool fSaveAll, const char *szFilename)
{
	// Fullscreen only
	if (Application.isEditor) return false;
	// back surface must be present
	if (!FullScreen.pSurface) return false;

	// save landscape
	if (fSaveAll)
	{
		// get viewport to draw in
		C4Viewport *pVP=::Viewports.GetFirstViewport(); if (!pVP) return false;
		// create image large enough to hold the landcape
		CPNGFile png; int32_t lWdt=GBackWdt,lHgt=GBackHgt;
		if (!png.Create(lWdt, lHgt, false)) return false;
		// get backbuffer size
		int32_t bkWdt=C4GUI::GetScreenWdt(), bkHgt=C4GUI::GetScreenHgt();
		if (!bkWdt || !bkHgt) return false;
		// facet for blitting
		C4TargetFacet bkFct;
		// mark background to be redrawn
		InvalidateBg();
		// backup and clear sky parallaxity
		int32_t iParX=::Landscape.Sky.ParX; ::Landscape.Sky.ParX=10;
		int32_t iParY=::Landscape.Sky.ParY; ::Landscape.Sky.ParY=10;
		// temporarily change viewport player
		int32_t iVpPlr=pVP->Player; pVP->Player=NO_OWNER;
		// blit all tiles needed
		for (int32_t iY=0; iY<lHgt; iY+=bkHgt) for (int32_t iX=0; iX<lWdt; iX+=bkWdt)
			{
				// get max width/height
				int32_t bkWdt2=bkWdt,bkHgt2=bkHgt;
				if (iX+bkWdt2>lWdt) bkWdt2-=iX+bkWdt2-lWdt;
				if (iY+bkHgt2>lHgt) bkHgt2-=iY+bkHgt2-lHgt;
				// update facet
				bkFct.Set(FullScreen.pSurface, 0, 0, bkWdt2, bkHgt2, iX, iY);
				// draw there
				pVP->Draw(bkFct, false);
				// render
				FullScreen.pSurface->PageFlip(); FullScreen.pSurface->PageFlip();
				// get output (locking primary!)
				if (FullScreen.pSurface->Lock())
				{
					// transfer each pixel - slooow...
					for (int32_t iY2=0; iY2<bkHgt2; ++iY2)
						for (int32_t iX2=0; iX2<bkWdt2; ++iX2)
							png.SetPix(iX+iX2, iY+iY2, lpDDraw->ApplyGammaTo(FullScreen.pSurface->GetPixDw(iX2, iY2, false)));
					// done; unlock
					FullScreen.pSurface->Unlock();
				}
			}
		// restore viewport player
		pVP->Player=iVpPlr;
		// restore parallaxity
		::Landscape.Sky.ParX=iParX;
		::Landscape.Sky.ParY=iParY;
		// save!
		return png.Save(szFilename);
	}
	// Save primary surface
	return FullScreen.pSurface->SavePNG(szFilename, false, true, false);
}

void C4GraphicsSystem::DeactivateDebugOutput()
{
	ShowVertices=false;
	ShowAction=false;
	ShowCommand=false;
	ShowEntrance=false;
	ShowPathfinder=false; // allow pathfinder! - why this??
	ShowSolidMask=false;
	ShowNetstatus=false;
}

void C4GraphicsSystem::DrawHoldMessages()
{
	if (!Application.isEditor && Game.HaltCount)
	{
		lpDDraw->TextOut("Pause", ::GraphicsResource.FontRegular,1.0,
		                           FullScreen.pSurface, C4GUI::GetScreenWdt()/2,
		                           C4GUI::GetScreenHgt()/2 - ::GraphicsResource.FontRegular.iLineHgt*2,
		                           CStdDDraw::DEFAULT_MESSAGE_COLOR, ACenter);
		::GraphicsSystem.OverwriteBg();
	}
}

void C4GraphicsSystem::FlashMessage(const char *szMessage)
{
	// Store message
	SCopy(szMessage, FlashMessageText, C4MaxTitle);
	// Calculate message time
	FlashMessageTime = SLen(FlashMessageText) * 2;
	// Initial position
	FlashMessageX = -1;
	FlashMessageY = 10;
	// Upper board active: stay below upper board
	if (Config.Graphics.UpperBoard)
		FlashMessageY += C4UpperBoardHeight;
	// More than one viewport: try to stay below portraits etc.
	if (::Viewports.GetViewportCount() > 1)
		FlashMessageY += 64;
	// New flash message: redraw background (might be drawing one message on top of another)
	InvalidateBg();
}

void C4GraphicsSystem::FlashMessageOnOff(const char *strWhat, bool fOn)
{
	StdStrBuf strMessage;
	strMessage.Format("%s: %s", strWhat, LoadResStr(fOn ? "IDS_CTL_ON" : "IDS_CTL_OFF"));
	FlashMessage(strMessage.getData());
}

void C4GraphicsSystem::DrawFlashMessage()
{
	if (!FlashMessageTime) return;
	if (Application.isEditor) return;
	lpDDraw->TextOut(FlashMessageText, ::GraphicsResource.FontRegular, 1.0, FullScreen.pSurface,
	                           (FlashMessageX==-1) ? C4GUI::GetScreenWdt()/2 : FlashMessageX,
	                           (FlashMessageY==-1) ? C4GUI::GetScreenHgt()/2 : FlashMessageY,
	                           CStdDDraw::DEFAULT_MESSAGE_COLOR,
	                           (FlashMessageX==-1) ? ACenter : ALeft);
	FlashMessageTime--;
	// Flash message timed out: redraw background
	if (!FlashMessageTime) InvalidateBg();
}

void C4GraphicsSystem::DrawHelp()
{
	if (!ShowHelp) return;
	if (Application.isEditor) return;
	int32_t iX = ::Viewports.ViewportArea.X, iY = ::Viewports.ViewportArea.Y;
	int32_t iWdt = ::Viewports.ViewportArea.Wdt;
	StdStrBuf strText;
	// left coloumn
	strText.AppendFormat("[%s]\n\n", LoadResStr("IDS_CTL_GAMEFUNCTIONS"));
	// main functions
	strText.AppendFormat("<c ffff00>%s</c> - %s\n", GetKeyboardInputName("ToggleShowHelp").getData(), LoadResStr("IDS_CON_HELP"));
	strText.AppendFormat("<c ffff00>%s</c> - %s\n", GetKeyboardInputName("MusicToggle").getData(), LoadResStr("IDS_CTL_MUSIC"));
	strText.AppendFormat("<c ffff00>%s</c> - %s\n", GetKeyboardInputName("NetClientListDlgToggle").getData(), LoadResStr("IDS_DLG_NETWORK"));
	// messages
	StdCopyStrBuf strAltChatKey(GetKeyboardInputName("ChatOpen", false, 0));
	strText.AppendFormat("\n<c ffff00>%s/%s</c> - %s\n", GetKeyboardInputName("ChatOpen", false, 1).getData(), strAltChatKey.getData(), LoadResStr("IDS_CTL_SENDMESSAGE"));
	strText.AppendFormat("<c ffff00>%s</c> - %s\n", GetKeyboardInputName("MsgBoardScrollUp").getData(), LoadResStr("IDS_CTL_MESSAGEBOARDBACK"));
	strText.AppendFormat("<c ffff00>%s</c> - %s\n", GetKeyboardInputName("MsgBoardScrollDown").getData(), LoadResStr("IDS_CTL_MESSAGEBOARDFORWARD"));
	// irc chat
	strText.AppendFormat("\n<c ffff00>%s</c> - %s\n", GetKeyboardInputName("ToggleChat").getData(), LoadResStr("IDS_CTL_IRCCHAT"));
	// scoreboard
	strText.AppendFormat("\n<c ffff00>%s</c> - %s\n", GetKeyboardInputName("ScoreboardToggle").getData(), LoadResStr("IDS_CTL_SCOREBOARD"));
	// screenshots
	strText.AppendFormat("\n<c ffff00>%s</c> - %s\n", GetKeyboardInputName("Screenshot").getData(), LoadResStr("IDS_CTL_SCREENSHOT"));
	strText.AppendFormat("<c ffff00>%s</c> - %s\n", GetKeyboardInputName("ScreenshotEx").getData(), LoadResStr("IDS_CTL_SCREENSHOTEX"));

	lpDDraw->TextOut(strText.getData(), ::GraphicsResource.FontRegular, 1.0, FullScreen.pSurface,
	                           iX + 128, iY + 64, CStdDDraw::DEFAULT_MESSAGE_COLOR, ALeft);

	// right coloumn
	strText.Clear();
	// game speed
	strText.AppendFormat("\n\n<c ffff00>%s</c> - %s\n", GetKeyboardInputName("GameSpeedUp").getData(), LoadResStr("IDS_CTL_GAMESPEEDUP"));
	strText.AppendFormat("<c ffff00>%s</c> - %s\n", GetKeyboardInputName("GameSlowDown").getData(), LoadResStr("IDS_CTL_GAMESPEEDDOWN"));
	// debug
	strText.AppendFormat("\n\n[%s]\n\n", "Debug");
	strText.AppendFormat("<c ffff00>%s</c> - %s\n", GetKeyboardInputName("DbgModeToggle").getData(), LoadResStr("IDS_CTL_DEBUGMODE"));
	strText.AppendFormat("<c ffff00>%s</c> - %s\n", GetKeyboardInputName("DbgShowVtxToggle").getData(), "Entrance+Vertices");
	strText.AppendFormat("<c ffff00>%s</c> - %s\n", GetKeyboardInputName("DbgShowActionToggle").getData(), "Actions/Commands/Pathfinder");
	strText.AppendFormat("<c ffff00>%s</c> - %s\n", GetKeyboardInputName("DbgShowSolidMaskToggle").getData(), "SolidMasks");
	lpDDraw->TextOut(strText.getData(), ::GraphicsResource.FontRegular, 1.0, FullScreen.pSurface,
	                           iX + iWdt/2 + 64, iY + 64, CStdDDraw::DEFAULT_MESSAGE_COLOR, ALeft);
}

void C4GraphicsSystem::SetGamma(DWORD dwClr1, DWORD dwClr2, DWORD dwClr3, int32_t iRampIndex)
{
	// No gamma effects
	if (Config.Graphics.DisableGamma) return;
	if (iRampIndex < 0 || iRampIndex >= C4MaxGammaRamps) return;
	// turn ramp index into array offset
	iRampIndex*=3;
	// set array members
	dwGamma[iRampIndex+0]=dwClr1;
	dwGamma[iRampIndex+1]=dwClr2;
	dwGamma[iRampIndex+2]=dwClr3;
	// mark gamma ramp to be recalculated
	fSetGamma=true;
}

void C4GraphicsSystem::ApplyGamma()
{
	// No gamma effects
	if (Config.Graphics.DisableGamma) return;
	//  calculate color channels by adding the difference between the gamma ramps to their normals
	int32_t ChanOff[3];
	DWORD Gamma[3];
	const int32_t DefChanVal[3] = { 0x00, 0x80, 0xff };
	// calc offset for curve points
	for (int32_t iCurve=0; iCurve<3; ++iCurve)
	{
		ZeroMemory(ChanOff, sizeof(int32_t)*3);
		// ...channels...
		for (int32_t iChan=0; iChan<3; ++iChan)
			// ...ramps...
			for (int32_t iRamp=0; iRamp<C4MaxGammaRamps; ++iRamp)
				// add offset
				ChanOff[iChan]+=(int32_t) BYTE(dwGamma[iRamp*3+iCurve]>>(16-iChan*8)) - DefChanVal[iCurve];
		// calc curve point
		Gamma[iCurve]=C4RGB(BoundBy<int32_t>(DefChanVal[iCurve]+ChanOff[0], 0, 255), BoundBy<int32_t>(DefChanVal[iCurve]+ChanOff[1], 0, 255), BoundBy<int32_t>(DefChanVal[iCurve]+ChanOff[2], 0, 255));
	}
	// set gamma
	lpDDraw->SetGamma(Gamma[0], Gamma[1], Gamma[2]);
}

bool C4GraphicsSystem::ToggleShowNetStatus()
{
	ShowNetstatus = !ShowNetstatus;
	return true;
}

bool C4GraphicsSystem::ToggleShowVertices()
{
	if (!Game.DebugMode && !Console.Active) { FlashMessage(LoadResStr("IDS_MSG_NODEBUGMODE")); return false; }
	Toggle(ShowVertices);
	Toggle(ShowEntrance); // vertices and entrance now toggled together
	FlashMessageOnOff("Entrance+Vertices", ShowVertices || ShowEntrance);
	return true;
}

bool C4GraphicsSystem::ToggleShowAction()
{
	if (!Game.DebugMode && !Console.Active) { FlashMessage(LoadResStr("IDS_MSG_NODEBUGMODE")); return false; }
	if (!(ShowAction || ShowCommand || ShowPathfinder))
		{ ShowAction = true; FlashMessage("Actions"); }
	else if (ShowAction)
		{ ShowAction = false; ShowCommand = true; FlashMessage("Commands"); }
	else if (ShowCommand)
		{ ShowCommand = false; ShowPathfinder = true; FlashMessage("Pathfinder"); }
	else if (ShowPathfinder)
		{ ShowPathfinder = false; FlashMessageOnOff("Actions/Commands/Pathfinder", false); }
	return true;
}

bool C4GraphicsSystem::ToggleShowSolidMask()
{
	if (!Game.DebugMode && !Console.Active) { FlashMessage(LoadResStr("IDS_MSG_NODEBUGMODE")); return false; }
	Toggle(ShowSolidMask);
	FlashMessageOnOff("SolidMasks", !!ShowSolidMask);
	return true;
}

bool C4GraphicsSystem::ToggleShowHelp()
{
	Toggle(ShowHelp);
	// Turned off? Invalidate background.
	if (!ShowHelp) InvalidateBg();
	return true;
}

C4GraphicsSystem GraphicsSystem;
