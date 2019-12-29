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

/* Operates viewports, message board and draws the game */

#include "C4Include.h"
#include "C4ForbidLibraryCompilation.h"
#include "game/C4GraphicsSystem.h"

#include "editor/C4Console.h"
#include "game/C4Application.h"
#include "game/C4FullScreen.h"
#include "game/C4Viewport.h"
#include "graphics/C4Draw.h"
#include "graphics/C4GraphicsResource.h"
#include "graphics/StdPNG.h"
#include "gui/C4Gui.h"
#include "gui/C4LoaderScreen.h"
#include "landscape/C4Landscape.h"
#include "landscape/C4Sky.h"
#include "network/C4Network2.h"
#include "object/C4GameObjects.h"

static const int MAX_BACKGROUND_FPS = 5;

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
	// Success
	return true;
}

void C4GraphicsSystem::Clear()
{
	// Clear message board
	MessageBoard.reset();
	// clear loader
	if (pLoaderScreen)
	{
		delete pLoaderScreen;
		pLoaderScreen = nullptr;
	}
	// Close viewports
	::Viewports.Clear();
	// No debug stuff
	DeactivateDebugOutput();
}

bool C4GraphicsSystem::StartDrawing()
{
	// only if ddraw is ready
	if (!pDraw)
	{
		return false;
	}
	if (!pDraw->Active)
	{
		return false;
	}

	// if the window is not focused, draw no more than MAX_BACKGROUND_FPS frames per second
	if (!Application.Active && (C4TimeMilliseconds::Now() - lastFrame) < 1000 / MAX_BACKGROUND_FPS)
	{
		return false;
	}
	
	// drawing OK
	return true;
}

void C4GraphicsSystem::FinishDrawing()
{
	if (!Application.isEditor)
	{
		FullScreen.pSurface->PageFlip();
	}
	lastFrame = C4TimeMilliseconds::Now();
}

void C4GraphicsSystem::Execute()
{
	// activity check
	if (!StartDrawing())
	{
		return;
	}

	bool did_draw_background = false;

	// If lobby running, message board only (page flip done by startup message board)
	if (!::pGUI->HasFullscreenDialog(true)) // allow for message board behind GUI
	{
		if (::Network.isLobbyActive() || !Game.IsRunning)
		{
			if (!Application.isEditor)
			{
				// Message board
				if (iRedrawBackground)
				{
					ClearFullscreenBackground();
				}
				MessageBoard->Execute();
				if (!C4GUI::IsActive())
				{
					FinishDrawing();
					return;
				}
				did_draw_background = true;
			}
		}
	}

	// fullscreen GUI?
	if (!Application.isEditor && C4GUI::IsActive() && (::pGUI->HasFullscreenDialog(false) || !Game.IsRunning))
	{
		if (!did_draw_background && iRedrawBackground)
		{
			ClearFullscreenBackground();
		}
		::pGUI->Render(!did_draw_background);
		FinishDrawing();
		return;
	}


	// Reset object audibility
	::Objects.ResetAudibility();

	// some hack to ensure the mouse is drawn after a dialog close and before any
	// movement messages
	if (!C4GUI::IsActive())
	{
		::pGUI->SetMouseInGUI(false, false);
	}

	// Viewports
	::Viewports.Execute(!Application.isEditor && iRedrawBackground);
	if (iRedrawBackground)
	{
		--iRedrawBackground;
	}

	if (!Application.isEditor)
	{
		// Upper board
		UpperBoard.Execute();

		// Message board
		MessageBoard->Execute();

		// Help & Messages
		DrawHelp();
		DrawHoldMessages();
		DrawFlashMessage();
	}

	// InGame-GUI
	if (C4GUI::IsActive())
	{
		::pGUI->Render(false);
	}

	// done
	FinishDrawing();
}

void C4GraphicsSystem::Default()
{
	MessageBoard = std::make_unique<C4MessageBoard>();
	InvalidateBg();
	ShowVertices = false;
	ShowAction = false;
	ShowCommand = false;
	ShowEntrance = false;
	ShowPathfinder = false;
	ShowNetstatus = false;
	Show8BitSurface = 0;
	ShowLights = false;
	ShowMenuInfo = false;
	ShowHelp = false;
	FlashMessageText[0] = 0;
	FlashMessageTime = 0;
	FlashMessageX = 0;
	FlashMessageY = 0;
	pLoaderScreen = nullptr;
}

