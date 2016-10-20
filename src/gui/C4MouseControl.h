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

#ifndef INC_C4MouseControl
#define INC_C4MouseControl

#include "graphics/C4Facet.h"
#include "object/C4ObjectList.h"
#include "lib/C4Rect.h"
struct ZoomData; // #include "graphics/C4Draw.h"

const int32_t C4MC_Button_None        = 0,
              C4MC_Button_LeftDown    = 1,
              C4MC_Button_LeftUp      = 2,
              C4MC_Button_RightDown   = 3,
              C4MC_Button_RightUp     = 4,
              C4MC_Button_LeftDouble  = 5,
              C4MC_Button_RightDouble = 6,
              C4MC_Button_Wheel       = 7,
              C4MC_Button_MiddleDown  = 8,
              C4MC_Button_MiddleUp    = 9,
              C4MC_Button_MiddleDouble= 10,
              C4MC_Button_X1Down      = 11,
              C4MC_Button_X1Up        = 12,
              C4MC_Button_X1Double    = 13,
              C4MC_Button_X2Down      = 14,
              C4MC_Button_X2Up        = 15,
              C4MC_Button_X2Double    = 16;

const int32_t C4MC_DragSensitivity = 5;

const int32_t C4MC_MD_DragSource = 1,
              C4MC_MD_DropTarget = 2,
              C4MC_MD_NoClick = 4;

const int32_t C4MC_Cursor_Select = 0,      // click cursor to select/click stuff in the GUI
              C4MC_Cursor_Crosshair = 1,   // standard ingame cursor
              C4MC_Cursor_DragDrop = 2,    // cursor when drag&dropping
              C4MC_Cursor_Up = 3,          // cursors for scrolling the viewport ...
              C4MC_Cursor_Down = 4,        // ...
              C4MC_Cursor_Left = 5,
              C4MC_Cursor_Right = 6,
              C4MC_Cursor_UpLeft = 7,
              C4MC_Cursor_UpRight = 8,
              C4MC_Cursor_DownLeft = 9,
              C4MC_Cursor_DownRight = 10,
              C4MC_Cursor_Passive = 11,    // passive cursor in records and and fog of war and outside viewport
              C4MC_Cursor_DropInto = 12;   // drop into contents

class C4MouseControl
{
	friend class C4Viewport;
public:
	C4MouseControl();
	~C4MouseControl();
protected:
	bool Active;
	bool fMouseOwned;
	int32_t Player;
	C4Player *pPlayer; // valid during Move()
	C4Viewport *Viewport; // valid during Move()

	int32_t Cursor;

	int32_t VpX,VpY; // Pixel coordinates of mouse pos
	float ViewX,ViewY; // Game coordinate scrolling offset of viewport
	float GameX,GameY; // Game coordinates of mouse pos
	float GuiX,GuiY; // GUI coorindates of mouse pos
	C4Facet fctViewport, fctViewportGame, fctViewportGUI;

	float DownX,DownY; // Game coordinates of mouse-down-pos while dragging

	int32_t ScrollSpeed;
	int32_t Drag;

	bool LeftButtonDown,RightButtonDown,LeftDoubleIgnoreUp;
	bool ButtonDownOnSelection;
	bool ControlDown;
	bool ShiftDown;
	bool AltDown;
	bool Scrolling;
	bool InitCentered;
	bool FogOfWar;
	bool Visible;

	C4ObjectList Selection; //obsolete!

	C4Object *DragObject;
	C4ID DragID;
	C4Def* DragImageDef;
	C4Object* DragImageObject;
	
	// Tooltip management

	// currently shown caption
	StdCopyStrBuf Caption;
	// tooltip text that will be shown when the mouse is kept in the tooltip rectangle for some time
	StdCopyStrBuf TooltipText;
	int32_t CaptionBottomY;
	int32_t KeepCaption;
	int32_t TimeInTooltipRectangle;
	C4Rect ToolTipRectangle;

	// Target object
	C4Object *TargetObject; // valid during Move()
	C4Object *DownTarget;
public:
	void Default();
	void Clear();
	bool Init(int32_t iPlayer);
	void Execute();
	void HideCursor();
	void ShowCursor();
	void Draw(C4TargetFacet &cgo, const ZoomData &GameZoom);
	void Move(int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyFlags, bool fCenter = false);
	void DoMoveInput();
	bool IsViewport(C4Viewport *pViewport);
	void ClearPointers(C4Object *pObj);
	void UpdateClip();  // update clipping region for mouse cursor
	void SetOwnedMouse(bool fToVal) { fMouseOwned = fToVal; }
	bool IsMouseOwned() { return fMouseOwned; }
	bool IsActive() { return !!Active; }
	bool GetLastCursorPos(int32_t *x_out_gui, int32_t *y_out_gui, int32_t *x_out_game, int32_t *y_out_game) const;

	const char *GetCaption();
	void SetTooltipText(const StdStrBuf &text);
	void SetTooltipRectangle(const C4Rect &rectangle);
protected:
	void UpdateFogOfWar();
	void RightUpDragNone();
	void ButtonUpDragScript();
	void LeftUpDragNone();
	void DragScript();
	void Wheel(DWORD dwFlags);
	void RightUp();
	void RightDown();
	void LeftDouble();
	void DragNone();
	void LeftUp();
	void LeftDown();
	void UpdateScrolling();
	void UpdateCursorTarget();
	int32_t UpdateSingleSelection();
	C4Object *GetTargetObject(); // get MouseSelection object at position
	bool IsPassive(); // return whether mouse is only used to look around
	void ScrollView(float iX, float iY, float ViewWdt, float ViewHgt); // in landscape coordinates

public:
	bool IsDragging();
	bool IsLeftDown() { return LeftButtonDown; }
	int32_t GetPlayer() { return Player; }
};

extern C4MouseControl MouseControl;
#endif
