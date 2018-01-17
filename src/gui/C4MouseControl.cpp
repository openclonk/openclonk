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

/* Mouse input */

#include "C4Include.h"
#include "gui/C4MouseControl.h"

#include "control/C4GameControl.h"
#include "game/C4Application.h"
#include "game/C4FullScreen.h"
#include "game/C4Viewport.h"
#include "graphics/C4Draw.h"
#include "graphics/C4GraphicsResource.h"
#include "gui/C4ChatDlg.h"
#include "gui/C4Gui.h"
#include "gui/C4ScriptGuiWindow.h"
#include "landscape/C4Landscape.h"
#include "lib/StdMesh.h"
#include "object/C4Def.h"
#include "object/C4Object.h"
#include "player/C4Player.h"
#include "player/C4PlayerList.h"

const int32_t C4MC_Drag_None            = 0,
              C4MC_Drag_Script          = 6,
              C4MC_Drag_Unhandled       = 7;

const int32_t C4MC_Tooltip_Delay = 20;

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
	pPlayer=nullptr;
	Viewport=nullptr;
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
	DragObject=nullptr;
	KeepCaption=0;
	Drag=C4MC_Drag_None;
	Selection.Default();
	TargetObject=DownTarget=nullptr;
	ControlDown=false;
	ShiftDown=false;
	AltDown=false;
	Scrolling=false;
	ScrollSpeed=10;
	DragImageDef=nullptr;
	DragImageObject=nullptr;
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
		if (AltDown) wKeyState|=MK_ALT;
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
	if (TargetObject==pObj) TargetObject=nullptr;
	if (DownTarget==pObj) DownTarget=nullptr;
	if (DragObject==pObj)
	{
		DragObject=nullptr;
		Drag=C4MC_Drag_None;
		DragImageDef=nullptr;
		DragImageObject=nullptr;
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
	if (!Active || !Application.Active || ::pGUI->HasMouseFocus()) { ClipCursor(nullptr); return; }
	// get controlled viewport
	C4Viewport *pVP=::Viewports.GetViewport(Player);
	if (!pVP) { ClipCursor(nullptr); return; }
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
		pPlayer = nullptr;
	// Check viewport
	if (!(Viewport=::Viewports.GetViewport(Player))) return;
	// get view position
	C4Rect rcViewport = Viewport->GetOutputRect();
	fctViewport.Set(nullptr, rcViewport.x, rcViewport.y, rcViewport.Wdt, rcViewport.Hgt);
	ViewX=Viewport->GetViewX(); ViewY=Viewport->GetViewY();
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

	// are custom menus active?
	bool menuProcessed = false;
	if (pPlayer)
		// adjust by viewport X/Y because the GUI windows calculate their positions (and thus check input) based on that
		menuProcessed = ::Game.ScriptGuiRoot->MouseInput(iButton, iX, iY, dwKeyFlags);

	if (menuProcessed)
		Cursor = C4MC_Cursor_Select;

	// if not caught by a menu
	if (!menuProcessed)
		// script handling of mouse control for everything but regular movement (which is sent at control frame intervals only)
		if (iButton != C4MC_Button_None)
			// not if blocked by selection object
			if (!TargetObject)
				// safety (can't really happen in !IsPassive, but w/e
				if (pPlayer && pPlayer->ControlSet)
					if (!menuProcessed && pPlayer->ControlSet->IsMouseControlAssigned(iButton))
						pPlayer->Control.DoMouseInput(0 /* only 1 mouse supported so far */, iButton, GameX, GameY, GuiX, GuiY, dwKeyFlags);
}

void C4MouseControl::DoMoveInput()
{
	// current mouse move to input queue
	// do sanity checks
	if (!Active || !fMouseOwned) return;
	if (!(pPlayer=::Players.Get(Player))) return;
	if (!pPlayer->ControlSet) return;
	if (!pPlayer->ControlSet->IsMouseControlAssigned(C4MC_Button_None)) return;
	pPlayer->Control.DoMouseInput(0 /* only 1 mouse supported so far */, C4MC_Button_None, GameX, GameY, GuiX, GuiY, (ControlDown && MK_CONTROL) | (ShiftDown && MK_SHIFT) | (AltDown && MK_ALT));
}

