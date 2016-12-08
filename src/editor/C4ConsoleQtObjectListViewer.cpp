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

#include "C4Include.h"
#include "script/C4Value.h"
#include "object/C4Def.h"
#include "object/C4Object.h"
#include "object/C4GameObjects.h"
#include "editor/C4ConsoleQtObjectListViewer.h"
#include "editor/C4Console.h"
#include "editor/C4EditCursor.h"
#include "script/C4Effect.h"
#include "object/C4GameObjects.h"


C4ConsoleQtObjectListModel::C4ConsoleQtObjectListModel() : last_row_count(0)
{
	// default font colors
	clr_deleted.setColor(QApplication::palette().color(QPalette::Mid));
	clr_effect.setColor(QApplication::palette().color(QPalette::Dark));
	// install self for callbacks from ObjectListDlg
	::Console.ObjectListDlg.SetModel(this);
}

C4ConsoleQtObjectListModel::~C4ConsoleQtObjectListModel()
{
	::Console.ObjectListDlg.SetModel(nullptr);
}

void C4ConsoleQtObjectListModel::Invalidate()
{
	// Kill any dead object pointers and force redraw
	emit layoutAboutToBeChanged();
	QModelIndexList list = this->persistentIndexList();
	for (auto idx : list)
	{
		if (idx.internalPointer())
		{
			QModelIndex new_index = GetModelIndexByItem(static_cast<C4PropList *>(idx.internalPointer()));
			this->changePersistentIndex(idx, new_index);
		}
	}
	QModelIndex topLeft = index(0, 0, QModelIndex());
	QModelIndex bottomRight = index(last_row_count, columnCount() - 1, QModelIndex());
	emit dataChanged(topLeft, bottomRight);
	emit layoutChanged();
}

int C4ConsoleQtObjectListModel::rowCount(const QModelIndex & parent) const
{
	int result = 0;
	if (parent.isValid())
	{
		// Child row count of object
		C4PropList *parent_item = GetItemByModelIndex(parent);
		if (!parent_item) return result;
		C4Object *obj = parent_item->GetObject();
		if (!obj || !obj->Status) return result;
		// Contained objects
		for (C4Object *contents : obj->Contents)
			if (contents && contents->Status)
				++result;
	}
	else
	{
		// Static lists
		result = IDX_Objects;
		// Main object count
		for (C4Object *obj : ::Objects)
			if (obj && obj->Status && !obj->Contained)
				++result;
		last_row_count = result;
	}
	return result;
}

int C4ConsoleQtObjectListModel::columnCount(const QModelIndex & parent) const
{
	return 1; // Name only
}

QVariant C4ConsoleQtObjectListModel::data(const QModelIndex & index, int role) const
{
	// Object list lookup is done in index(). Here we just use the pointer.
	C4PropList *data = GetItemByModelIndex(index);
	if (role == Qt::DisplayRole)
	{
		// Deleted proplist?
		if (!data) return QString("<deleted>");
		// Prefer own name
		const char *name = data->GetName();
		if (name && *name) return QString(name);
		// If no name is set, fall back to definition name for objects
		C4Object *obj = data->GetObject();
		if (obj) return QString(obj->Def->id.ToString());
	}
	else if (role == Qt::ForegroundRole)
	{
		// Deleted proplist?
		if (!data) return QVariant(clr_deleted);
		// Object?
		C4Object *obj = data->GetObject();
		if (obj) return QVariant(); // default
		// Effect
		return QVariant(clr_effect);
	}
	// Nothing to show
	return QVariant();
}

QModelIndex C4ConsoleQtObjectListModel::index(int row, int column, const QModelIndex &parent) const
{
	// Current index out of range?
	if (row < 0 || column != 0) return QModelIndex();
	int index = row;
	// Child element or main list?
	if (parent.isValid())
	{
		// Child of valid object?
		C4PropList *pobj = GetItemByModelIndex(parent);
		C4Object *container = pobj->GetObject();
		if (container)
		{
			for (C4Object *contents : container->Contents)
			{
				if (contents && contents->Status)
				{
					if (!index--)
					{
						return createIndex(row, column, static_cast<C4PropList *>(contents));
					}
				}
			}
		}
	}
	else
	{
		// Static entries
		if (index == IDX_Global)
		{
			return createIndex(row, column, static_cast<C4PropList *>(&::ScriptEngine));
		}
		else if (index == IDX_Scenario)
		{
			// This may create a null pointer entry. That's OK; it's only for the disabled display before the scenario is loaded
			return createIndex(row, column, static_cast<C4PropList *>(::GameScript.ScenPropList.getPropList()));
		}
		else
		{
			// Main object list
			for (C4Object *obj : ::Objects)
			{
				if (obj && obj->Status && !obj->Contained)
				{
					if (!index--)
					{
						return createIndex(row, column, static_cast<C4PropList *>(obj));
					}
				}
			}
		}
	}
	return QModelIndex(); // out of range
}

QModelIndex C4ConsoleQtObjectListModel::parent(const QModelIndex &index) const
{
	// Find parent of object
	if (!index.isValid()) return QModelIndex();
	C4PropList *data = GetItemByModelIndex(index);
	if (!data) return QModelIndex();
	C4Object *obj = data->GetObject();
	if (obj) return GetModelIndexByItem(obj->Contained);
	// Root item
	return QModelIndex();
}

QModelIndex C4ConsoleQtObjectListModel::GetModelIndexByItem(C4PropList *item) const
{
	// Deduce position in model list from item pointer
	// No position for invalid items
	if (!item) return QModelIndex();
	C4Object *obj = item->GetObject();
	if (obj && !obj->Status) return QModelIndex();
	// Default position for Global and Scenario object
	int row;
	if (item == &::ScriptEngine)
	{
		row = IDX_Global;
	}
	else if (item == ::GameScript.ScenPropList.getPropList())
	{
		row = IDX_Scenario;
	}
	else if (obj)
	{
		// Positions for object items
		row = IDX_Objects;
		const C4ObjectList *list = &::Objects;
		if (obj->Contained) list = &(obj->Contained->Contents);
		for (C4Object *cobj : *list)
		{
			if (cobj == obj) break;
			if (cobj && cobj->Status) ++row;
		}
	}
	else
	{
		return QModelIndex();
	}
	return createIndex(row, 0, static_cast<C4PropList *>(item));
}

C4PropList *C4ConsoleQtObjectListModel::GetItemByModelIndex(const QModelIndex &index) const
{
	// Get proplist from model index
	if (!index.isValid()) return nullptr;
	return static_cast<C4PropList *>(index.internalPointer());
}