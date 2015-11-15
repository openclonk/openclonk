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

/* Handles viewport editing in console mode */

#include <C4Include.h>
#include <C4EditCursor.h>

#include <C4Console.h>
#include <C4Object.h>
#include <C4Application.h>
#include <C4Random.h>
#include <C4MouseControl.h>
#include <C4Landscape.h>
#include <C4Texture.h>
#include <C4GraphicsResource.h>
#include <C4Game.h>
#include <C4GameObjects.h>
#include <C4GameControl.h>

#ifdef _WIN32
#include "resource.h"
#endif

#ifdef WITH_DEVELOPER_MODE
# include <C4Language.h>

# include <gtk/gtk.h>
#endif

C4EditCursor::C4EditCursor()
{
	Default();
}

C4EditCursor::~C4EditCursor()
{
	Clear();
}

void C4EditCursor::Execute()
{
	// drawing
	switch (Mode)
	{
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case C4CNS_ModeEdit:
		// Hold selection
		if (Hold)
			EMMoveObject(EMMO_Move, Fix0, Fix0, NULL, &Selection);
		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case C4CNS_ModeDraw:
		switch (Console.ToolsDlg.Tool)
		{
		case C4TLS_Fill:
			if (Hold) if (!Game.HaltCount) if (Console.Editing) ApplyToolFill();
			break;
		}
		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	}
	if (!::Game.iTick35)
		Console.PropertyDlgUpdate(Selection, false);
}

bool C4EditCursor::Init()
{

#ifdef USE_WIN32_WINDOWS
	if (!(hMenu = LoadMenu(Application.GetInstance(),MAKEINTRESOURCE(IDR_CONTEXTMENUS))))
		return false;
#else // _WIN32
#ifdef WITH_DEVELOPER_MODE
	menuContext = gtk_menu_new();

	itemDelete = gtk_menu_item_new_with_label(LoadResStr("IDS_MNU_DELETE"));
	itemDuplicate = gtk_menu_item_new_with_label(LoadResStr("IDS_MNU_DUPLICATE"));
	itemGrabContents = gtk_menu_item_new_with_label(LoadResStr("IDS_MNU_CONTENTS"));

	gtk_menu_shell_append(GTK_MENU_SHELL(menuContext), itemDelete);
	gtk_menu_shell_append(GTK_MENU_SHELL(menuContext), itemDuplicate);
	gtk_menu_shell_append(GTK_MENU_SHELL(menuContext), itemGrabContents);

	g_signal_connect(G_OBJECT(itemDelete), "activate", G_CALLBACK(OnDelete), this);
	g_signal_connect(G_OBJECT(itemDuplicate), "activate", G_CALLBACK(OnDuplicate), this);
	g_signal_connect(G_OBJECT(itemGrabContents), "activate", G_CALLBACK(OnGrabContents), this);

	gtk_widget_show_all(menuContext);
#endif // WITH_DEVELOPER_MODe
#endif // _WIN32
	Console.UpdateModeCtrls(Mode);

	return true;
}

void C4EditCursor::ClearPointers(C4Object *pObj)
{
	if (Target==pObj) Target=NULL;
	if (Selection.ClearPointers(pObj))
		OnSelectionChanged();
}

bool C4EditCursor::Move(float iX, float iY, DWORD dwKeyState)
{
	// alt check
	bool fAltIsDown = (dwKeyState & MK_ALT) != 0;
	if (fAltIsDown != fAltWasDown)
	{
		if ((fAltWasDown = fAltIsDown))
			AltDown();
		else
			AltUp();
	}

	// shift check
	bool fShiftIsDown = (dwKeyState & MK_SHIFT) != 0;
	if(fShiftIsDown != fShiftWasDown)
		fShiftWasDown = fShiftIsDown;

	// Offset movement
	float xoff = iX-X; float yoff = iY-Y;
	X=iX; Y=iY;

	switch (Mode)
	{
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case C4CNS_ModeEdit:
		// Hold
		if (!DragFrame && Hold)
		{
			MoveSelection(ftofix(xoff),ftofix(yoff));
			UpdateDropTarget(dwKeyState);
		}
		// Update target
		// Shift always indicates a target outside the current selection
		else
		{
			Target = (dwKeyState & MK_SHIFT) ? Selection.GetLastObject() : NULL;
			do
			{
				Target = Game.FindObject(NULL,X,Y,0,0,OCF_NotContained, Target);
			}
			while ((dwKeyState & MK_SHIFT) && Target && Selection.GetLink(Target));
		}
		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case C4CNS_ModeDraw:
		switch (Console.ToolsDlg.Tool)
		{
		case C4TLS_Brush:
			if (Hold) ApplyToolBrush();
			break;
		case C4TLS_Line: case C4TLS_Rect:
			break;
		}
		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	}

	// Update
	UpdateStatusBar();
	return true;
}

