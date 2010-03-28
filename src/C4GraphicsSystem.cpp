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

#ifdef _WIN32
#include "resource.h"
#endif

C4GraphicsSystem::C4GraphicsSystem()
{
	fViewportClassRegistered=false;
	Default();
}

C4GraphicsSystem::~C4GraphicsSystem()
{
	Clear();
}
#ifdef _WIN32
LRESULT APIENTRY ViewportWinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

BOOL C4GraphicsSystem::RegisterViewportClass(HINSTANCE hInst)
{
	// register landscape viewport class
	WNDCLASSEX WndClass;
	WndClass.cbSize=sizeof(WNDCLASSEX);
	WndClass.style         = CS_DBLCLKS | CS_BYTEALIGNCLIENT;
	WndClass.lpfnWndProc   = ViewportWinProc;
	WndClass.cbClsExtra    = 0;
	WndClass.cbWndExtra    = 0;
	WndClass.hInstance     = hInst;
	WndClass.hCursor       = LoadCursor (NULL, IDC_ARROW);
	WndClass.hbrBackground = (HBRUSH) COLOR_BACKGROUND;
	WndClass.lpszMenuName  = NULL;
	WndClass.lpszClassName = C4ViewportClassName;
	WndClass.hIcon         = LoadIcon (hInst, MAKEINTRESOURCE (IDI_01_C4S) );
	WndClass.hIconSm       = LoadIcon (hInst, MAKEINTRESOURCE (IDI_01_C4S) );
	if (!RegisterClassEx(&WndClass)) return false;
	// register GUI dialog class
	return C4GUI::Dialog::RegisterWindowClass(hInst);
}
#endif
bool C4GraphicsSystem::Init()
{
#ifdef _WIN32
	// Register viewport class
	if (!fViewportClassRegistered)
		if (!RegisterViewportClass(Application.GetInstance()))
			return false;
	fViewportClassRegistered=true;
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
	C4Viewport *next;
	while (FirstViewport)
	{
		next=FirstViewport->Next;
		delete FirstViewport;
		FirstViewport=next;
	}
	FirstViewport=NULL;
	// Clear video system
	Video.Clear();
	// No debug stuff
	DeactivateDebugOutput();
}

bool C4GraphicsSystem::SetPalette()
{
	// Set primary palette by game palette
	if (!Application.DDraw->SetPrimaryPalette(::GraphicsResource.GamePalette, ::GraphicsResource.AlphaPalette)) return false;
	return true;
}

extern int32_t iLastControlSize,iPacketDelay;
extern int32_t ControlQueueSize,ControlQueueDataSize;

int32_t ScreenTick=0, ScreenRate=1;

bool C4GraphicsSystem::StartDrawing()
{
	// only if ddraw is ready
	if (!Application.DDraw) return false;
	if (!Application.DDraw->Active) return false;

	// only if application is active or windowed (if config allows)
	if (!Application.Active && (Application.isFullScreen || !Config.Graphics.RenderInactiveEM)) return false;

	// drawing OK
	return true;
}

void C4GraphicsSystem::FinishDrawing()
{
	if (Application.isFullScreen) FullScreen.pSurface->PageFlip();
}

