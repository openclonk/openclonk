/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2003-2004  Matthes Bender
 * Copyright (c) 2001-2003, 2005-2007  Sven Eberhardt
 * Copyright (c) 2002, 2004  Peter Wortmann
 * Copyright (c) 2005-2006, 2008  Günther Brammer
 * Copyright (c) 2006  Armin Burgmeier
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

/* Mouse input */

#include <C4Include.h>
#include <C4MouseControl.h>

#include <C4Viewport.h>
#include <C4Object.h>
#include <C4Command.h>
#include <C4Application.h>
#include <C4FullScreen.h>
#include <C4Gui.h>
#include <C4Landscape.h>
#include <C4Game.h>
#include <C4Player.h>
#include "C4ChatDlg.h"
#include <C4GraphicsResource.h>
#include <C4GraphicsSystem.h>
#include <C4PlayerList.h>
#include <C4GameObjects.h>
#include <C4GameControl.h>

const int32_t C4MC_Drag_None					= 0,
					C4MC_Drag_Selecting			= 1,
					C4MC_Drag_Moving				= 2,
					//C4MC_Drag_Menu					= 3,
					//C4MC_Drag_MenuScroll		= 4,
					C4MC_Drag_Construct			= 5,
					C4MC_Drag_Script            = 6,

					C4MC_Selecting_Unknown	= 0;

const int32_t C4MC_Cursor_Region			= 0,
					C4MC_Cursor_Crosshair		= 1,
					C4MC_Cursor_Enter				= 2,
					C4MC_Cursor_Grab				= 3,
					C4MC_Cursor_Chop				= 4,
					C4MC_Cursor_Dig					= 5,
					C4MC_Cursor_Build				= 6,
					C4MC_Cursor_Select			= 7,
					C4MC_Cursor_Object			= 8,
					C4MC_Cursor_Ungrab			= 9,
					C4MC_Cursor_Up					= 10,
					C4MC_Cursor_Down				= 11,
					C4MC_Cursor_Left				= 12,
					C4MC_Cursor_Right				= 13,
					C4MC_Cursor_UpLeft			= 14,
					C4MC_Cursor_UpRight			= 15,
					C4MC_Cursor_DownLeft		= 16,
					C4MC_Cursor_DownRight		= 17,
					C4MC_Cursor_JumpLeft		= 18,
					C4MC_Cursor_JumpRight		= 19,
					C4MC_Cursor_Drop				= 20,
					C4MC_Cursor_ThrowRight	= 21,
					C4MC_Cursor_Put					= 22,
					//C4MC_Cursor_DragMenu		= 23,
					C4MC_Cursor_Vehicle			= 24,
					C4MC_Cursor_VehiclePut	= 25,
					C4MC_Cursor_ThrowLeft		= 26,
					C4MC_Cursor_DragDrop		= 26,
					C4MC_Cursor_Point				= 27,
					C4MC_Cursor_DigObject		= 28,
					C4MC_Cursor_Help				= 29,
					C4MC_Cursor_DigMaterial	= 30,
					C4MC_Cursor_Add					= 31,
					C4MC_Cursor_Construct		= 32,
					C4MC_Cursor_Attack			= 33,
					C4MC_Cursor_Nothing			= 34;

const int32_t C4MC_Time_on_Target			= 10;

C4MouseControl::C4MouseControl()
	{
	Default();
	}

C4MouseControl::~C4MouseControl()
	{
	Clear();
	}

void C4MouseControl::Default()
	{
	Active=false;
	Player=NO_OWNER;
	pPlayer=NULL;
	Viewport=NULL;
	Cursor=DownCursor=0;
	Caption.Clear();
	IsHelpCaption=false;
	CaptionBottomY=0;
	VpX=VpY=0;
	DownX=DownY=DownOffsetX=DownOffsetY=0;
	ViewX=ViewY=0;
	GuiX=GuiY=GameX=GameY=0;
	ShowPointX=ShowPointY=-1;
	LeftButtonDown=RightButtonDown=false;
	LeftDoubleIgnoreUp=false;
	ButtonDownOnSelection=false;
	Visible=true;
	InitCentered=false;
	Help=false;
	FogOfWar=false;
	DragID=C4ID_None;
	DragObject=NULL;
	KeepCaption=0;
	Drag=C4MC_Drag_None; DragSelecting=C4MC_Selecting_Unknown;
	Selection.Default();
	TargetObject=DownTarget=NULL;
	TimeOnTargetObject=0;
	ControlDown=false;
	ShiftDown=false;
	Scrolling=false;
	ScrollSpeed=10;
	TargetRegion=NULL;
	DownRegion.Default();
	DragImage.Default();
	DragImagePhase=0;
	fMouseOwned = true; // default mouse owned
	fctViewport.Default();
	}

void C4MouseControl::Clear()
	{
	Active = false;
	Selection.Clear();
	UpdateClip(); // reset mouse clipping!
	}

void C4MouseControl::Execute()
	{

	if (!Active || !fMouseOwned) return;

	// Scrolling/continuous update
	if (Scrolling || !::Game.iTick5)
		{
		WORD wKeyState=0;
		if (Application.IsControlDown()) wKeyState|=MK_CONTROL;
		if (Application.IsShiftDown()) wKeyState|=MK_SHIFT;
		Move(C4MC_Button_None,VpX,VpY,wKeyState);
		}
	}

bool C4MouseControl::Init(int32_t iPlayer)
	{
	Clear();
	Default();
	Active = true;
	Player = iPlayer;
	InitCentered = false;
	UpdateClip();
	return true;
	}