void C4EditCursor::UpdateStatusBar()
{
	int32_t X=this->X, Y=this->Y;
	StdStrBuf str;
	switch (Mode)
	{
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case C4CNS_ModePlay:
		if (::MouseControl.GetCaption()) str.CopyUntil(::MouseControl.GetCaption(),'|');
		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case C4CNS_ModeEdit:
		str.Format("%i/%i (%s)",X,Y,Target ? (Target->GetName()) : LoadResStr("IDS_CNS_NOTHING") );
		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case C4CNS_ModeDraw:
		str.Format("%i/%i (fg: %s, bg: %s)",X,Y,
                           MatValid(::Landscape.GetMat(X,Y)) ? ::MaterialMap.Map[::Landscape.GetMat(X,Y)].Name : LoadResStr("IDS_CNS_NOTHING"),
                           MatValid(::Landscape.GetBackMat(X,Y)) ? ::MaterialMap.Map[::Landscape.GetBackMat(X,Y)].Name : LoadResStr("IDS_CNS_NOTHING") );
		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	}
	Console.DisplayInfoText(C4ConsoleGUI::CONSOLE_Cursor, str);
}

void C4EditCursor::OnSelectionChanged()
{
	Console.PropertyDlgUpdate(Selection, false);
	Console.ObjectListDlg.Update(Selection);
}

void C4EditCursor::AddToSelection(C4Object *add_obj)
{
	if (!add_obj || !add_obj->Status) return;
	// add object to selection and do script callback
	Selection.Add(add_obj, C4ObjectList::stNone);
	::Control.DoInput(CID_EMMoveObj, new C4ControlEMMoveObject(EMMO_Select, Fix0, Fix0, add_obj), CDT_Decide);
}

bool C4EditCursor::RemoveFromSelection(C4Object *remove_obj)
{
	if (!remove_obj || !remove_obj->Status) return false;
	// remove object from selection and do script callback
	if (!Selection.Remove(remove_obj)) return false;
	::Control.DoInput(CID_EMMoveObj, new C4ControlEMMoveObject(EMMO_Deselect, Fix0, Fix0, remove_obj), CDT_Decide);
	return true;
}

void C4EditCursor::ClearSelection(C4Object *next_selection)
{
	// remove all objects from selection and do script callbacks
	// iterate safely because callback might delete selected objects!
	C4Object *obj;
	while ((obj = Selection.GetObject(0)))
	{
		Selection.Remove(obj);
		if (obj->Status)
		{
			int32_t next_selection_count = 0, *next_selection_nums = NULL;
			if (next_selection && next_selection->Status)
			{
				// Pass next selection. Always create new array becase the pointer is freed by C4ControlEMMoveObject dtor
				++next_selection_count;
				next_selection_nums = new int32_t[1];
				*next_selection_nums = next_selection->Number;
			}
			::Control.DoInput(CID_EMMoveObj, new C4ControlEMMoveObject(EMMO_Deselect, Fix0, Fix0, obj, next_selection_count, next_selection_nums), CDT_Decide);
		}
	}
	Selection.Clear();
}

bool C4EditCursor::LeftButtonDown(DWORD dwKeyState)
{

	// Hold
	Hold=true;

	switch (Mode)
	{
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case C4CNS_ModeEdit:
		if (dwKeyState & MK_CONTROL)
		{
			// Toggle target
			if (Target)
				if (!RemoveFromSelection(Target))
					AddToSelection(Target);
		}
		else
		{
			// Click on unselected: select single
			if (Target)
			{
				bool found = false;
				for (C4Object *obj : Selection)
				{
					if(obj->At(X, Y))
					{
						found = true;
						break;
					}
				}
				if(!found) // means loop didn't break
				{
					ClearSelection(Target);
					AddToSelection(Target);
				}
			}
			// Click on nothing: drag frame
			if (!Target)
				{ ClearSelection(); DragFrame=true; X2=X; Y2=Y; }
		}
		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case C4CNS_ModeDraw:
		switch (Console.ToolsDlg.Tool)
		{
		case C4TLS_Brush: ApplyToolBrush(); break;
		case C4TLS_Line: DragLine=true; X2=X; Y2=Y; break;
		case C4TLS_Rect: DragFrame=true; X2=X; Y2=Y; break;
		case C4TLS_Fill:
			if (Game.HaltCount)
				{ Hold=false; Console.Message(LoadResStr("IDS_CNS_FILLNOHALT")); return false; }
			break;
		case C4TLS_Picker: ApplyToolPicker(); break;
		}
		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	}

	DropTarget=NULL;

	OnSelectionChanged();
	return true;
}

bool C4EditCursor::RightButtonDown(DWORD dwKeyState)
{

	switch (Mode)
	{
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case C4CNS_ModeEdit:
		if ( (dwKeyState & MK_CONTROL) == 0)
		{
			// Check whether cursor is on anything in the selection
			bool fCursorIsOnSelection = false;
			for (C4Object *obj : Selection)
			{
				if (obj->At(X,Y))
				{
					fCursorIsOnSelection = true;
					break;
				}
			}
			if (!fCursorIsOnSelection)
			{
				// Click on unselected
				if (Target && !Selection.GetLink(Target))
				{
					ClearSelection(Target); AddToSelection(Target);
				}
				// Click on nothing
				if (!Target) ClearSelection();
			}
		}
		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	}

	OnSelectionChanged();
	return true;
}