void C4GraphicsSystem::ClearFullscreenBackground()
{
	pDraw->FillBG(0);
	--iRedrawBackground;
}

bool C4GraphicsSystem::InitLoaderScreen(const char *image_name)
{
	// create new loader; overwrite current only if successful
	C4LoaderScreen *loader = new C4LoaderScreen();
	loader->SetBlackScreen(false);
	if (!loader->Init(image_name))
	{
		delete loader;
		return false;
	}
	if (pLoaderScreen)
	{
		delete pLoaderScreen;
	}
	pLoaderScreen = loader;
	// done, success
	return true;
}

void C4GraphicsSystem::EnableLoaderDrawing()
{
	// reset black screen loader flag
	if (pLoaderScreen)
	{
		pLoaderScreen->SetBlackScreen(false);
	}
}

bool C4GraphicsSystem::SaveScreenshot(bool save_all, float zoom_factor_all)
{
	// Find a unique screenshot filename by iterating over all possible names
	// Keep static counter so multiple screenshots in succession do not use same filename even if the background thread hasn't started writing the file yet
	char filename[_MAX_PATH_LEN];
	static int32_t screenshot_index = 1;
	const char *filepath = nullptr;
	do
	{
		sprintf(filename,"Screenshot%03i.png", screenshot_index++);
	}
	while (FileExists(filepath = Config.AtScreenshotPath(filename)));

	// log if successful/where it has been stored
	bool can_save = DoSaveScreenshot(save_all, filepath, zoom_factor_all);
	if (!can_save)
	{
		LogF(LoadResStr("IDS_PRC_SCREENSHOTERROR"), Config.AtUserDataRelativePath(Config.AtScreenshotPath(filename)));
	}
	else
	{
		LogF(LoadResStr("IDS_PRC_SCREENSHOT"), Config.AtUserDataRelativePath(Config.AtScreenshotPath(filename)));
	}

	// return success
	return !!can_save;
}

