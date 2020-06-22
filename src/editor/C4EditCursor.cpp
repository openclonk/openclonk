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

#include "C4Include.h"
#include "editor/C4EditCursor.h"

#include "editor/C4Console.h"
#include "object/C4Def.h"
#include "object/C4Object.h"
#include "game/C4Application.h"
#include "lib/C4Random.h"
#include "gui/C4MouseControl.h"
#include "landscape/C4Landscape.h"
#include "landscape/C4Texture.h"
#include "graphics/C4GraphicsResource.h"
#include "object/C4GameObjects.h"
#include "control/C4GameControl.h"
#include "script/C4AulExec.h"
#ifdef WITH_QT_EDITOR
#include "editor/C4ConsoleQtShapes.h"
#endif
#include "lib/StdMesh.h"

#ifdef _WIN32
#include "res/resource.h"
#endif


StdStrBuf C4EditCursorSelection::GetDataString() const
{
	StdStrBuf Output;
	// Compose info text by selected object(s)
	int32_t obj_count = size();
	switch (obj_count)
	{
		// No selection
	case 0:
		Output = LoadResStr("IDS_CNS_NOOBJECT");
		break;
		// One selected object
	case 1:
	{
		C4Object *obj = GetObject();
		if (obj)
			Output.Take(obj->GetDataString());
		else
			Output.Take(front().GetDataString());
		break;
	}
		// Multiple selected objects
	default:
		Output.Format(LoadResStr("IDS_CNS_MULTIPLEOBJECTS"), obj_count);
		break;
	}
	return Output;
}

C4Object *C4EditCursorSelection::GetObject(int32_t index) const
{
	// Get indexed C4Object * in list
	C4Object *obj;
	for (const C4Value &v : (*this))
		if ((obj = v.getObj()))
			if (!index--)
				return obj;
	return nullptr;
}

C4Object *C4EditCursorSelection::GetLastObject() const
{
	C4Object *obj, *last = nullptr;
	for (const C4Value &v : (*this))
		if ((obj = v.getObj()))
			last = obj;
	return last;
}

void C4EditCursorSelection::ConsolidateEmpty()
{
	// remove nullptred entries that may happen because objects got deleted
	this->remove(C4VNull);
}

bool C4EditCursorSelection::ClearPointers(C4Object *obj)
{
	bool found = false;
	for (C4Value &v : (*this))
		if (obj == v.getObj())
		{
			found = true;
			v.Set0();
		}
	if (found) ConsolidateEmpty();
	return found;
}

bool C4EditCursorSelection::IsContained(C4PropList *obj) const
{
	for (const C4Value &v : (*this)) if (obj == v.getPropList()) return true;
	return false;
}

int32_t C4EditCursorSelection::ObjectCount() const
{
	// count only C4Object *
	int32_t count = 0;
	for (const C4Value &v : *this) if (v.getObj()) ++count;
	return count;
}


C4EditCursor::C4EditCursor()
#ifdef WITH_QT_EDITOR
	: shapes(new C4ConsoleQtShapes())
#endif
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
			EMMoveObject(fShiftWasDown ? EMMO_MoveForced : EMMO_Move, Fix0, Fix0, nullptr, &selection, nullptr, false);
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
	if (!::Game.iTick35 || ::Console.EditCursor.IsSelectionInvalidated())
	{
		selection.ConsolidateEmpty();
		Console.PropertyDlgUpdate(selection, false);
	}
}

bool C4EditCursor::Init()
{

#ifdef USE_WIN32_WINDOWS
	if (!(hMenu = LoadMenu(Application.GetInstance(),MAKEINTRESOURCE(IDR_CONTEXTMENUS))))
		return false;
#endif
	Console.UpdateModeCtrls(Mode);

	return true;
}

void C4EditCursor::ClearPointers(C4Object *pObj)
{
	if (Target==pObj) Target=nullptr;
	if (selection.ClearPointers(pObj))
		OnSelectionChanged();
}

