/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2007  Günther Brammer
 * Copyright (c) 2007-2009, RedWolf Design GmbH, http://www.clonk.de
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

/* A window listing all objects in the game */

#ifndef INC_C4ObjectListDlg
#define INC_C4ObjectListDlg

#ifdef WITH_DEVELOPER_MODE
#include <gtk/gtk.h>
#endif // WITH_DEVELOPER_MODE

#include "C4ObjectList.h"

class C4ObjectListDlg: public C4ObjectListChangeListener
{
public:
	C4ObjectListDlg();
	virtual ~C4ObjectListDlg();
	void Execute();
	void Open();
	void Update(C4ObjectList &rSelection);

	virtual void OnObjectRemove(C4ObjectList * pList, C4ObjectLink * pLnk);
	virtual void OnObjectAdded(C4ObjectList * pList, C4ObjectLink * pLnk);
	virtual void OnObjectRename(C4ObjectList * pList, C4ObjectLink * pLnk);

#ifdef WITH_DEVELOPER_MODE
private:
	GtkWidget * window;
	GtkWidget * treeview;
	GObject * model;
	bool updating_selection;

	static void OnDestroy(GtkWidget * widget, C4ObjectListDlg * dlg);
	static void OnSelectionChanged(GtkTreeSelection * selection, C4ObjectListDlg * dlg);
#endif // WITH_DEVELOPER_MODE
};

#endif //INC_C4ObjectListDlg