bool C4GraphicsSystem::DoSaveScreenshot(bool save_all, const char *filename, float zoom_factor_all)
{
	// Fullscreen only
	if (Application.isEditor)
	{
		Log(LoadResStr("IDS_PRC_SCREENSHOTERROREDITOR"));
		return false;
	}

	// back surface must be present
	if (!FullScreen.pSurface)
	{
		return false;
	}

	// save landscape
	if (save_all)
	{
		// Create full map screenshots at zoom 2x. Fractional zooms (like 1.7x) should work but might cause some trouble at screen borders.
		float zoom = zoom_factor_all;

		// get viewport to draw in
		C4Viewport *viewport = ::Viewports.GetFirstViewport();
		if (!viewport)
		{
			return false;
		}

		// create image large enough to hold the landscape
		std::unique_ptr<CPNGFile> png(new CPNGFile());
		int32_t lWdt = ::Landscape.GetWidth() * zoom;
		int32_t lHgt = ::Landscape.GetHeight() * zoom;
		if (!png->Create(lWdt, lHgt, false))
		{
			return false;
		}

		// get backbuffer size
		int32_t bkWdt = C4GUI::GetScreenWdt();
		int32_t bkHgt = C4GUI::GetScreenHgt();
		if (!bkWdt || !bkHgt)
		{
			return false;
		}

		// facet for blitting
		C4TargetFacet bkFct;
		// mark background to be redrawn
		InvalidateBg();
		// draw on one big viewport
		viewport->SetOutputSize(0, 0, 0, 0, bkWdt, bkHgt);
		// backup and clear sky parallaxity
		int32_t SkyParX = ::Landscape.GetSky().ParX;
		::Landscape.GetSky().ParX = 10;
		int32_t SkyParY = ::Landscape.GetSky().ParY;
		::Landscape.GetSky().ParY = 10;

		// backup and clear viewport borders
		FLOAT_RECT vp_borders = { viewport->BorderLeft, viewport->BorderRight, viewport->BorderTop, viewport->BorderBottom };
		viewport->BorderLeft = viewport->BorderRight = viewport->BorderTop = viewport->BorderBottom = 0.0f;

		// temporarily change viewport player
		int32_t viewport_player = viewport->Player;
		viewport->Player = NO_OWNER;

		// blit all tiles needed
		for (int32_t iY = 0; iY < lHgt; iY += bkHgt)
		{
			for (int32_t iX = 0; iX < lWdt; iX += bkWdt)
			{
				// get max width/height
				int32_t bkWdt2 = bkWdt;
				int32_t bkHgt2 = bkHgt;
				if (iX + bkWdt2 > lWdt)
				{
					bkWdt2 -= iX + bkWdt2 - lWdt;
				}
				if (iY + bkHgt2 > lHgt)
				{
					bkHgt2 -= iY + bkHgt2 - lHgt;
				}
				// update facet
				bkFct.Set(FullScreen.pSurface, 0, 0, ceil(float(bkWdt2)/zoom), ceil(float(bkHgt2)/zoom), iX/zoom, iY/zoom, zoom, 0, 0);
				// draw there
				viewport->Draw(bkFct, true, false);
				// render
				FullScreen.pSurface->PageFlip();
				FullScreen.pSurface->PageFlip();

				// get output (locking primary!)
				if (FullScreen.pSurface->Lock())
				{
					// transfer each pixel - slooow...
					for (int32_t iY2 = 0; iY2 < bkHgt2; ++iY2)
					{
#ifndef USE_CONSOLE
						glReadPixels(0, FullScreen.pSurface->Hgt - iY2 - 1, bkWdt2, 1, GL_BGR, GL_UNSIGNED_BYTE, reinterpret_cast<BYTE *>(png->GetRow(iY + iY2)) + iX * 3);
#else
						for (int32_t iX2 = 0; iX2 < bkWdt2; ++iX2)
						{
							png->SetPix(iX + iX2, iY + iY2, FullScreen.pSurface->GetPixDw(iX2, iY2, false));
						}
#endif
					}
					// done; unlock
					FullScreen.pSurface->Unlock();
					// This can take a long time and we would like to pump messages
					// However, we're currently hogging the primary surface and horrible things might happen if we do that, including initiation of another screenshot
					// The only thing that can be safely run is music (sound could play but that would just make them out of sync of the game)
					::Application.MusicSystem.Execute(true);
				}
			}
		}

		// restore viewport player
		viewport->Player = viewport_player;
		// restore viewport borders
		viewport->BorderLeft = vp_borders.left;
		viewport->BorderTop = vp_borders.top;
		viewport->BorderRight = vp_borders.right;
		viewport->BorderBottom = vp_borders.bottom;

		// restore parallaxity
		::Landscape.GetSky().ParX = SkyParX;
		::Landscape.GetSky().ParY = SkyParY;

		// restore viewport size
		::Viewports.RecalculateViewports();

		// save!
		CPNGFile::ScheduleSaving(png.release(), filename);
		return true;
	}
	// Save primary surface in background thread
	return FullScreen.pSurface->SavePNG(filename, false, false, true);
}

void C4GraphicsSystem::DeactivateDebugOutput()
{
	ShowVertices = false;
	ShowAction = false;
	ShowCommand = false;
	ShowEntrance = false;
	ShowPathfinder = false; // allow pathfinder! - why this??
	ShowLights = false;
	Show8BitSurface = 0;
	ShowNetstatus = false;
	ShowMenuInfo = false;
}

void C4GraphicsSystem::DrawHoldMessages()
{
	if (!Application.isEditor && Game.HaltCount)
	{
		pDraw->TextOut("Pause", ::GraphicsResource.FontRegular, 1.0,
		                           FullScreen.pSurface, C4GUI::GetScreenWdt()/2,
		                           C4GUI::GetScreenHgt()/2 - ::GraphicsResource.FontRegular.GetLineHeight()*2,
		                           C4Draw::DEFAULT_MESSAGE_COLOR, ACenter);
		::GraphicsSystem.OverwriteBg();
	}
}

void C4GraphicsSystem::FlashMessage(const char *message)
{
	// Store message
	SCopy(message, FlashMessageText, C4MaxTitle);
	// Calculate message time
	FlashMessageTime = SLen(FlashMessageText) * 2;
	// Initial position
	FlashMessageX = -1;
	FlashMessageY = 10;
	// Upper board active: stay below upper board
	if (Config.Graphics.UpperBoard)
	{
		FlashMessageY += C4UpperBoardHeight;
	}
	// More than one viewport: try to stay below portraits etc.
	if (::Viewports.GetViewportCount() > 1)
	{
		FlashMessageY += 64;
	}
	// New flash message: redraw background (might be drawing one message on top of another)
	InvalidateBg();
}

