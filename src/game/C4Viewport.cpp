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

/* A viewport to each player */

#include "C4Include.h"
#include "C4ForbidLibraryCompilation.h"
#include "game/C4Viewport.h"

#include "editor/C4Console.h"
#include "editor/C4ViewportWindow.h"
#include "game/C4FullScreen.h"
#include "game/C4GraphicsSystem.h"
#include "graphics/C4Draw.h"
#include "graphics/C4GraphicsResource.h"
#include "gui/C4GameMessage.h"
#include "gui/C4MouseControl.h"
#include "gui/C4ScriptGuiWindow.h"
#include "landscape/C4Landscape.h"
#include "landscape/C4PXS.h"
#include "landscape/C4Particles.h"
#include "landscape/C4Sky.h"
#include "landscape/fow/C4FoWRegion.h"
#include "lib/C4Stat.h"
#include "network/C4Network2.h"
#include "object/C4Def.h"
#include "object/C4GameObjects.h"
#include "object/C4Object.h"
#include "object/C4ObjectMenu.h"
#include "player/C4Player.h"
#include "player/C4PlayerList.h"

void C4Viewport::DropFile(const char* filename, float x, float y)
{
	Game.DropFile(filename, GetViewX()+x/Zoom, GetViewY()+y/Zoom);
}

bool C4Viewport::UpdateOutputSize(int32_t new_width, int32_t new_height)
{
	if (!pWindow)
	{
		return false;
	}
	// Output size
	C4Rect rect;
	if (new_width)
	{
		rect.x = rect.y = 0;
		rect.Wdt = new_width;
		rect.Hgt = new_height;
	}
	else
	{
#if defined(WITH_QT_EDITOR)
		// Never query the window - size is always passed from Qt.
		return false;
#else
		if (!pWindow->GetSize(&rect))
		{
			return false;
		}
#endif
	}
	OutX = rect.x;
	OutY = rect.y;
	ViewWdt = rect.Wdt;
	ViewHgt = rect.Hgt;
	ScrollView(0,0);
	// Scroll bars
	ScrollBarsByViewPosition();
	// Reset menus
	ResetMenuPositions = true;
	// update internal GL size
	if (pWindow && pWindow->pSurface)
	{
		pWindow->pSurface->UpdateSize(ViewWdt, ViewHgt);
	}
	// Update zoom limits based on new size
	C4Player *player = ::Players.Get(Player);
	if (player)
	{
		player->ZoomLimitsToViewport(this);
	}
	// Done
	return true;
}

C4Viewport::C4Viewport()
{
	Player = 0;
	viewX = viewY = 0;
	targetViewX = targetViewY = 0;
	ViewWdt = ViewHgt = 0;
	BorderLeft = BorderTop = BorderRight = BorderBottom = 0;
	OutX = OutY = ViewWdt = ViewHgt = 0;
	DrawX = DrawY = 0;
	Zoom = 1.0;
	ZoomTarget = 0.0;
	ViewportOpenFrame = 0;
	ZoomLimitMin = ZoomLimitMax = 0; // no limit
	Next = nullptr;
	PlayerLock = true;
	ResetMenuPositions = false;
	viewOffsX = viewOffsY = 0;
	fIsNoOwnerViewport = false;
}

C4Viewport::~C4Viewport()
{
	DisableFoW();
	if (pWindow)
	{
		delete pWindow->pSurface;
		pWindow->Clear();
	}
}

void C4Viewport::DrawOverlay(C4TargetFacet &cgo, const ZoomData &GameZoom)
{
	if (!Game.C4S.Head.Film || !Game.C4S.Head.Replay)
	{
		// Player info
		C4ST_STARTNEW(PInfoStat, "C4Viewport::DrawOverlay: Player Info")
		DrawPlayerInfo(cgo);
		C4ST_STOP(PInfoStat)
		C4ST_STARTNEW(MenuStat, "C4Viewport::DrawOverlay: Menu")
		DrawMenu(cgo);
		C4ST_STOP(MenuStat)
	}

	// Control overlays (if not film/replay)
	if (!Game.C4S.Head.Film || !Game.C4S.Head.Replay)
	{
		// Mouse control
		if (::MouseControl.IsViewport(this))
		{
			C4ST_STARTNEW(MouseStat, "C4Viewport::DrawOverlay: Mouse")
			::MouseControl.Draw(cgo, GameZoom);
			// Draw GUI-mouse in EM if active
			if (pWindow)
			{
				::pGUI->RenderMouse(cgo);
			}
			C4ST_STOP(MouseStat)
		}
	}
}

void C4Viewport::DrawMenu(C4TargetFacet &cgo0)
{
	// Get player
	C4Player *player = ::Players.Get(Player);

	// for menus, cgo is using GUI-syntax: TargetX/Y marks the drawing offset; x/y/Wdt/Hgt marks the offset rect
	C4TargetFacet cgo; cgo.Set(cgo0);
	cgo.X = 0;
	cgo.Y = 0;
	cgo.Wdt = cgo0.Wdt * cgo0.Zoom;
	cgo.Hgt = cgo0.Hgt * cgo0.Zoom;
	cgo.TargetX = float(cgo0.X);
	cgo.TargetY = float(cgo0.Y);
	cgo.Zoom = 1;
	pDraw->SetZoom(cgo.X, cgo.Y, cgo.Zoom);

	// Player eliminated
	if (player && player->Eliminated)
	{
		pDraw->TextOut(FormatString(LoadResStr(player->Surrendered ? "IDS_PLR_SURRENDERED" :  "IDS_PLR_ELIMINATED"),player->GetName()).getData(),
		                           ::GraphicsResource.FontRegular, 1.0, cgo.Surface,cgo.TargetX+cgo.Wdt/2,cgo.TargetY+2*cgo.Hgt/3,0xfaFF0000,ACenter);
		return;
	}

	// Player cursor object menu
	if (player && player->Cursor && player->Cursor->Menu)
	{
		if (ResetMenuPositions)
		{
			player->Cursor->Menu->ResetLocation();
		}
		// if mouse is dragging, make it transparent to easy construction site drag+drop
		bool mouse_dragging = false;
		if (::MouseControl.IsDragging() && ::MouseControl.IsViewport(this))
		{
			mouse_dragging = true;
			pDraw->ActivateBlitModulation(0x4fffffff);
		}
		// draw menu
		player->Cursor->Menu->Draw(cgo);
		// reset modulation for dragging
		if (mouse_dragging)
		{
			pDraw->DeactivateBlitModulation();
		}
	}
	// Player menu
	if (player && player->Menu.IsActive())
	{
		if (ResetMenuPositions)
		{
			player->Menu.ResetLocation();
		}
		player->Menu.Draw(cgo);
	}
	// Fullscreen menu
	if (FullScreen.MainMenu && FullScreen.MainMenu->IsActive())
	{
		if (ResetMenuPositions)
		{
			FullScreen.MainMenu->ResetLocation();
		}
		FullScreen.MainMenu->Draw(cgo);
	}

	// Flag done
	ResetMenuPositions = false;

	// restore Zoom
	pDraw->SetZoom(cgo0.X, cgo0.Y, cgo0.Zoom);
}

