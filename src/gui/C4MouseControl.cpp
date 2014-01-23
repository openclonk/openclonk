/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2013, The OpenClonk Team and contributors
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

/* Mouse input */

#include <C4Include.h>
#include <C4MouseControl.h>

#include <C4DefList.h>
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

const int32_t C4MC_Drag_None            = 0,
              C4MC_Drag_Script          = 6,
              C4MC_Drag_Unhandled       = 7;

const int32_t C4MC_Cursor_Select      = 0,		// click cursor to select/click stuff in the GUI
              C4MC_Cursor_Crosshair   = 1,		// standard ingame cursor
			  C4MC_Cursor_DragDrop    = 3,		// cursor when drag&dropping
              C4MC_Cursor_Up          = 4,		// cursors for scrolling the viewport ...
              C4MC_Cursor_Down        = 5,		// ...
              C4MC_Cursor_Left        = 6,
              C4MC_Cursor_Right       = 7,
              C4MC_Cursor_UpLeft      = 8,
              C4MC_Cursor_UpRight     = 9,
              C4MC_Cursor_DownLeft    = 10,
              C4MC_Cursor_DownRight   = 11,
			  C4MC_Cursor_Passive     = 12;		// passive cursor in records and an fog of war

const int32_t C4MC_Time_on_Target     = 10;

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
	Cursor=0;
	Caption.Clear();
	CaptionBottomY=0;
	VpX=VpY=0;
	DownX=DownY=0;
	ViewX=ViewY=0;
	GuiX=GuiY=GameX=GameY=0;
	LeftButtonDown=RightButtonDown=false;
	LeftDoubleIgnoreUp=false;
	ButtonDownOnSelection=false;
	Visible=true;
	InitCentered=false;
	FogOfWar=false;
	DragID=C4ID::None;
	DragObject=NULL;
	KeepCaption=0;
	Drag=C4MC_Drag_None;
	Selection.Default();
	TargetObject=DownTarget=NULL;
	TimeOnTargetObject=0;
	ControlDown=false;
	ShiftDown=false;
	AltDown=false;
	Scrolling=false;
	ScrollSpeed=10;
	DragImageDef=NULL;
	DragImageObject=NULL;
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
		if (ControlDown) wKeyState|=MK_CONTROL;
		if (ShiftDown) wKeyState|=MK_SHIFT;
		Move(C4MC_Button_None, VpX, VpY, wKeyState);
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
		DragImageDef=NULL;
		DragImageObject=NULL;
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
#ifdef USE_WIN32_WINDOWS
	// fullscreen only
	if (Application.isEditor) return;
	// application or mouse control not active? remove any clips
	if (!Active || !Application.Active || ::pGUI->HasMouseFocus()) { ClipCursor(NULL); return; }
	// get controlled viewport
	C4Viewport *pVP=::Viewports.GetViewport(Player);
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
	::pGUI->SetPreferredDlgRect(C4Rect(pVP->OutX, pVP->OutY, pVP->ViewWdt, pVP->ViewHgt));
#endif
	//StdWindow manages this.
}