void C4MouseControl::ClearPointers(C4Object *pObj)
	{
	if (TargetObject==pObj) TargetObject=NULL;
	if (DownTarget==pObj) DownTarget=NULL;
	if (DragObject==pObj)
	{
		DragObject=NULL;
		Drag=C4MC_Drag_None;
		DragImage.Default();
	}
	Selection.ClearPointers(pObj);
	}

bool C4MouseControl::IsViewport(C4Viewport *pViewport)
	{
	return (Viewport==pViewport);
	}

void C4MouseControl::UpdateClip()
	{
#ifdef _DEBUG
	// never in debug
	return;
#endif
#ifdef _WIN32
	// fullscreen only
	if (!Application.isFullScreen) return;
	// application or mouse control not active? remove any clips
	if (!Active || !Application.Active || (::pGUI && ::pGUI->HasMouseFocus())) { ClipCursor(NULL); return; }
	// get controlled viewport
	C4Viewport *pVP=::GraphicsSystem.GetViewport(Player);
	if (!pVP) { ClipCursor(NULL); return; }
	// adjust size by viewport size
	RECT vpRct;
	vpRct.left=pVP->OutX; vpRct.top=pVP->OutY; vpRct.right=pVP->OutX+pVP->ViewWdt; vpRct.bottom=pVP->OutY+pVP->ViewHgt;
	// adjust by window pos
	RECT rtWindow;
	if (GetWindowRect(FullScreen.hWindow, &rtWindow))
		{
		vpRct.left += rtWindow.left; vpRct.top += rtWindow.top;
		vpRct.right += rtWindow.left; vpRct.bottom+= rtWindow.top;
		}
	ClipCursor(&vpRct);
	// and inform GUI
	if (::pGUI)
		::pGUI->SetPreferredDlgRect(C4Rect(pVP->OutX, pVP->OutY, pVP->ViewWdt, pVP->ViewHgt));
#endif
	//StdWindow manages this.
	}

void C4MouseControl::Move(int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyFlags, bool fCenter)
{
	// Active
	if (!Active || !fMouseOwned) return;
	// Execute caption
	if (KeepCaption) KeepCaption--; else { Caption.Clear(); IsHelpCaption=false; CaptionBottomY=0; }
	// Check player
	if ((Player>NO_OWNER) && !(pPlayer=::Players.Get(Player))) { Active=false; return; }
	// Check viewport
	if (!(Viewport=::GraphicsSystem.GetViewport(Player))) return;
	// get view position
	C4Rect rcViewport = Viewport->GetOutputRect();
	fctViewport.Set(NULL, rcViewport.x, rcViewport.y, rcViewport.Wdt, rcViewport.Hgt);
	ViewX=Viewport->ViewX; ViewY=Viewport->ViewY;
	fctViewportGame = Viewport->last_game_draw_cgo;
	fctViewportGUI = Viewport->last_gui_draw_cgo;
	// First time viewport attachment: center mouse
#ifdef _WIN32
	if (!InitCentered || fCenter)
	{
		iX = Viewport->ViewWdt/2;
		iY = Viewport->ViewHgt/2;
		if (Application.isFullScreen)
		{
			int32_t iMidX = Viewport->OutX + iX;
			int32_t iMidY = Viewport->OutY + iY;
			RECT rtWindow;
			if (GetWindowRect(Application.pWindow->hWindow, &rtWindow))
			{
				iMidX += rtWindow.left; iMidY += rtWindow.top;
			}
			SetCursorPos(iMidX, iMidY);
		}
		InitCentered = true;
	}
#else
	if (!InitCentered || fCenter)
	{
		iX = Viewport->ViewWdt/2;
		iY = Viewport->ViewHgt/2;
		if (Application.isFullScreen)
		{
			//int32_t iMidX = Viewport->OutX + iX;
			//int32_t iMidY = Viewport->OutY + iY;
			//FIXMESetCursorPos(iMidX, iMidY);
		}
		InitCentered = true;
	}
#endif
	// passive mode: scrolling and player buttons only
	if (IsPassive())
	{
		if (iButton != C4MC_Button_Wheel)
		{
			VpX=iX; VpY=iY;
			GameX=ViewX+VpX/Viewport->Zoom; GameY=ViewY+VpY/Viewport->Zoom;
			GuiX=float(VpX)/C4GUI::GetZoom(); GuiY=float(VpY)/C4GUI::GetZoom();
		}
		UpdateTargetRegion();
		UpdateScrolling();
		if (iButton == C4MC_Button_LeftDown) LeftDown();
		else if (iButton == C4MC_Button_LeftUp) LeftUp();
		else UpdateCursorTarget();
		return;
	}

	if (iButton != C4MC_Button_Wheel)
	{
		// Position
		VpX=iX; VpY=iY;
		GameX=ViewX+VpX/Viewport->Zoom; GameY=ViewY+VpY/Viewport->Zoom;
		GuiX=float(VpX)/C4GUI::GetZoom(); GuiY=float(VpY)/C4GUI::GetZoom();
		// Control state
		ControlDown=false; if (dwKeyFlags & MK_CONTROL) ControlDown=true;
		ShiftDown=false; if (dwKeyFlags & MK_SHIFT) ShiftDown=true;
		// Target region
		UpdateTargetRegion();
		// Scrolling
		UpdateScrolling();
		// Fog of war
		UpdateFogOfWar();

		// Blocked by fog of war: evaluate button up, dragging and region controls only
		if (FogOfWar && Drag == C4MC_Drag_None && !TargetRegion)
		{
			// Left button up
			if (iButton==C4MC_Button_LeftUp)
			{
				LeftButtonDown=false;
				// End any drag
				Drag=C4MC_Drag_None;
			}
			// Right button up
			if (iButton==C4MC_Button_RightUp)
			{
				RightButtonDown=false;
			}
		}
	}

	// Move execution by button/drag status
	switch (iButton)
	{
		//------------------------------------------------------------------------------------------
		case C4MC_Button_None:
			switch (Drag)
			{
				case C4MC_Drag_None: DragNone(); break;
				case C4MC_Drag_Selecting: DragSelect(); break;
				case C4MC_Drag_Moving: DragMoving(); break;
				case C4MC_Drag_Construct: DragConstruct(); break;
				case C4MC_Drag_Script: DragScript(); break;
			}
			break;
		//------------------------------------------------------------------------------------------
		case C4MC_Button_LeftDown: LeftDown(); break;
		//------------------------------------------------------------------------------------------
		case C4MC_Button_LeftUp: LeftUp(); break;
		//------------------------------------------------------------------------------------------
		case C4MC_Button_LeftDouble: LeftDouble(); break;
		//------------------------------------------------------------------------------------------
		case C4MC_Button_RightDown: RightDown(); break;
		//------------------------------------------------------------------------------------------
		case C4MC_Button_RightUp: RightUp(); break;
		//------------------------------------------------------------------------------------------
		case C4MC_Button_Wheel: Wheel(dwKeyFlags); break;
	}

	// script handling of mouse control for everything but regular movement (which is sent at control frame intervals only)
	if (iButton != C4MC_Button_None)
		// not if blocked by selection object
		if (!TargetRegion && !TargetObject)
			// safety (can't really happen in !IsPassive, but w/e
			if (pPlayer && pPlayer->ControlSet)
			{
				if (pPlayer->ControlSet->IsMouseControlAssigned(iButton))
				{
					int wheel_dir = 0;
					if (iButton == C4MC_Button_Wheel) wheel_dir = (short)(dwKeyFlags >> 16);
					pPlayer->Control.DoMouseInput(0 /* only 1 mouse supported so far */, iButton, GameX, GameY, GuiX, GuiY, ControlDown, ShiftDown, Application.IsAltDown(), wheel_dir);
				}
			}
}