void C4Viewport::Draw(C4TargetFacet &cgo0, bool draw_game, bool draw_overlay)
{
#ifdef USE_CONSOLE
	// No drawing in console mode
	return;
#endif
	C4TargetFacet cgo; cgo.Set(cgo0);
	ZoomData GameZoom;
	GameZoom.X = cgo.X;
	GameZoom.Y = cgo.Y;
	GameZoom.Zoom = cgo.Zoom;

	// Draw landscape borders
	if (BorderLeft > 0.0f)   pDraw->BlitSurfaceTile(::GraphicsResource.fctBackground.Surface, cgo.Surface, DrawX, DrawY, BorderLeft, ViewHgt, -DrawX, -DrawY, nullptr);
	if (BorderTop > 0.0f)    pDraw->BlitSurfaceTile(::GraphicsResource.fctBackground.Surface, cgo.Surface, DrawX + BorderLeft, DrawY, ViewWdt - BorderLeft - BorderRight, BorderTop, -DrawX - BorderLeft, -DrawY, nullptr);
	if (BorderRight > 0.0f)  pDraw->BlitSurfaceTile(::GraphicsResource.fctBackground.Surface, cgo.Surface, DrawX + ViewWdt - BorderRight, DrawY, BorderRight, ViewHgt, -DrawX - ViewWdt + BorderRight, -DrawY, nullptr);
	if (BorderBottom > 0.0f) pDraw->BlitSurfaceTile(::GraphicsResource.fctBackground.Surface, cgo.Surface, DrawX + BorderLeft, DrawY + ViewHgt - BorderBottom, ViewWdt - BorderLeft - BorderRight, BorderBottom, -DrawX - BorderLeft, -DrawY - ViewHgt + BorderBottom, nullptr);

	// Compute non-bordered viewport area
	cgo.X += BorderLeft;
	cgo.Y += BorderTop;
	cgo.Wdt -= (BorderLeft + BorderRight) / cgo.Zoom;
	cgo.Hgt -= (BorderTop + BorderBottom) / cgo.Zoom;
	cgo.TargetX += BorderLeft / Zoom;
	cgo.TargetY += BorderTop / Zoom;

	// Apply Zoom
	GameZoom.X = cgo.X;
	GameZoom.Y = cgo.Y;
	pDraw->SetZoom(GameZoom);

	// Set clipper to integer bounds around floating point viewport region
	const FLOAT_RECT clipRect = { DrawX + BorderLeft, DrawX + ViewWdt - BorderRight, DrawY + BorderTop, DrawY + ViewHgt - BorderBottom };
	const C4Rect& clipRectInt(clipRect);
	pDraw->SetPrimaryClipper(clipRectInt.x, clipRectInt.y, clipRectInt.x + clipRectInt.Wdt - 1, clipRectInt.y + clipRectInt.Hgt - 1);

	last_game_draw_cgo = cgo;

	if (draw_game)
	{
		// --- activate FoW here ---

		// Render FoW only if active for player
		C4Player *player = ::Players.Get(Player);
		C4FoWRegion* FoW = nullptr;
		if (player && player->fFogOfWar)
		{
			FoW = this->pFoW.get();
		}

		// Update FoW
		if (FoW)
		{
			// Viewport region in landscape coordinates
			const FLOAT_RECT vpRect = { cgo.TargetX, cgo.TargetX + cgo.Wdt, cgo.TargetY, cgo.TargetY + cgo.Hgt };
			// Region in which the light is calculated
			// At the moment, just choose integer coordinates to surround the viewport
			const C4Rect lightRect(vpRect);
			if (!lightRect.Wdt || !lightRect.Hgt)
			{
				// Do not bother initializing FoW on empty region; would cause errors in drawing proc
				FoW = nullptr;
			}
			else
			{
				FoW->Update(lightRect, vpRect);

				if (!FoW->Render())
				{
					// If FoW init fails, do not set it for further drawing
					FoW = nullptr;
				}
			}
		}

		pDraw->SetFoW(FoW);

		C4ST_STARTNEW(SkyStat, "C4Viewport::Draw: Sky")
			::Landscape.GetSky().Draw(cgo);
		C4ST_STOP(SkyStat)

			::Objects.Draw(cgo, Player, -2147483647 - 1 /* INT32_MIN */, 0);

		// Draw Landscape
		C4ST_STARTNEW(LandStat, "C4Viewport::Draw: Landscape")
			::Landscape.Draw(cgo, FoW);
		C4ST_STOP(LandStat)

		// draw PXS (unclipped!)
		C4ST_STARTNEW(PXSStat, "C4Viewport::Draw: PXS")
			::PXS.Draw(cgo);
		C4ST_STOP(PXSStat)

		// Draw objects which are behind the particle plane.
		const int particlePlane = 900;
		C4ST_STARTNEW(ObjStat, "C4Viewport::Draw: Objects (1)")
			::Objects.Draw(cgo, Player, 1, particlePlane);
		C4ST_STOP(ObjStat)

		// Draw global dynamic particles on a specific Plane
		// to enable scripters to put objects both behind and in front of particles.
		C4ST_STARTNEW(PartStat, "C4Viewport::Draw: Dynamic Particles")
			::Particles.DrawGlobalParticles(cgo);
		C4ST_STOP(PartStat)

		// Now the remaining objects in front of the particles (e.g. GUI elements)
		C4ST_STARTNEW(Obj2Stat, "C4Viewport::Draw: Objects (2)")
			::Objects.Draw(cgo, Player, particlePlane + 1, 2147483647 /* INT32_MAX */);
		C4ST_STOP(Obj2Stat)

		// Draw everything else without FoW
		pDraw->SetFoW(nullptr);
	}
	else
	{
		pDraw->DrawBoxDw(cgo.Surface, cgo.X, cgo.Y, cgo.X + cgo.Wdt, cgo.Y + cgo.Hgt, 0xff000000);
	}

	// Draw PathFinder
	if (::GraphicsSystem.ShowPathfinder)
	{
		Game.PathFinder.Draw(cgo);
	}

	// Draw overlay
	if (!Game.C4S.Head.Film || !Game.C4S.Head.Replay)
	{
		Game.DrawCrewOverheadText(cgo, Player);
	}

	// Lights overlay
	if (::GraphicsSystem.ShowLights && pFoW)
	{
		pFoW->Render(&cgo);
	}

	if (draw_overlay)
	{
		// Determine zoom of overlay
		float gui_zoom = GetGUIZoom();
		// now restore complete cgo range for overlay drawing
		pDraw->SetZoom(DrawX, DrawY, gui_zoom);
		pDraw->SetPrimaryClipper(DrawX, DrawY, DrawX+(ViewWdt-1), DrawY+(ViewHgt-1));
		C4TargetFacet gui_cgo;
		gui_cgo.Set(cgo0);

		gui_cgo.X = DrawX;
		gui_cgo.Y = DrawY;
		gui_cgo.Zoom = gui_zoom;
		gui_cgo.Wdt = int(float(ViewWdt)/gui_zoom);
		gui_cgo.Hgt = int(float(ViewHgt)/gui_zoom);
		gui_cgo.TargetX = GetViewX();
		gui_cgo.TargetY = GetViewY();

		last_gui_draw_cgo = gui_cgo;

		// draw custom GUI objects
		::Objects.ForeObjects.DrawIfCategory(gui_cgo, Player, C4D_Foreground, false);

		// Draw overlay
		C4ST_STARTNEW(OvrStat, "C4Viewport::Draw: Overlay")

		if (Application.isEditor)
		{
			::Console.EditCursor.Draw(cgo);
		}

		// Game messages
		C4ST_STARTNEW(MsgStat, "C4Viewport::DrawOverlay: Messages")
		pDraw->SetZoom(0, 0, 1.0);
		::Messages.Draw(gui_cgo, cgo, Player);
		C4ST_STOP(MsgStat)

		// ingame menus
		C4ST_STARTNEW(GuiWindowStat, "C4Viewport::DrawOverlay: Menus")
		pDraw->SetZoom(0, 0, 1.0);
		::Game.ScriptGuiRoot->DrawAll(gui_cgo, Player);
		C4ST_STOP(GuiWindowStat)

		DrawOverlay(gui_cgo, GameZoom);

		// Netstats
		if (::GraphicsSystem.ShowNetstatus)
		{
			::Network.DrawStatus(gui_cgo);
		}

		C4ST_STOP(OvrStat)

	}

	// Remove zoom n clippers
	pDraw->SetZoom(0, 0, 1.0);
	pDraw->NoPrimaryClipper();

}