void C4GraphicsSystem::FlashMessageOnOff(const char *description, bool switch_on)
{
	StdStrBuf message;
	message.Format("%s: %s", description, LoadResStr(switch_on ? "IDS_CTL_ON" : "IDS_CTL_OFF"));
	FlashMessage(message.getData());
}

void C4GraphicsSystem::DrawFlashMessage()
{
	if (!FlashMessageTime || Application.isEditor)
	{
		return;
	}
	pDraw->TextOut(FlashMessageText, ::GraphicsResource.FontRegular, 1.0, FullScreen.pSurface,
	                           (FlashMessageX==-1) ? C4GUI::GetScreenWdt()/2 : FlashMessageX,
	                           (FlashMessageY==-1) ? C4GUI::GetScreenHgt()/2 : FlashMessageY,
	                           C4Draw::DEFAULT_MESSAGE_COLOR,
	                           (FlashMessageX==-1) ? ACenter : ALeft);
	FlashMessageTime--;
	// Flash message timed out: redraw background
	if (!FlashMessageTime)
	{
		InvalidateBg();
	}
}

void C4GraphicsSystem::DrawHelp()
{
	if (!ShowHelp || Application.isEditor)
	{
		return;
	}
	int32_t x = ::Viewports.ViewportArea.X;
	int32_t y = ::Viewports.ViewportArea.Y;
	int32_t wdt = ::Viewports.ViewportArea.Wdt;
	StdStrBuf text;
	// left coloumn
	text.AppendFormat("[%s]\n\n", LoadResStr("IDS_CTL_GAMEFUNCTIONS"));
	// main functions
	text.AppendFormat("<c ffff00>%s</c> - %s\n", GetKeyboardInputName("ToggleShowHelp").getData(), LoadResStr("IDS_CON_HELP"));
	text.AppendFormat("<c ffff00>%s</c> - %s\n", GetKeyboardInputName("MusicToggle").getData(), LoadResStr("IDS_CTL_MUSIC"));
	text.AppendFormat("<c ffff00>%s</c> - %s\n", GetKeyboardInputName("NetClientListDlgToggle").getData(), LoadResStr("IDS_DLG_NETWORK"));
	// messages
	StdCopyStrBuf strAltChatKey(GetKeyboardInputName("ChatOpen", false, 0));
	text.AppendFormat("\n<c ffff00>%s/%s</c> - %s\n", GetKeyboardInputName("ChatOpen", false, 1).getData(), strAltChatKey.getData(), LoadResStr("IDS_CTL_SENDMESSAGE"));
	text.AppendFormat("<c ffff00>%s</c> - %s\n", GetKeyboardInputName("MsgBoardScrollUp").getData(), LoadResStr("IDS_CTL_MESSAGEBOARDBACK"));
	text.AppendFormat("<c ffff00>%s</c> - %s\n", GetKeyboardInputName("MsgBoardScrollDown").getData(), LoadResStr("IDS_CTL_MESSAGEBOARDFORWARD"));
	// irc chat
	text.AppendFormat("\n<c ffff00>%s</c> - %s\n", GetKeyboardInputName("ToggleChat").getData(), LoadResStr("IDS_CTL_IRCCHAT"));
	// scoreboard
	text.AppendFormat("\n<c ffff00>%s</c> - %s\n", GetKeyboardInputName("ScoreboardToggle").getData(), LoadResStr("IDS_CTL_SCOREBOARD"));
	// screenshots
	text.AppendFormat("\n<c ffff00>%s</c> - %s\n", GetKeyboardInputName("Screenshot").getData(), LoadResStr("IDS_CTL_SCREENSHOT"));
	text.AppendFormat("<c ffff00>%s</c> - %s\n", GetKeyboardInputName("ScreenshotEx").getData(), LoadResStr("IDS_CTL_SCREENSHOTEX"));

	pDraw->TextOut(text.getData(), ::GraphicsResource.FontRegular, 1.0, FullScreen.pSurface, x + 128, y + 64, C4Draw::DEFAULT_MESSAGE_COLOR, ALeft);

	// right coloumn
	text.Clear();
	// game speed
	text.AppendFormat("\n\n<c ffff00>%s</c> - %s\n", GetKeyboardInputName("GameSpeedUp").getData(), LoadResStr("IDS_CTL_GAMESPEEDUP"));
	text.AppendFormat("<c ffff00>%s</c> - %s\n", GetKeyboardInputName("GameSlowDown").getData(), LoadResStr("IDS_CTL_GAMESPEEDDOWN"));
	// debug
	text.AppendFormat("\n\n[%s]\n\n", "Debug");
	text.AppendFormat("<c ffff00>%s</c> - %s\n", GetKeyboardInputName("DbgModeToggle").getData(), LoadResStr("IDS_CTL_DEBUGMODE"));
	text.AppendFormat("<c ffff00>%s</c> - %s\n", GetKeyboardInputName("DbgShowVtxToggle").getData(), "Entrance+Vertices");
	text.AppendFormat("<c ffff00>%s</c> - %s\n", GetKeyboardInputName("DbgShowActionToggle").getData(), "Actions/Commands/Pathfinder/Lights/Menus");
	text.AppendFormat("<c ffff00>%s</c> - %s\n", GetKeyboardInputName("DbgShow8BitSurface").getData(), "8-bit surfaces");
	pDraw->TextOut(text.getData(), ::GraphicsResource.FontRegular, 1.0, FullScreen.pSurface, x + wdt/2 + 64, y + 64, C4Draw::DEFAULT_MESSAGE_COLOR, ALeft);
}

