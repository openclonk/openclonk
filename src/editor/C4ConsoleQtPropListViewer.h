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

#ifndef INC_C4ConsoleQtPropListViewer
#define INC_C4ConsoleQtPropListViewer
#ifdef WITH_QT_EDITOR

#include "C4Include.h" // needed for automoc
#include "editor/C4ConsoleGUI.h" // for glew.h
#include "editor/C4ConsoleQt.h"

// Prop list view implemented as a model view
class C4ConsoleQtPropListModel : public QAbstractTableModel
{
	Q_OBJECT

	std::unique_ptr<class C4Value> proplist;
	std::vector< C4RefCntPointer<C4String> > properties;
public:
	C4ConsoleQtPropListModel();
	~C4ConsoleQtPropListModel();

	void SetPropList(class C4PropList *new_proplist);

protected:
	virtual int rowCount(const QModelIndex & parent = QModelIndex()) const;
	virtual int columnCount(const QModelIndex & parent = QModelIndex()) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
};

#endif // WITH_QT_EDITOR
#endif // INC_C4ConsoleQtPropListViewer