void C4GraphicsSystem::Execute()
{
	// activity check
	if (!StartDrawing()) return;

	bool fBGDrawn = false;

	// If lobby running, message board only (page flip done by startup message board)
	if (!::pGUI || !::pGUI->HasFullscreenDialog(true)) // allow for message board behind GUI
		if (::Network.isLobbyActive() || !Game.IsRunning)
			if (Application.isFullScreen)
			{
				// Message board
				if (iRedrawBackground) ClearFullscreenBackground();
				MessageBoard.Execute();
				if (!::pGUI || !C4GUI::IsActive())
					{ FinishDrawing(); return; }
				fBGDrawn = true;
			}

	// fullscreen GUI?
	if (Application.isFullScreen && ::pGUI && C4GUI::IsActive() && (::pGUI->HasFullscreenDialog(false) || !Game.IsRunning))
	{
		if (!fBGDrawn && iRedrawBackground) ClearFullscreenBackground();
		::pGUI->Render(!fBGDrawn);
		FinishDrawing();
		return;
	}

	// Fixed screen rate in old network
	ScreenRate = 1;

	// Background redraw
	if (Application.isFullScreen)
		if (iRedrawBackground)
			DrawFullscreenBackground();

	// Screen rate skip frame draw
	ScreenTick++; if (ScreenTick>=ScreenRate) ScreenTick=0;

	// Reset object audibility
	::Objects.ResetAudibility();

	// some hack to ensure the mouse is drawn after a dialog close and before any
	// movement messages
	if (::pGUI && !C4GUI::IsActive())
		SetMouseInGUI(false, false);

	// Viewports
	for (C4Viewport *cvp=FirstViewport; cvp; cvp=cvp->Next)
		cvp->Execute();

	if (Application.isFullScreen)
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

	// Palette update
	if (fSetPalette) { SetPalette(); /*SetDarkColorTable();*/ fSetPalette=false; }

	// gamma update
	if (fSetGamma)
	{
		ApplyGamma();
		fSetGamma=false;
	}

	// Video record & status (fullsrceen)
	if (Application.isFullScreen)
		Video.Execute();

	// done
	FinishDrawing();
}

bool C4GraphicsSystem::CloseViewport(C4Viewport * cvp)
{
	if (!cvp) return false;
	/*C4Viewport *next,*prev=NULL;
	for (C4Viewport *cvp2=FirstViewport; cvp2; cvp2=next)
	  {
	  next=cvp2->Next;
	  if (cvp2 == cvp)
	    {
	    delete cvp;
	    StartSoundEffect("CloseViewport");
	    if (prev) prev->Next=next;
	    else FirstViewport=next;
	    }
	  else
	    prev=cvp2;
	  }*/
	// Chop the start of the chain off
	if (FirstViewport == cvp)
	{
		FirstViewport = cvp->Next;
		delete cvp;
		StartSoundEffect("CloseViewport");
	}
	// Take out of the chain
	else for (C4Viewport * prev = FirstViewport; prev; prev = prev->Next)
		{
			if (prev->Next == cvp)
			{
				prev->Next = cvp->Next;
				delete cvp;
				StartSoundEffect("CloseViewport");
			}
		}
	// Recalculate viewports
	RecalculateViewports();
	// Done
	return true;
}
#ifdef _WIN32
C4Viewport* C4GraphicsSystem::GetViewport(HWND hwnd)
{
	for (C4Viewport *cvp=FirstViewport; cvp; cvp=cvp->Next)
		if (cvp->pWindow->hWindow==hwnd)
			return cvp;
	return NULL;
}
#endif
bool C4GraphicsSystem::CreateViewport(int32_t iPlayer, bool fSilent)
{
	// Create and init new viewport, add to viewport list
	int32_t iLastCount = GetViewportCount();
	C4Viewport *nvp = new C4Viewport;
	bool fOkay = false;
	if (Application.isFullScreen)
		fOkay = nvp->Init(iPlayer, false);
	else
		fOkay = nvp->Init(&Console,&Application,iPlayer);
	if (!fOkay) { delete nvp; return false; }
	C4Viewport *pLast;
	for (pLast=FirstViewport; pLast && pLast->Next; pLast=pLast->Next) {}
	if (pLast) pLast->Next=nvp; else FirstViewport=nvp;
	// Recalculate viewports
	RecalculateViewports();
	// Viewports start off at centered position
	nvp->CenterPosition();
	// Action sound
	if (GetViewportCount()!=iLastCount) if (!fSilent)
			StartSoundEffect("CloseViewport");
	return true;
}

void C4GraphicsSystem::ClearPointers(C4Object *pObj)
{
	for (C4Viewport *cvp=FirstViewport; cvp; cvp=cvp->Next)
		cvp->ClearPointers(pObj);
}