bool C4EditCursor::LeftButtonUp(DWORD dwKeyState)
{
	// Finish edit/tool
	switch (Mode)
	{
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case C4CNS_ModeEdit:
		if (DragFrame) FrameSelection();
		if (DropTarget) PutContents();
		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case C4CNS_ModeDraw:
		switch (Console.ToolsDlg.Tool)
		{
		case C4TLS_Line:
			if (DragLine) ApplyToolLine();
			break;
		case C4TLS_Rect:
			if (DragFrame) ApplyToolRect();
			break;
		}
		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	}

	// Release
	Hold=false;
	DragFrame=false;
	DragLine=false;
	DropTarget=NULL;
	// Update
	UpdateStatusBar();
	return true;
}

bool C4EditCursor::KeyDown(C4KeyCode KeyCode, DWORD dwKeyState)
{
	// alt check
	bool fAltIsDown = (dwKeyState & MK_ALT) != 0;
	fAltIsDown = fAltIsDown || KeyCode == K_ALT_L || KeyCode == K_ALT_R;
	if (fAltIsDown != fAltWasDown)
	{
		if ((fAltWasDown = fAltIsDown))
			AltDown();
		else
			AltUp();
	}

	// shift check
	bool fShiftIsDown = (dwKeyState & MK_SHIFT) != 0;
	fShiftIsDown = fShiftIsDown || KeyCode == K_SHIFT_L || KeyCode == K_SHIFT_R;
	if(fShiftIsDown != fShiftWasDown)
		fShiftWasDown = fShiftIsDown;

	return true;
}

bool C4EditCursor::KeyUp(C4KeyCode KeyCode, DWORD dwKeyState)
{
	// alt check
	bool fAltIsDown = (dwKeyState & MK_ALT) != 0;
	fAltIsDown = fAltIsDown && !( KeyCode == K_ALT_L || KeyCode == K_ALT_R);
	if (fAltIsDown != fAltWasDown)
	{
		if ((fAltWasDown = fAltIsDown))
			AltDown();
		else
			AltUp();
	}

	// shift check
	bool fShiftIsDown = (dwKeyState & MK_SHIFT) != 0;
	fShiftIsDown = fShiftIsDown && !(KeyCode == K_SHIFT_L || KeyCode == K_SHIFT_R);
	if(fShiftIsDown != fShiftWasDown)
		fShiftWasDown = fShiftIsDown;

	return true;
}

#ifdef USE_WIN32_WINDOWS
bool SetMenuItemEnable(HMENU hMenu, WORD id, bool fEnable)
{
	return !!EnableMenuItem(hMenu,id,MF_BYCOMMAND | MF_ENABLED | ( fEnable ? 0 : MF_GRAYED));
}

bool SetMenuItemText(HMENU hMenu, WORD id, const char *szText)
{
	MENUITEMINFOW minfo;
	ZeroMem(&minfo,sizeof(minfo));
	minfo.cbSize = sizeof(minfo);
	minfo.fMask = MIIM_ID | MIIM_TYPE | MIIM_DATA;
	minfo.fType = MFT_STRING;
	minfo.wID = id;
	StdBuf td = GetWideCharBuf(szText);
	minfo.dwTypeData = getMBufPtr<wchar_t>(td);
	minfo.cch = wcslen(minfo.dwTypeData);
	return !!SetMenuItemInfoW(hMenu,id,false,&minfo);
}
#endif

bool C4EditCursor::RightButtonUp(DWORD dwKeyState)
{
	Target=NULL;

	DoContextMenu(dwKeyState);

	// Update
	UpdateStatusBar();
	return true;
}

bool C4EditCursor::Delete()
{
	if (!EditingOK()) return false;
	EMMoveObject(EMMO_Remove, Fix0, Fix0, NULL, &Selection);
	if (::Control.isCtrlHost())
	{
		OnSelectionChanged();
	}
	return true;
}

bool C4EditCursor::OpenPropTools()
{
	switch (Mode)
	{
	case C4CNS_ModeEdit: case C4CNS_ModePlay:
		Console.PropertyDlgOpen();
		Console.PropertyDlgUpdate(Selection, false);
		break;
	case C4CNS_ModeDraw:
		Console.ToolsDlg.Open();
		break;
	}
	return true;
}

bool C4EditCursor::Duplicate()
{
	EMMoveObject(EMMO_Duplicate, Fix0, Fix0, NULL, &Selection);
	return true;
}