void C4Viewport::BlitOutput()
{
	if (pWindow)
	{
		C4Rect src, dst;
		src.x = DrawX; src.y = DrawY;  src.Wdt = ViewWdt; src.Hgt = ViewHgt;
		dst.x = OutX;  dst.y = OutY;   dst.Wdt = ViewWdt; dst.Hgt = ViewHgt;
		pWindow->pSurface->PageFlip(&src, &dst);
	}
}

void C4Viewport::Execute()
{
	// Adjust position
	AdjustZoomAndPosition();
	// Current graphics output
	C4TargetFacet cgo;
	C4Surface *target = pWindow ? pWindow->pSurface : FullScreen.pSurface;
	cgo.Set(target,DrawX,DrawY,float(ViewWdt)/Zoom,float(ViewHgt)/Zoom,GetViewX(),GetViewY(),Zoom);
	pDraw->PrepareRendering(target);
	// Load script uniforms from Global.Uniforms
	auto uniform_pop = pDraw->scriptUniform.Push(::GameScript.ScenPropList.getPropList());
	// Do not spoil game contents on owner-less viewport
	bool draw_game = true;
	if (Player == NO_OWNER)
	{
		if (!::Application.isEditor && !::Game.DebugMode)
		{
			if (!::Network.isEnabled() || !::Network.Clients.GetLocal() || !::Network.Clients.GetLocal()->isObserver())
			{
				if (::Game.PlayerInfos.GetJoinIssuedPlayerCount() > 0) // free scrolling allowed if the scenario was started explicitely without players to inspect the landscape
				{
					if (Game.C4S.Landscape.Secret)
					{
						draw_game = false;
					}
				}
			}
		}
	}
	// Draw
	Draw(cgo, draw_game, true);
	// Blit output
	BlitOutput();
}

/* This method is called whenever the viewport size is changed. Thus, its job 
   is to recalculate the zoom and zoom limits with the new values for ViewWdt
   and ViewHgt. */
void C4Viewport::CalculateZoom()
{
	// Zoom is only initialized by player or global setting during viewport creation time, because after that
	// the player may have changed to another preferred zoom.
	// However, viewports may change multiple times during startup (because of NO_OWNER viewport being deleted
	// and possible other player joins). So check by frame counter. Zoom changes done in paused mode on the
	// player init frame will be lost, but that should not be a problem.
	if(ViewportOpenFrame >= Game.FrameCounter)
	{
		InitZoom();
	}

	C4Player *player = Players.Get(Player);
	if (player)
	{
		player->ZoomLimitsToViewport(this);
	}
	else
	{
		SetZoomLimits(0.8*std::min<float>(float(ViewWdt)/::Landscape.GetWidth(),float(ViewHgt)/::Landscape.GetHeight()), 8);
	}

}