void C4MouseControl::Draw(C4TargetFacet &cgo, const ZoomData &GameZoom)
{
	int32_t iOffsetX,iOffsetY;
	float wdt = GfxR->fctMouseCursor.Wdt, hgt = GfxR->fctMouseCursor.Hgt;
	// Cursor size relative to height - does not matter with current square graphics.
	float zoom = Config.Graphics.MouseCursorSize / hgt;
	hgt *= zoom;
	wdt *= zoom;

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
		iOffsetX = wdt/2;
		iOffsetY = hgt/2;
		// calculate the hotspot for the scrolling cursors
		switch (Cursor)
		{
		case C4MC_Cursor_Up: iOffsetY += -hgt/2; break;
		case C4MC_Cursor_Down:iOffsetY += +hgt/2; break;
		case C4MC_Cursor_Left: iOffsetX += -wdt/2; break;
		case C4MC_Cursor_Right: iOffsetX += +wdt/2; break;
		case C4MC_Cursor_UpLeft: iOffsetX += -wdt/2; iOffsetY += -hgt/2; break;
		case C4MC_Cursor_UpRight: iOffsetX += +wdt/2; iOffsetY += -hgt/2; break;
		case C4MC_Cursor_DownLeft: iOffsetX += -wdt/2; iOffsetY += +hgt/2; break;
		case C4MC_Cursor_DownRight: iOffsetX += +wdt/2; iOffsetY += +hgt/2; break;
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
			else if (pGfx->Type == C4DefGraphics::TYPE_Mesh)
			{
				// Note bounding box is in OGRE coordinate system
				ImageWdt = pGfx->Mesh->GetBoundingBox().y2 - pGfx->Mesh->GetBoundingBox().y1;
				ImageHgt = pGfx->Mesh->GetBoundingBox().z2 - pGfx->Mesh->GetBoundingBox().z1;
			}
			else
			{
				ImageWdt = ImageHgt = 1.0f;
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
				DragImageObject->ColorMod = (Drag == C4MC_Drag_Script) ? 0x7fffffff : (/*DragImagePhase*/false ? 0x8f7f0000 : 0x1f007f00);
				DragImageObject->BlitMode = C4GFXBLIT_MOD2;

				DragImageObject->DrawPicture(ccgo, false, nullptr);

				DragImageObject->ColorMod = ColorMod;
				DragImageObject->BlitMode = BlitMode;
			}
			else
			{
				// draw in special modulation mode
				pDraw->SetBlitMode(C4GFXBLIT_MOD2);
				// draw DragImage in red or green, according to the phase to be used
				pDraw->ActivateBlitModulation((Drag == C4MC_Drag_Script) ? 0x7fffffff : (/*DragImagePhase*/false ? 0x8f7f0000 : 0x1f007f00));

				DragImageDef->Draw(ccgo, false, pPlayer ? pPlayer->ColorDw : 0xff0000ff, nullptr, 0, 0, nullptr);

				// reset color
				pDraw->DeactivateBlitModulation();
				pDraw->SetBlitMode(0);
			}

			if (fIsGameZoom) pDraw->SetZoom(GuiZoom);
			// reset cursor hotspot offset for script drawing
			iOffsetX = wdt/2;
			iOffsetY = hgt/2;
		}
		// Cursor
		if ( (!DragImageDef && !DragImageObject) || (Drag == C4MC_Drag_Script))
		{
			GfxR->fctMouseCursor.DrawX(cgo.Surface, cgo.X+GuiX-iOffsetX, cgo.Y+GuiY-iOffsetY, wdt, hgt,  Cursor);
		}
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
		TargetObject=nullptr;
	}
	else
	{
		// Target object
		TargetObject=GetTargetObject();
		if (TargetObject && FogOfWar && !(TargetObject->Category & C4D_IgnoreFoW)) TargetObject = nullptr;

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
				TargetObject = nullptr;
		}

		// passive cursor
		if (IsPassive())
			Cursor=C4MC_Cursor_Passive;

		// update tooltip information
		if (OldTargetObject != TargetObject)
		{
			C4String *newTooltip = nullptr;
			if (TargetObject && (Cursor == C4MC_Cursor_Select) && (TargetObject->Category & C4D_MouseSelect) && (newTooltip = TargetObject->GetPropertyStr(P_Tooltip)))
			{
				float objX, objY;
				TargetObject->GetViewPos(objX, objY, -fctViewportGUI.X, -fctViewportGUI.Y, fctViewportGUI);
				objX += TargetObject->Shape.x;
				objY += TargetObject->Shape.y - TargetObject->addtop();
				SetTooltipRectangle(C4Rect(objX, objY, TargetObject->Shape.Wdt, TargetObject->Shape.Hgt + TargetObject->addtop()));
				SetTooltipText(StdStrBuf(newTooltip->GetCStr()));
			}
			else
			{
				SetTooltipRectangle(C4Rect(0, 0, 0, 0));
			}
		}

		if (!KeepCaption
			&& ToolTipRectangle.Wdt != 0
			&& Inside(GuiX, ToolTipRectangle.x, ToolTipRectangle.x + ToolTipRectangle.Wdt)
			&& Inside(GuiY, ToolTipRectangle.y, ToolTipRectangle.y + ToolTipRectangle.Hgt))
		{
			++TimeInTooltipRectangle;

			if (TimeInTooltipRectangle >= C4MC_Tooltip_Delay)
			{
				Caption = TooltipText;
			}
		}
		else
		{
			// disable tooltip pop-up; whatever set it in the first place will set it again on the next mouse-enter
			TimeInTooltipRectangle = 0;
			ToolTipRectangle.Wdt = 0;
		}
	}

	// Make a script callback if the object being hovered changes
	if(!IsPassive() && OldTargetObject != TargetObject)
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
	if(!RightButtonDown) DownTarget = nullptr;
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
			C4Object *drag_image_obj; C4Def * drag_image_def;

			// Drag only if MD_SOURCE is set and drag image is present
			if ( (DownTarget->GetPropertyInt(P_MouseDrag) & C4MC_MD_DragSource) &&
			      DownTarget->GetDragImage(&drag_image_obj, &drag_image_def))
			{
				Drag=C4MC_Drag_Script;

				if(drag_image_obj) DragImageObject = drag_image_obj;
				else DragImageDef = drag_image_def;

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
	if(!LeftButtonDown) DownTarget = nullptr;
}