void C4EditCursor::Draw(C4TargetFacet &cgo)
{
	ZoomDataStackItem zdsi(cgo.X, cgo.Y, cgo.Zoom);
	// Draw selection marks
	for (C4Object *cobj : Selection)
	{
		// target pos (parallax)
		float offX, offY, newzoom;
		cobj->GetDrawPosition(cgo, offX, offY, newzoom);
		ZoomDataStackItem zdsi(cgo.X, cgo.Y, newzoom);
		FLOAT_RECT frame =
		{
			offX+cobj->Shape.x,
			offX+cobj->Shape.x + cobj->Shape.Wdt,
			offY+cobj->Shape.y,
			offY+cobj->Shape.y + cobj->Shape.Hgt
		};
		DrawSelectMark(cgo, frame);
		// highlight selection if shift is pressed
		if (fShiftWasDown)
		{
			uint32_t dwOldMod = cobj->ColorMod;
			uint32_t dwOldBlitMode = cobj->BlitMode;
			cobj->ColorMod = 0xffffffff;
			cobj->BlitMode = C4GFXBLIT_CLRSFC_MOD2 | C4GFXBLIT_ADDITIVE;

			if(cobj->pMeshInstance)
				cobj->pMeshInstance->SetFaceOrdering(StdSubMeshInstance::FO_NearestToFarthest);

			cobj->Draw(cgo,-1);
			cobj->DrawTopFace(cgo, -1);

			if(cobj->pMeshInstance)
				cobj->pMeshInstance->SetFaceOrderingForClrModulation(cobj->ColorMod);

			cobj->ColorMod = dwOldMod;
			cobj->BlitMode = dwOldBlitMode;
		}
	}
	// Draw drag frame
	if (DragFrame)
		pDraw->DrawFrameDw(cgo.Surface,
		                               std::min(X, X2) + cgo.X - cgo.TargetX, std::min(Y, Y2) + cgo.Y - cgo.TargetY,
		                               std::max(X, X2) + cgo.X - cgo.TargetX, std::max(Y, Y2) + cgo.Y - cgo.TargetY, 0xffffffff);
	// Draw drag line
	if (DragLine)
		pDraw->DrawLineDw(cgo.Surface,
		                              X + cgo.X - cgo.TargetX, Y + cgo.Y - cgo.TargetY,
		                              X2 + cgo.X - cgo.TargetX, Y2 + cgo.Y - cgo.TargetY, 0xffffffff);
	// Draw drop target
	if (DropTarget)
		::GraphicsResource.fctDropTarget.Draw(cgo.Surface,
		                                      DropTarget->GetX() + cgo.X - cgo.TargetX - ::GraphicsResource.fctDropTarget.Wdt / 2,
		                                      DropTarget->GetY() + DropTarget->Shape.y + cgo.Y - cgo.TargetY - ::GraphicsResource.fctDropTarget.Hgt);
}


void C4EditCursor::DrawSelectMark(C4Facet &cgo, FLOAT_RECT frame)
{
	if ((cgo.Wdt<1) || (cgo.Hgt<1)) return;

	if (!cgo.Surface) return;

	const float EDGE_WIDTH = 2.f;

	const C4BltVertex vertices[] = {
		{ 0.f, 0.f, { 0xFF, 0xFF, 0xFF, 0xFF }, frame.left+EDGE_WIDTH, frame.top, 0.f },
		{ 0.f, 0.f, { 0xFF, 0xFF, 0xFF, 0xFF }, frame.left, frame.top, 0.f },
		{ 0.f, 0.f, { 0xFF, 0xFF, 0xFF, 0xFF }, frame.left, frame.top, 0.f },
		{ 0.f, 0.f, { 0xFF, 0xFF, 0xFF, 0xFF }, frame.left, frame.top+EDGE_WIDTH, 0.f },

		{ 0.f, 0.f, { 0xFF, 0xFF, 0xFF, 0xFF }, frame.left+EDGE_WIDTH, frame.bottom-1, 0.f },
		{ 0.f, 0.f, { 0xFF, 0xFF, 0xFF, 0xFF }, frame.left, frame.bottom-1, 0.f },
		{ 0.f, 0.f, { 0xFF, 0xFF, 0xFF, 0xFF }, frame.left, frame.bottom-1, 0.f },
		{ 0.f, 0.f, { 0xFF, 0xFF, 0xFF, 0xFF }, frame.left, frame.bottom-1-EDGE_WIDTH, 0.f },

		{ 0.f, 0.f, { 0xFF, 0xFF, 0xFF, 0xFF }, frame.right-1-EDGE_WIDTH, frame.top, 0.f },
		{ 0.f, 0.f, { 0xFF, 0xFF, 0xFF, 0xFF }, frame.right-1, frame.top, 0.f },
		{ 0.f, 0.f, { 0xFF, 0xFF, 0xFF, 0xFF }, frame.right-1, frame.top, 0.f },
		{ 0.f, 0.f, { 0xFF, 0xFF, 0xFF, 0xFF }, frame.right-1, frame.top+EDGE_WIDTH, 0.f },

		{ 0.f, 0.f, { 0xFF, 0xFF, 0xFF, 0xFF }, frame.right-1-EDGE_WIDTH, frame.bottom-1, 0.f },
		{ 0.f, 0.f, { 0xFF, 0xFF, 0xFF, 0xFF }, frame.right-1, frame.bottom-1, 0.f },
		{ 0.f, 0.f, { 0xFF, 0xFF, 0xFF, 0xFF }, frame.right-1, frame.bottom-1, 0.f },
		{ 0.f, 0.f, { 0xFF, 0xFF, 0xFF, 0xFF }, frame.right-1, frame.bottom-1-EDGE_WIDTH, 0.f },
	};

	const unsigned int n_vertices = sizeof(vertices) / sizeof(vertices[0]);

	pDraw->PerformMultiLines(cgo.Surface, vertices, n_vertices, 1., NULL);
}


void C4EditCursor::MoveSelection(C4Real XOff, C4Real YOff)
{
	EMMoveObject(EMMO_Move, XOff, YOff, NULL, &Selection);
}