void C4Viewport::InitZoom()
{
	C4Player *player = Players.Get(Player);
	if (player)
	{
		player->ZoomToViewport(this, true);
	}
	else
	{
		ZoomTarget = std::max<float>(float(ViewWdt)/::Landscape.GetWidth(), 1.0f);
		Zoom = ZoomTarget;
	}
}

void C4Viewport::ChangeZoom(float by_factor)
{
	ZoomTarget *= by_factor;
	if (ZoomLimitMin && ZoomTarget < ZoomLimitMin) ZoomTarget = ZoomLimitMin;
	if (ZoomLimitMax && ZoomTarget > ZoomLimitMax) ZoomTarget = ZoomLimitMax;
}

void C4Viewport::SetZoom(float to_value, bool direct)
{
	ZoomTarget = to_value;
	if (Player != NO_OWNER || !::Application.isEditor)
	{
		if (ZoomLimitMin && ZoomTarget < ZoomLimitMin)
		{
			ZoomTarget = ZoomLimitMin;
		}
		if (ZoomLimitMax && ZoomTarget > ZoomLimitMax)
		{
			ZoomTarget = ZoomLimitMax;
		}
	}
	// direct: Set zoom without scrolling to it
	if (direct)
	{
		Zoom = ZoomTarget;
	}
}

void C4Viewport::SetZoomLimits(float to_min_zoom, float to_max_zoom)
{
	ZoomLimitMin = to_min_zoom;
	ZoomLimitMax = to_max_zoom;
	if (ZoomLimitMax && ZoomLimitMax < ZoomLimitMin)
	{
		ZoomLimitMax = ZoomLimitMin;
	}
	ChangeZoom(1); // Constrains zoom to limit.
}

float C4Viewport::GetZoomByViewRange(int32_t size_x, int32_t size_y) const
{
	// set zoom such that both size_x and size_y are guarantueed to fit into the viewport range
	// determine whether zoom is to be calculated by x or by y
	bool zoom_by_y = false;
	if (size_x && size_y)
	{
		zoom_by_y = (size_y * ViewWdt > size_x * ViewHgt);
	}
	else if (size_y)
	{
		// no x size passed - zoom by y
		zoom_by_y = true;
	}
	else
	{
		// 0/0 size passed - zoom to default
		if (!size_x)
		{
			size_x = C4VP_DefViewRangeX * 2;
		}
		zoom_by_y = false;
	}
	// zoom calculation
	if (zoom_by_y)
	{
		return float(ViewHgt) / size_y;
	}
	else
	{
		return float(ViewWdt) / size_x;
	}
}

void C4Viewport::SetZoom(float zoom_value)
{
	Zoom = zoom_value;
	// also set target to prevent zoom from changing back
	ZoomTarget = zoom_value;
}

void C4Viewport::AdjustZoomAndPosition()
{
	// Move zoom towards target zoom
	if (ZoomTarget < 0.000001f)
	{
		CalculateZoom();
	}
	// Change Zoom

	if (Zoom != ZoomTarget)
	{
		float DeltaZoom = Zoom / ZoomTarget;
		if (DeltaZoom < 1)
		{
			DeltaZoom = 1 / DeltaZoom;
		}

		// Minimal Zoom change factor
		static const float Z0 = pow(C4GFX_ZoomStep, 1.0f / 8.0f);

		// We change zoom based on (logarithmic) distance of current zoom
		// to target zoom. The greater the distance the more we adjust the
		// zoom in one frame. There is a minimal zoom change Z0 to make sure
		// we reach ZoomTarget in finite time.
		float ZoomAdjustFactor = Z0 * pow(DeltaZoom, 1.0f / 8.0f);

		if (Zoom == 0)
		{
			Zoom = ZoomTarget;
		}
		else
		{
			// Remember old viewport center
			float view_mid_x = this->viewX + float(this->ViewWdt) / Zoom / 2.0f;
			float view_mid_y = this->viewY + float(this->ViewHgt) / Zoom / 2.0f;

			if (Zoom < ZoomTarget)
			{
				Zoom = std::min(Zoom * ZoomAdjustFactor, ZoomTarget);
			}
			if (Zoom > ZoomTarget)
			{
				Zoom = std::max(Zoom / ZoomAdjustFactor, ZoomTarget);
			}

			// Restore new viewport center
			this->viewX = view_mid_x - float(this->ViewWdt) / Zoom / 2.0f;
			this->viewY = view_mid_y - float(this->ViewHgt) / Zoom / 2.0f;
		}
	}
	// Adjust position
	AdjustPosition(false);
}