void C4MouseControl::DoMoveInput()
{
	// current mouse move to input queue
	// do sanity checks
	if (!Active || !fMouseOwned) return;
	if (!(pPlayer=::Players.Get(Player))) return;
	if (!pPlayer->ControlSet) return;
	if (!pPlayer->ControlSet->IsMouseControlAssigned(C4MC_Button_None)) return;
	pPlayer->Control.DoMouseInput(0 /* only 1 mouse supported so far */, C4MC_Button_None, GameX, GameY, GuiX, GuiY, ControlDown, ShiftDown, Application.IsAltDown(), 0);
}

void C4MouseControl::Draw(C4TargetFacet &cgo, const ZoomData &GameZoom)
{
	int32_t iOffsetX,iOffsetY;

	ZoomData GuiZoom;
	lpDDraw->GetZoom(&GuiZoom);

	// Hidden
	if (!Visible || !fMouseOwned) return;

	// Draw selection
	if (!IsPassive())
	{
		Selection.DrawSelectMark(cgo, 1.0f);
	}

	// Draw control
	switch (Drag)
	{
		//------------------------------------------------------------------------------------------
		case C4MC_Drag_None: case C4MC_Drag_Moving: case C4MC_Drag_Construct: case C4MC_Drag_Script:
			// Hotspot offset: Usually, hotspot is in center
			iOffsetX = GfxR->fctMouseCursor.Wdt/2;
			iOffsetY = GfxR->fctMouseCursor.Hgt/2;
			// Previously, there used to be custom-defined hotspots for all cursors. Calc them in.
			if (GfxR->fOldStyleCursor)
			{
				switch (Cursor)
				{
					case C4MC_Cursor_Select: case C4MC_Cursor_Region: //case C4MC_Cursor_DragMenu:
						iOffsetX=iOffsetY=0;
						break;
					case C4MC_Cursor_Dig: case C4MC_Cursor_DigMaterial:
						iOffsetX=0; iOffsetY=GfxR->fctMouseCursor.Hgt;
						break;
					case C4MC_Cursor_Construct:
					case C4MC_Cursor_DragDrop:
						// calculated when dragimage is drawn
						break;
				}
			}
			else
			{
				// for new cursors, this hotspot exists for the scrolling cursors only
				switch (Cursor)
				{
					case C4MC_Cursor_Up: iOffsetY += -GfxR->fctMouseCursor.Hgt/2; break;
					case C4MC_Cursor_Down:iOffsetY += +GfxR->fctMouseCursor.Hgt/2; break;
					case C4MC_Cursor_Left: iOffsetX += -GfxR->fctMouseCursor.Wdt/2; break;
					case C4MC_Cursor_Right: iOffsetX += +GfxR->fctMouseCursor.Wdt/2; break;
					case C4MC_Cursor_UpLeft: iOffsetX += -GfxR->fctMouseCursor.Wdt/2; iOffsetY += -GfxR->fctMouseCursor.Hgt/2; break;
					case C4MC_Cursor_UpRight: iOffsetX += +GfxR->fctMouseCursor.Wdt/2; iOffsetY += -GfxR->fctMouseCursor.Hgt/2; break;
					case C4MC_Cursor_DownLeft: iOffsetX += -GfxR->fctMouseCursor.Wdt/2; iOffsetY += +GfxR->fctMouseCursor.Hgt/2; break;
					case C4MC_Cursor_DownRight: iOffsetX += +GfxR->fctMouseCursor.Wdt/2; iOffsetY += +GfxR->fctMouseCursor.Hgt/2; break;
				}
			}
			// Add mark
			bool fAddMark; fAddMark=false;
			if (ShiftDown)
				if ((Cursor!=C4MC_Cursor_Region) && (Cursor!=C4MC_Cursor_Select) //&& (Cursor!=C4MC_Cursor_DragMenu)
				 && (Cursor!=C4MC_Cursor_JumpLeft) && (Cursor!=C4MC_Cursor_JumpRight))
					if (!IsPassive())
						fAddMark=true;
			// Drag image
			if (DragImage.Surface)
			{
				// zoom mode: Drag in GUI or Game depending on source object
				bool fIsGameZoom = true;
				if (Drag == C4MC_Drag_Script && DragObject && (DragObject->Category & C4D_Foreground))
					fIsGameZoom = false;
				// drag image in game zoom
				float XDraw, YDraw, ZoomDraw;
				if (fIsGameZoom)
				{
					lpDDraw->SetZoom(GameZoom);
					XDraw = GameX; YDraw = GameY;
					ZoomDraw = 1.0f;
					// for drag construct: draw rounded to game pixels, because construction site will be placed at rounded game pixel positions
					if (Drag == C4MC_Drag_Construct) { XDraw=floor(XDraw); YDraw=floor(YDraw); }
				}
				else
				{
					ZoomDraw = 64.0f / DragImage.Wdt;
					XDraw = GuiX; YDraw = GuiY;
				}
				// draw in special modulation mode
				lpDDraw->SetBlitMode(C4GFXBLIT_MOD2);
				// draw DragImage in red or green, according to the phase to be used
				iOffsetX=int(ZoomDraw*DragImage.Wdt/2);
				if (Drag == C4MC_Drag_Construct)
					iOffsetY=int(ZoomDraw*DragImage.Hgt);
				else
					iOffsetY=int(ZoomDraw*DragImage.Hgt/2);
				lpDDraw->ActivateBlitModulation((Drag == C4MC_Drag_Script) ? 0x7fffffff : (DragImagePhase ? 0x8f7f0000 : 0x1f007f00));
				lpDDraw->Blit(DragImage.Surface,
				              float(DragImage.X), float(DragImage.Y), float(DragImage.Wdt), float(DragImage.Hgt),
				              cgo.Surface,
				              XDraw + cgo.X - iOffsetX, YDraw + cgo.Y - iOffsetY, float(DragImage.Wdt)*ZoomDraw, float(DragImage.Hgt)*ZoomDraw,true);
				// reset color
				lpDDraw->DeactivateBlitModulation();
				lpDDraw->SetBlitMode(0);
				if (fIsGameZoom) lpDDraw->SetZoom(GuiZoom);
				// reset cursor hotspot offset for script drawing
				iOffsetX = GfxR->fctMouseCursor.Wdt/2;
				iOffsetY = GfxR->fctMouseCursor.Hgt/2;
			}
			// Cursor
			if (!DragImage.Surface || (Drag == C4MC_Drag_Script))
				GfxR->fctMouseCursor.Draw(cgo.Surface,cgo.X+GuiX-iOffsetX,cgo.Y+GuiY-iOffsetY,Cursor);
			// Point
			if ((ShowPointX!=-1) && (ShowPointY!=-1))
				GfxR->fctMouseCursor.Draw( cgo.Surface,
													int32_t(cgo.X+(ShowPointX-cgo.TargetX)*GameZoom.Zoom / GuiZoom.Zoom-GfxR->fctMouseCursor.Wdt/2),
													int32_t(cgo.Y+(ShowPointY-cgo.TargetY)*GameZoom.Zoom / GuiZoom.Zoom-GfxR->fctMouseCursor.Hgt/2),
													C4MC_Cursor_Point );
			// Add mark
			if (fAddMark)
				GfxR->fctMouseCursor.Draw( cgo.Surface,
																	 int32_t(cgo.X+GuiX-iOffsetX+8),
																	 int32_t(cgo.Y+GuiY-iOffsetY+8),
																	 C4MC_Cursor_Add );
			break;
		//------------------------------------------------------------------------------------------
		case C4MC_Drag_Selecting:
			// Draw frame
			Application.DDraw->DrawFrame( cgo.Surface,
															int32_t(cgo.X + GuiX),
															int32_t(cgo.Y + GuiY),
															int32_t(cgo.X + (DownX - cgo.TargetX) * GameZoom.Zoom / GuiZoom.Zoom),
															int32_t(cgo.Y + (DownY - cgo.TargetY) * GameZoom.Zoom / GuiZoom.Zoom),
															CRed );
			break;
		//------------------------------------------------------------------------------------------
	}

	// Draw caption
	if (Caption)
	{
		if (IsHelpCaption && ::pGUI)
		{
			// Help: Tooltip style
			C4TargetFacet cgoTip; cgoTip = static_cast<const C4Facet &>(cgo);
			C4GUI::Screen::DrawToolTip(Caption.getData(), cgoTip, cgo.X+GuiX, cgo.Y+GuiY);
		}
		else
		{
			// Otherwise red mouse control style
			int32_t iWdt,iHgt;
			::GraphicsResource.FontRegular.GetTextExtent(Caption.getData(), iWdt, iHgt, true);
			Application.DDraw->TextOut(Caption.getData(), ::GraphicsResource.FontRegular, 1.0,
													cgo.Surface,
													float(cgo.X)+BoundBy<float>(GuiX,float(iWdt)/2+1,float(cgo.Wdt)-iWdt/2-1),
													float(cgo.Y)+Min<float>( CaptionBottomY ? float(CaptionBottomY-iHgt-1) : GuiY+13, float(cgo.Hgt-iHgt)),
													0xfaFF0000,ACenter);
		}
	}

}

