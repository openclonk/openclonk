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
#include "object/C4DefList.h"
#include "editor/C4ConsoleQtDefinitionListViewer.h"
#include "editor/C4Console.h"
#include "editor/C4EditCursor.h"


/* Defintion tree */

void C4ConsoleQtDefinitionListModel::DefListNode::SortByName()
{
	// sort self
	std::sort(items.begin(), items.end(),
		[](const std::unique_ptr<DefListNode> & a, const std::unique_ptr<DefListNode> & b) -> bool
		{ return a->name.Compare(b->name) < 0; });
	// sort children recursively
	int32_t idx = 0;
	for (auto & child : items)
	{
		child->SortByName();
		child->idx = idx++; // re-assign indices to reflect new sorting
	}
}

/* Defintion view model */

C4ConsoleQtDefinitionListModel::C4ConsoleQtDefinitionListModel()
{
	// initial model data
	ReInit();
}

C4ConsoleQtDefinitionListModel::~C4ConsoleQtDefinitionListModel() = default;

void C4ConsoleQtDefinitionListModel::EnsureInit()
{
	// Init if not already done
	if (!root.get() || root->items.empty())
		if (::Definitions.GetDefCount())
			ReInit();
}

void C4ConsoleQtDefinitionListModel::ReInit()
{
	// Re-fill definition model with all loaded definitions matching condition
	// (TODO: Add conditional lists)
	root = std::make_unique<C4ConsoleQtDefinitionListModel::DefListNode>();
	int32_t index = 0; C4Def *def;
	while ((def = ::Definitions.GetDef(index++)))
	{
		// Ignore hidden defs
		if (def->HideInCreator) continue;
		// Build path leading to this definition
		DefListNode *node_parent = root.get();
		StdCopyStrBuf fn(def->Filename), fn2;
		StdCopyStrBuf fn_full;
		fn.ReplaceChar(AltDirectorySeparator, DirectorySeparator);
		for (;;)
		{
			bool is_parent_folder = fn.SplitAtChar(DirectorySeparator, &fn2);
			if (fn_full.getLength())
			{
				fn_full.AppendChar(DirectorySeparator);
			}
			fn_full.Append(fn);
			if (!is_parent_folder || WildcardMatch(C4CFN_DefFiles, fn.getData())) // ignore non-.ocd-folders (except for final definition)
			{
				// Find if path is already there
				RemoveExtension(&fn);
				DefListNode *node_child = nullptr;
				for (auto &test_node_child : node_parent->items)
					if (test_node_child->filename == fn)
					{
						node_child = &*test_node_child;
						break;
					}
				// If not, create it
				if (!node_child)
				{
					node_parent->items.emplace_back((node_child = new DefListNode()));
					node_child->idx = node_parent->items.size() - 1;
					node_child->parent = node_parent;
					const char *localized_name = ::Definitions.GetLocalizedGroupFolderName(fn_full.getData());
					node_child->name.Copy(localized_name ? localized_name : fn.getData());
					node_child->filename.Copy(fn);
				}
				// And fill in node if this is not a parent folder
				if (!is_parent_folder)
				{
					node_child->def = def;
					const char *def_name = def->GetName();
					if (def_name && *def_name) node_child->name.Copy(def_name);
					break;
				}
				else
				{
					// Parent folder: Next path segment
					node_parent = node_child;
				}
			}
			fn = fn2;
		}
	}
	// Descend into singleton root classes. I.e. if all elements are children of Objects/Items, move the root in there.
	DefListNode *new_root = root.get();
	while (new_root->items.size() == 1 && !new_root->items[0]->def)
	{
		std::unique_ptr<DefListNode> tmp(new_root->items[0].release());
		root.reset(tmp.release());
		new_root = root.get();
	}
	root->parent = nullptr;
	// Copy group path names into definitions for later lookup by script
	QStringList group_names;
	DefListNode *node = root.get();
	while (node)
	{
		if (node->def)
		{
			node->def->ConsoleGroupPath.Copy(group_names.join('/').toUtf8());
		}
		// Walk over tree. Remember groups in group_names string list.
		if (!node->items.empty())
		{
			if (node != root.get()) group_names.append(node->name.getData());
			node = node->items[0].get();
		}
		else
		{
			int32_t idx = node->idx + 1;
			while ((node = node->parent))
			{
				if (node->items.size() > idx)
				{
					node = node->items[idx].get();
					break;
				}
				if (group_names.size()) group_names.pop_back();
				idx = node->idx + 1;
			}
		}
	}
	// Sort everything by display name (recursively)
	root->SortByName();
	// Model reset to invalidate all indexes
	beginResetModel();
	endResetModel();
}