void C4Viewport::AdjustPosition(bool immediate)
{
	if (ViewWdt == 0 || ViewHgt == 0)
	{
		// zero-sized viewport, possibly minimized editor window
		// don't do anything then
		return;
	}

	assert(Zoom > 0);
	assert(ZoomTarget > 0);

	float ViewportScrollBorder = fIsNoOwnerViewport ? 0 : float(C4ViewportScrollBorder);
	C4Player *player = ::Players.Get(Player);

	// View position
	if (PlayerLock && ValidPlr(Player))
	{
		float scrollRange = 0;
		float extraBoundsX = 0;
		float extraBoundsY = 0;

		// target view position (landscape coordinates)
		float targetCenterViewX = fixtof(player->ViewX);
		float targetCenterViewY = fixtof(player->ViewY);

		if (player->ViewMode == C4PVM_Scrolling)
		{
			extraBoundsX = extraBoundsY = ViewportScrollBorder;
		}
		else
		{
			scrollRange = std::min(ViewWdt/(10*Zoom),ViewHgt/(10*Zoom));

			// if view is close to border, allow scrolling
			if (targetCenterViewX < ViewportScrollBorder)
			{
				extraBoundsX = std::min<float>(ViewportScrollBorder - targetCenterViewX, ViewportScrollBorder);
			}
			else if (targetCenterViewX >= ::Landscape.GetWidth() - ViewportScrollBorder)
			{
				extraBoundsX = std::min<float>(targetCenterViewX - ::Landscape.GetWidth(), 0) + ViewportScrollBorder;
			}
			if (targetCenterViewY < ViewportScrollBorder)
			{
				extraBoundsY = std::min<float>(ViewportScrollBorder - targetCenterViewY, ViewportScrollBorder);
			}
			else if (targetCenterViewY >= ::Landscape.GetHeight() - ViewportScrollBorder)
			{
				extraBoundsY = std::min<float>(targetCenterViewY - ::Landscape.GetHeight(), 0) + ViewportScrollBorder;
			}
		}

		extraBoundsX = std::max(extraBoundsX, (ViewWdt/Zoom - ::Landscape.GetWidth())/2 + 1);
		extraBoundsY = std::max(extraBoundsY, (ViewHgt/Zoom - ::Landscape.GetHeight())/2 + 1);

		// add mouse auto scroll
		if (player->MouseControl && ::MouseControl.InitCentered && Config.Controls.MouseAutoScroll)
		{
			float strength = Config.Controls.MouseAutoScroll/100.0f;
			targetCenterViewX += strength*(::MouseControl.VpX - ViewWdt/2.0f)/Zoom;
			targetCenterViewY += strength*(::MouseControl.VpY - ViewHgt/2.0f)/Zoom;
		}
		
		// scroll range
		if (!immediate)
		{
			targetCenterViewX = Clamp(targetCenterViewX, targetCenterViewX - scrollRange, targetCenterViewX + scrollRange);
			targetCenterViewY = Clamp(targetCenterViewY, targetCenterViewY - scrollRange, targetCenterViewY + scrollRange);
		}
		// bounds
		targetCenterViewX = Clamp(targetCenterViewX, ViewWdt/Zoom/2 - extraBoundsX, ::Landscape.GetWidth() - ViewWdt/Zoom/2 + extraBoundsX);
		targetCenterViewY = Clamp(targetCenterViewY, ViewHgt/Zoom/2 - extraBoundsY, ::Landscape.GetHeight() - ViewHgt/Zoom/2 + extraBoundsY);

		targetViewX = targetCenterViewX - ViewWdt/Zoom/2 + viewOffsX;
		targetViewY = targetCenterViewY - ViewHgt/Zoom/2 + viewOffsY;
		
		if (immediate)
		{
			// immediate scroll
			SetViewX(targetViewX);
			SetViewY(targetViewY);
		}
		else
		{
			// smooth scroll
			int32_t smooth = Clamp<int32_t>(Config.General.ScrollSmooth, 1, 50);
			ScrollView((targetViewX - viewX) / smooth, (targetViewY - viewY) / smooth);
		}
	}

	UpdateBordersX();
	UpdateBordersY();

	// NO_OWNER can't scroll
	if (fIsNoOwnerViewport)
	{
		viewOffsX = 0;
		viewOffsY = 0;
	}
}

void C4Viewport::CenterPosition()
{
	// center viewport position on map
	// set center position
	SetViewX(::Landscape.GetWidth()/2 + ViewWdt/Zoom/2);
	SetViewY(::Landscape.GetHeight()/2 + ViewHgt/Zoom/2);
}

void C4Viewport::UpdateBordersX()
{
	BorderLeft = std::max(-GetViewX() * Zoom, 0.0f);
	BorderRight = std::max(ViewWdt - ::Landscape.GetWidth() * Zoom + GetViewX() * Zoom, 0.0f);
}

void C4Viewport::UpdateBordersY()
{
	BorderTop = std::max(-GetViewY() * Zoom, 0.0f);
	BorderBottom = std::max(ViewHgt - ::Landscape.GetHeight() * Zoom + GetViewY() * Zoom, 0.0f);
}

void C4Viewport::DrawPlayerInfo(C4TargetFacet &cgo)
{
	C4Facet ccgo;
	if (!ValidPlr(Player))
	{
		return;
	}
	// Controls
	DrawPlayerStartup(cgo);
}

bool C4Viewport::Init(int32_t for_player, bool set_temporary_only)
{
	// Fullscreen viewport initialization
	// Set Player
	if (!ValidPlr(for_player))
	{
		for_player = NO_OWNER;
	}
	Player = for_player;
	ViewportOpenFrame = Game.FrameCounter;
	if (!set_temporary_only) fIsNoOwnerViewport = (for_player == NO_OWNER);
	if (Application.isEditor)
	{
		// Console viewport initialization
		// Create window
		pWindow = std::make_unique<C4ViewportWindow>(this);
		if (!pWindow->Init(Player))
		{
			return false;
		}
		UpdateOutputSize();
		// Disable player lock on unowned viewports
		if (!ValidPlr(Player))
		{
			TogglePlayerLock();
		}
		// Don't call Execute right away since it is not yet guaranteed that
		// the Player has set this as its Viewport, and the drawing routines rely
		// on that.
	}
	else
	{
		// Owned viewport: clear any flash message explaining observer menu
		if (ValidPlr(for_player))
		{
			::GraphicsSystem.FlashMessage("");
		}
	}

	EnableFoW();
	return true;
}

void C4Viewport::DisableFoW()
{
	pFoW.reset();
}

void C4Viewport::EnableFoW()
{
	if (::Landscape.HasFoW() && Player != NO_OWNER)
	{
		pFoW = std::make_unique<C4FoWRegion>(::Landscape.GetFoW(), ::Players.Get(Player));
	}
	else
	{
		DisableFoW();
	}
}

extern int32_t DrawMessageOffset;