void C4MouseControl::UpdateCursorTarget()
{
	int32_t iLastCursor = Cursor;

	// Scrolling: no other target
	if (Scrolling) { TargetObject=NULL; return; }

	// On target region
	if (TargetRegion)
	{
		TargetObject=NULL;
		if (Help) Cursor=C4MC_Cursor_Help;
		return;
	}

	// Check player cursor
	C4Object *pPlrCursor = pPlayer ? pPlayer->Cursor : NULL;

	// Target object
	TargetObject=GetTargetObject();
	if (TargetObject && FogOfWar && !(TargetObject->Category & C4D_IgnoreFoW)) TargetObject = NULL;

	// Movement
	if (!FogOfWar && !IsPassive()) Cursor=C4MC_Cursor_Crosshair;

	// Target action
	if (TargetObject && !IsPassive())
	{
		// default cursor for object; also set if not in FoW
		Cursor=C4MC_Cursor_Crosshair;
		// get position
		float iObjX, iObjY; TargetObject->GetViewPos(iObjX, iObjY, ViewX, ViewY, fctViewport);
		// select custom region
		if (TargetObject->Category & C4D_MouseSelect)
			Cursor=C4MC_Cursor_Select;
	}
	
	// Help
	if (Help)
		Cursor=C4MC_Cursor_Help;
	// passive cursor
	else if (IsPassive())
		Cursor=C4MC_Cursor_Region;
	
	// Time on target: caption
	if (Cursor==iLastCursor)
	{
		TimeOnTargetObject++;
		if (TimeOnTargetObject>=C4MC_Time_on_Target)
		{
			const char* idCaption = 0;
			const char *szName = "";
			bool fDouble = false;
			if (TargetObject) szName=TargetObject->GetName();
			// Target caption by cursor
			switch (Cursor)
			{
				case C4MC_Cursor_Select: idCaption="IDS_CON_SELECT"; break;
				case C4MC_Cursor_Help: idCaption="IDS_CON_NAME"; break;
			}
			// Set caption
			if (idCaption) if (!KeepCaption)
			{
				// Caption by cursor
				Caption.Format(LoadResStr(idCaption), szName);
				if (fDouble) { Caption.AppendChar('|'); Caption.Append(LoadResStr("IDS_CON_DOUBLECLICK")); }
				IsHelpCaption = false;
			}
		}
	}
	else
		TimeOnTargetObject=0;

}