void C4EditCursor::FrameSelection()
{
	ClearSelection();
	for (C4Object *cobj : Objects)
	{
		if (cobj->Status && cobj->OCF & OCF_NotContained)
		{
			if (Inside(cobj->GetX(),std::min(X,X2),std::max(X,X2)) && Inside(cobj->GetY(),std::min(Y,Y2),std::max(Y,Y2)))
				AddToSelection(cobj);
		}
	}
	OnSelectionChanged();
}

bool C4EditCursor::In(const char *szText)
{
	::Console.RegisterRecentInput(szText, C4Console::MRU_Object);
	EMMoveObject(EMMO_Script, Fix0, Fix0, NULL, &Selection, szText);
	::Console.PropertyDlgUpdate(Selection, true);
	return true;
}

void C4EditCursor::Default()
{
	fAltWasDown=false;
	fShiftWasDown=false;
	Mode=C4CNS_ModePlay;
	X=Y=X2=Y2=0;
	Target=DropTarget=NULL;
#ifdef USE_WIN32_WINDOWS
	hMenu=NULL;
#endif
	Hold=DragFrame=DragLine=false;
	Selection.Default();
}

void C4EditCursor::Clear()
{
#ifdef USE_WIN32_WINDOWS
	if (hMenu) DestroyMenu(hMenu); hMenu=NULL;
#endif
#ifdef WITH_DEBUG_MODE
	ObjselectDelItems();
#endif
	Selection.Clear();
}

bool C4EditCursor::SetMode(int32_t iMode)
{
	// Store focus
#ifdef USE_WIN32_WINDOWS
	HWND hFocus=GetFocus();
#endif
	// Update console buttons (always)
	Console.UpdateModeCtrls(iMode);
	// No change
	if (iMode==Mode) return true;
	// Set mode
	Mode = iMode;
	// Update prop tools by mode
	switch (Mode)
	{
	case C4CNS_ModeEdit: case C4CNS_ModePlay:
		Console.ToolsDlgClose();
		break;
	case C4CNS_ModeDraw:
		Console.PropertyDlgClose();
		break;
	}
	if (Mode == C4CNS_ModePlay)
	{
		::MouseControl.ShowCursor();
	}
	else
	{
		OpenPropTools();
		::MouseControl.HideCursor();
	}
	// Restore focus
#ifdef USE_WIN32_WINDOWS
	SetFocus(hFocus);
#endif
	// Done
	return true;
}

bool C4EditCursor::ToggleMode()
{

	if (!EditingOK()) return false;

	// Step through modes
	int32_t iNewMode;
	switch (Mode)
	{
	case C4CNS_ModePlay: iNewMode=C4CNS_ModeEdit; break;
	case C4CNS_ModeEdit: iNewMode=C4CNS_ModeDraw; break;
	case C4CNS_ModeDraw: iNewMode=C4CNS_ModePlay; break;
	default:             iNewMode=C4CNS_ModePlay; break;
	}

	// Set new mode
	SetMode(iNewMode);

	return true;

}

void C4EditCursor::ApplyToolBrush()
{
	if (!EditingOK()) return;
	C4ToolsDlg *pTools=&Console.ToolsDlg;
	// execute/send control
	EMControl(CID_EMDrawTool, new C4ControlEMDrawTool(EMDT_Brush, ::Landscape.Mode, X,Y,0,0, pTools->Grade, pTools->Material, pTools->Texture, pTools->BackMaterial, pTools->BackTexture));
}

void C4EditCursor::ApplyToolLine()
{
	if (!EditingOK()) return;
	C4ToolsDlg *pTools=&Console.ToolsDlg;
	// execute/send control
	EMControl(CID_EMDrawTool, new C4ControlEMDrawTool(EMDT_Line, ::Landscape.Mode, X,Y,X2,Y2, pTools->Grade, pTools->Material,pTools->Texture, pTools->BackMaterial, pTools->BackTexture));
}

void C4EditCursor::ApplyToolRect()
{
	if (!EditingOK()) return;
	C4ToolsDlg *pTools=&Console.ToolsDlg;
	// execute/send control
	EMControl(CID_EMDrawTool, new C4ControlEMDrawTool(EMDT_Rect, ::Landscape.Mode, X,Y,X2,Y2, pTools->Grade, pTools->Material, pTools->Texture, pTools->BackMaterial, pTools->BackTexture));
}

void C4EditCursor::ApplyToolFill()
{
	if (!EditingOK()) return;
	C4ToolsDlg *pTools=&Console.ToolsDlg;
	// execute/send control
	EMControl(CID_EMDrawTool, new C4ControlEMDrawTool(EMDT_Fill, ::Landscape.Mode, X,Y,0,Y2, pTools->Grade, pTools->Material, NULL, NULL, NULL));
}

