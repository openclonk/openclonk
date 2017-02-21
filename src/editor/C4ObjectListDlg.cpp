/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2007-2009, RedWolf Design GmbH, http://www.clonk.de/
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

/* A window listing all objects in the game */

#include "C4Include.h"
#include "editor/C4ObjectListDlg.h"
#include "editor/C4Console.h"
#include "object/C4Object.h"
#include "object/C4GameObjects.h"
#include "script/C4Effect.h"


C4ObjectListDlg::C4ObjectListDlg()
{
}

C4ObjectListDlg::~C4ObjectListDlg()
{
}

void C4ObjectListDlg::Execute()
{
}

void C4ObjectListDlg::Open()
{
}

#ifdef WITH_QT_EDITOR

#include "editor/C4ConsoleQtObjectListViewer.h"

void C4ObjectListDlg::Update(C4EditCursorSelection &rSelection)
{
	// Update done through console
	::Console.OnObjectSelectionChanged(rSelection);
}

// Could do some crazy fine-grained updates. But updating is cheap enough...
void C4ObjectListDlg::OnObjectRemove(C4ObjectList * pList, C4ObjectLink * pLnk)
{
	if (view_model) view_model->Invalidate();
}

void C4ObjectListDlg::OnObjectAdded(C4ObjectList * pList, C4ObjectLink * pLnk)
{
	if (view_model) view_model->Invalidate();
}

void C4ObjectListDlg::OnObjectRename(C4ObjectList * pList, C4ObjectLink * pLnk)
{
	if (view_model) view_model->Invalidate();
}

void C4ObjectListDlg::OnObjectContainerChanged(C4Object *obj, C4Object *old_container, C4Object *new_container)
{
	if (view_model) view_model->Invalidate();
}


#else

void C4ObjectListDlg::Update(C4EditCursorSelection &rSelection)
{
}

void C4ObjectListDlg::OnObjectRemove(C4ObjectList * pList, C4ObjectLink * pLnk)
{
}

void C4ObjectListDlg::OnObjectAdded(C4ObjectList * pList, C4ObjectLink * pLnk)
{
}

void C4ObjectListDlg::OnObjectRename(C4ObjectList * pList, C4ObjectLink * pLnk)
{
}

void C4ObjectListDlg::OnObjectContainerChanged(C4Object *obj, C4Object *old_container, C4Object *new_container)
{
}


#endif // WITH_QT_EDITOR


C4ObjectListChangeListener & ObjectListChangeListener = Console.ObjectListDlg;
