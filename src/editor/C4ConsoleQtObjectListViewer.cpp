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


/* Object list information cache */

// Cached copy of displayed tree
// (Leaf nodes only validated on demand)
class C4ConsoleQtObjectListModel::Node
{
	Node *parent;
	C4Value data; // root data
	std::vector<class Node> children; // child vector - is kept around and usually only increased on query because most updates will just re-set the same object
	int32_t num_children; // children valid until this point
	bool valid; // if false, re-fill node (and all children) on next data query

public:
	Node(Node *parent) : parent(parent), valid(false), num_children(0) {}

	void Validate();
	void Invalidate() { valid = false; }
	bool IsValid() const { return valid; }

	void SetData(class C4PropList *to_data) { data.SetPropList(to_data); Invalidate(); }
	C4PropList *GetData() const { return data.getPropList(); }
	Node *_GetChild(int32_t idx) { return &children[idx]; } // unchecked child access
	Node *GetParent() const { return parent; }
	int32_t GetNumChildren() const { return num_children; }
	void EnsureValid() { if (!valid) Validate(); }

	void AddChild(C4PropList *p)
	{
		if (num_children >= children.size()) children.resize(num_children + 1, Node(this));
		children[num_children++].SetData(p);
	}

	int32_t GetChildIndex(class C4PropList *data)
	{
		for (int32_t idx = 0; idx < num_children; ++idx)
			if (_GetChild(idx)->GetData() == data)
				return idx;
		return -1; // not found
	}
};

void C4ConsoleQtObjectListModel::Node::Validate()
{
	num_children = 0;
	// Fill from object list (either main list or contents)
	C4Object *obj = data.getObj();
	C4ObjectList *list = NULL;
	if (!parent)
		list = &::Objects;
	else if (obj)
		list = &obj->Contents;
	if (list)
		for (auto cobj : *list)
			if (cobj && cobj->Contained == obj)
				AddChild(cobj);
	// Add object effects
	if (obj)
	{
		// TODO
	}
	valid = true;
}

C4ConsoleQtObjectListModel::C4ConsoleQtObjectListModel(class QTreeView *view)
	: selection_model(NULL), view(view), is_updating(0)
{
	root.reset(new Node(NULL));
	// install the item model
	view->setModel(this);
	// get the selection model to control it
	selection_model = view->selectionModel();
	connect(selection_model, &QItemSelectionModel::selectionChanged, this, &C4ConsoleQtObjectListModel::OnSelectionChanged);
	// install self for callbacks from ObjectListDlg
	::Console.ObjectListDlg.SetModel(this);
}

C4ConsoleQtObjectListModel::~C4ConsoleQtObjectListModel()
{
	::Console.ObjectListDlg.SetModel(NULL);
}

void C4ConsoleQtObjectListModel::Invalidate()
{
	// Invalidate everything
	int32_t num_invalid = root->GetNumChildren();
	root->Invalidate();
	// Force redraw
	QModelIndex topLeft = index(0, 0, QModelIndex());
	QModelIndex bottomRight = index(num_invalid, columnCount() - 1, QModelIndex());
	emit dataChanged(topLeft, bottomRight);
	emit layoutChanged();
}

void C4ConsoleQtObjectListModel::SetSelection(class C4EditCursorSelection &rSelection)
{
	// Reflect selection change in view
	++is_updating;
	selection_model->clearSelection();
	QModelIndex last_idx;
	for (C4Value &v : rSelection)
	{
		QModelIndex idx = GetModelIndexByItem(v.getPropList());
		if (idx.isValid())
		{
			selection_model->select(idx, QItemSelectionModel::Select);
			last_idx = idx;
		}
	}
	if (last_idx.isValid()) view->scrollTo(last_idx);
	--is_updating;
}

int C4ConsoleQtObjectListModel::rowCount(const QModelIndex & parent) const
{
	Node *parent_node = this->GetNodeByIndex(parent);
	if (!parent_node) return 0;
	parent_node->Validate();
	return parent_node->GetNumChildren();
}

int C4ConsoleQtObjectListModel::columnCount(const QModelIndex & parent) const
{
	return 1; // Name only
}