int32_t C4MouseControl::UpdateSingleSelection()
	{

	// Set single selection if cursor on selection object (clear prior object selection)
	if (TargetObject && (Cursor==C4MC_Cursor_Select))
		{	Selection.Clear(); Selection.Add(TargetObject, C4ObjectList::stNone);	}

	// Cursor has moved off single object (or target object) selection: clear selection
	else if (Selection.GetObject())
		if (::Players.Get(Player)->ObjectInCrew(Selection.GetObject())
			|| (Selection.GetObject()->Category & C4D_MouseSelect))
			Selection.Clear();

	return Selection.ObjectCount();
	}

void C4MouseControl::UpdateScrolling()
	{
	// Assume no scrolling
	Scrolling=false;
	// No scrolling if on region
	if (TargetRegion) return;
	// Scrolling on border
	if (VpX==0)
		{ Cursor=C4MC_Cursor_Left; ScrollView(-ScrollSpeed,0,Viewport->ViewWdt,Viewport->ViewHgt); Scrolling=true; }
	if (VpY==0)
		{ Cursor=C4MC_Cursor_Up; ScrollView(0,-ScrollSpeed,Viewport->ViewWdt,Viewport->ViewHgt); Scrolling=true; }
	if (VpX==Viewport->ViewWdt-1)
		{ Cursor=C4MC_Cursor_Right; ScrollView(+ScrollSpeed,0,Viewport->ViewWdt,Viewport->ViewHgt); Scrolling=true; }
	if (VpY==Viewport->ViewHgt-1)
		{ Cursor=C4MC_Cursor_Down; ScrollView(0,+ScrollSpeed,Viewport->ViewWdt,Viewport->ViewHgt); Scrolling=true; }
	// Set correct cursor
	if ((VpX==0) && (VpY==0))	Cursor=C4MC_Cursor_UpLeft;
	if ((VpX==Viewport->ViewWdt-1) && (VpY==0))	Cursor=C4MC_Cursor_UpRight;
	if ((VpX==0) && (VpY==Viewport->ViewHgt-1))	Cursor=C4MC_Cursor_DownLeft;
	if ((VpX==Viewport->ViewWdt-1) && (VpY==Viewport->ViewHgt-1))	Cursor=C4MC_Cursor_DownRight;
	}