void C4MouseControl::Wheel(DWORD dwFlags)
{
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
	DragImageObject = nullptr;
	DragImageDef = nullptr;
	C4Object *DragObject = this->DragObject;
	this->DragObject = nullptr;
	C4Object *DropObject = TargetObject;
	// drag object must exist; drop object is optional
	if (!DragObject) return;
	if (DropObject && (~DropObject->GetPropertyInt(P_MouseDrag) & C4MC_MD_DropTarget))
		DropObject = nullptr;
	// no commands if player is eliminated or doesn't exist any more
	C4Player *pPlr = ::Players.Get(Player);
	if (!pPlr || pPlr->Eliminated) return;
	// todo: Perform drag/drop validity check
	// now drag/drop is handled by script
	Game.Input.Add(CID_PlrMouseMove, C4ControlPlayerMouse::DragDrop(::Players.Get(Player), DropObject, DragObject));
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
	// TODO: Check C4FoWRegion... should maybe be passed as a parameter?
	// pDraw->GetFoW() might not be current at this time.
	if (/*(pPlayer->fFogOfWar && !pPlayer->FoWIsVisible(int32_t(GameX),int32_t(GameY))) || */GameX<0 || GameY<0 || int32_t(GameX)>=::Landscape.GetWidth() || int32_t(GameY)>=::Landscape.GetHeight())
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

void C4MouseControl::SetTooltipRectangle(const C4Rect &rectangle)
{
	// Set the tooltip rectangle slightly larger than originally requested.
	// The tooltip will be removed when the cursor leaves the rectangle, so make sure that the tooltip is not already disabled when the cursor moves to the border-pixel of the GUI item
	// in case the GUI item uses a different check for bounds (< vs <=) than the tooltip rectangle.
	ToolTipRectangle = C4Rect(rectangle.x - 2, rectangle.y - 2, rectangle.Wdt + 4, rectangle.Hgt + 4);
	TimeInTooltipRectangle = 0;
}

void C4MouseControl::SetTooltipText(const StdStrBuf &text)
{
	TooltipText = text;
}

C4Object *C4MouseControl::GetTargetObject()
{
	// find object
	// gui object position currently wrong...will fall apart once GUIZoom is activated
	C4Object *pObj = Game.FindVisObject(ViewX, ViewY, Player, fctViewportGame, fctViewportGUI, GameX,GameY, C4D_MouseSelect, GuiX-fctViewportGUI.X, GuiY-fctViewportGUI.Y);
	if (!pObj) return nullptr;
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
		Viewport->ScrollView(iX, iY);
	}

}

bool C4MouseControl::IsDragging()
{
	return Active && Drag == C4MC_Drag_Script;
}

bool C4MouseControl::GetLastCursorPos(int32_t *x_out_gui, int32_t *y_out_gui, int32_t *x_out_game, int32_t *y_out_game) const
{
	// safety
	if (!Active || !fMouseOwned) return false;
	// OK; assign last known pos
	*x_out_gui = GuiX; *y_out_gui = GuiY;
	*x_out_game = GameX; *y_out_game = GameY;
	return true;
}
