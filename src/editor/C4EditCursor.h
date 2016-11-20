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

/* Handles viewport editing in console mode */

#ifndef INC_C4EditCursor
#define INC_C4EditCursor

#include "object/C4ObjectList.h"
#include "control/C4Control.h"
#include "lib/C4Rect.h"
#include "script/C4Value.h"
#include <vector>
#include "object/C4DefGraphics.h"


// Currently selected elements in editcursor. May be objects and other prop lists.
class C4EditCursorSelection : public std::list<C4Value>
{
public:
	StdStrBuf GetDataString() const; // Return a string like "n objects".
	C4Object *GetObject(int32_t index=0) const; // Get indexed C4Object * in list
	C4Object *GetLastObject() const;
	void ConsolidateEmpty(); // remove nullptred entries that may happen because objects got deleted
	bool ClearPointers(C4Object *obj);
	bool IsContained(C4PropList *obj) const;
	int32_t ObjectCount() const; // count only C4Object *
};

class C4EditCursor
{
public:
	C4EditCursor();
	~C4EditCursor();

protected:
	bool fAltWasDown;
	bool fShiftWasDown;
	bool has_mouse_hover;
	bool selection_invalid; // if true, the property list should be updated on next execution
	int32_t Mode;
	float X,Y,X2,Y2,Zoom;
	bool Hold,DragFrame,DragLine,DragShape,DragTransform;
	int32_t DragRot0, DragCon0, DragRotLast, DragConLast;
	C4Object *Target,*DropTarget;
	C4Value highlighted_object;
	class C4Def *creator_def;
	std::unique_ptr<C4GraphicsOverlay> creator_overlay;
	struct ObjselItemDt {
		C4EditCursor* EditCursor;
		C4Object* Object;
		StdCopyStrBuf Command;
#if defined(USE_WIN32_WINDOWS)
		UINT_PTR ItemId;
#endif
	};
	std::vector<ObjselItemDt> itemsObjselect;
#ifdef USE_WIN32_WINDOWS
	HMENU hMenu;
#endif
	// Selection may either be any number of objects or a single non-object prop list
	C4EditCursorSelection selection;
#ifdef WITH_QT_EDITOR
	std::unique_ptr<class C4ConsoleQtShapes> shapes;
#endif
public:
	void Default();
	void Clear();
	void Execute();
	void ClearPointers(C4Object *pObj);
	bool ToggleMode();
	void Draw(C4TargetFacet &cgo);
	int32_t GetMode();
	C4Object* GetTarget();
	bool SetMode(int32_t iMode);
	bool In(const char *szText);
	bool Duplicate();
	bool OpenPropTools();
	bool Delete();
	void GrabContents();
	bool LeftButtonUp(DWORD dwKeyState);
	bool LeftButtonDown(DWORD dwKeyState);
	bool RightButtonUp(DWORD dwKeyState);
	bool RightButtonDown(DWORD dwKeyState);
	bool KeyDown(C4KeyCode KeyCode, DWORD dwKeyState);
	bool KeyUp(C4KeyCode KeyCode, DWORD dwKeyState);
	bool Move(float iX, float iY, float zoom, DWORD dwKeyState);
	bool Move(DWORD new_key_state);
	bool Init();
	bool EditingOK(bool for_landscape_drawing=false);
	C4EditCursorSelection &GetSelection() { return selection; }
	void SetHold(bool fToState) { Hold = fToState; }
	void OnSelectionChanged(bool by_objectlist=false);
	bool AltDown();
	bool AltUp();
	void SetMouseHover(bool h) { has_mouse_hover = h; }
#ifdef WITH_QT_EDITOR
	class C4ConsoleQtShapes *GetShapes() const { return shapes.get(); }
#endif
	bool HasTransformCursor() const { return DragTransform || IsHoveringTransformMarker(); }
	bool IsHoveringTransformMarker() const;
protected:
	void UpdateStatusBar();
	void ApplyCreateObject(bool contained);
	void ApplyToolPicker();
	void ToolFailure();
	void PutContents();
	void UpdateDropTarget(DWORD dwKeyState);
	bool DoContextMenu(DWORD dwKeyState);
	void AppendMenuItem(int num, const StdStrBuf & label);
	void ApplyToolFill();
	void ApplyToolRect();
	void ApplyToolLine();
	void ApplyToolBrush();
	void DrawObject(C4TargetFacet &cgo, C4Object *cobj, uint32_t select_mark_color, bool highlight, bool draw_transform_marker);
	void DrawSelectMark(C4Facet &cgo, FLOAT_RECT r, float width, uint32_t color = 0xffffffff);
	bool HasTransformMarker(float *x, float *y, float zoom) const;
	void FrameSelection();
	void MoveSelection(C4Real iXOff, C4Real iYOff, bool drag_finished);
	void EMMoveObject(enum C4ControlEMObjectAction eAction, C4Real tx, C4Real ty, C4Object *pTargetObj, const C4EditCursorSelection *pObjs = nullptr, const char *szScript = nullptr, bool drag_finished = false);
	void DoContextObjCommand(C4Object *, const char *cmd);
	void ObjselectDelItems();

public:
	void DoContextObjsel(C4Object *, bool clear);
	void PerformDuplication(int32_t *object_numbers, int32_t object_count, bool local_call);
	void PerformDuplicationLegacy(int32_t *pObjects, int32_t iObjectNum, bool fLocalCall);

protected:
public:
	void AddToSelection(C4PropList *add_obj);         // add object to selection and do script callback. Doesn't do OnSelectionChanged().
	bool RemoveFromSelection(C4PropList *remove_obj); // remove object from selection and do script callback. return true if object was in selection before. Doesn't do OnSelectionChanged().
	void ClearSelection(C4PropList *next_selection=nullptr);  // remove all objects from selection and do script callback. if next_selection is non-null, passes that to the deselection callbacks. Doesn't do OnSelectionChanged().
	// Type of object to create in object creation mode
	void SetCreatorDef(C4Def *new_def) { creator_def = new_def; creator_overlay.reset(nullptr); }
	C4Def *GetCreatorDef() { return creator_def; }

	void EMControl(enum C4PacketType eCtrlType, class C4ControlPacket *pCtrl);
	void InvalidateSelection() { selection_invalid = true; }
	void ValidateSelection() { selection_invalid = false; }
	bool IsSelectionInvalidated() const { return selection_invalid; }
	bool GetCurrentSelectionPosition(int32_t *x, int32_t *y); // return center of first selected object
	void SetHighlightedObject(C4Object *new_highlight);
};

#endif