void C4MouseControl::UpdateTargetRegion()
{
	// Assume no region
	TargetRegion=NULL;
	// Find region
	if (!(TargetRegion=Viewport->Regions.Find(GuiX,GuiY))) return;
	// Region found: no target object
	TargetObject=NULL;
	// Cursor
	Cursor=C4MC_Cursor_Region;
	// Stop drag selecting (reset down cursor, too)
	if (Drag==C4MC_Drag_Selecting)
		{ Drag=C4MC_Drag_None; DownCursor=C4MC_Cursor_Nothing; }
	// Caption
	Caption.Copy(TargetRegion->Caption);
	IsHelpCaption = false;
	CaptionBottomY=TargetRegion->Y; KeepCaption=0;
	// Help region caption by region target object; not in menu, because this would be the cursor object
	if (Help)
		if (TargetRegion->Target /*&& Cursor!=C4MC_Cursor_DragMenu*/)
		{
			if (TargetRegion->Target->Def->GetDesc())
				Caption.Format("%s: %s",TargetRegion->Target->GetName(), TargetRegion->Target->Def->GetDesc());
			else
				Caption.Copy(TargetRegion->Target->GetName());
			IsHelpCaption = true;
		}
	// MoveOverCom (on region change)
	static int32_t iLastRegionX,iLastRegionY;
	if (TargetRegion->MoveOverCom)
	{
		if ((TargetRegion->X!=iLastRegionX) || (TargetRegion->Y!=iLastRegionY))
		{
			iLastRegionX=TargetRegion->X; iLastRegionY=TargetRegion->Y;

			// Control queue
			Game.Input.Add(CID_PlrControl,
				new C4ControlPlayerControl(Player,TargetRegion->MoveOverCom,TargetRegion->Data));
		}
	}
	else
	{
		iLastRegionX=iLastRegionY=-1;
	}
}

void C4MouseControl::LeftDown()
{
	// Set flag
	LeftButtonDown=true;
	// Store down values (same MoveRightDown -> use StoreDown)
	DownX=GameX; DownY=GameY;
	DownCursor=Cursor;
	DownTarget=TargetObject;
	DownRegion.Default();
	if (TargetRegion)
	{
		DownRegion=(*TargetRegion);
		DownTarget=TargetRegion->Target;
		DownOffsetX=TargetRegion->X-GuiX; DownOffsetY=TargetRegion->Y-GuiY;
	}
}

void C4MouseControl::DragSelect()
{
	// don't select into FoW - simply don't update selection
	if (FogOfWar) return;
	switch (DragSelecting)
	{
		case C4MC_Selecting_Unknown:
			// Determine selection type
			// No selection in engine right now...
			break;
	}
}

void C4MouseControl::LeftUp()
{
	// Update status flag
	LeftButtonDown=false;
	// Ignore left up after double click
	if (LeftDoubleIgnoreUp) { LeftDoubleIgnoreUp=false; return; }
	// Evaluate by drag status
	switch (Drag)
	{
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4MC_Drag_None: LeftUpDragNone(); break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4MC_Drag_Selecting:	ButtonUpDragSelecting(); break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4MC_Drag_Moving: ButtonUpDragMoving(); break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4MC_Drag_Construct: ButtonUpDragConstruct(); break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4MC_Drag_Script: ButtonUpDragScript(); break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	}
}

void C4MouseControl::DragMoving()
{
	ShowPointX=ShowPointY=-1;

	// do not drag objects into FoW
	if (FogOfWar) { Cursor = C4MC_Cursor_Nothing; return; }

}

void C4MouseControl::DragNone()
{
	// Holding left down
	if (LeftButtonDown)
	{
		switch (Cursor)
		{
			// Hold down on region
			case C4MC_Cursor_Region:
				if (!::Game.iTick5)
					if (DownRegion.HoldCom)
						SendControl(DownRegion.HoldCom);
				break;
		}
	}

	// Cursor movement
	UpdateCursorTarget();
	// Update selection
	UpdateSingleSelection();

	// Button down: begin drag
	if ( (LeftButtonDown || RightButtonDown)
		&& ((Abs(GameX-DownX)>C4MC_DragSensitivity) || (Abs(GameY-DownY)>C4MC_DragSensitivity)) )
	{
		// don't begin dragging from FoW; unless it's a menu
		if (FogOfWar && DownCursor != C4MC_Cursor_Region) return;
		bool fAllowDrag = true;
		switch (DownCursor)
		{
				/*
			// Drag start selecting in landscape
			case C4MC_Cursor_Crosshair:
				Selection.Clear();
				Drag=C4MC_Drag_Selecting; DragSelecting=C4MC_Selecting_Unknown;
				break;
				*/
			// Help: no dragging
			case C4MC_Cursor_Help:
				fAllowDrag = false;
				break;
		}
		// check if target object allows scripted dragging
		if (TargetObject)
		{
			C4Object *drag_image_obj; C4ID drag_image_id;
			if (TargetObject->GetDragImage(&drag_image_obj, &drag_image_id))
			{
				Drag=C4MC_Drag_Script;
				CreateDragImage(drag_image_id, drag_image_obj, true);
				DragObject = TargetObject;
				DragImagePhase=0;
			}
		}
	}
}

void C4MouseControl::LeftDouble()
	{
	// Update status flag
	LeftButtonDown=false;
	// Set ignore flag for next left up
	LeftDoubleIgnoreUp=true;
	// Evaluate left double by drag status (can only be C4MC_Drag_None really)
	switch (Drag)
		{
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4MC_Drag_None:
			// Double left click (might be on a target)
			break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		}
	}

void C4MouseControl::RightDown()
	{
	// Update status flag
	RightButtonDown=true;
	// Store down values (same MoveLeftDown -> use StoreDown)
	DownX=GameX; DownY=GameY;
	DownCursor=Cursor;
	DownTarget=TargetObject;
	DownRegion.Default();
	if (TargetRegion)
		{
		DownRegion=(*TargetRegion);
		DownTarget=TargetRegion->Target;
		DownOffsetX=TargetRegion->X-GuiX; DownOffsetY=TargetRegion->Y-GuiY;
		}
	}