void C4MouseControl::Move(int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyFlags, bool fCenter)
{
	// Control state
	ControlDown=false; if (dwKeyFlags & MK_CONTROL) ControlDown=true;
	ShiftDown=false; if (dwKeyFlags & MK_SHIFT) ShiftDown=true;
	AltDown=false; if(dwKeyFlags & MK_ALT) AltDown=true;
	// Active
	if (!Active || !fMouseOwned) return;
	// Execute caption
	if (KeepCaption) KeepCaption--; else { Caption.Clear(); CaptionBottomY=0; }
	// Check player
	if (Player>NO_OWNER)
	{
		pPlayer=::Players.Get(Player);
		if (!pPlayer) { Active=false; return; }
	}
	else
		pPlayer = NULL;
	// Check viewport
	if (!(Viewport=::Viewports.GetViewport(Player))) return;
	// get view position
	C4Rect rcViewport = Viewport->GetOutputRect();
	fctViewport.Set(NULL, rcViewport.x, rcViewport.y, rcViewport.Wdt, rcViewport.Hgt);
	ViewX=Viewport->ViewX; ViewY=Viewport->ViewY;
	fctViewportGame = Viewport->last_game_draw_cgo;
	fctViewportGUI = Viewport->last_gui_draw_cgo;
	// First time viewport attachment: center mouse
#ifdef USE_WIN32_WINDOWS
	if (!InitCentered || fCenter)
	{
		iX = Viewport->ViewWdt/2;
		iY = Viewport->ViewHgt/2;
		if (!Application.isEditor)
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
		if (!Application.isEditor)
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
			GuiX=float(VpX)/Viewport->GetGUIZoom(); GuiY=float(VpY)/Viewport->GetGUIZoom();
		}
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
		GuiX=float(VpX)/Viewport->GetGUIZoom(); GuiY=float(VpY)/Viewport->GetGUIZoom();
		// Scrolling
		UpdateScrolling();
		// Fog of war
		UpdateFogOfWar();

		// Blocked by fog of war: evaluate button up, dragging and region controls only
		if (FogOfWar && Drag == C4MC_Drag_None)
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
		case C4MC_Drag_Unhandled: break; // nothing to do
		case C4MC_Drag_None: DragNone(); break;
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
		if (!TargetObject)
			// safety (can't really happen in !IsPassive, but w/e
			if (pPlayer && pPlayer->ControlSet)
			{
				if (pPlayer->ControlSet->IsMouseControlAssigned(iButton))
				{
					int wheel_dir = 0;
					if (iButton == C4MC_Button_Wheel) wheel_dir = (short)(dwKeyFlags >> 16);
					pPlayer->Control.DoMouseInput(0 /* only 1 mouse supported so far */, iButton, GameX, GameY, GuiX, GuiY, (dwKeyFlags & MK_CONTROL) != 0, (dwKeyFlags & MK_SHIFT) != 0, (dwKeyFlags & MK_ALT) != 0, wheel_dir);
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
	pPlayer->Control.DoMouseInput(0 /* only 1 mouse supported so far */, C4MC_Button_None, GameX, GameY, GuiX, GuiY, ControlDown, ShiftDown, AltDown, 0);
}

void C4MouseControl::Draw(C4TargetFacet &cgo, const ZoomData &GameZoom)
{
	int32_t iOffsetX,iOffsetY;

	ZoomData GuiZoom;
	pDraw->GetZoom(&GuiZoom);

	// Hidden
	if (!Visible || !fMouseOwned) return;

	// Draw selection
	if (!IsPassive())
	{
		Selection.DrawSelectMark(cgo);
	}

	// Draw control
	switch (Drag)
	{
		//------------------------------------------------------------------------------------------
	case C4MC_Drag_None: case C4MC_Drag_Script: case C4MC_Drag_Unhandled:
		// Hotspot offset: Usually, hotspot is in center
		iOffsetX = GfxR->fctMouseCursor.Wdt/2;
		iOffsetY = GfxR->fctMouseCursor.Hgt/2;
		// calculate the hotspot for the scrolling cursors
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
		// Drag image
		if (DragImageObject || DragImageDef)
		{
			C4DefGraphics* pGfx;
			if(DragImageObject)
				pGfx = DragImageObject->GetGraphics();
			else
				pGfx = &DragImageDef->Graphics;

			// Determine image boundaries
			float ImageWdt;
			float ImageHgt;
			if (pGfx->Type == C4DefGraphics::TYPE_Bitmap)
			{
				C4Def* Def = (DragImageObject ? DragImageObject->Def : DragImageDef);
				ImageWdt = Def->PictureRect.Wdt;
				ImageHgt = Def->PictureRect.Hgt;
			}
			else
			{
				// Note bounding box is in OGRE coordinate system
				ImageWdt = pGfx->Mesh->GetBoundingBox().y2 - pGfx->Mesh->GetBoundingBox().y1;
				ImageHgt = pGfx->Mesh->GetBoundingBox().z2 - pGfx->Mesh->GetBoundingBox().z1;
			}

			// zoom mode: Drag in GUI or Game depending on source object
			bool fIsGameZoom = true;
			if (Drag == C4MC_Drag_Script && DragObject && (DragObject->Category & C4D_Foreground))
				fIsGameZoom = false;
			// drag image in game zoom
			float XDraw, YDraw, ZoomDraw;
			if (fIsGameZoom)
			{
				pDraw->SetZoom(GameZoom);
				XDraw = GameX; YDraw = GameY;
				ZoomDraw = 1.0f;
			}
			else
			{
				ZoomDraw = std::min(64.0f / ImageWdt, 64.0f / ImageHgt);
				XDraw = GuiX; YDraw = GuiY;
			}

			iOffsetX=int(ZoomDraw*ImageWdt/2);
			iOffsetY=int(ZoomDraw*ImageHgt/2);

			C4TargetFacet ccgo;
			ccgo.Set(cgo.Surface, XDraw + cgo.X - iOffsetX, YDraw + cgo.Y - iOffsetY, float(ImageWdt)*ZoomDraw, float(ImageHgt)*ZoomDraw);

			if (DragImageObject)
			{
				uint32_t ColorMod = DragImageObject->ColorMod;
				uint32_t BlitMode = DragImageObject->BlitMode;
				DragImageObject->ColorMod = (Drag == C4MC_Drag_Script) ? 0x7fffffff : (/*DragImagePhase*/0 ? 0x8f7f0000 : 0x1f007f00);
				DragImageObject->BlitMode = C4GFXBLIT_MOD2;
	
				DragImageObject->DrawPicture(ccgo, false, NULL);

				DragImageObject->ColorMod = ColorMod;
				DragImageObject->BlitMode = BlitMode;
			}
			else
			{
				// draw in special modulation mode
				pDraw->SetBlitMode(C4GFXBLIT_MOD2);
				// draw DragImage in red or green, according to the phase to be used
				pDraw->ActivateBlitModulation((Drag == C4MC_Drag_Script) ? 0x7fffffff : (/*DragImagePhase*/0 ? 0x8f7f0000 : 0x1f007f00));

				DragImageDef->Draw(ccgo, false, pPlayer ? pPlayer->ColorDw : 0xff0000ff, NULL, 0, 0, NULL);

				// reset color
				pDraw->DeactivateBlitModulation();
				pDraw->SetBlitMode(0);
			}

			if (fIsGameZoom) pDraw->SetZoom(GuiZoom);
			// reset cursor hotspot offset for script drawing
			iOffsetX = GfxR->fctMouseCursor.Wdt/2;
			iOffsetY = GfxR->fctMouseCursor.Hgt/2;
		}
		// Cursor
		if ( (!DragImageDef && !DragImageObject) || (Drag == C4MC_Drag_Script))
			GfxR->fctMouseCursor.Draw(cgo.Surface,cgo.X+GuiX-iOffsetX,cgo.Y+GuiY-iOffsetY,Cursor);
		break;
		//------------------------------------------------------------------------------------------
	}

	// Draw caption
	if (Caption && ::pGUI)
	{
		C4TargetFacet cgoTip;
		cgoTip = static_cast<const C4Facet &>(cgo);
		C4GUI::Screen::DrawToolTip(Caption.getData(), cgoTip, cgo.X+GuiX, cgo.Y+GuiY);
	}

}

void C4MouseControl::UpdateCursorTarget()
{
	C4Object* OldTargetObject = TargetObject;

	if (Scrolling)
	{
		// Scrolling: no other target
		TargetObject=NULL;
	}
	else
	{
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

			// select custom region. Can select an object if it does not have the MD_NoClick
			// flag set. If we are currently dragging then selection depends on it being a drop target.
			bool CanSelect;
			if(Drag == C4MC_Drag_Script)
				CanSelect = (TargetObject->GetPropertyInt(P_MouseDrag) & C4MC_MD_DropTarget) != 0;
			else
				CanSelect = (TargetObject->GetPropertyInt(P_MouseDrag) & C4MC_MD_NoClick) == 0;

			if ( (TargetObject->Category & C4D_MouseSelect) && CanSelect)
				Cursor=C4MC_Cursor_Select;
			else
				TargetObject = NULL;
		}

		// passive cursor
		if (IsPassive())
			Cursor=C4MC_Cursor_Passive;

		// Time on target: caption
		if (TargetObject && Cursor == C4MC_Cursor_Select)
		{
			TimeOnTargetObject++;
			if (TimeOnTargetObject>=C4MC_Time_on_Target)
			{
				if (TargetObject->Category & C4D_MouseSelect)
				{
					// set caption
					if (!KeepCaption)
					{
						C4String *tooltip = TargetObject->GetPropertyStr(P_Tooltip);
						if(tooltip) {
							Caption = TargetObject->GetPropertyStr(P_Tooltip)->GetData();
						}
					}
				}
			}
		}
		else
			TimeOnTargetObject=0;
	}

	// Make a script callback if the object being hovered changes
	if(OldTargetObject != TargetObject)
	{
		// TODO: This might put a heavy load on the network, depending on the number of
		// selectable objects around. If it turns out to be a problem we might want to
		// deduce these hover callbacks client-side instead.
		// Or, make sure to send this at most once per control frame.
		Game.Input.Add(CID_PlrMouseMove, C4ControlPlayerMouse::Hover(::Players.Get(Player), TargetObject, OldTargetObject, DragObject));
	}
}

int32_t C4MouseControl::UpdateSingleSelection()
{
	// Set single selection if cursor on selection object (clear prior object selection)
	if (TargetObject && (Cursor==C4MC_Cursor_Select))
		{ Selection.Clear(); Selection.Add(TargetObject, C4ObjectList::stNone); }

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
	// No scrolling if disabled by player
	if (pPlayer) if (pPlayer->IsViewLocked()) return;
	// Scrolling on border
	if (VpX==0)
		{ Cursor=C4MC_Cursor_Left; ScrollView(-ScrollSpeed/Viewport->Zoom,0,Viewport->ViewWdt/Viewport->Zoom,Viewport->ViewHgt/Viewport->Zoom); Scrolling=true; }
	if (VpY==0)
		{ Cursor=C4MC_Cursor_Up; ScrollView(0,-ScrollSpeed/Viewport->Zoom,Viewport->ViewWdt/Viewport->Zoom,Viewport->ViewHgt/Viewport->Zoom); Scrolling=true; }
	if (VpX==Viewport->ViewWdt-1)
		{ Cursor=C4MC_Cursor_Right; ScrollView(+ScrollSpeed/Viewport->Zoom,0,Viewport->ViewWdt/Viewport->Zoom,Viewport->ViewHgt/Viewport->Zoom); Scrolling=true; }
	if (VpY==Viewport->ViewHgt-1)
		{ Cursor=C4MC_Cursor_Down; ScrollView(0,+ScrollSpeed/Viewport->Zoom,Viewport->ViewWdt/Viewport->Zoom,Viewport->ViewHgt/Viewport->Zoom); Scrolling=true; }
	// Set correct cursor
	if ((VpX==0) && (VpY==0)) Cursor=C4MC_Cursor_UpLeft;
	if ((VpX==Viewport->ViewWdt-1) && (VpY==0)) Cursor=C4MC_Cursor_UpRight;
	if ((VpX==0) && (VpY==Viewport->ViewHgt-1)) Cursor=C4MC_Cursor_DownLeft;
	if ((VpX==Viewport->ViewWdt-1) && (VpY==Viewport->ViewHgt-1)) Cursor=C4MC_Cursor_DownRight;
}

void C4MouseControl::LeftDown()
{
	// Set flag
	LeftButtonDown=true;
	// Store down values (same MoveRightDown -> use StoreDown)
	DownX=GameX; DownY=GameY;
	DownTarget=TargetObject;
}

void C4MouseControl::LeftUp()
{
	// Ignore left up after double click
	if (LeftDoubleIgnoreUp)
	{
		LeftDoubleIgnoreUp=false;
	}
	else
	{
		// Evaluate by drag status
		switch (Drag)
		{
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4MC_Drag_Unhandled: // nobreak
		case C4MC_Drag_None: LeftUpDragNone(); break;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case C4MC_Drag_Script: ButtonUpDragScript(); break;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		}
	}
	// Update status flag
	LeftButtonDown=false;
	if(!RightButtonDown) DownTarget = NULL;
}

void C4MouseControl::DragNone()
{
	// Cursor movement
	UpdateCursorTarget();
	// Update selection
	UpdateSingleSelection();

	// Button down: begin drag
	if ( (LeftButtonDown || RightButtonDown)
	     && ((Abs(GameX-DownX)>C4MC_DragSensitivity) || (Abs(GameY-DownY)>C4MC_DragSensitivity)) )
	{
		bool fAllowDrag = true;
		// check if target object allows scripted dragging
		if (fAllowDrag && DownTarget && (!FogOfWar || (DownTarget->Category & C4D_IgnoreFoW)))
		{
			C4Object *drag_image_obj; C4ID drag_image_id;

			// Drag only if MD_SOURCE is set and drag image is present
			if ( (DownTarget->GetPropertyInt(P_MouseDrag) & C4MC_MD_DragSource) &&
			      DownTarget->GetDragImage(&drag_image_obj, &drag_image_id))
			{
				Drag=C4MC_Drag_Script;

				if(drag_image_obj) DragImageObject = drag_image_obj;
				else DragImageDef = C4Id2Def(drag_image_id);

				DragObject = DownTarget;
			}
		}

		// dragging somewhere unhandled - mark drag process so moving over a draggable object won't start a drag
		if (Drag == C4MC_Drag_None)
		{
			Drag=C4MC_Drag_Unhandled;
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
	DownTarget=TargetObject;
}

void C4MouseControl::RightUp()
{
	// Evaluate by drag status
	switch (Drag)
	{
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case C4MC_Drag_Unhandled: // nobreak
	case C4MC_Drag_None: RightUpDragNone(); break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case C4MC_Drag_Script: ButtonUpDragScript(); break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	}
	// Update status flag
	RightButtonDown=false;
	if(!LeftButtonDown) DownTarget = NULL;
}

void C4MouseControl::Wheel(DWORD dwFlags)
{
	//short iDelta = (short)(dwFlags >> 16);

	// Normal wheel: control zoom
	if (!ControlDown)
	{
		//if(iDelta > 0) Viewport->ChangeZoom(C4GFX_ZoomStep);
		//if(iDelta < 0) Viewport->ChangeZoom(1.0f/C4GFX_ZoomStep);
	}
	// Ctrl + Wheel: pass to player control (might be used for inventory or such)
	else
	{
		// TODO
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
	for (int32_t cnt=0; (pPlr=::Players.Get(cnt)); cnt++)
		if (pMenu == &(pPlr->Menu))
			if (pMenu->IsActive())
				return true;
	// No match found
	return false;
}

bool C4MouseControl::SendControl(int32_t iCom, int32_t iData)
{
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

void C4MouseControl::DragScript()
{
	// script drag should update target and selection so selection highlight on drop target is visible
	// Cursor movement
	UpdateCursorTarget();
	// Update selection
	UpdateSingleSelection();
	Cursor=C4MC_Cursor_DragDrop;
}

void C4MouseControl::LeftUpDragNone()
{
	// might be in Drag_Unknown
	Drag = C4MC_Drag_None;
	// Single left click (might be on a target)
	switch (Cursor)
	{
	case C4MC_Cursor_Select:
		// Object selection to control queue
		if (!IsPassive() && Selection.GetObject() == DownTarget)
			Game.Input.Add(CID_PlrSelect, new C4ControlPlayerSelect(Player,Selection,false));
		break;
	default:
		// done in script
		break;
	}
	// Clear selection
	Selection.Clear();
}

void C4MouseControl::ButtonUpDragScript()
{
	// Determine drag+drop targets
	UpdateCursorTarget();
	// Finish drag
	Drag=C4MC_Drag_None;
	DragID=C4ID::None;
	DragImageObject = NULL;
	DragImageDef = NULL;
	C4Object *DragObject = this->DragObject;
	this->DragObject = NULL;
	C4Object *DropObject = TargetObject;
	// drag object must exist; drop object is optional
	if (!DragObject) return;
	if (DropObject && (~DropObject->GetPropertyInt(P_MouseDrag) & C4MC_MD_DropTarget))
		DropObject = NULL;
	// no commands if player is eliminated or doesn't exist any more
	C4Player *pPlr = ::Players.Get(Player);
	if (!pPlr || pPlr->Eliminated) return;
	// todo: Perform drag/drop validity check
	// now drag/drop is handled by script
	Game.Input.Add(CID_PlrMouseMove, C4ControlPlayerMouse::DragDrop(::Players.Get(Player), DropObject, DragObject));
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

	// might be in Drag_Unknown
	Drag = C4MC_Drag_None;

	// Alternative object selection
	if (Cursor==C4MC_Cursor_Select && !IsPassive() && Selection.GetObject() == DownTarget)
		{ Game.Input.Add(CID_PlrSelect, new C4ControlPlayerSelect(Player,Selection,true)); }

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
		if (!Scrolling && (!TargetObject || !(TargetObject->Category & C4D_IgnoreFoW)))
		{
			Cursor=C4MC_Cursor_Passive;
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
	C4Object *pObj = Game.FindVisObject(ViewX, ViewY, Player, fctViewportGame, fctViewportGUI, GameX,GameY, C4D_MouseSelect, GuiX-fctViewportGUI.X, GuiY-fctViewportGUI.Y);
	if (!pObj) return NULL;
	return pObj;
}

bool C4MouseControl::IsPassive()
{
	return ::Control.isReplay() || Player<=NO_OWNER;
}

void C4MouseControl::ScrollView(float iX, float iY, float ViewWdt, float ViewHgt)
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
	return Active && Drag == C4MC_Drag_Script;
}

bool C4MouseControl::GetLastGUIPos(int32_t *x_out, int32_t *y_out) const
{
	// safety
	if (!Active || !fMouseOwned) return false;
	// OK; assign last known pos
	*x_out = GuiX; *y_out = GuiY;
	return true;
}