void C4GraphicsSystem::Default()
{
	UpperBoard.Default();
	MessageBoard.Default();
	FirstViewport=NULL;
	InvalidateBg();
	ViewportArea.Default();
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
	fSetPalette=false;
	Video.Default();
	for (int32_t iRamp=0; iRamp<3*C4MaxGammaRamps; iRamp+=3)
		{ dwGamma[iRamp+0]=0; dwGamma[iRamp+1]=0x808080; dwGamma[iRamp+2]=0xffffff; }
	fSetGamma=false;
	pLoaderScreen=NULL;
}

void C4GraphicsSystem::DrawFullscreenBackground()
{
	for (int i=0, iNum=BackgroundAreas.GetCount(); i<iNum; ++i)
	{
		const C4Rect &rc = BackgroundAreas.Get(i);
		Application.DDraw->BlitSurfaceTile(::GraphicsResource.fctBackground.Surface,FullScreen.pSurface,rc.x,rc.y,rc.Wdt,rc.Hgt,-rc.x,-rc.y);
	}
	--iRedrawBackground;
}

void C4GraphicsSystem::ClearFullscreenBackground()
{
	Application.DDraw->FillBG(0);
	--iRedrawBackground;
}

void OnSurfaceRestore()
{
	::GraphicsSystem.InvalidateBg();
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

bool C4GraphicsSystem::CloseViewport(int32_t iPlayer, bool fSilent)
{
	// Close all matching viewports
	int32_t iLastCount = GetViewportCount();
	C4Viewport *next,*prev=NULL;
	for (C4Viewport *cvp=FirstViewport; cvp; cvp=next)
	{
		next=cvp->Next;
		if (cvp->Player==iPlayer || (iPlayer==NO_OWNER && cvp->fIsNoOwnerViewport))
		{
			delete cvp;
			if (prev) prev->Next=next;
			else FirstViewport=next;
		}
		else
			prev=cvp;
	}
	// Recalculate viewports
	RecalculateViewports();
	// Action sound
	if (GetViewportCount()!=iLastCount) if (!fSilent)
			StartSoundEffect("CloseViewport");
	return true;
}

void C4GraphicsSystem::RecalculateViewports()
{

	// Fullscreen only
	if (!Application.isFullScreen) return;

	// Sort viewports
	SortViewportsByPlayerControl();

	// Viewport area
	int32_t iBorderTop = 0, iBorderBottom = 0;
	if (Config.Graphics.UpperBoard)
		iBorderTop = C4UpperBoardHeight;
	iBorderBottom = MessageBoard.Output.Hgt;
	ViewportArea.Set(FullScreen.pSurface,0,iBorderTop, C4GUI::GetScreenWdt(), C4GUI::GetScreenHgt()-iBorderTop-iBorderBottom);

	// Redraw flag
	InvalidateBg();
#ifdef _WIN32
	// reset mouse clipping
	ClipCursor(NULL);
#else
	// StdWindow handles this.
#endif
	// reset GUI dlg pos
	if (::pGUI)
		::pGUI->SetPreferredDlgRect(C4Rect(ViewportArea.X, ViewportArea.Y, ViewportArea.Wdt, ViewportArea.Hgt));

	// fullscreen background: First, cover all of screen
	BackgroundAreas.Clear();
	BackgroundAreas.AddRect(C4Rect(ViewportArea.X, ViewportArea.Y, ViewportArea.Wdt, ViewportArea.Hgt));

	// Viewports
	C4Viewport *cvp;
	int32_t iViews = 0;
	for (cvp=FirstViewport; cvp; cvp=cvp->Next) iViews++;
	if (!iViews) return;
	int32_t iViewsH = (int32_t) sqrt(float(iViews));
	int32_t iViewsX = iViews / iViewsH;
	int32_t iViewsL = iViews % iViewsH;
	int32_t cViewH,cViewX,ciViewsX;
	int32_t cViewWdt,cViewHgt,cOffWdt,cOffHgt,cOffX,cOffY;
	cvp=FirstViewport;
	for (cViewH=0; cViewH<iViewsH; cViewH++)
	{
		ciViewsX = iViewsX; if (cViewH<iViewsL) ciViewsX++;
		for (cViewX=0; cViewX<ciViewsX; cViewX++)
		{
			cViewWdt = ViewportArea.Wdt/ciViewsX;
			cViewHgt = ViewportArea.Hgt/iViewsH;
			cOffX = ViewportArea.X;
			cOffY = ViewportArea.Y;
			cOffWdt = cOffHgt = 0;
			if (ciViewsX * cViewWdt < ViewportArea.Wdt)
				cOffX = (ViewportArea.Wdt - ciViewsX * cViewWdt) / 2;
			if (iViewsH * cViewHgt < ViewportArea.Hgt)
				cOffY = (ViewportArea.Hgt - iViewsH * cViewHgt) / 2 + ViewportArea.Y;
			if (Config.Graphics.SplitscreenDividers)
			{
				if (cViewX < ciViewsX - 1) cOffWdt=4;
				if (cViewH < iViewsH - 1) cOffHgt=4;
			}
			int32_t coViewWdt=cViewWdt-cOffWdt;
			int32_t coViewHgt=cViewHgt-cOffHgt;
			C4Rect rcOut(cOffX+cViewX*cViewWdt, cOffY+cViewH*cViewHgt, coViewWdt, coViewHgt);
			cvp->SetOutputSize(rcOut.x,rcOut.y,rcOut.x,rcOut.y,rcOut.Wdt,rcOut.Hgt);
			cvp=cvp->Next;
			// clip down area avaiable for background drawing
			BackgroundAreas.ClipByRect(rcOut);
		}
	}
}

int32_t C4GraphicsSystem::GetViewportCount()
{
	int32_t iResult = 0;
	for (C4Viewport *cvp=FirstViewport; cvp; cvp=cvp->Next) iResult++;
	return iResult;
}

C4Viewport* C4GraphicsSystem::GetViewport(int32_t iPlayer)
{
	for (C4Viewport *cvp=FirstViewport; cvp; cvp=cvp->Next)
		if (cvp->Player==iPlayer || (iPlayer==NO_OWNER && cvp->fIsNoOwnerViewport))
			return cvp;
	return NULL;
}

void C4GraphicsSystem::SortViewportsByPlayerControl()
{

	// Sort
	bool fSorted;
	C4Player *pPlr1,*pPlr2;
	C4Viewport *pView,*pNext,*pPrev;
	do
	{
		fSorted = true;
		for (pPrev=NULL,pView=FirstViewport; pView && (pNext = pView->Next); pView=pNext)
		{
			// Get players
			pPlr1 = ::Players.Get(pView->Player);
			pPlr2 = ::Players.Get(pNext->Player);
			// Swap order
			if (pPlr1 && pPlr2 && pPlr1->ControlSet && pPlr2->ControlSet && ( pPlr1->ControlSet->GetLayoutOrder() > pPlr2->ControlSet->GetLayoutOrder() ))
			{
				if (pPrev) pPrev->Next = pNext; else FirstViewport = pNext;
				pView->Next = pNext->Next;
				pNext->Next = pView;
				pPrev = pNext;
				pNext = pView;
				fSorted = false;
			}
			// Don't swap
			else
			{
				pPrev = pView;
			}
		}
	}
	while (!fSorted);

}

void C4GraphicsSystem::MouseMove(int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam, class C4Viewport *pVP)
{
	// pass on to GUI
	// Special: Don't pass if dragging and button is not upped
	if (::pGUI && ::pGUI->IsActive() && !::MouseControl.IsDragging())
	{
		bool fResult = ::pGUI->MouseInput(iButton, iX, iY, dwKeyParam, NULL, pVP);
		if (::pGUI && ::pGUI->HasMouseFocus()) { SetMouseInGUI(true, true); return; }
		// non-exclusive GUI: inform mouse-control about GUI-result
		SetMouseInGUI(fResult, true);
		// abort if GUI processed it
		if (fResult) return;
	}
	else
		// no GUI: mouse is not in GUI
		SetMouseInGUI(false, true);
	// mouse control enabled?
	if (!::MouseControl.IsActive())
	{
		// enable mouse in GUI, if a mouse-only-dlg is displayed
		if (::pGUI && ::pGUI->GetMouseControlledDialogCount())
			SetMouseInGUI(true, true);
		return;
	}
	// Pass on to mouse controlled viewport
	MouseMoveToViewport(iButton, iX, iY, dwKeyParam);
}

void C4GraphicsSystem::MouseMoveToViewport(int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam)
{
	// Pass on to mouse controlled viewport
	for (C4Viewport *cvp=FirstViewport; cvp; cvp=cvp->Next)
		if (::MouseControl.IsViewport(cvp))
			::MouseControl.Move( iButton,
			                     BoundBy<int32_t>(iX-cvp->OutX,0,cvp->ViewWdt-1),
			                     BoundBy<int32_t>(iY-cvp->OutY,0,cvp->ViewHgt-1),
			                     dwKeyParam );
}

void C4GraphicsSystem::SetMouseInGUI(bool fInGUI, bool fByMouse)
{
	// inform mouse control and GUI
	if (::pGUI)
	{
		::pGUI->Mouse.SetOwnedMouse(fInGUI);
		// initial movement to ensure mouse control pos is correct
		if (!::MouseControl.IsMouseOwned() && !fInGUI && !fByMouse)
		{
			::MouseControl.SetOwnedMouse(true);
			MouseMoveToViewport(C4MC_Button_None, int32_t(::pGUI->Mouse.x*C4GUI::GetZoom()), int32_t(::pGUI->Mouse.y*C4GUI::GetZoom()), ::pGUI->Mouse.dwKeys);
		}
	}
	::MouseControl.SetOwnedMouse(!fInGUI);
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
	if (!Application.isFullScreen) return false;
	// back surface must be present
	if (!FullScreen.pSurface) return false;

	// save landscape
	if (fSaveAll)
	{
		// get viewport to draw in
		C4Viewport *pVP=GetFirstViewport(); if (!pVP) return false;
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
							png.SetPix(iX+iX2, iY+iY2, Application.DDraw->ApplyGammaTo(FullScreen.pSurface->GetPixDw(iX2, iY2, false)));
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
	if (Application.isFullScreen && Game.HaltCount)
	{
		Application.DDraw->TextOut("Pause", ::GraphicsResource.FontRegular,1.0,
		                           FullScreen.pSurface, C4GUI::GetScreenWdt()/2,
		                           C4GUI::GetScreenHgt()/2 - ::GraphicsResource.FontRegular.iLineHgt*2,
		                           CStdDDraw::DEFAULT_MESSAGE_COLOR, ACenter);
		::GraphicsSystem.OverwriteBg();
	}
}

BYTE FindPaletteColor(BYTE *bypPalette, int32_t iRed, int32_t iGreen, int32_t iBlue)
{
	int32_t iClosest=0;
	for (int32_t iColor=1; iColor<256; iColor++)
		if (Abs(bypPalette[iColor*3+0]-iRed)+Abs(bypPalette[iColor*3+1]-iGreen)+Abs(bypPalette[iColor*3+2]-iBlue)
		    < Abs(bypPalette[iClosest*3+0]-iRed)+Abs(bypPalette[iClosest*3+1]-iGreen)+Abs(bypPalette[iClosest*3+2]-iBlue) )
			iClosest = iColor;
	return iClosest;
}

void C4GraphicsSystem::SetDarkColorTable()
{
	const int32_t iDarkening=80;
	// Using GamePalette
	BYTE *bypPalette = ::GraphicsResource.GamePalette;
	for (int32_t iColor=0; iColor<256; iColor++)
		DarkColorTable[iColor]=FindPaletteColor(bypPalette,Max<int32_t>(bypPalette[iColor*3+0]-iDarkening,0),Max<int32_t>(bypPalette[iColor*3+1]-iDarkening,0),Max<int32_t>(bypPalette[iColor*3+2]-iDarkening,0));
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
	if (GetViewportCount() > 1)
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
	if (!Application.isFullScreen) return;
	Application.DDraw->TextOut(FlashMessageText, ::GraphicsResource.FontRegular, 1.0, FullScreen.pSurface,
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
	if (!Application.isFullScreen) return;
	int32_t iX = ViewportArea.X, iY = ViewportArea.Y;
	int32_t iWdt = ViewportArea.Wdt;
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

	Application.DDraw->TextOut(strText.getData(), ::GraphicsResource.FontRegular, 1.0, FullScreen.pSurface,
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
	Application.DDraw->TextOut(strText.getData(), ::GraphicsResource.FontRegular, 1.0, FullScreen.pSurface,
	                           iX + iWdt/2 + 64, iY + 64, CStdDDraw::DEFAULT_MESSAGE_COLOR, ALeft);
}

int32_t C4GraphicsSystem::GetAudibility(int32_t iX, int32_t iY, int32_t *iPan, int32_t iAudibilityRadius)
{
	// default audibility radius
	if (!iAudibilityRadius) iAudibilityRadius = C4AudibilityRadius;
	// Accumulate audibility by viewports
	int32_t iAudible=0; *iPan = 0;
	for (C4Viewport *cvp=FirstViewport; cvp; cvp=cvp->Next)
	{
		iAudible = Max( iAudible,
		                BoundBy<int32_t>(100-100*Distance(cvp->ViewX+cvp->ViewWdt/2,cvp->ViewY+cvp->ViewHgt/2,iX,iY)/C4AudibilityRadius,0,100) );
		*iPan += (iX-(cvp->ViewX+cvp->ViewWdt/2)) / 5;
	}
	*iPan = BoundBy<int32_t>(*iPan, -100, 100);
	return iAudible;
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
	Application.DDraw->SetGamma(Gamma[0], Gamma[1], Gamma[2]);
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

bool C4GraphicsSystem::ViewportNextPlayer()
{
	// safety: switch valid?
	if ((!Game.C4S.Head.Film || !Game.C4S.Head.Replay) && !::GraphicsSystem.GetViewport(NO_OWNER)) return false;
	// do switch then
	C4Viewport *vp = GetFirstViewport();
	if (!vp) return false;
	vp->NextPlayer();
	return true;
}

bool C4GraphicsSystem::FreeScroll(C4Vec2D vScrollBy)
{
	// safety: move valid?
	if ((!Game.C4S.Head.Replay || !Game.C4S.Head.Film) && !::GraphicsSystem.GetViewport(NO_OWNER)) return false;
	C4Viewport *vp = GetFirstViewport();
	if (!vp) return false;
	// move then (old static code crap...)
	static int32_t vp_vx=0; static int32_t vp_vy=0; static int32_t vp_vf=0;
	int32_t dx=vScrollBy.x; int32_t dy=vScrollBy.y;
	if (Game.FrameCounter-vp_vf < 5)
		{ dx += vp_vx; dy += vp_vy; }
	vp_vx=dx; vp_vy=dy; vp_vf=Game.FrameCounter;
	vp->ViewX+=dx; vp->ViewY+=dy;
	return true;
}

bool C4GraphicsSystem::ViewportZoomOut()
{
	for (C4Viewport *vp = FirstViewport; vp; vp = vp->Next) vp->ChangeZoom(1.0f/C4GFX_ZoomStep);
	if (FirstViewport) FlashMessage(FormatString("%s: %f", "[!]Zoom", (float)FirstViewport->ZoomTarget).getData());
	return true;
}

bool C4GraphicsSystem::ViewportZoomIn()
{
	for (C4Viewport *vp = FirstViewport; vp; vp = vp->Next) vp->ChangeZoom(C4GFX_ZoomStep);
	if (FirstViewport) FlashMessage(FormatString("%s: %f", "[!]Zoom", (float)FirstViewport->ZoomTarget).getData());
	return true;
}

C4GraphicsSystem GraphicsSystem;