bool C4EditCursor::DoContextMenu(DWORD dwKeyState)
{
	bool fObjectSelected = !!Selection.ObjectCount();
#ifdef USE_WIN32_WINDOWS
	POINT point; GetCursorPos(&point);
	HMENU hContext = GetSubMenu(hMenu,0);
	SetMenuItemEnable( hContext, IDM_VIEWPORT_DELETE, fObjectSelected && Console.Editing);
	SetMenuItemEnable( hContext, IDM_VIEWPORT_DUPLICATE, fObjectSelected && Console.Editing);
	SetMenuItemEnable( hContext, IDM_VIEWPORT_CONTENTS, fObjectSelected && Selection.GetObject()->Contents.ObjectCount() && Console.Editing);
	SetMenuItemText(hContext,IDM_VIEWPORT_DELETE,LoadResStr("IDS_MNU_DELETE"));
	SetMenuItemText(hContext,IDM_VIEWPORT_DUPLICATE,LoadResStr("IDS_MNU_DUPLICATE"));
	SetMenuItemText(hContext,IDM_VIEWPORT_CONTENTS,LoadResStr("IDS_MNU_CONTENTS"));

	// Add selection and custom command entries for any objects at the cursor
	ObjselectDelItems(); // clear previous entries
	C4FindObjectAtPoint pFO(X,Y);
	C4ValueArray * atcursor; atcursor = pFO.FindMany(::Objects, ::Objects.Sectors); // needs freeing (single object ptr)
	int itemcount = atcursor->GetSize();
	if(itemcount > 0)
	{
		// Count required entries for all objects and their custom commands
		int entrycount = itemcount;
		for (int i_item = 0; i_item < itemcount; ++i_item)
		{
			C4Object *pObj = (*atcursor)[i_item].getObj(); assert(pObj);
			C4ValueArray *custom_commands = pObj->GetPropertyArray(P_EditCursorCommands);
			if (custom_commands) entrycount += custom_commands->GetSize();
		}
		// If too many entries would be shown, add a "..." in the end
		const int maxentries = 25; // Maximum displayed objects. if you raise it, also change note with IDM_VPORTDYN_FIRST in resource.h
		bool has_too_many_entries = (entrycount > maxentries);
		if (has_too_many_entries) entrycount = maxentries + 1;
		itemsObjselect.resize(entrycount + 1); // +1 for a separator
		// Add a separator bar
		itemsObjselect[0].ItemId = IDM_VPORTDYN_FIRST;
		itemsObjselect[0].Object = NULL;
		itemsObjselect[0].Command.Clear();
		AppendMenu(hContext, MF_SEPARATOR, IDM_VPORTDYN_FIRST, NULL);
		// Add all objects
		int i_entry = 0;
		for (int i_item = 0; i_item < itemcount; ++i_item)
		{
			++i_entry; if (i_entry >= maxentries) break;
			// Add selection entry
			C4Object *obj = (*atcursor)[i_item].getObj();
			itemsObjselect[i_entry].ItemId = IDM_VPORTDYN_FIRST + i_entry;
			itemsObjselect[i_entry].Object = obj;
			itemsObjselect[i_entry].Command.Clear();
			AppendMenu(hContext, MF_STRING, IDM_VPORTDYN_FIRST + i_entry, FormatString("%s #%i (%i/%i)", obj->GetName(), obj->Number, obj->GetX(), obj->GetY()).GetWideChar());
			// Add custom command entries
			C4ValueArray *custom_commands = obj->GetPropertyArray(P_EditCursorCommands);
			if (custom_commands) for (int i_cmd = 0; i_cmd < custom_commands->GetSize(); ++i_cmd)
			{
				++i_entry; if (i_entry >= maxentries) break;
				const C4Value &cmd = custom_commands->GetItem(i_cmd);
				StdStrBuf custom_command_szstr; C4AulFunc *custom_command; C4String *custom_command_string;
				// Custom command either by string or by function pointer
				if ((custom_command = cmd.getFunction()))
					custom_command_szstr.Format("%s()", custom_command->GetName());
				else if ((custom_command_string = cmd.getStr()))
					custom_command_szstr.Copy(custom_command_string->GetData()); // copy just in case script get reloaded inbetween
				if (custom_command_szstr.getLength())
				{
					itemsObjselect[i_entry].ItemId = IDM_VPORTDYN_FIRST + i_entry;
					itemsObjselect[i_entry].Object = obj;
					itemsObjselect[i_entry].Command.Take(custom_command_szstr); 
					AppendMenu(hContext, MF_STRING, IDM_VPORTDYN_FIRST + i_entry, FormatString("%s->%s", obj->GetName(), custom_command_szstr.getData()).GetWideChar());
				}
				else
				{
					// invalid entry in commands list. do not create a menu item.
					itemsObjselect[i_entry].ItemId = 0;
				}
			}
		}
		if (has_too_many_entries)
		{
			AppendMenu(hContext, MF_GRAYED, IDM_VPORTDYN_FIRST + maxentries + 1, L"...");
			itemsObjselect[maxentries + 1].ItemId = IDM_VPORTDYN_FIRST + maxentries + 1;
			itemsObjselect[maxentries + 1].Object = NULL;
			itemsObjselect[maxentries + 1].Command.Clear();
		}
	}
	delete atcursor;

	int32_t iItem = TrackPopupMenu(
	                  hContext,
	                  TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_LEFTBUTTON | TPM_NONOTIFY,
	                  point.x,point.y, 0,
	                  Console.hWindow,
	                  NULL);
	switch (iItem)
	{
	case IDM_VIEWPORT_DELETE: Delete(); break;
	case IDM_VIEWPORT_DUPLICATE: Duplicate(); break;
	case IDM_VIEWPORT_CONTENTS: GrabContents(); break;
	case 0: break;
	default:
		for(std::vector<ObjselItemDt>::iterator it = itemsObjselect.begin() + 1; it != itemsObjselect.end(); ++it)
			if(it->ItemId == iItem)
			{
				if (it->Command.getLength())
					DoContextObjCommand(it->Object, it->Command.getData());
				else
					DoContextObjsel(it->Object, (dwKeyState & MK_SHIFT) == 0);
				break;
			}
		break;
	}
	ObjselectDelItems();
#else
#ifdef WITH_DEVELOPER_MODE
	gtk_widget_set_sensitive(itemDelete, fObjectSelected && Console.Editing);
	gtk_widget_set_sensitive(itemDuplicate, fObjectSelected && Console.Editing);
	gtk_widget_set_sensitive(itemGrabContents, fObjectSelected && Selection.GetObject()->Contents.ObjectCount() && Console.Editing);

	ObjselectDelItems();
	C4FindObjectAtPoint pFO(X,Y);
	C4ValueArray * atcursor; atcursor = pFO.FindMany(::Objects, ::Objects.Sectors); // needs freeing
	int itemcount = atcursor->GetSize();
	if(itemcount > 0)
	{
		itemsObjselect.resize(itemcount+1); // +1 for a separator
		itemsObjselect[0].MenuItem = gtk_separator_menu_item_new();
		itemsObjselect[0].EditCursor = this;
		gtk_menu_shell_append(GTK_MENU_SHELL(menuContext), itemsObjselect[0].MenuItem);
		int i = 0;
		for(std::vector<ObjselItemDt>::iterator it = itemsObjselect.begin() + 1; it != itemsObjselect.end(); ++it, ++i)
		{
			it->EditCursor = this;
			C4Object * obj = (*atcursor)[i].getObj();
			assert(obj);
			it->Object = obj;
			GtkWidget * wdg = gtk_menu_item_new_with_label(FormatString("%s #%i (%i/%i)", obj->GetName(), obj->Number, obj->GetX(), obj->GetY()).getData());
			it->MenuItem = wdg;
			gtk_menu_shell_append(GTK_MENU_SHELL(menuContext), wdg);
			g_signal_connect(G_OBJECT(wdg), "activate", G_CALLBACK(OnObjselect), &*it);
		}
	}
	delete atcursor;
	gtk_widget_show_all(menuContext);

	gtk_menu_popup(GTK_MENU(menuContext), NULL, NULL, NULL, NULL, 3, 0);
#endif
#endif
	return true;
}

