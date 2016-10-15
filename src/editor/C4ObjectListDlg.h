/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2007-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2013-2016, The OpenClonk Team and contributors
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

#ifndef INC_C4ObjectListDlg
#define INC_C4ObjectListDlg


#include "object/C4ObjectList.h"

class C4ObjectListDlg: public C4ObjectListChangeListener
{
#ifdef WITH_QT_EDITOR
	class C4ConsoleQtObjectListModel *view_model; // forward into Qt object list model
#endif
public:
	C4ObjectListDlg();
	virtual ~C4ObjectListDlg();
#ifdef WITH_QT_EDITOR
	void SetModel(C4ConsoleQtObjectListModel *new_view_model) { view_model = new_view_model;  }
#endif
	
	void Execute();
	void Open();
	void Update(class C4EditCursorSelection &rSelection);

	virtual void OnObjectRemove(C4ObjectList * pList, C4ObjectLink * pLnk) override;
	virtual void OnObjectAdded(C4ObjectList * pList, C4ObjectLink * pLnk) override;
	virtual void OnObjectRename(C4ObjectList * pList, C4ObjectLink * pLnk) override;
	virtual void OnObjectContainerChanged(C4Object *obj, C4Object *old_container, C4Object *new_container) override;
};

#endif //INC_C4ObjectListDlg