void C4ConsoleQtDefinitionListModel::OnItemRemoved(C4Def *p)
{
	for (auto idx : this->persistentIndexList())
		if (idx.internalPointer() == p)
			this->changePersistentIndex(idx, QModelIndex());
	ReInit();
}

class C4Def *C4ConsoleQtDefinitionListModel::GetDefByModelIndex(const QModelIndex &idx)
{
	DefListNode *node = static_cast<DefListNode *>(idx.internalPointer());
	if (node) return node->def; else return nullptr;
}

const char *C4ConsoleQtDefinitionListModel::GetNameByModelIndex(const QModelIndex &idx)
{
	DefListNode *node = static_cast<DefListNode *>(idx.internalPointer());
	if (node) return node->name.getData(); else return nullptr;
}

int C4ConsoleQtDefinitionListModel::rowCount(const QModelIndex & parent) const
{
	int result = 0;
	DefListNode *parent_node = parent.isValid() ? static_cast<DefListNode *>(parent.internalPointer()) : nullptr;
	if (!parent_node) parent_node = root.get();
	if (parent_node) result = parent_node->items.size();
	return result;
}

int C4ConsoleQtDefinitionListModel::columnCount(const QModelIndex & parent) const
{
	return 1; // Name only
}

QVariant C4ConsoleQtDefinitionListModel::data(const QModelIndex & index, int role) const
{
	// Object list lookup is done in index(). Here we just use the pointer.
	DefListNode *node = static_cast<DefListNode *>(index.internalPointer());
	if (!node) return QVariant();
	if (role == Qt::DisplayRole)
	{
		return QString(node->name.getData());
	}
	// Nothing to show
	return QVariant();
}

QModelIndex C4ConsoleQtDefinitionListModel::index(int row, int column, const QModelIndex &parent) const
{
	// Current index out of range?
	if (row < 0 || column != 0) return QModelIndex();
	DefListNode *parent_node = parent.isValid() ? static_cast<DefListNode *>(parent.internalPointer()) : nullptr;
	if (!parent_node) parent_node = root.get();
	if (parent_node->items.size() <= row) return QModelIndex();
	// Index into tree
	DefListNode *node = parent_node->items[row].get();
	return createIndex(row, column, node);
}

QModelIndex C4ConsoleQtDefinitionListModel::parent(const QModelIndex &index) const
{
	// Find parent through tree
	DefListNode *node = static_cast<DefListNode *>(index.internalPointer());
	if (!node) return QModelIndex();
	DefListNode *parent_node = node->parent;
	if (!parent_node || parent_node == root.get()) return QModelIndex();
	return createIndex(parent_node->idx, index.column(), parent_node);
}

QModelIndex C4ConsoleQtDefinitionListModel::GetModelIndexByItem(C4Def *def) const
{
	// Just search tree
	DefListNode *node = root.get();
	while (node)
	{
		if (node->def == def) break;
		if (!node->items.empty())
			node = node->items[0].get();
		else
		{
			int32_t idx = node->idx + 1;
			while ((node = node->parent))
			{
				if (node->items.size() > idx)
				{
					node = node->items[idx].get();
					break;
				}
				idx = node->idx + 1;
			}
		}
	}
	// Def found in tree?
	if (node)
	{
		return createIndex(node->idx, 0, node);
	}
	else
	{
		return QModelIndex();
	}
}
