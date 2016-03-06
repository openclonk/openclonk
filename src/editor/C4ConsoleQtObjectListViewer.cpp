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
#include <C4Object.h>
#include <C4GameObjects.h>
#include <C4ConsoleQtObjectListViewer.h>
#include <C4Console.h>
#include <C4EditCursor.h>
#include <C4Effect.h>
#include <C4GameObjects.h>


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
	::Console.ObjectListDlg.SetModel(NULL);
}

void C4ConsoleQtObjectListModel::Invalidate()
{
	// Force redraw
	QModelIndex topLeft = index(0, 0, QModelIndex());
	QModelIndex bottomRight = index(last_row_count, columnCount() - 1, QModelIndex());
	emit dataChanged(topLeft, bottomRight);
	emit layoutChanged();
}

void C4ConsoleQtObjectListModel::OnItemRemoved(C4PropList *p)
{
	for (auto idx : this->persistentIndexList())
		if (idx.internalPointer() == p)
			this->changePersistentIndex(idx, QModelIndex());
	Invalidate();
}

int C4ConsoleQtObjectListModel::rowCount(const QModelIndex & parent) const
{
	int result = 0;
	if (parent.isValid())
	{
		// Child row count of object
		C4PropList *parent_item = static_cast<C4PropList *>(parent.internalPointer());
		if (!parent_item) return result;
		C4Object *obj = parent_item->GetObject();
		if (!obj) return result;
		// Contained objects plus effects
		for (C4Object *contents : obj->Contents)
			if (contents && contents->Status)
				++result;
		for (C4Effect *fx = obj->pEffects; fx; fx = fx->pNext) if (fx->IsActive())
			++result;
	}
	else
	{
		// Main object + effect count
		for (C4Object *obj : ::Objects)
			if (obj && obj->Status && !obj->Contained)
				++result;
		for (C4Effect *fx = ::Game.pGlobalEffects; fx; fx = fx->pNext) if (fx->IsActive())
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
	C4PropList *data = static_cast<C4PropList *>(index.internalPointer());
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
		// Fallback to effect names for effects
		return QString("Fx???");
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
		C4PropList *parent_item = static_cast<C4PropList *>(parent.internalPointer());
		if (!parent_item) return QModelIndex();
		C4Object *obj = parent_item->GetObject();
		if (!obj) return QModelIndex();
		// Contained objects plus effects
		for (C4Object *contents : obj->Contents)
			if (contents && contents->Status)
				if (!index--)
					return createIndex(row, column, contents);
		for (C4Effect *fx = obj->pEffects; fx; fx = fx->pNext) if (fx->IsActive())
			if (!index--)
				return createIndex(row, column, fx);
	}
	else
	{
		// Main object list
		for (C4Object *obj : ::Objects)
			if (obj && obj->Status && !obj->Contained)
				if (!index--)
					return createIndex(row, column, obj);
	}
	return QModelIndex(); // out of range
}

QModelIndex C4ConsoleQtObjectListModel::parent(const QModelIndex &index) const
{
	// Find parent of object or effect
	if (!index.isValid()) return QModelIndex();
	C4PropList *data = static_cast<C4PropList *>(index.internalPointer());
	if (!data) return QModelIndex();
	C4Object *obj = data->GetObject();
	if (obj) return GetModelIndexByItem(obj->Contained);
	// Parent object of effect
	// TODO: Effects currently don't keep track of their owners.
	// If this ever changes, lookup can be much more efficient...
	C4Effect *fx = data->GetEffect();
	if (fx)
	{
		for (C4Object *cobj : ::Objects) if (cobj && cobj->Status)
		{
			for (C4Effect *cfx = cobj->pEffects; cfx; cfx = cfx->pNext)
				if (cfx == fx) { obj = cobj; break; }
			if (obj) break;
		}
		return GetModelIndexByItem(obj); // returns root index for obj==NULL, i.e. global effects
	}
	// Can't happen
	return QModelIndex();
}

QModelIndex C4ConsoleQtObjectListModel::GetModelIndexByItem(C4PropList *item) const
{
	// Deduce position in model list from item pointer
	if (!item) return QModelIndex();
	C4Object *obj; C4Effect *fx;
	int row=0;
	if ((obj = item->GetObject()))
	{
		const C4ObjectList *list = &::Objects;
		if (obj->Contained) list = &(obj->Contained->Contents);
		for (C4Object *cobj : *list)
		{
			if (cobj == obj) break;
			if (cobj && cobj->Status) ++row;
		}
	}
	else if ((fx = item->GetEffect()))
	{
		// TODO: Effects currently don't keep track of their owners.
		// If this ever changes, lookup can be much more efficient...
		bool found = false;
		for (C4Object *cobj : ::Objects) if (cobj && cobj->Status)
		{
			row = 0;
			for (C4Effect *cfx = cobj->pEffects; cfx; cfx = cfx->pNext)
				if (cfx == fx) { obj = cobj; found = true; break; } else ++row;
			if (obj) break;
		}
		// Also search global effect list
		if (!found)
		{
			row = 0;
			for (C4Effect *cfx = ::Game.pGlobalEffects; cfx; cfx = cfx->pNext) if (cfx->IsActive())
				if (cfx == fx) { found = true; break; } else ++row;
			if (!found) return QModelIndex();
		}
		// Add other objects on top of this index
		const C4ObjectList *list = &::Objects;
		if (obj) list = &obj->Contents;
		for (C4Object *cobj : *list)
			if (cobj && cobj->Status)
				++row;
	}
	return createIndex(row, 0, item);
}