bool C4EditCursor::Move(float iX, float iY, float iZoom, DWORD dwKeyState)
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
	X = iX; Y = iY; Zoom = iZoom;

	// Drag rotation/scale of object
	if (DragTransform)
	{
		C4Object *obj = selection.GetObject();
		if (obj)
		{
			int32_t new_rot = (DragRot0 + int32_t(float(X - X2)*Zoom)) % 360;
			if (new_rot < 0) new_rot += 360;
			if (fShiftIsDown) new_rot = (new_rot + 23) / 45 * 45;
			int32_t new_con = DragCon0 + int32_t(float(Y2 - Y)*Zoom*(FullCon / 200));
			int32_t con_step = FullCon / 5;
			if (fShiftIsDown) new_con = (new_con + con_step/2) / con_step * con_step;
			if (!obj->Def->Oversize) new_con = std::min<int32_t>(new_con, FullCon);
			new_con = std::max<int32_t>(new_con, fShiftIsDown ? 1 : con_step);
			bool any_change = false;
			if (obj->Def->Rotateable)
				if (new_rot != DragRotLast)
					any_change = true;
			if (obj->Def->GrowthType)
				if (new_con != DragConLast)
					any_change = true;
			if (any_change)
			{
				EMMoveObject(EMMO_Transform, itofix(new_rot, 1), itofix(new_con, FullCon/100), obj, nullptr);
				DragRotLast = new_rot;
				DragConLast = new_con;
			}
		}
		
	}

	switch (Mode)
	{
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case C4CNS_ModeEdit:
#ifdef WITH_QT_EDITOR
		shapes->MouseMove(X, Y, Hold, 3.0f/Zoom, !!(dwKeyState & MK_SHIFT), !!(dwKeyState & MK_CONTROL));
#endif
		// Hold
		if (!DragFrame && Hold && !DragShape && !DragTransform)
		{
			MoveSelection(ftofix(xoff),ftofix(yoff), false);
			UpdateDropTarget(dwKeyState);
		}
		// Update target
		// Shift always indicates a target outside the current selection
		else
		{
			Target = (dwKeyState & MK_SHIFT) ? selection.GetLastObject() : nullptr;
			do
			{
				Target = Game.FindObject(nullptr,X,Y,0,0,OCF_NotContained, Target);
			}
			while ((dwKeyState & MK_SHIFT) && Target && selection.IsContained(Target));
		}
		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case C4CNS_ModeCreateObject:
		// Drop target for contained object creation
		UpdateDropTarget(dwKeyState);
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

bool C4EditCursor::Move(DWORD new_key_state)
{
	// Move at last position with new key state
	return Move(X, Y, Zoom, new_key_state);
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
	case C4CNS_ModeCreateObject:
		str.Format(LoadResStr("IDS_CNS_CREATESTATUS"), creator_def ? (creator_def->GetName()) : LoadResStr("IDS_CNS_NOTHING"));
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

void C4EditCursor::OnSelectionChanged(bool by_objectlist)
{
	::Console.PropertyDlgUpdate(selection, true);
	if (!by_objectlist) ::Console.ObjectListDlg.Update(selection);
}

void C4EditCursor::AddToSelection(C4PropList *add_proplist)
{
	if (!add_proplist) return;
	C4Object *add_obj = add_proplist->GetObject();
	if (add_obj && !add_obj->Status) return;
	if (selection.IsContained(add_proplist)) return;
	// add object to selection and do script callback
	selection.push_back(C4VPropList(add_proplist));
}

bool C4EditCursor::RemoveFromSelection(C4PropList *remove_proplist)
{
	if (!remove_proplist) return false;
	C4Object *remove_obj = remove_proplist->GetObject();
	if (remove_obj && !remove_obj->Status) return false;
	// remove object from selection and do script callback
	if (!selection.IsContained(remove_proplist)) return false;
	selection.remove(C4VPropList(remove_proplist));
	return true;
}

void C4EditCursor::ClearSelection(C4PropList *next_selection)
{
	// remove everything from selection
	selection.clear();
}

bool C4EditCursor::LeftButtonDown(DWORD dwKeyState)
{

	// Hold
	Hold=true;

	switch (Mode)
	{
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case C4CNS_ModeEdit:
		// Click on shape?
#ifdef WITH_QT_EDITOR
		if (shapes->MouseDown(X, Y, 3.0f / Zoom, !!(dwKeyState & MK_SHIFT), !!(dwKeyState & MK_CONTROL)))
		{
			DragShape = shapes->IsDragging();
			break;
		}
#endif
		if (dwKeyState & MK_CONTROL)
		{
			// Toggle target
			if (Target)
				if (!RemoveFromSelection(Target))
					AddToSelection(Target);
		}
		else
		{
			// Click rotate/scale marker?
			if (IsHoveringTransformMarker())
			{
				DragTransform = true;
				X2 = X; Y2 = Y;
				C4Object *dragged_obj = selection.GetObject();
				DragRot0 = DragRotLast = dragged_obj->GetR();
				DragCon0 = DragConLast = dragged_obj->GetCon();
				break;
			}
			// Click on unselected: select single
			if (Target)
			{
				bool found = false;
				for (C4Value &obj : selection)
				{
					if(obj.getObj() && obj.getObj()->At(X, Y))
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
		OnSelectionChanged();
		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case C4CNS_ModeCreateObject:
		ApplyCreateObject(!!(dwKeyState & MK_CONTROL));
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

	DropTarget=nullptr;

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
			for (C4Value &obj : selection)
			{
				if (obj.getObj() && obj.getObj()->At(X,Y))
				{
					fCursorIsOnSelection = true;
					break;
				}
			}
			if (!fCursorIsOnSelection)
			{
				// Click on unselected
				if (Target && !selection.IsContained(Target))
				{
					ClearSelection(Target); AddToSelection(Target);
				}
				// Click on nothing
				if (!Target) ClearSelection();
			}
		}
		OnSelectionChanged();
		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	}

	return true;
}

bool C4EditCursor::LeftButtonUp(DWORD dwKeyState)
{
	// Finish edit/tool
	switch (Mode)
	{
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case C4CNS_ModeEdit:
		// Finish object drag
		if (!DragFrame && Hold && !DragShape && !DragTransform)
		{
			MoveSelection(Fix0, Fix0, true);
		}
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
#ifdef WITH_QT_EDITOR
	shapes->MouseUp(X, Y, !!(dwKeyState & MK_SHIFT), !!(dwKeyState & MK_CONTROL));
#endif
	Hold=false;
	DragFrame=false;
	DragLine=false;
	DragShape = false;
	DragTransform = false;
	DropTarget=nullptr;
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
	Target=nullptr;
#ifndef WITH_QT_EDITOR
	DoContextMenu(dwKeyState);
#endif
	// Update
	UpdateStatusBar();

	return true;
}

bool C4EditCursor::Delete()
{
	if (!EditingOK()) return false;
	EMMoveObject(EMMO_Remove, Fix0, Fix0, nullptr, &selection);
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
		Console.PropertyDlgUpdate(selection, false);
		break;
	case C4CNS_ModeDraw:
		Console.ToolsDlg.Open();
		break;
	}
	return true;
}

bool C4EditCursor::Duplicate()
{
	EMMoveObject(EMMO_Duplicate, Fix0, Fix0, nullptr, &selection);
	return true;
}

void C4EditCursor::PerformDuplication(int32_t *object_numbers, int32_t object_count, bool local_call)
{
	if (!object_count) return;
	// Remember last OEI so duplicated objects can be determined
	int32_t prev_oei = C4PropListNumbered::GetEnumerationIndex();
	// Get serialized objects
	C4RefCntPointer<C4ValueArray> object_numbers_c4v = new C4ValueArray();
	object_numbers_c4v->SetSize(object_count);
	int32_t total_object_count = 0;
	for (int32_t i = 0; i < object_count; ++i)
	{
		C4Object *obj = ::Objects.SafeObjectPointer(object_numbers[i]);
		if (!obj) continue;
		total_object_count = obj->AddObjectAndContentsToArray(object_numbers_c4v, total_object_count);
	}
	object_numbers_c4v->SetSize(total_object_count);
	int32_t objects_file_handle = ::ScriptEngine.CreateUserFile();
	C4AulParSet pars(C4VInt(objects_file_handle), C4VArray(object_numbers_c4v.Get()));
	C4Value result_c4v(::ScriptEngine.GetPropList()->Call(PSF_SaveScenarioObjects, &pars));
	bool result = !!result_c4v;
	if (result_c4v.GetType() == C4V_Nil)
	{
		// Function returned nil: This usually means there was a script error during object writing.
		// It could also mean the scripter overloaded global func SaveScenarioObjects and returned nil.
		// In either case, no the objects file will contain garbage so no objects were duplicated.
		LogF("ERROR: No valid result from global func " PSF_SaveScenarioObjects ". Regular object duplication failed.");
	}
	else
	{
		// Function completed successfully (returning true or false)
		C4AulUserFile *file = ::ScriptEngine.GetUserFile(objects_file_handle);
		if (!result || !file || !file->GetFileLength())
		{
			// Nothing written? Then we don't have objects.
			// That's OK; not an error.
		}
		else
		{
			// Create copy of objects by executing duplication script
			StdStrBuf data = file->GrabFileContents();
			AulExec.DirectExec(&::ScriptEngine, data.getData(), "object duplication", false, nullptr, true);
		}
	}
	::ScriptEngine.CloseUserFile(objects_file_handle);
	// Did duplication work?
	bool any_duplicates = false;
	for (C4Object *obj : ::Objects)
	{
		if (obj->Number > prev_oei)
		{
			any_duplicates = true;
			break;
		}
	}
	// If duplication created no objects, the user probably tried to copy a non-saved object
	// Just copy the old way then
	if (!any_duplicates)
	{
		PerformDuplicationLegacy(object_numbers, object_count, local_call);
		return;
	}
	// Update local client status: Put new objects into selection
	if (local_call)
	{
		selection.clear();
		int64_t X_all = 0, Y_all = 0, n_selected = 0;
		float old_x = X, old_y = Y;
		for (C4Object *obj : ::Objects)
		{
			if (obj->Number > prev_oei)
			{
				selection.push_back(C4VObj(obj));
				X_all += obj->GetX();
				Y_all += obj->GetY();
				++n_selected;
			}
		}
		// Reset EditCursor pos to center of duplicated objects, so they will be dragged along with the cursor
		if (n_selected)
		{
			X = X_all / n_selected;
			Y = Y_all / n_selected;
		}
		SetHold(true);
		OnSelectionChanged();
		// Ensure duplicated objects moved to the cursor without extra mouse movement
		// Move with shift key pressed to allow initial shift of HorizontalFixed items
		DWORD last_key_state = MK_SHIFT;
		if (fAltWasDown) last_key_state |= MK_ALT;
		bool shift_was_down = fShiftWasDown;
		Move(old_x, old_y, Zoom, last_key_state);
		fShiftWasDown = shift_was_down;
	}
}

void C4EditCursor::PerformDuplicationLegacy(int32_t *pObjects, int32_t iObjectNum, bool fLocalCall)
{
	// Old-style object copying: Just create new objects at old object position with same prototype
	C4Object *pOldObj, *pObj;
	for (int i = 0; i<iObjectNum; ++i)
		if ((pOldObj = ::Objects.SafeObjectPointer(pObjects[i])))
		{
			pObj = Game.CreateObject(pOldObj->GetPrototype(), pOldObj, pOldObj->Owner, pOldObj->GetX(), pOldObj->GetY());
			if (pObj && pObj->Status)
			{
				// local call? adjust selection then
				// do callbacks for all clients for sync reasons
				if (fLocalCall) selection.push_back(C4VObj(pObj));
				C4AulParSet pars(C4VObj(pObj));
				if (pOldObj->Status) pOldObj->Call(PSF_EditCursorDeselection, &pars);
				if (pObj->Status) pObj->Call(PSF_EditCursorSelection);
			}
		}
	// update status
	if (fLocalCall)
	{
		for (int i = 0; i<iObjectNum; ++i)
			if ((pOldObj = ::Objects.SafeObjectPointer(pObjects[i])))
				selection.remove(C4VObj(pOldObj));
		SetHold(true);
		OnSelectionChanged();
	}
}

void C4EditCursor::DrawObject(C4TargetFacet &cgo, C4Object *cobj, uint32_t select_mark_color, bool highlight, bool draw_transform_marker)
{
	// target pos (parallax)
	float line_width = std::max<float>(1.0f, 1.0f / cgo.Zoom);
	float offX, offY, newzoom;
	cobj->GetDrawPosition(cgo, offX, offY, newzoom);
	ZoomDataStackItem zdsi(cgo.X, cgo.Y, newzoom);
	if (select_mark_color)
	{
		FLOAT_RECT frame =
		{
			offX + cobj->Shape.x,
			offX + cobj->Shape.x + cobj->Shape.Wdt,
			offY + cobj->Shape.y,
			offY + cobj->Shape.y + cobj->Shape.Hgt
		};
		DrawSelectMark(cgo, frame, line_width, select_mark_color);
	}
	if (highlight)
	{
		uint32_t dwOldMod = cobj->ColorMod;
		uint32_t dwOldBlitMode = cobj->BlitMode;
		cobj->ColorMod = 0xffffffff;
		cobj->BlitMode = C4GFXBLIT_CLRSFC_MOD2 | C4GFXBLIT_ADDITIVE;

		if (cobj->pMeshInstance)
			cobj->pMeshInstance->SetFaceOrdering(StdSubMeshInstance::FO_NearestToFarthest);

		cobj->Draw(cgo, -1);
		cobj->DrawTopFace(cgo, -1);

		if (cobj->pMeshInstance)
			cobj->pMeshInstance->SetFaceOrderingForClrModulation(cobj->ColorMod);

		cobj->ColorMod = dwOldMod;
		cobj->BlitMode = dwOldBlitMode;
	}
	// Transformer knob
	if (draw_transform_marker)
	{
		float transform_marker_x = 0.0f, transform_marker_y = 0.0f;
		if (HasTransformMarker(&transform_marker_x, &transform_marker_y, cgo.Zoom))
		{
			transform_marker_x += offX; transform_marker_y += offY;
			float sz = float(::GraphicsResource.fctTransformKnob.Hgt) / cgo.Zoom;
			C4Facet transform_target_sfc(cgo.Surface, transform_marker_x-sz/2, transform_marker_y-sz/2, sz, sz);
			::GraphicsResource.fctTransformKnob.Draw(transform_target_sfc);
			// Transform knob while dragging
			if (DragTransform)
			{
				pDraw->SetBlitMode(C4GFXBLIT_ADDITIVE);
				transform_target_sfc.X += X - X2;
				transform_target_sfc.Y += Y - Y2;
				::GraphicsResource.fctTransformKnob.Draw(transform_target_sfc);
				pDraw->ResetBlitMode();
			}
		}
	}
}

bool C4EditCursor::HasTransformMarker(float *x, float *y, float zoom) const
{
	// Single selection only (assume obj is in selection)
	if (selection.size() != 1) return false;
	C4Object *obj = selection.GetObject();
	if (!obj) return false;
	// Show knob only for objects that can be scaled or rotated
	if (!obj->Def->GrowthType && !obj->Def->Rotateable) return false;
	// Show knob only if the shape has a certain minimum size in either extent (so small objects can still be moved)
	float vis_wdt = float(obj->Shape.Wdt) * zoom;
	float vis_hgt = float(obj->Shape.Wdt) * zoom;
	if (vis_wdt < ::GraphicsResource.fctTransformKnob.Hgt && vis_hgt < ::GraphicsResource.fctTransformKnob.Hgt) return false;
	// It's visible: Put it to the bottom of the shape without the shape expansion through rotation
	*x = 0;
	*y = float(obj->Def->Shape.y + obj->Def->Shape.Hgt) * obj->GetCon() / FullCon - float(::GraphicsResource.fctTransformKnob.Hgt) / (zoom*2);
	return true;
}

void C4EditCursor::Draw(C4TargetFacet &cgo)
{
	ZoomDataStackItem zdsi(cgo.X, cgo.Y, cgo.Zoom);
	float line_width = std::max<float>(1.0f, 1.0f / cgo.Zoom);
#ifdef WITH_QT_EDITOR
	// Draw shapes of selection
	shapes->Draw(cgo);
#endif
	// Draw selection marks
	for (C4Value &obj : selection)
	{
		C4Object *cobj = obj.getObj();
		if (!cobj) continue;
		DrawObject(cgo, cobj, 0xffffffff, fShiftWasDown, true); // highlight selection if shift is pressed
	}
	// Draw drag frame
	if (DragFrame)
		pDraw->DrawFrameDw(cgo.Surface,
		                               std::min(X, X2) + cgo.X - cgo.TargetX, std::min(Y, Y2) + cgo.Y - cgo.TargetY,
		                               std::max(X, X2) + cgo.X - cgo.TargetX, std::max(Y, Y2) + cgo.Y - cgo.TargetY, 0xffffffff, line_width);
	// Draw drag line
	if (DragLine)
		pDraw->DrawLineDw(cgo.Surface,
		                              X + cgo.X - cgo.TargetX, Y + cgo.Y - cgo.TargetY,
		                              X2 + cgo.X - cgo.TargetX, Y2 + cgo.Y - cgo.TargetY, 0xffffffff, line_width);
	// Draw drop target
	if (DropTarget)
		::GraphicsResource.fctMouseCursor.DrawX(cgo.Surface,
				DropTarget->GetX() + cgo.X - cgo.TargetX - ::GraphicsResource.fctMouseCursor.Wdt / 2 / cgo.Zoom,
				DropTarget->GetY() + DropTarget->Shape.y + cgo.Y - cgo.TargetY - ::GraphicsResource.fctMouseCursor.Hgt / cgo.Zoom,
				float(::GraphicsResource.fctMouseCursor.Wdt) / cgo.Zoom,
				float(::GraphicsResource.fctMouseCursor.Hgt) / cgo.Zoom, C4MC_Cursor_DropInto);
	// Draw paint circle
	if (Mode == C4CNS_ModeDraw && has_mouse_hover && ::Console.ToolsDlg.Grade>0 && ::Console.ToolsDlg.IsGradedTool())
	{
		// shadow for recognition on white background/material
		pDraw->DrawCircleDw(cgo.Surface, X + cgo.X - cgo.TargetX + 1.0f/cgo.Zoom, Y + cgo.Y - cgo.TargetY + 1.0f / cgo.Zoom, ::Console.ToolsDlg.Grade, 0xff000000, line_width);
		// actual circle
		pDraw->DrawCircleDw(cgo.Surface, X + cgo.X - cgo.TargetX, Y + cgo.Y - cgo.TargetY, ::Console.ToolsDlg.Grade, 0xffffffff, line_width);
	}
	// Draw creator preview
	if (Mode == C4CNS_ModeCreateObject && has_mouse_hover && creator_def)
	{
		C4TargetFacet cgo_creator;
		// Add the shape's offset and the shape's width / 2 (or height, resp.).
		// This does nothing for most objects, where these two sum up to 0.
		// However, for some objects, this fixes the preview, so it actually lines up with
		// the position where it is placed.
		cgo_creator.Set(cgo.Surface,
			X + cgo.X - cgo.TargetX + creator_def->Shape.x + creator_def->Shape.Wdt / 2,
			Y + cgo.Y - cgo.TargetY + creator_def->Shape.y + creator_def->Shape.Hgt / 2,
			creator_def->Shape.Wdt, creator_def->Shape.Hgt, 0, 0, cgo.Zoom, 0, 0);
		if (!creator_overlay)
		{
			creator_overlay = std::make_unique<C4GraphicsOverlay>();
			creator_overlay->SetAsBase(&creator_def->Graphics, C4GFXBLIT_ADDITIVE);
		}
		creator_overlay->Draw(cgo_creator, nullptr, NO_OWNER);
	}
	// Draw object highlight
	C4Object *highlight = highlighted_object.getObj();
	if (highlight) DrawObject(cgo, highlight, 0xffff8000, true, false); // highlight selection if shift is pressed
}


void C4EditCursor::DrawSelectMark(C4Facet &cgo, FLOAT_RECT frame, float width, uint32_t color)
{
	if ((cgo.Wdt<1) || (cgo.Hgt<1)) return;

	if (!cgo.Surface) return;

	const float EDGE_WIDTH = 2.f;

	unsigned char c[4] = {
		static_cast<unsigned char>((color >> 16) & 0xff),
		static_cast<unsigned char>((color >>  8) & 0xff),
		static_cast<unsigned char>((color >>  0) & 0xff),
		static_cast<unsigned char>((color >> 24) & 0xff)
	};

	const C4BltVertex vertices[] = {
		{ 0.f, 0.f, { c[0], c[1], c[2], c[3] }, frame.left + EDGE_WIDTH, frame.top, 0.f },
		{ 0.f, 0.f, { c[0], c[1], c[2], c[3] }, frame.left, frame.top, 0.f },
		{ 0.f, 0.f, { c[0], c[1], c[2], c[3] }, frame.left, frame.top, 0.f },
		{ 0.f, 0.f, { c[0], c[1], c[2], c[3] }, frame.left, frame.top+EDGE_WIDTH, 0.f },

		{ 0.f, 0.f, { c[0], c[1], c[2], c[3] }, frame.left+EDGE_WIDTH, frame.bottom-1, 0.f },
		{ 0.f, 0.f, { c[0], c[1], c[2], c[3] }, frame.left, frame.bottom-1, 0.f },
		{ 0.f, 0.f, { c[0], c[1], c[2], c[3] }, frame.left, frame.bottom-1, 0.f },
		{ 0.f, 0.f, { c[0], c[1], c[2], c[3] }, frame.left, frame.bottom-1-EDGE_WIDTH, 0.f },

		{ 0.f, 0.f, { c[0], c[1], c[2], c[3] }, frame.right-1-EDGE_WIDTH, frame.top, 0.f },
		{ 0.f, 0.f, { c[0], c[1], c[2], c[3] }, frame.right-1, frame.top, 0.f },
		{ 0.f, 0.f, { c[0], c[1], c[2], c[3] }, frame.right-1, frame.top, 0.f },
		{ 0.f, 0.f, { c[0], c[1], c[2], c[3] }, frame.right-1, frame.top+EDGE_WIDTH, 0.f },

		{ 0.f, 0.f, { c[0], c[1], c[2], c[3] }, frame.right-1-EDGE_WIDTH, frame.bottom-1, 0.f },
		{ 0.f, 0.f, { c[0], c[1], c[2], c[3] }, frame.right-1, frame.bottom-1, 0.f },
		{ 0.f, 0.f, { c[0], c[1], c[2], c[3] }, frame.right-1, frame.bottom-1, 0.f },
		{ 0.f, 0.f, { c[0], c[1], c[2], c[3] }, frame.right-1, frame.bottom-1-EDGE_WIDTH, 0.f },
	};

	const unsigned int n_vertices = sizeof(vertices) / sizeof(vertices[0]);

	pDraw->PerformMultiLines(cgo.Surface, vertices, n_vertices, width, nullptr);
}


void C4EditCursor::MoveSelection(C4Real XOff, C4Real YOff, bool drag_finished)
{
	EMMoveObject(fShiftWasDown ? EMMO_MoveForced : EMMO_Move, XOff, YOff, nullptr, &selection, nullptr, drag_finished);
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
	EMMoveObject(EMMO_Script, Fix0, Fix0, nullptr, &selection, szText);
	selection.ConsolidateEmpty();
	::Console.PropertyDlgUpdate(selection, true);
	return true;
}

void C4EditCursor::Default()
{
	fAltWasDown=false;
	fShiftWasDown=false;
	Mode = C4CNS_ModeEdit;
	X=Y=X2=Y2=0;
	Target=DropTarget=nullptr;
#ifdef USE_WIN32_WINDOWS
	hMenu=nullptr;
#endif
	Hold=DragFrame=DragLine=DragShape=DragTransform=false;
	selection.clear();
	creator_def = nullptr;
	creator_overlay = nullptr;
	has_mouse_hover = false;
	selection_invalid = false;
	DragRot0 = DragRotLast = 0; DragCon0 = DragConLast = FullCon;
}

void C4EditCursor::Clear()
{
#ifdef USE_WIN32_WINDOWS
	if (hMenu) DestroyMenu(hMenu); hMenu=nullptr;
#endif
#ifdef WITH_DEBUG_MODE
	ObjselectDelItems();
#endif
	selection.clear();
	Console.PropertyDlgUpdate(selection, false);
	creator_overlay.reset(nullptr);
#ifdef WITH_QT_EDITOR
	shapes->ClearShapes(); // Should really be empty already
#endif
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
	case C4CNS_ModeEdit: case C4CNS_ModePlay: case C4CNS_ModeCreateObject:
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
#ifdef WITH_QT_EDITOR
	case C4CNS_ModeEdit: iNewMode=C4CNS_ModeCreateObject; break;
#else
	case C4CNS_ModeEdit: iNewMode = C4CNS_ModeDraw; break;
#endif
	case C4CNS_ModeCreateObject: iNewMode = C4CNS_ModeDraw; break;
	case C4CNS_ModeDraw: iNewMode=C4CNS_ModePlay; break;
	default:             iNewMode=C4CNS_ModePlay; break;
	}

	// Set new mode
	SetMode(iNewMode);

	return true;
}

void C4EditCursor::ApplyCreateObject(bool container)
{
	if (!EditingOK()) return;
	if (!creator_def) return;
	if (container && !DropTarget) return;
	// execute/send control
	EMControl(CID_EMMoveObj, C4ControlEMMoveObject::CreateObject(creator_def->id, ftofix(X), ftofix(Y), container ? DropTarget : nullptr));
}

void C4EditCursor::ApplyToolBrush()
{
	if (!EditingOK(true)) return;
	C4ToolsDlg *pTools=&Console.ToolsDlg;
	// execute/send control
	EMControl(CID_EMDrawTool, new C4ControlEMDrawTool(EMDT_Brush, ::Landscape.GetMode(), X,Y,0,0, pTools->Grade, pTools->Material, pTools->Texture, pTools->BackMaterial, pTools->BackTexture));
}

void C4EditCursor::ApplyToolLine()
{
	if (!EditingOK(true)) return;
	C4ToolsDlg *pTools=&Console.ToolsDlg;
	// execute/send control
	EMControl(CID_EMDrawTool, new C4ControlEMDrawTool(EMDT_Line, ::Landscape.GetMode(), X,Y,X2,Y2, pTools->Grade, pTools->Material,pTools->Texture, pTools->BackMaterial, pTools->BackTexture));
}

void C4EditCursor::ApplyToolRect()
{
	if (!EditingOK(true)) return;
	C4ToolsDlg *pTools=&Console.ToolsDlg;
	// execute/send control
	EMControl(CID_EMDrawTool, new C4ControlEMDrawTool(EMDT_Rect, ::Landscape.GetMode(), X,Y,X2,Y2, pTools->Grade, pTools->Material, pTools->Texture, pTools->BackMaterial, pTools->BackTexture));
}

void C4EditCursor::ApplyToolFill()
{
	if (!EditingOK(true)) return;
	C4ToolsDlg *pTools=&Console.ToolsDlg;
	// execute/send control
	EMControl(CID_EMDrawTool, new C4ControlEMDrawTool(EMDT_Fill, ::Landscape.GetMode(), X,Y,0,Y2, pTools->Grade, pTools->Material, nullptr, nullptr, nullptr));
}

void C4EditCursor::AppendMenuItem(int num, const StdStrBuf & label)
{
#ifdef USE_WIN32_WINDOWS
	itemsObjselect[num].ItemId = IDM_VPORTDYN_FIRST + num;
	if (num)
		AppendMenu(GetSubMenu(hMenu,0), MF_STRING, IDM_VPORTDYN_FIRST + num, label.GetWideChar());
	else
		AppendMenu(GetSubMenu(hMenu,0), MF_SEPARATOR, IDM_VPORTDYN_FIRST, nullptr);
#endif
}

bool C4EditCursor::DoContextMenu(DWORD dwKeyState)
{
#ifdef USE_WIN32_WINDOWS
	bool fObjectSelected = !!selection.GetObject();
	POINT point; GetCursorPos(&point);
	HMENU hContext = GetSubMenu(hMenu,0);
	SetMenuItemEnable(hContext, IDM_VIEWPORT_DELETE, fObjectSelected && Console.Editing);
	SetMenuItemEnable(hContext, IDM_VIEWPORT_DUPLICATE, fObjectSelected && Console.Editing);
	SetMenuItemEnable(hContext, IDM_VIEWPORT_CONTENTS, fObjectSelected && selection.GetObject()->Contents.ObjectCount() && Console.Editing);
	SetMenuItemText(hContext,IDM_VIEWPORT_DELETE,LoadResStr("IDS_MNU_DELETE"));
	SetMenuItemText(hContext,IDM_VIEWPORT_DUPLICATE,LoadResStr("IDS_MNU_DUPLICATE"));
	SetMenuItemText(hContext,IDM_VIEWPORT_CONTENTS,LoadResStr("IDS_MNU_CONTENTS"));
#endif

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
#ifdef USE_WIN32_WINDOWS
		// If too many entries would be shown, add a "..." in the end
		const int maxentries = 25; // Maximum displayed objects. if you raise it, also change note with IDM_VPORTDYN_FIRST in resource.h
		bool has_too_many_entries = (entrycount > maxentries);
		if (has_too_many_entries) entrycount = maxentries + 1;
#else
		const int maxentries = std::numeric_limits<int>::max();
#endif
		itemsObjselect.resize(entrycount + 1); // +1 for a separator
		// Add a separator bar
		itemsObjselect[0].Object = nullptr;
		itemsObjselect[0].Command.Clear();
		itemsObjselect[0].EditCursor = this;
		AppendMenuItem(0, StdStrBuf());
		// Add all objects
		int i_entry = 0;
		for (int i_item = 0; i_item < itemcount; ++i_item)
		{
			++i_entry; if (i_entry >= maxentries) break;
			// Add selection entry
			C4Object *obj = (*atcursor)[i_item].getObj();
			assert(obj);
			itemsObjselect[i_entry].Object = obj;
			itemsObjselect[i_entry].Command.Clear();
			itemsObjselect[i_entry].EditCursor = this;
			AppendMenuItem(i_entry, FormatString("%s #%i (%i/%i)", obj->GetName(), obj->Number, obj->GetX(), obj->GetY()));

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
					itemsObjselect[i_entry].Object = obj;
					itemsObjselect[i_entry].Command.Take(custom_command_szstr);
					itemsObjselect[i_entry].EditCursor = this;
					AppendMenuItem(i_entry, FormatString("%s->%s", obj->GetName(), custom_command_szstr.getData()));
				}
			}
		}
#ifdef USE_WIN32_WINDOWS
		if (has_too_many_entries)
		{
			AppendMenu(hContext, MF_GRAYED, IDM_VPORTDYN_FIRST + maxentries + 1, L"...");
			itemsObjselect[maxentries + 1].ItemId = IDM_VPORTDYN_FIRST + maxentries + 1;
			itemsObjselect[maxentries + 1].Object = nullptr;
			itemsObjselect[maxentries + 1].Command.Clear();
		}
#endif
	}
	delete atcursor;

#ifdef USE_WIN32_WINDOWS
	int32_t iItem = TrackPopupMenu(
	                  hContext,
	                  TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_LEFTBUTTON | TPM_NONOTIFY,
	                  point.x,point.y, 0,
	                  Console.hWindow,
	                  nullptr);
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
#endif
	return true;
}