void C4EditCursor::GrabContents()
{
	// Set selection
	C4Object *pFrom;
	if (!( pFrom = Selection.GetObject() )) return;
	Selection.Copy(pFrom->Contents);
	OnSelectionChanged();
	Hold=true;

	// Exit all objects
	EMMoveObject(EMMO_Exit, Fix0, Fix0, NULL, &Selection);
}

void C4EditCursor::UpdateDropTarget(DWORD dwKeyState)
{

	DropTarget=NULL;

	if (dwKeyState & MK_CONTROL)
		if (Selection.GetObject())
			for (C4Object *cobj : Objects)
			{
				if (cobj->Status)
					if (!cobj->Contained)
						if (Inside<int32_t>(X-(cobj->GetX()+cobj->Shape.x),0,cobj->Shape.Wdt-1))
							if (Inside<int32_t>(Y-(cobj->GetY()+cobj->Shape.y),0,cobj->Shape.Hgt-1))
								if (!Selection.GetLink(cobj))
									{ DropTarget=cobj; break; }
			}

}

void C4EditCursor::PutContents()
{
	if (!DropTarget) return;
	EMMoveObject(EMMO_Enter, Fix0, Fix0, DropTarget, &Selection);
}

C4Object *C4EditCursor::GetTarget()
{
	return Target;
}

bool C4EditCursor::EditingOK()
{
	if (!Console.Editing)
	{
		Hold=false;
		Console.Message(LoadResStr("IDS_CNS_NONETEDIT"));
		return false;
	}
	return true;
}

int32_t C4EditCursor::GetMode()
{
	return Mode;
}

void C4EditCursor::ToolFailure()
{
	C4ToolsDlg *pTools=&Console.ToolsDlg;
	Hold=false;
	Console.Message(FormatString(LoadResStr("IDS_CNS_NOMATDEF"),pTools->Material,pTools->Texture).getData());
}

