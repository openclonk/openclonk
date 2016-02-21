/*
* OpenClonk, http://www.openclonk.org
*
* Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
* Copyright (c) 2013, The OpenClonk Team and contributors
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

/* Proplist table view */

#ifndef INC_C4ConsoleQtObjectListViewer
#define INC_C4ConsoleQtObjectListViewer
#ifdef WITH_QT_EDITOR

#include <C4Include.h> // needed for automoc
#include <C4ConsoleGUI.h> // for glew.h
#include <C4Value.h>
#include <C4ConsoleQt.h>

// Prop list view implemented as a model view
class C4ConsoleQtObjectListModel : public QAbstractItemModel
{
	Q_OBJECT

	class QItemSelectionModel *selection_model;
	QTreeView *view;
	int32_t is_updating;
	mutable int32_t last_row_count;
	QBrush clr_deleted, clr_effect;

public:
	C4ConsoleQtObjectListModel(class QTreeView *view);
	~C4ConsoleQtObjectListModel();

	// Refresh object list on next redraw
	void Invalidate();

	// Callback from EditCursor when selection was changed e.g. from viewport
	void SetSelection(class C4EditCursorSelection &rSelection);

private:
	QModelIndex GetModelIndexByItem(class C4PropList *item) const;

protected:
	int rowCount(const QModelIndex & parent = QModelIndex()) const override;
	int columnCount(const QModelIndex & parent = QModelIndex()) const override;
	QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
	QModelIndex index(int row, int column, const QModelIndex &parent) const override;
	QModelIndex parent(const QModelIndex &index) const override;

	// signal callback when user changed selection in dialogue
	void OnSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected);
};

#endif // WITH_QT_EDITOR
#endif // INC_C4ConsoleQtObjectListViewer
