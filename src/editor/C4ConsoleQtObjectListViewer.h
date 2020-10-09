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

#include "C4Include.h" // needed for automoc
#include "editor/C4ConsoleGUI.h" // for OpenGL
#include "script/C4Value.h"
#include "editor/C4ConsoleQt.h"

// Prop list view implemented as a model view
class C4ConsoleQtObjectListModel : public QAbstractItemModel
{
	Q_OBJECT

	mutable int32_t last_row_count{0};
	QBrush clr_deleted, clr_effect;

	// model indices for static proplists
	enum
	{
		IDX_Global = 0,
		IDX_Scenario = 1,
		IDX_Objects = 2
	};

public:
	C4ConsoleQtObjectListModel();
	~C4ConsoleQtObjectListModel() override;

	// Refresh object list on next redraw
	void Invalidate();

	QModelIndex GetModelIndexByItem(class C4PropList *item) const;
	C4PropList *GetItemByModelIndex(const QModelIndex &index) const;

protected:
	int rowCount(const QModelIndex & parent = QModelIndex()) const override;
	int columnCount(const QModelIndex & parent = QModelIndex()) const override;
	QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
	QModelIndex index(int row, int column, const QModelIndex &parent) const override;
	QModelIndex parent(const QModelIndex &index) const override;
};

#endif // WITH_QT_EDITOR
#endif // INC_C4ConsoleQtObjectListViewer