void C4Viewport::DrawPlayerStartup(C4TargetFacet &cgo)
{
	C4Player *player;
	if (!(player = ::Players.Get(Player)))
	{
		return;
	}
	if (!player->LocalControl || !player->ShowStartup)
	{
		return;
	}
	int32_t name_height_offset = 0;

	// Control
	// unnecessary with the current control sets
	if (player && player->ControlSet)
	{
		C4Facet controlset_facet = player->ControlSet->GetPicture();
		if (controlset_facet.Wdt)
		{
			controlset_facet.Draw(cgo.Surface,
			    cgo.X + (cgo.Wdt - controlset_facet.Wdt)/2,
			    cgo.Y + cgo.Hgt * 2/3 + DrawMessageOffset,
			    0, 0);
		}
		name_height_offset = GfxR->fctKeyboard.Hgt;
	}

	// Name
	pDraw->TextOut(player->GetName(), ::GraphicsResource.FontRegular, 1.0, cgo.Surface,
	                           cgo.X+cgo.Wdt/2, cgo.Y+cgo.Hgt*2/3+name_height_offset + DrawMessageOffset,
	                           player->ColorDw | 0xff000000, ACenter);
}

void C4Viewport::ScrollView(float by_x, float by_y)
{
	SetViewX(viewX + by_x);
	SetViewY(viewY + by_y);
}

void C4Viewport::SetViewX(float x)
{
	viewX = x;

	if (fIsNoOwnerViewport)
	{
		if(::Landscape.GetWidth() < ViewWdt / Zoom)
		{
			viewX = ::Landscape.GetWidth()/2 - ViewWdt / Zoom / 2;
		}
		else
		{
			viewX = Clamp(x, 0.0f, ::Landscape.GetWidth() - ViewWdt / Zoom);
		}
	}

	UpdateBordersX();
}

void C4Viewport::SetViewY(float y)
{
	viewY = y;

	if (fIsNoOwnerViewport)
	{
		if(::Landscape.GetHeight() < ViewHgt / Zoom)
		{
			viewY = ::Landscape.GetHeight()/2 - ViewHgt / Zoom / 2;
		}
		else
		{
			viewY = Clamp(y, 0.0f, ::Landscape.GetHeight() - ViewHgt / Zoom);
		}
	}

	UpdateBordersY();
}

void C4Viewport::SetOutputSize(int32_t draw_x, int32_t draw_y, int32_t out_x, int32_t out_y, int32_t out_wdt, int32_t out_hgt)
{
	int32_t deltaWidth = ViewWdt - out_wdt;
	int32_t deltaHeight = ViewHgt - out_hgt;

	// update output parameters
	DrawX = draw_x;
	DrawY = draw_y;
	OutX = out_x;
	OutY = out_y;
	ViewWdt = out_wdt;
	ViewHgt = out_hgt;
	// update view position: Remain centered at previous position
	// scrolling the view must be done after setting the new view width and height
	ScrollView(deltaWidth/2, deltaHeight/2);
	CalculateZoom();
	// Reset menus
	ResetMenuPositions = true;
	// player uses mouse control? then clip the cursor
	C4Player *player;
	if ((player = ::Players.Get(Player)))
	{
		if (player->MouseControl)
		{
			::MouseControl.UpdateClip();
			// and inform GUI
			::pGUI->SetPreferredDlgRect(C4Rect(out_x, out_y, out_wdt, out_hgt));
		}
	}
}

void C4Viewport::ClearPointers(C4Object *obj)
{

}

void C4Viewport::NextPlayer()
{
	C4Player *player;
	int32_t player_nr;
	if (!(player = ::Players.Get(Player)))
	{
		if (!(player = ::Players.First))
		{
			return;
		}
	}
	else if (!(player = player->Next))
	{
		if (Game.C4S.Head.Film && Game.C4S.Head.Replay)
		{
			player = ::Players.First; // cycle to first in film mode only; in network obs mode allow NO_OWNER-view
		}
	}
	if (player)
	{
		player_nr = player->Number;
	}
	else
	{
		player_nr = NO_OWNER;
	}
	if (player_nr != Player)
	{
		Init(player_nr, true);
	}
}

bool C4Viewport::IsViewportMenu(class C4Menu *menu)
{
	// check all associated menus
	// Get player
	C4Player *player = ::Players.Get(Player);
	// Player eliminated: No menu
	if (player && player->Eliminated)
	{
		return false;
	}
	// Player cursor object menu
	if (player && player->Cursor && player->Cursor->Menu == menu)
	{
		return true;
	}
	// Player menu
	if (player && player->Menu.IsActive() && &(player->Menu) == menu)
	{
		return true;
	}
	// Fullscreen menu (if active, only one viewport can exist)
	if (FullScreen.MainMenu && FullScreen.MainMenu->IsActive() && FullScreen.MainMenu == menu)
	{
		return true;
	}
	// no match
	return false;
}

// C4ViewportList

C4ViewportList Viewports;

C4ViewportList::C4ViewportList()
{
	ViewportArea.Default();
}
C4ViewportList::~C4ViewportList() = default;
void C4ViewportList::Clear()
{
	C4Viewport *next;
	while (FirstViewport)
	{
		next = FirstViewport->Next;
		delete FirstViewport;
		FirstViewport = next;
	}
	FirstViewport = nullptr;
}

void C4ViewportList::Execute(bool draw_background)
{
	// Background redraw
	if (draw_background)
	{
		DrawFullscreenBackground();
	}
	for (C4Viewport *viewport = FirstViewport; viewport; viewport = viewport->Next)
	{
		if (viewport->GetWindow())
		{
			viewport->GetWindow()->RequestUpdate();
		}
		else
		{
			viewport->Execute();
		}
	}
}

void C4ViewportList::DrawFullscreenBackground()
{
	for (int i = 0, num = BackgroundAreas.GetCount(); i < num; ++i)
	{
		const C4Rect &rc = BackgroundAreas.Get(i);
		pDraw->BlitSurfaceTile(::GraphicsResource.fctBackground.Surface, FullScreen.pSurface, rc.x, rc.y, rc.Wdt, rc.Hgt, -rc.x, -rc.y, nullptr);
	}
}