bool C4GraphicsSystem::ToggleShowNetStatus()
{
	ShowNetstatus = !ShowNetstatus;
	return true;
}

bool C4GraphicsSystem::ToggleShowVertices()
{
	if (!Game.DebugMode && !Console.Active)
	{
		FlashMessage(LoadResStr("IDS_MSG_NODEBUGMODE"));
		return false;
	}
	ShowVertices = !ShowVertices;
	ShowEntrance = !ShowEntrance; // vertices and entrance now toggled together
	FlashMessageOnOff("Entrance+Vertices", ShowVertices || ShowEntrance);
	return true;
}

bool C4GraphicsSystem::ToggleShowAction()
{
	if (!Game.DebugMode && !Console.Active)
	{
		FlashMessage(LoadResStr("IDS_MSG_NODEBUGMODE"));
		return false;
	}
	if (!(ShowAction || ShowCommand || ShowPathfinder || ShowLights || ShowMenuInfo))
	{
		ShowAction = true;
		FlashMessage("Actions");
	}
	else if (ShowAction)
	{
		ShowAction = false;
		ShowCommand = true;
		FlashMessage("Commands");
	}
	else if (ShowCommand)
	{
		ShowCommand = false;
		ShowPathfinder = true;
		FlashMessage("Pathfinder");
	}
	else if (ShowPathfinder)
	{
		ShowPathfinder = false;
		ShowLights = true;
		FlashMessage("Lights");
	}
	else if (ShowLights)
	{
		ShowLights = false;
		ShowMenuInfo = true;
		FlashMessage("Menu Info");
	}
	else if (ShowMenuInfo)
	{
		ShowMenuInfo = false;
		FlashMessageOnOff("Actions/Commands/Pathfinder/Lights/Menus", false);
	}
	return true;
}

bool C4GraphicsSystem::ToggleShow8BitSurface()
{
	if (!Game.DebugMode && !Console.Active)
	{
		FlashMessage(LoadResStr("IDS_MSG_NODEBUGMODE"));
		return false;
	}
	Show8BitSurface = (Show8BitSurface + 1) % 3;
	if (Show8BitSurface == 0)
	{
		FlashMessage("Default view");
	}
	else if (Show8BitSurface == 1)
	{
		FlashMessage("Foreground 8-bit landscape");
	}
	else if (Show8BitSurface == 2)
	{
		FlashMessage("Background 8-bit landscape");
	}
	return true;
}

bool C4GraphicsSystem::ToggleShowHelp()
{
	ShowHelp = !ShowHelp;
	// Turned off? Invalidate background.
	if (!ShowHelp)
	{
		InvalidateBg();
	}
	return true;
}