void C4EditCursor::GrabContents()
{
	// Set selection
	C4Object *pFrom;
	if (!( pFrom = selection.GetObject() )) return;
	ClearSelection();
	for (C4Object *cont : pFrom->Contents) AddToSelection(cont);
	OnSelectionChanged();
	Hold=true;

	// Exit all objects
	EMMoveObject(EMMO_Exit, Fix0, Fix0, nullptr, &selection);
}

void C4EditCursor::UpdateDropTarget(DWORD dwKeyState)
{
	// A drop target is set if holding down control either while moving an object or in object creation mode
	DropTarget=nullptr;

	if (dwKeyState & MK_CONTROL)
		if (selection.GetObject() || (Mode == C4CNS_ModeCreateObject && creator_def))
			for (C4Object *cobj : Objects)
			{
				if (cobj->Status)
					if (!cobj->Contained)
						if (Inside<int32_t>(X-(cobj->GetX()+cobj->Shape.x),0,cobj->Shape.Wdt-1))
							if (Inside<int32_t>(Y-(cobj->GetY()+cobj->Shape.y),0,cobj->Shape.Hgt-1))
								if (!selection.IsContained(cobj))
									{ DropTarget=cobj; break; }
			}

}

void C4EditCursor::PutContents()
{
	if (!DropTarget) return;
	EMMoveObject(EMMO_Enter, Fix0, Fix0, DropTarget, &selection);
}

