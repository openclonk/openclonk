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

#ifndef BIG_C4INCLUDE
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
#endif

const int32_t C4MC_Drag_None					= 0,
					C4MC_Drag_Selecting			= 1,
					C4MC_Drag_Moving				= 2,
					//C4MC_Drag_Menu					= 3,
					//C4MC_Drag_MenuScroll		= 4,
					C4MC_Drag_Construct			= 5,

					C4MC_Selecting_Unknown	= 0,
					C4MC_Selecting_Crew			= 1,
					C4MC_Selecting_Objects	= 2;

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
				// Evaluate single right click: select next crew
				if (Drag==C4MC_Drag_None)
					SendPlayerSelectNext();
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
		lpDDraw->SetZoom(GameZoom);
		Selection.DrawSelectMark(cgo, 1.0f);
		lpDDraw->SetZoom(GuiZoom);
		}

	// Draw control
	switch (Drag)
		{
		//------------------------------------------------------------------------------------------
		case C4MC_Drag_None: case C4MC_Drag_Moving: case C4MC_Drag_Construct:
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
				// drag image in game zoom
				// draw rounded to game pixels, because construction site will be placed at rounded game pixel positions
				lpDDraw->SetZoom(GameZoom);
				// draw in special modulation mode
				lpDDraw->SetBlitMode(C4GFXBLIT_MOD2);
				// draw DragImage in red or green, according to the phase to be used
				iOffsetX=DragImage.Wdt/2; iOffsetY=DragImage.Hgt;
				lpDDraw->ActivateBlitModulation(DragImagePhase ? 0x8f7f0000 : 0x1f007f00);
				lpDDraw->Blit(DragImage.Surface,
				              float(DragImage.X), float(DragImage.Y), float(DragImage.Wdt), float(DragImage.Hgt),
				              cgo.Surface,
				              floor(GameX) + cgo.X - iOffsetX, floor(GameY) + cgo.Y - iOffsetY, float(DragImage.Wdt), float(DragImage.Hgt),true);
				// reset color
				lpDDraw->DeactivateBlitModulation();
				lpDDraw->SetBlitMode(0);
				lpDDraw->SetZoom(GuiZoom);
				}
			// Cursor
			else
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
  DWORD ocf=OCF_Grab | OCF_Chop | OCF_Container | OCF_Construct | OCF_Living | OCF_Carryable | OCF_Container | OCF_Exclusive;
	if (Help) ocf|=OCF_All;
  TargetObject=GetTargetObject(GameX,GameY,ocf);
	if (TargetObject && FogOfWar && !(TargetObject->Category & C4D_IgnoreFoW)) TargetObject = NULL;

  // Movement
  if (!FogOfWar && !IsPassive()) Cursor=C4MC_Cursor_Crosshair;

	// Dig
	if (!FogOfWar && GBackSolid(int32_t(GameX),int32_t(GameY)) && !IsPassive())
		{
		Cursor=C4MC_Cursor_Dig;
		if (ControlDown) Cursor=C4MC_Cursor_DigMaterial;
		}

	// Target action
  if (TargetObject && !IsPassive())
    {
		// default cursor for object; also set if not in FoW
		Cursor=C4MC_Cursor_Crosshair;
		// get position
		float iObjX, iObjY; TargetObject->GetViewPos(iObjX, iObjY, ViewX, ViewY, fctViewport);
		// Enter (containers)
    if (ocf & OCF_Container)
			if (TargetObject->OCF & OCF_Entrance)
				Cursor=C4MC_Cursor_Enter;
		// Grab / Ungrab
    if (ocf & OCF_Grab)
			{
			Cursor=C4MC_Cursor_Grab;
			if (pPlrCursor)
				if (pPlrCursor->GetProcedure()==DFA_PUSH)
					if (pPlrCursor->Action.Target==TargetObject)
						Cursor=C4MC_Cursor_Ungrab;
			}
		// Object
		if (ocf & OCF_Carryable)
			{
			Cursor=C4MC_Cursor_Object;
			if (ocf & OCF_InSolid) Cursor=C4MC_Cursor_DigObject;
			}
		// Chop (reduced range)
    if (ocf & OCF_Chop)
			if (Inside<float>(GameX-iObjX,-float(TargetObject->Shape.Wdt)/3,+float(TargetObject->Shape.Wdt)/3))
				if (Inside<float>(GameY-iObjY,-float(TargetObject->Shape.Wdt)/2,+float(TargetObject->Shape.Wdt)/3))
					Cursor=C4MC_Cursor_Chop;
		// Enter
    if (ocf & OCF_Entrance)
			Cursor=C4MC_Cursor_Enter;
		// Build
    if (ocf & OCF_Construct) Cursor=C4MC_Cursor_Build;
		// Select
    if (ocf & OCF_Alive)
      if (ValidPlr(Player))
        if (::Players.Get(Player)->ObjectInCrew(TargetObject))
          Cursor=C4MC_Cursor_Select;
		// select custom region
		if (TargetObject->Category & C4D_MouseSelect)
			Cursor=C4MC_Cursor_Select;
		// Attack
		if (ocf & OCF_Alive)
			if (TargetObject->GetAlive())
				if (Hostile(Player,TargetObject->Owner))
					Cursor=C4MC_Cursor_Attack;
		}

	// Jump - no parallaxity regarded here...
	if (pPlrCursor)
		if (!pPlrCursor->Contained)
			if (pPlrCursor->GetProcedure()==DFA_WALK)
				if (Inside<float>(GameY-pPlrCursor->GetY(),-25.0f,-10.0f))
					{
					if (Inside<float>(GameX-pPlrCursor->GetX(),-15,-1)) Cursor=C4MC_Cursor_JumpLeft;
					if (Inside<float>(GameX-pPlrCursor->GetX(),+1,+15)) Cursor=C4MC_Cursor_JumpRight;
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
			C4Def *pDef;
			if (TargetObject) szName=TargetObject->GetName();
			// Target caption by cursor
			switch (Cursor)
				{
				case C4MC_Cursor_Select: idCaption="IDS_CON_SELECT"; break;
				case C4MC_Cursor_JumpLeft: case C4MC_Cursor_JumpRight: idCaption="IDS_CON_JUMP"; break;
				case C4MC_Cursor_Grab: idCaption="IDS_CON_GRAB"; fDouble=true; break;
				case C4MC_Cursor_Ungrab: idCaption="IDS_CON_UNGRAB"; fDouble=true; break;
				case C4MC_Cursor_Build: idCaption="IDS_CON_BUILD"; fDouble=true; break;
				case C4MC_Cursor_Chop: idCaption="IDS_CON_CHOP"; fDouble=true; break;
				case C4MC_Cursor_Object: idCaption="IDS_CON_COLLECT"; fDouble=true; break;
				case C4MC_Cursor_DigObject: idCaption="IDS_CON_DIGOUT"; fDouble=true; break;
				case C4MC_Cursor_Enter: idCaption="IDS_CON_ENTER"; fDouble=true; break;
				case C4MC_Cursor_Attack: idCaption="IDS_CON_ATTACK"; fDouble=true; break;
				case C4MC_Cursor_Help: idCaption="IDS_CON_NAME"; break;
				case C4MC_Cursor_DigMaterial:
					{
					int32_t iMat = GBackMat(int32_t(GameX),int32_t(GameY));
					if (MatValid(iMat))
						if (pDef=C4Id2Def(::MaterialMap.Map[iMat].Dig2Object))
							{ idCaption="IDS_CON_DIGOUT"; fDouble=true; szName=pDef->GetName(); }
					}
					break;
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

int32_t C4MouseControl::UpdateCrewSelection()
	{
	Selection.Clear();
	// Add all active crew objects in drag frame to Selection
  C4Object *cObj; C4ObjectLink *cLnk;
  for (cLnk=pPlayer->Crew.First; cLnk && (cObj=cLnk->Obj); cLnk=cLnk->Next)
		if (!cObj->CrewDisabled)
			{
			float iObjX, iObjY; cObj->GetViewPos(iObjX, iObjY, ViewX, ViewY, fctViewport);
			if (Inside<float>(iObjX,Min<float>(GameX,DownX),Max(GameX,DownX)))
				if (Inside<float>(iObjY,Min<float>(GameY,DownY),Max(GameY,DownY)))
					Selection.Add(cObj, C4ObjectList::stNone);
			}
	return Selection.ObjectCount();
	}

int32_t C4MouseControl::UpdateObjectSelection()
	{
	Selection.Clear();
	// Add all collectible objects in drag frame to Selection
  C4Object *cObj; C4ObjectLink *cLnk;
  for (cLnk=::Objects.First; cLnk && (cObj=cLnk->Obj); cLnk=cLnk->Next)
		if (cObj->Status)
			if (cObj->OCF & OCF_Carryable)
				if (!cObj->Contained)
					{
					float iObjX, iObjY; cObj->GetViewPos(iObjX, iObjY, ViewX, ViewY, fctViewport);
					if (Inside<float>(iObjX,Min<float>(GameX,DownX),Max<float>(GameX,DownX)))
						if (Inside<float>(iObjY,Min<float>(GameY,DownY),Max<float>(GameY,DownY)))
							{
							Selection.Add(cObj, C4ObjectList::stNone);
							if (Selection.ObjectCount()>=20) break; // max. 20 objects
							}
					}
	return Selection.ObjectCount();
	}

int32_t C4MouseControl::UpdateSingleSelection()
	{

	// Set single crew selection if cursor on crew (clear prior object selection)
	if (TargetObject && (Cursor==C4MC_Cursor_Select))
		{	Selection.Clear(); Selection.Add(TargetObject, C4ObjectList::stNone);	}

	// Cursor has moved off single crew (or target object) selection: clear selection
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

bool C4MouseControl::UpdatePutTarget(bool fVehicle)
	{

	// Target object
  DWORD ocf=OCF_Container;
	StdStrBuf sName;
  if (TargetObject=GetTargetObject(GameX,GameY,ocf))
		{
		// Cursor
		if (fVehicle) Cursor=C4MC_Cursor_VehiclePut;
		else Cursor=C4MC_Cursor_Put;
		// Caption
		if (Selection.GetObject())
			{
			if (Selection.ObjectCount()>1)
				// Multiple object name
				sName.Format("%d %s",Selection.ObjectCount(),LoadResStr(fVehicle ? "IDS_CON_VEHICLES" : "IDS_CON_ITEMS") );
			else
				// Single object name
				sName.Ref(Selection.GetObject()->GetName());
			// Set caption
			Caption.Format(LoadResStr(fVehicle ? "IDS_CON_VEHICLEPUT" : "IDS_CON_PUT"),sName.getData(),TargetObject->GetName());
			IsHelpCaption = false;
			}
		// Put target found
		return true;
		}

	return false;
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

		// Send Com on mouse button down when using AutoStopControl to send
		// corresponding release event in LeftUpDragNone. Only do this for normal
		// coms, single and double coms are handled in LeftUpDragNone.
		if(!Help && pPlayer && pPlayer->OldPrefControlStyle && (DownRegion.Com & (COM_Double | COM_Single)) == 0)
			SendControl(DownRegion.Com, DownRegion.Data);
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
			if (UpdateCrewSelection()) { DragSelecting=C4MC_Selecting_Crew; break; }
			if (UpdateObjectSelection()) { DragSelecting=C4MC_Selecting_Objects; break; }
			break;
		case C4MC_Selecting_Crew:
			// Select crew
			UpdateCrewSelection();
			break;
		case C4MC_Selecting_Objects:
			// Select objects
			UpdateObjectSelection();
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
		}
	}

void C4MouseControl::DragMoving()
	{
	ShowPointX=ShowPointY=-1;

	// do not drag objects into FoW
	if (FogOfWar) { Cursor = C4MC_Cursor_Nothing; return; }

	// Carryable object
	if (Selection.GetObject() && (Selection.GetObject()->OCF & OCF_Carryable))
		{
		// Default object cursor
		Cursor = C4MC_Cursor_Object;
		// Check for put target
		if (ControlDown)
			if (UpdatePutTarget(false))
				return;
		// In liquid: drop
		if (GBackLiquid(int32_t(GameX),int32_t(GameY)))
			{ Cursor = C4MC_Cursor_Drop; return; }
		// In free: drop or throw
		if (!GBackSolid(int32_t(GameX),int32_t(GameY)))
			{
			// Check drop
			int32_t iX=int32_t(GameX),iY=int32_t(GameY);
			while ((iY<GBackHgt) && !GBackSolid(iX,iY)) iY++;
			if (Inside<int32_t>(int32_t(GameX)-iX,-5,+5) && Inside<int32_t>(int32_t(GameY)-iY,-5,+5))
				{ Cursor = C4MC_Cursor_Drop; return; }
			// Throwing physical
			FIXED fixThrow = ValByPhysical(400, 50000);
			if (pPlayer->Cursor) fixThrow=ValByPhysical(400, pPlayer->Cursor->GetPhysical()->Throw);
			// Preferred throwing direction
			int32_t iDir=+1; if (pPlayer->Cursor) if (pPlayer->Cursor->GetX() > int32_t(GameX)) iDir=-1;
			// Throwing height
			int32_t iHeight=20; if (pPlayer->Cursor) iHeight=pPlayer->Cursor->Shape.Hgt;
			// Check throw
			if (FindThrowingPosition(int32_t(GameX),int32_t(GameY),fixThrow*iDir,-fixThrow,iHeight,iX,iY)
			 || FindThrowingPosition(int32_t(GameX),int32_t(GameY),fixThrow*(iDir*=-1),-fixThrow,iHeight,iX,iY))
				{
				Cursor = (iDir==-1) ? C4MC_Cursor_ThrowLeft : C4MC_Cursor_ThrowRight;
				ShowPointX=iX; ShowPointY=iY;
				return;
				}
			}
		}

	// Vehicle
	else
		{
		// PushTo
		Cursor = C4MC_Cursor_Vehicle;
		// Check for put target
		if (ControlDown)
			UpdatePutTarget(true);
		}

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

	// Button down: begin drag
	if ( (LeftButtonDown || RightButtonDown)
		&& ((Abs(GameX-DownX)>C4MC_DragSensitivity) || (Abs(GameY-DownY)>C4MC_DragSensitivity)) )
		{
		// don't begin dragging from FoW; unless it's a menu
		if (FogOfWar && DownCursor != C4MC_Cursor_Region) return;
		switch (DownCursor)
			{
			// Drag start selecting in landscape
			case C4MC_Cursor_Crosshair: case C4MC_Cursor_Dig:
				Selection.Clear();
				Drag=C4MC_Drag_Selecting; DragSelecting=C4MC_Selecting_Unknown;
				break;
			// Drag object from landscape
			case C4MC_Cursor_Object: case C4MC_Cursor_DigObject:
				if (DownTarget)
					{
					Drag=C4MC_Drag_Moving;
					// Down target is not part of selection: drag single object
					if (!Selection.GetLink(DownTarget))
						{	Selection.Clear(); Selection.Add(DownTarget, C4ObjectList::stNone); }
					}
				break;
			// Drag vehicle from landscape
			case C4MC_Cursor_Grab: case C4MC_Cursor_Ungrab:
				if (DownTarget)
					if (DownTarget->Def->Grab == 1)
						{ Drag=C4MC_Drag_Moving; Selection.Clear(); Selection.Add(DownTarget, C4ObjectList::stNone); }
				break;
			// Drag from region
			case C4MC_Cursor_Region:
				// Drag object(s) or vehicle(s)
				if ( DownRegion.Target
					&& ((DownRegion.Target->OCF & OCF_Carryable) || (DownRegion.Target->Def->Grab==1)) && !FogOfWar)
						{
						Drag=C4MC_Drag_Moving;
						Selection.Clear();
						// Multiple object selection from container
						if (RightButtonDown && DownRegion.Target->Contained && (DownRegion.Target->Contained->Contents.ObjectCount(DownRegion.Target->id)>1) )
							{
							for (C4ObjectLink *cLnk=DownRegion.Target->Contained->Contents.First; cLnk && cLnk->Obj; cLnk=cLnk->Next)
								if (cLnk->Obj->id==DownRegion.Target->id)
									Selection.Add(cLnk->Obj, C4ObjectList::stNone);
							}
						// Single object selection
						else
							Selection.Add(DownRegion.Target, C4ObjectList::stNone);
						break;
						}
				// Drag id (construction)
				C4Def *pDef;
				if (DownRegion.id)
					if ((pDef=C4Id2Def(DownRegion.id)) && pDef->Constructable)
						{ StartConstructionDrag(DownRegion.id); break; }
				break;
			// Help: no dragging
			case C4MC_Cursor_Help:
				break;
			}
		}

	// Cursor movement
	UpdateCursorTarget();
	// Update selection
	UpdateSingleSelection();

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
			switch (Cursor)
				{
				case C4MC_Cursor_Attack: SendCommand(C4CMD_Attack,int32_t(GameX),int32_t(GameY),TargetObject); break;
				case C4MC_Cursor_Grab: SendCommand(C4CMD_Grab,0,0,TargetObject); break; // grab at zero-offset!
				case C4MC_Cursor_Ungrab: SendCommand(C4CMD_UnGrab,int32_t(GameX),int32_t(GameY),TargetObject); break;
				case C4MC_Cursor_Build: SendCommand(C4CMD_Build,int32_t(GameX),int32_t(GameY),TargetObject); break;
				case C4MC_Cursor_Chop: SendCommand(C4CMD_Chop,int32_t(GameX),int32_t(GameY),TargetObject); break;
				case C4MC_Cursor_Enter: SendCommand(C4CMD_Enter,int32_t(GameX),int32_t(GameY),TargetObject); break;
				case C4MC_Cursor_Object: case C4MC_Cursor_DigObject: SendCommand(C4CMD_Get,0,0,TargetObject); break;
				case C4MC_Cursor_Dig: SendCommand(C4CMD_Dig,int32_t(GameX),int32_t(GameY),NULL); break;
				case C4MC_Cursor_DigMaterial: SendCommand(C4CMD_Dig,int32_t(GameX),int32_t(GameY),NULL,NULL,true); break;
				}
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
		}
	}

void C4MouseControl::Wheel(DWORD dwFlags)
	{
	short iDelta = (short)(dwFlags >> 16);

	// Normal wheel: control zoom
	if(!ControlDown)
		{
		if(iDelta > 0) Viewport->ChangeZoom(C4GFX_ZoomStep);
		if(iDelta < 0) Viewport->ChangeZoom(1.0f/C4GFX_ZoomStep);
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
	Game.Input.Add(CID_PlrControl,
    new C4ControlPlayerControl(Player,iCom,iData));
	// Done
	return true;
	}

void C4MouseControl::CreateDragImage(C4ID id)
	{
	// Get definition
	C4Def *pDef=C4Id2Def(id); if (!pDef) return;
	// in newgfx, it's just the base image, drawn differently...
	if (pDef->DragImagePicture)
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

void C4MouseControl::LeftUpDragNone()
	{
	// Player pressed down a region button and uses AutoStopControl. Send now
	// a corresponding release event, not caring about where the cursor may have
	// been moved.
	if(DownCursor == C4MC_Cursor_Region && !Help && pPlayer && pPlayer->OldPrefControlStyle && (DownRegion.Com & (COM_Double | COM_Single)) == 0)
	{
		// + 16 for release, there is no COM_Release to be |ed...
	  SendControl(DownRegion.Com + 16, DownRegion.Data);
		return;
	}

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
			// Crew selection to control queue
			if (!IsPassive()) Game.Input.Add(CID_PlrSelect,
        new C4ControlPlayerSelect(Player,Selection));
			break;
		// Jump
		case C4MC_Cursor_JumpLeft: case C4MC_Cursor_JumpRight:
			SendCommand(C4CMD_Jump,int32_t(GameX),int32_t(GameY),NULL);
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
		// Movement
		default:
			// MoveTo command to control queue
			SendCommand(C4CMD_MoveTo,int32_t(GameX),int32_t(GameY),NULL);
			break;
		}
	// Clear selection
	Selection.Clear();
	}

void C4MouseControl::ButtonUpDragSelecting()
	{
	// Finish drag
	Drag=C4MC_Drag_None;
	// Crew selection to control queue
	if (DragSelecting==C4MC_Selecting_Crew)
		{
		Game.Input.Add(CID_PlrSelect,
      new C4ControlPlayerSelect(Player,Selection));
		Selection.Clear();
		}
	// Object selection: just keep selection
	if (DragSelecting==C4MC_Selecting_Objects)
		{

		}
	}

void C4MouseControl::ButtonUpDragMoving()
	{
	// Finish drag
	Drag=C4MC_Drag_None;
	// Evaluate to command by cursor for each selected object
	C4ObjectLink *pLnk; C4Object *pObj;
	int32_t iCommand; C4Object *pTarget1,*pTarget2;
	int32_t iAddMode; iAddMode=C4P_Command_Set;
	int32_t iX = int32_t(GameX), iY = int32_t(GameY);
	for (pLnk=Selection.First; pLnk && (pObj=pLnk->Obj); pLnk=pLnk->Next)
		{
		iCommand=C4CMD_None; pTarget1=pTarget2=NULL;
		switch (Cursor)
			{
			case C4MC_Cursor_ThrowLeft: case C4MC_Cursor_ThrowRight:
				iCommand=C4CMD_Throw; pTarget1=pObj; ShowPointX=ShowPointY=-1; break;
			case C4MC_Cursor_Drop:
				iCommand=C4CMD_Drop; pTarget1=pObj; break;
			case C4MC_Cursor_Put:
				iCommand=C4CMD_Put; pTarget1=TargetObject; iX=0; iY=0; pTarget2=pObj; break;
			case C4MC_Cursor_Vehicle:
				iCommand=C4CMD_PushTo; pTarget1=pObj; break;
			case C4MC_Cursor_VehiclePut:
				iCommand=C4CMD_PushTo; pTarget1=pObj; pTarget2=TargetObject; break;
			}
		// Set first command, append all following
		SendCommand(iCommand,iX,iY,pTarget1,pTarget2,0,iAddMode);
		iAddMode=C4P_Command_Append;
		}
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
	Game.Input.Add(CID_PlrCommand,
    new C4ControlPlayerCommand(Player,iCommand,iX,iY,pTarget,pTarget2,iData,iAddMode));
	}

void C4MouseControl::RightUpDragNone()
	{

	// Region: send control
	if (Cursor==C4MC_Cursor_Region)
		{ SendControl(DownRegion.RightCom); return; }

	// Help: end
	if (Help)
		{ Help=false; KeepCaption=0; return; }

	// Selection: send selection (not exclusive)
	if (Cursor==C4MC_Cursor_Select)
	  Game.Input.Add(CID_PlrSelect,
      new C4ControlPlayerSelect(Player,Selection));

	// Check for any secondary context target objects
	DWORD ocf=OCF_All;
	if (!TargetObject) TargetObject=GetTargetObject(GameX,GameY,ocf);
	// Avoid stinkin' Windrad - cheaper goes it not
	if (TargetObject && (TargetObject->id==C4Id("WWNG"))) TargetObject=GetTargetObject(GameX,GameY,ocf,TargetObject);

	// Target object: context menu
	if (TargetObject)
		{
		SendCommand(C4CMD_Context,int32_t(GameX-Viewport->ViewX),int32_t(GameY-Viewport->ViewY),NULL,TargetObject,0,C4P_Command_Add);
		return;
		}

	// Free click: select next clonk
	SendPlayerSelectNext();

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

void C4MouseControl::SendPlayerSelectNext()
	{
	C4ObjectLink *cLnk;
	if (cLnk=pPlayer->Crew.GetLink(pPlayer->Cursor))
		for (cLnk=cLnk->Next; cLnk; cLnk=cLnk->Next)
			if (cLnk->Obj->Status && !cLnk->Obj->CrewDisabled) break;
	if (!cLnk)
		for (cLnk=pPlayer->Crew.First; cLnk; cLnk=cLnk->Next)
			if (cLnk->Obj->Status && !cLnk->Obj->CrewDisabled) break;
	if (cLnk)
		{
		// Crew selection to control queue
		Selection.Clear(); Selection.Add(cLnk->Obj, C4ObjectList::stNone);
	  Game.Input.Add(CID_PlrSelect,
      new C4ControlPlayerSelect(Player,Selection));
		Selection.Clear();
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


C4Object *C4MouseControl::GetTargetObject(float iX, float iY, DWORD &dwOCF, C4Object *pExclude)
	{
	// find object
	C4Object *pObj = Game.FindVisObject(ViewX, ViewY, Player, fctViewport, iX,iY,0,0, dwOCF, pExclude);
	if (!pObj) return NULL;
	// adjust OCF
	pObj->GetOCFForPos(iX, iY, dwOCF);
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
	CreateDragImage(DragID);
	Selection.Clear();
	}

C4MouseControl MouseControl;