void C4EditCursor::ApplyToolPicker()
{
	int32_t iMaterial;
	BYTE byIndex;
	switch (::Landscape.Mode)
	{
	case C4LSC_Static:
		{
			bool material_set = false;
			int32_t x = X/::Landscape.MapZoom;
			int32_t y = Y/::Landscape.MapZoom;
			// Material-texture from map
			if ((byIndex = ::Landscape.GetMapIndex(x, y)))
			{
				const C4TexMapEntry *pTex = ::TextureMap.GetEntry(byIndex);
				if (pTex && pTex->GetMaterialName() && *pTex->GetMaterialName())
				{
					const BYTE byIndexBkg = Landscape.GetBackMapIndex(x, y);
					Console.ToolsDlg.SelectMaterial(pTex->GetMaterialName());
					Console.ToolsDlg.SelectTexture(pTex->GetTextureName());

					// Set background index if GUI backend supports it
					if (Console.ToolsDlg.ModeBack)
					{
						const C4TexMapEntry *pBgTex = ::TextureMap.GetEntry(byIndexBkg);
						if (pBgTex && !pBgTex->isNull())
						{
							Console.ToolsDlg.SelectBackMaterial(pBgTex->GetMaterialName());
							Console.ToolsDlg.SelectBackTexture(pBgTex->GetTextureName());
						}
						else
						{
							Console.ToolsDlg.SelectBackMaterial(C4TLS_MatSky);
						}
					}
					else
					{
						Console.ToolsDlg.SetIFT(byIndexBkg != 0);
					}

					material_set = true;
				}
			}
			// default to sky, because invalid materials are always rendered as sky
			if (!material_set) Console.ToolsDlg.SelectMaterial(C4TLS_MatSky);
			break;
		}
	case C4LSC_Exact:
		// Material only from landscape
		if (MatValid(iMaterial=GBackMat(X,Y)))
		{
			Console.ToolsDlg.SelectMaterial(::MaterialMap.Map[iMaterial].Name);
			Console.ToolsDlg.SetIFT(Landscape.GetBackPix(X, Y) != 0);
		}
		else
			Console.ToolsDlg.SelectMaterial(C4TLS_MatSky);
		break;
	}
	Hold=false;
}

void C4EditCursor::EMMoveObject(C4ControlEMObjectAction eAction, C4Real tx, C4Real ty, C4Object *pTargetObj, const C4ObjectList *pObjs, const char *szScript)
{
	// construct object list
	int32_t iObjCnt = 0; int32_t *pObjIDs = NULL;
	if (pObjs && (iObjCnt = pObjs->ObjectCount()))
	{
		pObjIDs = new int32_t [iObjCnt];
		// fill
		int32_t i = 0;
		for (C4Object *obj : *pObjs)
		{
			if (obj && obj->Status)
				pObjIDs[i++] = obj->Number;
			else
				pObjIDs[i++] = 0;
		}
	}

	// execute control
	EMControl(CID_EMMoveObj, new C4ControlEMMoveObject(eAction, tx, ty, pTargetObj, iObjCnt, pObjIDs, szScript));

}

void C4EditCursor::EMControl(C4PacketType eCtrlType, C4ControlPacket *pCtrl)
{
	::Control.DoInput(eCtrlType, pCtrl, CDT_Decide);
}

#ifdef WITH_DEVELOPER_MODE
// GTK+ callbacks
void C4EditCursor::OnDelete(GtkWidget* widget, gpointer data)
{
	static_cast<C4EditCursor*>(data)->Delete();
}

void C4EditCursor::OnDuplicate(GtkWidget* widget, gpointer data)
{
	static_cast<C4EditCursor*>(data)->Duplicate();
}

void C4EditCursor::OnGrabContents(GtkWidget* widget, gpointer data)
{
	static_cast<C4EditCursor*>(data)->GrabContents();
}

void C4EditCursor::OnObjselect(GtkWidget* widget, gpointer data)
{
	bool IsShiftDown = false;
	GdkEvent* event = gtk_get_current_event();
	if(event)
	{
		if(event->type == GDK_BUTTON_PRESS)
			IsShiftDown = ( ((GdkEventButton*)event)->state & MK_SHIFT) != 0;
		else if(event->type == GDK_KEY_PRESS)
			IsShiftDown = ( ((GdkEventKey*)event)->state & MK_SHIFT) != 0;

		gdk_event_free(event);
	}

	static_cast<ObjselItemDt*>(data)->EditCursor->DoContextObjsel(static_cast<ObjselItemDt*>(data)->Object, !IsShiftDown);
	static_cast<ObjselItemDt*>(data)->EditCursor->ObjselectDelItems();
}

#endif

void C4EditCursor::ObjselectDelItems() {
	if(!itemsObjselect.size()) return;
	std::vector<ObjselItemDt>::iterator it = itemsObjselect.begin();
	while(it != itemsObjselect.end()) {
		#if defined(WITH_DEVELOPER_MODE)
		gtk_widget_destroy(it->MenuItem);
		#elif defined(USE_WIN32_WINDOWS)
		if(!it->ItemId) { ++it; continue; }
		HMENU hContext = GetSubMenu(hMenu,0);
		DeleteMenu(hContext, it->ItemId, MF_BYCOMMAND);
		#endif
		++it;
	}
	itemsObjselect.resize(0);
}

bool C4EditCursor::AltDown()
{
	// alt only has an effect in draw mode (picker)
	if (Mode == C4CNS_ModeDraw)
	{
		Console.ToolsDlg.SetAlternateTool();
	}
	// key not processed - allow further usages of Alt
	return false;
}

bool C4EditCursor::AltUp()
{
	if (Mode == C4CNS_ModeDraw)
	{
		Console.ToolsDlg.ResetAlternateTool();
	}
	// key not processed - allow further usages of Alt
	return false;
}

void C4EditCursor::DoContextObjsel(C4Object * obj, bool clear)
{
	if(clear)
		Selection.Clear();

	Selection.Add(obj, C4ObjectList::stNone);
	OnSelectionChanged();
}

void C4EditCursor::DoContextObjCommand(C4Object * obj, const char *cmd)
{
	// Command going through queue for sync
	if (!obj || !cmd) return;
	In(FormatString("Object(%d)->%s", obj->Number, cmd).getData());
}