C4Object *C4EditCursor::GetTarget()
{
	return Target;
}

bool C4EditCursor::EditingOK(bool for_landscape_drawing)
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
	switch (::Landscape.GetMode())
	{
	case LandscapeMode::Static:
		{
			bool material_set = false;
			int32_t x = X/::Landscape.GetMapZoom();
			int32_t y = Y/::Landscape.GetMapZoom();
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
	case LandscapeMode::Exact:
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

void C4EditCursor::EMMoveObject(C4ControlEMObjectAction eAction, C4Real tx, C4Real ty, C4Object *pTargetObj, const C4EditCursorSelection *pObjs, const char *szScript, bool drag_finished)
{
	// construct object list
	int32_t iObjCnt = 0; int32_t *pObjIDs = nullptr;
	if (pObjs && (iObjCnt = pObjs->ObjectCount()))
	{
		pObjIDs = new int32_t [iObjCnt];
		// fill
		int32_t i = 0;
		for (const C4Value &vobj : *pObjs)
		{
			C4Object *obj = vobj.getObj();
			if (!obj) continue;
			if (obj && obj->Status)
				pObjIDs[i++] = obj->Number;
			else
				pObjIDs[i++] = 0;
		}
	}

	// execute control
	EMControl(CID_EMMoveObj, new C4ControlEMMoveObject(eAction, tx, ty, pTargetObj, iObjCnt, pObjIDs, szScript, drag_finished));

}

void C4EditCursor::EMControl(C4PacketType eCtrlType, C4ControlPacket *pCtrl)
{
	::Control.DoInput(eCtrlType, pCtrl, CDT_Decide);
}


void C4EditCursor::ObjselectDelItems() {
	if(!itemsObjselect.size()) return;
	std::vector<ObjselItemDt>::iterator it = itemsObjselect.begin();
	while(it != itemsObjselect.end()) {
		#if defined(USE_WIN32_WINDOWS)
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
	if (clear) ClearSelection(obj);
	AddToSelection(obj);
	OnSelectionChanged();
}

void C4EditCursor::DoContextObjCommand(C4Object * obj, const char *cmd)
{
	// Command going through queue for sync
	if (!obj || !cmd) return;
	In(FormatString("Object(%d)->%s", obj->Number, cmd).getData());
}

bool C4EditCursor::GetCurrentSelectionPosition(int32_t *x, int32_t *y)
{
	C4Object *obj = selection.GetObject();
	if (!obj || !obj->Status) return false;
	*x = obj->GetX();
	*y = obj->GetY();
	return true;
}

void C4EditCursor::SetHighlightedObject(C4Object *new_highlight)
{
	highlighted_object = C4VObj(new_highlight);
}

bool C4EditCursor::IsHoveringTransformMarker() const
{
	float trf_marker_x, trf_marker_y;
	if (HasTransformMarker(&trf_marker_x, &trf_marker_y, Zoom))
	{
		C4Object *obj = selection.GetObject();
		float dx = (float(X - obj->GetX()) - trf_marker_x) * Zoom;
		float dy = (float(Y - obj->GetY()) - trf_marker_y) * Zoom;
		if (dx*dx + dy*dy <= ::GraphicsResource.fctTransformKnob.Hgt * ::GraphicsResource.fctTransformKnob.Hgt / 4)
			return true;
	}
	return false;
}
