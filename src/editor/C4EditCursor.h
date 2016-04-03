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

#ifdef USE_GTK
#include <gtk/gtk.h>
#endif

// Currently selected elements in editcursor. May be objects and other prop lists.
class C4EditCursorSelection : public std::list<C4Value>
{
public:
	StdStrBuf GetDataString() const; // Return a string like "n objects".
	C4Object *GetObject(int32_t index=0) const; // Get indexed C4Object * in list
	C4Object *GetLastObject() const;
	void ConsolidateEmpty(); // remove NULLed entries that may happen because objects got deleted
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
	int32_t Mode;
	float X,Y,X2,Y2;
	bool Hold,DragFrame,DragLine;
	C4Object *Target,*DropTarget;
	class C4Def *creator_def;
	std::unique_ptr<C4GraphicsOverlay> creator_overlay;
	struct ObjselItemDt {
		C4EditCursor* EditCursor;
		C4Object* Object;
		StdCopyStrBuf Command;
#if defined(USE_WIN32_WINDOWS)
		UINT_PTR ItemId;
#elif defined(USE_GTK)
		GtkWidget* MenuItem;
#endif
	};
	std::vector<ObjselItemDt> itemsObjselect;
#ifdef USE_WIN32_WINDOWS
	HMENU hMenu;
#elif defined(USE_GTK)
	GtkWidget* menuContext;
	GtkWidget* itemDelete;
	GtkWidget* itemDuplicate;
	GtkWidget* itemGrabContents;
	GtkWidget* itemProperties;
#endif
	// Selection may either be any number of objects or a single non-object prop list
	C4EditCursorSelection selection;
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
	bool Move(float iX, float iY, DWORD dwKeyState);
	bool Init();
	bool EditingOK(bool for_landscape_drawing=false);
	C4EditCursorSelection &GetSelection() { return selection; }
	void SetHold(bool fToState) { Hold = fToState; }
	void OnSelectionChanged(bool by_objectlist=false);
	bool AltDown();
	bool AltUp();
	void SetMouseHover(bool h) { has_mouse_hover = h; }
protected:
	void UpdateStatusBar();
	void ApplyCreateObject(bool contained);
	void ApplyToolPicker();
	void ToolFailure();
	void PutContents();
	void UpdateDropTarget(DWORD dwKeyState);
	void AppendMenuItem(int num, const StdStrBuf & label);
	bool DoContextMenu(DWORD dwKeyState);
	void ApplyToolFill();
	void ApplyToolRect();
	void ApplyToolLine();
	void ApplyToolBrush();
	void DrawSelectMark(C4Facet &cgo, FLOAT_RECT r, float width);
	void FrameSelection();
	void MoveSelection(C4Real iXOff, C4Real iYOff);
	void EMMoveObject(enum C4ControlEMObjectAction eAction, C4Real tx, C4Real ty, C4Object *pTargetObj, const C4EditCursorSelection *pObjs = NULL, const char *szScript = NULL);
	void EMControl(enum C4PacketType eCtrlType, class C4ControlPacket *pCtrl);
	void DoContextObjsel(C4Object *, bool clear);
	void DoContextObjCommand(C4Object *, const char *cmd);
	void ObjselectDelItems();

#ifdef USE_GTK
	static void OnDelete(GtkWidget* widget, gpointer data);
	static void OnDuplicate(GtkWidget* widget, gpointer data);
	static void OnGrabContents(GtkWidget* widget, gpointer data);
	static void OnObjselect(GtkWidget* widget, gpointer data);
#endif
public:
	void AddToSelection(C4PropList *add_obj);         // add object to selection and do script callback. Doesn't do OnSelectionChanged().
	bool RemoveFromSelection(C4PropList *remove_obj); // remove object from selection and do script callback. return true if object was in selection before. Doesn't do OnSelectionChanged().
	void ClearSelection(C4PropList *next_selection=NULL);  // remove all objects from selection and do script callback. if next_selection is non-null, passes that to the deselection callbacks. Doesn't do OnSelectionChanged().
	// Type of object to create in object creation mode
	void SetCreatorDef(C4Def *new_def) { creator_def = new_def; creator_overlay.reset(NULL); }
	C4Def *GetCreatorDef() { return creator_def; }
};

#endif