void C4MouseControl::RightUp()
	{
	// Update status flag
	RightButtonDown=false;
	// Evaluate by drag status
	switch (Drag)
		{
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4MC_Drag_None: RightUpDragNone(); break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4MC_Drag_Selecting:	ButtonUpDragSelecting(); break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4MC_Drag_Moving: ButtonUpDragMoving(); break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4MC_Drag_Construct: ButtonUpDragConstruct(); break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4MC_Drag_Script: ButtonUpDragScript(); break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		}
	}

void C4MouseControl::Wheel(DWORD dwFlags)
	{
	short iDelta = (short)(dwFlags >> 16);

	// Normal wheel: control zoom
	if(!ControlDown)
		{
		//if(iDelta > 0) Viewport->ChangeZoom(C4GFX_ZoomStep);
		//if(iDelta < 0) Viewport->ChangeZoom(1.0f/C4GFX_ZoomStep);
		}
	// Ctrl + Wheel: pass to player control (might be used for inventory or such)
	else
		{
		// 2do
		//if(iDelta > 0) Game.LocalPlayerControl(Player, COM_WheelUp);
		//if(iDelta < 0) Game.LocalPlayerControl(Player, COM_WheelDown);
		}
	}

bool C4MouseControl::IsValidMenu(C4Menu *pMenu)
	{
	// Local control fullscreen menu
	if (pMenu == FullScreen.pMenu)
		if (pMenu->IsActive())
			return true;
	// Local control player menu
	C4Player *pPlr;
	for (int32_t cnt=0; pPlr=::Players.Get(cnt); cnt++)
		if (pMenu == &(pPlr->Menu))
			if (pMenu->IsActive())
				return true;
	// No match found
	return false;
	}

bool C4MouseControl::SendControl(int32_t iCom, int32_t iData)
	{
	// Help
	if (iCom==COM_Help)
		{
		Help=true;
		return true;
		}
	// Activate player menu / fullscreen main menu (local control)
	if (iCom==COM_PlayerMenu)
		{
		if (IsPassive() && FullScreen.Active)
			FullScreen.ActivateMenuMain();
		else
			pPlayer->ActivateMenuMain();
		return true;
		}
	// Open chat
	if (iCom==COM_Chat)
		{
		C4ChatDlg::ShowChat();
		return true;
		}
	// other controls not valid in passive mode
	if (IsPassive()) return false;
	// Player control queue
	Game.Input.Add(CID_PlrControl, new C4ControlPlayerControl(Player,iCom,iData));
	// Done
	return true;
	}

void C4MouseControl::CreateDragImage(C4ID id, C4Object *obj, bool fPicture)
	{
	// todo: real object drawing
	if (obj) id = obj->id;
	// Get definition
	C4Def *pDef=C4Id2Def(id); if (!pDef) return;
	// in newgfx, it's just the base image, drawn differently...
	if (pDef->DragImagePicture || fPicture)
		DragImage.Set(pDef->Graphics.GetBitmap(),pDef->PictureRect.x,pDef->PictureRect.y,pDef->PictureRect.Wdt,pDef->PictureRect.Hgt);
	else
		DragImage=pDef->GetMainFace(&pDef->Graphics);
	}

void C4MouseControl::DragConstruct()
	{
	Cursor=C4MC_Cursor_Construct;
	// Check site
	DragImagePhase=1;
	if (!FogOfWar && ConstructionCheck(C4Id2Def(DragID),int32_t(GameX),int32_t(GameY))) DragImagePhase=0;
	}

void C4MouseControl::DragScript()
{
	// script drag should update target and selection so selection highlight on drop target is visible
	// Cursor movement
	UpdateCursorTarget();
	// Update selection
	UpdateSingleSelection();
	Cursor=C4MC_Cursor_DragDrop;
	// todo: Phase to 1 if drag is not possible. Interface to determine valid drag/drop does not exist yet
	DragImagePhase=0;
}

void C4MouseControl::LeftUpDragNone()
	{
	// Single left click (might be on a target)
	switch (Cursor)
		{
		// Region
		case C4MC_Cursor_Region:
			// Help click on region: ignore
			if (Help) break;
			// Region com & data
			SendControl(DownRegion.Com,DownRegion.Data);
			break;
		// Selection
		case C4MC_Cursor_Select:
			// Object selection to control queue
			if (!IsPassive()) Game.Input.Add(CID_PlrSelect, new C4ControlPlayerSelect(Player,Selection,false));
			break;
		// Help
		case C4MC_Cursor_Help:
			if (DownTarget)
				{
				if (DownTarget->Def->GetDesc())
					Caption.Format("%s: %s",DownTarget->GetName(), DownTarget->Def->GetDesc());
				else
					Caption.Copy(DownTarget->GetName());
				KeepCaption=Caption.getLength()/2;
				IsHelpCaption = true;
				}
			break;
		// Nothing
		case C4MC_Cursor_Nothing:
			break;
		// Movement?
		default:
			// done in script
			break;
		}
	// Clear selection
	Selection.Clear();
	}

void C4MouseControl::ButtonUpDragSelecting()
	{
	// Finish drag
	Drag=C4MC_Drag_None;
	}

void C4MouseControl::ButtonUpDragMoving()
	{
	// Finish drag
	Drag=C4MC_Drag_None;
	// Clear selection
	Selection.Clear();
	}