bool C4ViewportList::CloseViewport(C4Viewport * viewport)
{
	if (!viewport)
	{
		return false;
	}
	// Chop the start of the chain off
	if (FirstViewport == viewport)
	{
		FirstViewport = viewport->Next;
		delete viewport;
		StartSoundEffect("UI::CloseViewport");
	}
	// Take out of the chain
	else for (C4Viewport * prev = FirstViewport; prev; prev = prev->Next)
	{
		if (prev->Next == viewport)
		{
			prev->Next = viewport->Next;
			delete viewport;
			StartSoundEffect("UI::CloseViewport");
		}
	}
	// Deleting a viewport may leave us with no context selected
	if (pDraw)
	{
		pDraw->EnsureMainContextSelected();
	}
	// Recalculate viewports
	RecalculateViewports();
	// Done
	return true;
}
#ifdef USE_WIN32_WINDOWS
C4Viewport* C4ViewportList::GetViewport(HWND hwnd)
{
	for (C4Viewport *cvp=FirstViewport; cvp; cvp=cvp->Next)
	{
		if (cvp->pWindow->hWindow==hwnd)
		{
			return cvp;
		}
	}
	return nullptr;
}
#endif
bool C4ViewportList::CreateViewport(int32_t player_nr, bool silent)
{
	// Create and init new viewport, add to viewport list
	int32_t last_count = GetViewportCount();
	C4Viewport *viewport = new C4Viewport;
	bool is_okay = viewport->Init(player_nr, false);
	if (!is_okay)
	{
		delete viewport;
		return false;
	}
	C4Viewport *last;
	for (last = FirstViewport; last && last->Next; last = last->Next)
	{

	}
	if (last)
	{
		last->Next = viewport;
	}
	else
	{
		FirstViewport = viewport;
	}
	// Recalculate viewports
	RecalculateViewports();
	// Viewports start off at centered position
	viewport->CenterPosition();
	// Initial player zoom values to viewport (in case they were set early in InitializePlayer, loaded from savegame, etc.)
	C4Player *player = ::Players.Get(player_nr);
	if (player)
	{
		player->ZoomToViewport(viewport, true, false, false);
		player->ZoomLimitsToViewport(viewport);
	}
	// Action sound
	if (!silent && GetViewportCount() != last_count)
	{
			StartSoundEffect("UI::CloseViewport");
	}
	return true;
}

void C4ViewportList::DisableFoW()
{
	for (C4Viewport *viewport = FirstViewport; viewport; viewport = viewport->Next)
	{
		viewport->DisableFoW();
	}
}

void C4ViewportList::EnableFoW()
{
	for (C4Viewport *viewport = FirstViewport; viewport; viewport = viewport->Next)
	{
		viewport->EnableFoW();
	}
}

void C4ViewportList::ClearPointers(C4Object *obj)
{
	for (C4Viewport *viewport = FirstViewport; viewport; viewport = viewport->Next)
	{
		viewport->ClearPointers(obj);
	}
}

bool C4ViewportList::CloseViewport(int32_t player_nr, bool silent)
{
	// Close all matching viewports
	int32_t last_count = GetViewportCount();
	C4Viewport *next;
	C4Viewport *prev = nullptr;
	for (C4Viewport *viewport = FirstViewport; viewport; viewport = next)
	{
		next = viewport->Next;
		if (viewport->Player == player_nr || (player_nr == NO_OWNER && viewport->fIsNoOwnerViewport))
		{
			delete viewport;
			if (prev)
			{
				prev->Next = next;
			}
			else
			{
				FirstViewport = next;
			}
		}
		else
		{
			prev = viewport;
		}
	}
	// Anything was done?
	if (GetViewportCount() != last_count)
	{
		// Recalculate viewports
		RecalculateViewports();
		// Action sound
		if (!silent)
		{
			StartSoundEffect("UI::CloseViewport");
		}
	}
	return true;
}

void C4ViewportList::RecalculateViewports()
{

	// Fullscreen only
	if (Application.isEditor) return;

	// Sort viewports
	SortViewportsByPlayerControl();

	// Viewport area
	int32_t border_top = 0;
	if (Config.Graphics.UpperBoard)
	{
		border_top = C4UpperBoardHeight;
	}
	ViewportArea.Set(FullScreen.pSurface, 0, border_top, C4GUI::GetScreenWdt(), C4GUI::GetScreenHgt() - border_top);

	// Redraw flag
	::GraphicsSystem.InvalidateBg();
#ifdef _WIN32
	// reset mouse clipping
	ClipCursor(nullptr);
#else
	// StdWindow handles this.
#endif
	// reset GUI dlg pos
	::pGUI->SetPreferredDlgRect(C4Rect(ViewportArea.X, ViewportArea.Y, ViewportArea.Wdt, ViewportArea.Hgt));

	// fullscreen background: First, cover all of screen
	BackgroundAreas.Clear();
	BackgroundAreas.AddRect(C4Rect(ViewportArea.X, ViewportArea.Y, ViewportArea.Wdt, ViewportArea.Hgt));

	// Viewports
	C4Viewport *viewport;
	int32_t view_count = 0;
	for (viewport = FirstViewport; viewport; viewport = viewport->Next)
	{
		view_count++;
	}
	if (!view_count)
	{
		return;
	}
	int32_t iViewsH = (int32_t) sqrt(float(view_count));
	int32_t iViewsX = view_count / iViewsH;
	int32_t iViewsL = view_count % iViewsH;
	int32_t cViewH, cViewX, ciViewsX;
	int32_t cViewWdt, cViewHgt, cOffWdt, cOffHgt, cOffX, cOffY;
	viewport=FirstViewport;
	for (cViewH = 0; cViewH < iViewsH; cViewH++)
	{
		ciViewsX = iViewsX;
		if (cViewH < iViewsL)
		{
			ciViewsX++;
		}
		for (cViewX=0; cViewX < ciViewsX; cViewX++)
		{
			cViewWdt = ViewportArea.Wdt/ciViewsX;
			cViewHgt = ViewportArea.Hgt/iViewsH;
			cOffX = ViewportArea.X;
			cOffY = ViewportArea.Y;
			cOffWdt = cOffHgt = 0;
			if (ciViewsX * cViewWdt < ViewportArea.Wdt)
			{
				cOffX = (ViewportArea.Wdt - ciViewsX * cViewWdt) / 2;
			}
			if (iViewsH * cViewHgt < ViewportArea.Hgt)
			{
				cOffY = (ViewportArea.Hgt - iViewsH * cViewHgt) / 2 + ViewportArea.Y;
			}
			if (Config.Graphics.SplitscreenDividers)
			{
				if (cViewX < ciViewsX - 1) cOffWdt=4;
				if (cViewH < iViewsH - 1) cOffHgt=4;
			}
			int32_t coViewWdt = cViewWdt-cOffWdt;
			int32_t coViewHgt = cViewHgt-cOffHgt;
			C4Rect rcOut(cOffX+cViewX*cViewWdt, cOffY+cViewH*cViewHgt, coViewWdt, coViewHgt);
			viewport->SetOutputSize(rcOut.x,rcOut.y,rcOut.x,rcOut.y,rcOut.Wdt,rcOut.Hgt);
			viewport = viewport->Next;
			// clip down area avaiable for background drawing
			BackgroundAreas.ClipByRect(rcOut);
		}
	}
	// and finally recalculate script menus
	if (::Game.ScriptGuiRoot)
	{
		::Game.ScriptGuiRoot->RequestLayoutUpdate();
	}
}

