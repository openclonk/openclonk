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

#include <C4Include.h>
#include <C4Value.h>
#include <C4ConsoleQtPropListViewer.h>

C4ConsoleQtPropListModel::C4ConsoleQtPropListModel()
{
	proplist.reset(new C4Value());
}

C4ConsoleQtPropListModel::~C4ConsoleQtPropListModel()
{
}

void C4ConsoleQtPropListModel::SetPropList(class C4PropList *new_proplist)
{
	// Update properties
	proplist->SetPropList(new_proplist);
	if (new_proplist) properties = new_proplist->GetSortedLocalProperties();
	QModelIndex topLeft = index(0, 0);
	QModelIndex bottomRight = index(rowCount() - 1, columnCount() - 1);
	emit dataChanged(topLeft, bottomRight);
	emit layoutChanged();
}

int C4ConsoleQtPropListModel::rowCount(const QModelIndex & parent) const
{
	return properties.size();
}

int C4ConsoleQtPropListModel::columnCount(const QModelIndex & parent) const
{
	return 2; // Name + Data
}

QVariant C4ConsoleQtPropListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	// Table headers
	if (role == Qt::DisplayRole && orientation == Qt::Orientation::Horizontal)
	{
		if (section == 0) return QVariant(LoadResStr("IDS_CTL_NAME"));
		if (section == 1) return QVariant(LoadResStr("IDS_CTL_VALUE"));
	}
	return QVariant();
}

QVariant C4ConsoleQtPropListModel::data(const QModelIndex & index, int role) const
{
	// Query latest data from prop list
	C4PropList *props = proplist->getPropList();
	if (role == Qt::DisplayRole && props)
	{
		int row = index.row();
		if (row < 0 || row >= properties.size()) return QVariant();
		C4String *prop_name = properties[row].Get();
		if (!prop_name) return QVariant();
		switch (index.column())
		{
		case 0: // First col: Property Name
			return QVariant(prop_name->GetCStr());
		case 1: // Second col: Property value
		{
			C4Value v;
			if (!props->GetPropertyByS(prop_name, &v)) return QVariant("???"); /* Property got removed between update calls */
			return QVariant(v.GetDataString().getData());
		}
		}
	}
	// Nothing to show
	return QVariant();
}