void C4MouseControl::ButtonUpDragConstruct()
	{
	// Finish drag
	Drag=C4MC_Drag_None;
	DragImage.Default();
	// Command
	if (DragImagePhase==0) // if ConstructionCheck was okay (check again?)
		SendCommand(C4CMD_Construct,int32_t(GameX),int32_t(GameY),NULL,NULL,DragID);
	// Clear selection (necessary?)
	Selection.Clear();
	}

void C4MouseControl::ButtonUpDragScript()
{
	// Finish drag
	Drag=C4MC_Drag_None;
	DragImage.Default();
	DragID=C4ID_None;
	// Determine drag+drop targets
	UpdateCursorTarget();
	C4Object *DragObject = this->DragObject;
	this->DragObject = NULL;
	C4Object *DropObject = TargetObject;
	// drag object must exist; drop object is optional
	if (!DragObject) return;
	// no commands if player is eliminated or doesn't exist any more
	C4Player *pPlr = ::Players.Get(Player);
	if (!pPlr || pPlr->Eliminated) return;
	// todo: Perform drag/drop validity check
	// now drag/drop is handled by script
	if (DropObject)
		Game.Input.Add(CID_Script, new C4ControlScript(
		  FormatString("%s(%d,Object(%d),Object(%d))", PSF_MouseDragDrop, (int)Player, (int)(DragObject->Number), (int)(DropObject->Number)).getData()));
	else
		Game.Input.Add(CID_Script, new C4ControlScript(
		  FormatString("%s(%d,Object(%d),nil)", PSF_MouseDragDrop, (int)Player, (int)(DragObject->Number)).getData()));
}

void C4MouseControl::SendCommand(int32_t iCommand, int32_t iX, int32_t iY, C4Object *pTarget, C4Object *pTarget2, int32_t iData, int32_t iAddMode)
	{
	// no commands in passive mode
	if (IsPassive()) return;
	// no commands if player is eliminated or doesn't exist any more
	C4Player *pPlr = ::Players.Get(Player);
	if (!pPlr || pPlr->Eliminated) return;
	// User add multiple command mode
	if (ShiftDown) iAddMode|=C4P_Command_Append;
	// Command to control queue
	Game.Input.Add(CID_PlrCommand, new C4ControlPlayerCommand(Player,iCommand,iX,iY,pTarget,pTarget2,iData,iAddMode));
	}

void C4MouseControl::RightUpDragNone()
	{

	// Region: send control
	if (Cursor==C4MC_Cursor_Region)
		{ SendControl(DownRegion.RightCom); return; }

	// Alternative object selection
	if (Cursor==C4MC_Cursor_Select && !IsPassive())
		{ Game.Input.Add(CID_PlrSelect, new C4ControlPlayerSelect(Player,Selection,true)); }

	// Help: end
	if (Help)
		{ Help=false; KeepCaption=0; return; }

	// TODO: Evaluate right click

	}

void C4MouseControl::UpdateFogOfWar()
	{
	// Assume no fog of war
	FogOfWar=false;
	// Check for fog of war
	if ((pPlayer->fFogOfWar && !pPlayer->FoWIsVisible(int32_t(GameX),int32_t(GameY))) || GameX<0 || GameY<0 || int32_t(GameX)>=GBackWdt || int32_t(GameY)>=GBackHgt)
		{
		FogOfWar=true;
		// allow dragging, scrolling, region selection and manipulations of objects not affected by FoW
		if (!TargetRegion && !Scrolling && (!TargetObject || !(TargetObject->Category & C4D_IgnoreFoW)))
			{
			Cursor=C4MC_Cursor_Nothing;
			ShowPointX=ShowPointY=-1;
			// dragging will reset the cursor
			}
		}
	}

void C4MouseControl::ShowCursor()
	{
	Visible=true;
	}

void C4MouseControl::HideCursor()
	{
	Visible=false;
	}

const char *C4MouseControl::GetCaption()
	{
	return Caption.getData();
	}

C4Object *C4MouseControl::GetTargetObject()
{
	// find object
	// gui object position currently wrong...will fall apart once GUIZoom is activated
	C4Object *pObj = Game.FindVisObject(ViewX, ViewY, Player, fctViewportGame, fctViewportGUI, GameX,GameY, Help ? C4D_All : C4D_MouseSelect, GuiX-fctViewportGUI.X, GuiY-fctViewportGUI.Y);
	if (!pObj) return NULL;
	return pObj;
}

bool C4MouseControl::IsPassive()
	{
	return ::Control.isReplay() || Player<=NO_OWNER;
	}

void C4MouseControl::ScrollView(int32_t iX, int32_t iY, int32_t ViewWdt, int32_t ViewHgt)
	{
	// player assigned: scroll player view
	if (pPlayer)
		pPlayer->ScrollView(iX, iY, ViewWdt, ViewHgt);
	else if (Viewport)
		{
		// no player: Scroll fullscreen viewport
		Viewport->ViewX = Viewport->ViewX+iX;
		Viewport->ViewY = Viewport->ViewY+iY;
		Viewport->UpdateViewPosition();
		}

	}

bool C4MouseControl::IsDragging()
	{
	// no selection drag; return true for object drag only
	return Active && (Drag == C4MC_Drag_Moving || Drag == C4MC_Drag_Construct);
	}

void C4MouseControl::StartConstructionDrag(C4ID id)
	{
	Drag=C4MC_Drag_Construct;
	DragID=id;
	CreateDragImage(DragID,NULL,false);
	Selection.Clear();
	}