int32_t C4ViewportList::GetViewportCount()
{
	int32_t amount = 0;
	for (C4Viewport *viewport = FirstViewport; viewport; viewport = viewport->Next)
	{
		amount++;
	}
	return amount;
}

C4Viewport* C4ViewportList::GetViewport(int32_t player, C4Viewport* prev)
{
	for (C4Viewport *viewport = prev ? prev->Next : FirstViewport; viewport ; viewport = viewport ->Next)
	{
		if (viewport->Player == player || (player == NO_OWNER && viewport ->fIsNoOwnerViewport))
		{
			return viewport;
		}
	}
	return nullptr;
}

int32_t C4ViewportList::GetAudibility(int32_t x, int32_t y, int32_t *pan, int32_t audibility_radius, int32_t *out_player)
{
	// default audibility radius
	if (!audibility_radius)
	{
		audibility_radius = C4AudibilityRadius;
	}
	// Accumulate audibility by viewports
	int32_t audible = 0;
	*pan = 0;
	for (C4Viewport *viewport = FirstViewport; viewport; viewport = viewport->Next)
	{
		float distanceToCenterOfViewport = Distance(viewport->GetViewCenterX(), viewport->GetViewCenterY(), x, y);
		int32_t audibility = Clamp<int32_t>(100 - 100 * distanceToCenterOfViewport / C4AudibilityRadius, 0, 100);
		if (audibility > audible)
		{
			audible = audibility;
			if (out_player)
			{
				*out_player = viewport->Player;
			}
		}
		*pan += (x-(viewport->GetViewCenterX())) / 5;
	}
	*pan = Clamp<int32_t>(*pan, -100, 100);
	return audible;
}

void C4ViewportList::SortViewportsByPlayerControl()
{

	// Sort
	bool is_sorted;
	C4Player *player_a, *player_b;
	C4Viewport *viewport, *next, *prev;
	do
	{
		is_sorted = true;
		for (prev = nullptr, viewport = FirstViewport; viewport && (next = viewport->Next); viewport=next)
		{
			// Get players
			player_a = ::Players.Get(viewport->Player);
			player_b = ::Players.Get(next->Player);
			// Swap order
			if (player_a && player_b && player_a->ControlSet && player_b->ControlSet && ( player_a->ControlSet->GetLayoutOrder() > player_b->ControlSet->GetLayoutOrder() ))
			{
				if (prev) prev->Next = next; else FirstViewport = next;
				viewport->Next = next->Next;
				next->Next = viewport;
				prev = next;
				next = viewport;
				is_sorted = false;
			}
			// Don't swap
			else
			{
				prev = viewport;
			}
		}
	}
	while (!is_sorted);

}

bool C4ViewportList::ViewportNextPlayer()
{
	// safety: switch valid?
	if ((!Game.C4S.Head.Film || !Game.C4S.Head.Replay) && !GetViewport(NO_OWNER))
	{
		return false;
	}
	// do switch then
	C4Viewport *viewport = GetFirstViewport();
	if (!viewport)
	{
		return false;
	}
	viewport->NextPlayer();
	return true;
}

bool C4ViewportList::FreeScroll(C4Vec2D scroll_by)
{
	// safety: move valid?
	if ((!Game.C4S.Head.Replay || !Game.C4S.Head.Film) && !GetViewport(NO_OWNER))
	{
		return false;
	}
	C4Viewport *viewport = GetFirstViewport();
	if (!viewport)
	{
		return false;
	}
	// move then (old static code crap...)
	static int32_t vp_vx = 0;
	static int32_t vp_vy = 0;
	static int32_t vp_vf = 0;
	int32_t dx = scroll_by.x;
	int32_t dy = scroll_by.y;
	if (Game.FrameCounter-vp_vf < 5)
	{
		dx += vp_vx;
		dy += vp_vy;
	}
	vp_vx = dx;
	vp_vy = dy;
	vp_vf = Game.FrameCounter;
	viewport->ScrollView(dx, dy);
	return true;
}

bool C4ViewportList::ViewportZoomOut()
{
	for (C4Viewport *viewport = FirstViewport; viewport; viewport = viewport->Next) viewport->ChangeZoom(1.0f/C4GFX_ZoomStep);
	return true;
}

bool C4ViewportList::ViewportZoomIn()
{
	for (C4Viewport *viewport = FirstViewport; viewport; viewport = viewport->Next) viewport->ChangeZoom(C4GFX_ZoomStep);
	return true;
}

void C4ViewportList::MouseMoveToViewport(int32_t button, int32_t x, int32_t y, DWORD key_param)
{
	// Pass on to mouse controlled viewport
	for (C4Viewport *viewport = FirstViewport; viewport; viewport = viewport->Next)
	{
		if (::MouseControl.IsViewport(viewport))
		{
			::MouseControl.Move( button,
			                     Clamp<int32_t>(x-viewport->OutX,0,viewport->ViewWdt-1),
			                     Clamp<int32_t>(y-viewport->OutY,0,viewport->ViewHgt-1),
			                     key_param );
		}
	}
}
