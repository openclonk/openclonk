/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2001, 2005  Sven Eberhardt
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

/* Handles viewport editing in console mode */

#ifndef INC_C4EditCursor
#define INC_C4EditCursor

#include "C4ObjectList.h"
#include "C4Control.h"

#ifdef WITH_DEVELOPER_MODE
#include <gtk/gtk.h>
#endif

class C4EditCursor
{
public:
	C4EditCursor();
	~C4EditCursor();
protected:
	bool fAltWasDown;
	bool fSelectionChanged;
	int32_t Mode;
	float X,Y,X2,Y2;
	bool Hold,DragFrame,DragLine;
	C4Object *Target,*DropTarget;
#ifdef _WIN32
	HMENU hMenu;
#else
#ifdef WITH_DEVELOPER_MODE
	GtkWidget* menuContext;

	GtkWidget* itemDelete;
	GtkWidget* itemDuplicate;
	GtkWidget* itemGrabContents;
	GtkWidget* itemProperties;
#endif
#endif // _WIN32
	C4ObjectList Selection;
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
	bool LeftButtonUp();
	bool LeftButtonDown(bool fControl);
	bool RightButtonUp();
	bool RightButtonDown(bool fControl);
	bool Move(float iX, float iY, WORD wKeyFlags);
	bool Init();
	bool EditingOK();
	C4ObjectList &GetSelection() { return Selection; }
	void SetHold(bool fToState) { Hold = fToState; }
	void OnSelectionChanged();
	bool AltDown();
	bool AltUp();
protected:
	void UpdateStatusBar();
	void ApplyToolPicker();
	void ToolFailure();
	void PutContents();
	void UpdateDropTarget(WORD wKeyFlags);
	void GrabContents();
	bool DoContextMenu();
	void ApplyToolFill();
	void ApplyToolRect();
	void ApplyToolLine();
	void ApplyToolBrush();
	void DrawSelectMark(C4Facet &cgo, FLOAT_RECT r);
	void FrameSelection();
	void MoveSelection(C4Real iXOff, C4Real iYOff);
	void EMMoveObject(enum C4ControlEMObjectAction eAction, C4Real tx, C4Real ty, C4Object *pTargetObj, const C4ObjectList *pObjs = NULL, const char *szScript = NULL);
	void EMControl(enum C4PacketType eCtrlType, class C4ControlPacket *pCtrl);

#ifdef WITH_DEVELOPER_MODE
	static void OnDelete(GtkWidget* widget, gpointer data);
	static void OnDuplicate(GtkWidget* widget, gpointer data);
	static void OnGrabContents(GtkWidget* widget, gpointer data);
	static void OnProperties(GtkWidget* widget, gpointer data);
#endif
};

#endif