QVariant C4ConsoleQtObjectListModel::data(const QModelIndex & index, int role) const
{
	// Object list lookup is done in index(). Here we just use the pointer.
	Node *node = static_cast<Node *>(index.internalPointer());
	if (!node) return QVariant();
	if (role == Qt::DisplayRole)
	{
		// Deleted proplist?
		C4PropList *data = node->GetData();
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
	// Nothing to show
	return QVariant();
}

C4ConsoleQtObjectListModel::Node *C4ConsoleQtObjectListModel::GetNodeByIndex(const QModelIndex &index) const
{
	// Find node recursively
	if (!index.isValid()) return root.get();
	Node *parent = GetNodeByIndex(index.parent());
	// Out of range
	if (index.row() < 0 || index.column() != 0) return NULL;
	// Get indexed child node
	parent->EnsureValid();
	if (index.row() >= parent->GetNumChildren()) return NULL;
	return parent->_GetChild(index.row());
}

QModelIndex C4ConsoleQtObjectListModel::index(int row, int column, const QModelIndex &parent) const
{
	Node *parent_node = GetNodeByIndex(parent);
	// Parent out of range?
	if (!parent_node) return QModelIndex();
	// Make sure it's updated
	parent_node->EnsureValid();
	// Current index out of range?
	if (row < 0 || column != 0 || row >= parent_node->GetNumChildren()) return QModelIndex();
	// This item is OK. Create an index!
	return createIndex(row, column, parent_node->_GetChild(row));
}

QModelIndex C4ConsoleQtObjectListModel::parent(const QModelIndex &index) const
{
	// Look up parent through node structure
	if (!index.isValid()) return QModelIndex();
	Node *node = static_cast<Node *>(index.internalPointer());
	if (!node) return QModelIndex();
	Node *parent = node->GetParent();
	if (!parent) return QModelIndex();
	// Parent must not be the root
	Node *grandparent = parent->GetParent();
	if (!grandparent) return QModelIndex();
	// Find index of parent
	for (int idx = 0; idx < grandparent->GetNumChildren(); ++idx)
		if (grandparent->_GetChild(idx) == parent)
			return createIndex(idx, 0, parent);
	return QModelIndex();
}

void C4ConsoleQtObjectListModel::OnSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected)
{
	if (is_updating) return;
	// Forward to EditCursor
	Node *node; C4PropList *p;
	for (const QItemSelectionRange &item_range : deselected)
		if (item_range.isValid())
			for (const QModelIndex &item : item_range.indexes())
				if ((node = static_cast<Node *>(item.internalPointer())))
					if ((p = node->GetData()))
						::Console.EditCursor.RemoveFromSelection(p);
	for (const QItemSelectionRange &item_range : selected)
		if (item_range.isValid())
			for (const QModelIndex &item : item_range.indexes())
				if ((node = static_cast<Node *>(item.internalPointer())))
					if ((p = node->GetData()))
						::Console.EditCursor.AddToSelection(p);
	::Console.EditCursor.OnSelectionChanged(true);
}

bool C4ConsoleQtObjectListModel::GetNodeByItem(class C4PropList *item, C4ConsoleQtObjectListModel::Node **out_parent_node, int32_t *out_index) const
{
	// Deduce node and parent index from item pointer
	if (!item) return false;
	Node *parent_node;
	int32_t idx;
	C4Object *obj = item->GetObject();
	if (obj)
	{
		if (!obj->Contained)
		{
			// Uncontained object
			parent_node = root.get();
		}
		else
		{
			// Contained object
			if (!GetNodeByItem(obj->Contained, &parent_node, &idx)) return false;
			parent_node = parent_node->_GetChild(idx);
		}
	}
	else
	{
		// Effect
		C4Effect *fx = item->GetEffect();
		if (!fx) return false;
		// Parent object of effect
		// TODO: Effects currently don't keep track of their owners.
		// If this ever changes, lookup can be much faster
		for (C4Object *cobj : ::Objects)
		{
			for (C4Effect *cfx = obj->pEffects; cfx; cfx = cfx->pNext)
				if (cfx == fx) { obj = cobj; break; }
			if (obj) break;
		}
		if (!GetNodeByItem(obj, &parent_node, &idx)) return false;
		parent_node = parent_node->_GetChild(idx);
	}
	parent_node->EnsureValid();
	idx = parent_node->GetChildIndex(obj);
	if (idx < 0) return false;
	*out_index = idx;
	*out_parent_node = parent_node;
	return true;			
}

QModelIndex C4ConsoleQtObjectListModel::GetModelIndexByItem(C4PropList *item) const
{
	// Deduce position in model list from item pointer
	Node *parent_node=NULL; int32_t row=0;
	if (!GetNodeByItem(item, &parent_node, &row)) return QModelIndex();
	return createIndex(row, 0, parent_node->_GetChild(row));
}
