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

/* List of loaded definitions */

#ifndef INC_C4ConsoleQtDefinitionListViewer
#define INC_C4ConsoleQtDefinitionListViewer
#ifdef WITH_QT_EDITOR

#include "C4Include.h" // needed for automoc
#include "editor/C4ConsoleGUI.h" // for OpenGL
#include "script/C4Value.h"
#include "editor/C4ConsoleQt.h"

// Prop list view implemented as a model view
class C4ConsoleQtDefinitionListModel : public QAbstractItemModel
{
	Q_OBJECT

	mutable int32_t last_row_count{0};

	// Tree structure of definition list
	struct DefListNode
	{
		std::vector<std::unique_ptr<DefListNode> > items;
		C4Def *def{nullptr};
		StdCopyStrBuf name, filename;
		int32_t idx{0};
		DefListNode *parent{nullptr};

		DefListNode() = default;
		void SortByName(); // sort self and children
	};
	std::unique_ptr<DefListNode> root;

public:
	C4ConsoleQtDefinitionListModel();
	~C4ConsoleQtDefinitionListModel() override;

	// Refresh definition list (on initialization or e.g. after ReloadDef)
	void ReInit();
	void EnsureInit();
	void OnItemRemoved(class C4Def *def);

	// Callback from EditCursor when selection was changed e.g. from property window
	void SetSelection(C4Def *new_selection);
	
	class C4Def *GetDefByModelIndex(const QModelIndex &idx);
	const char *GetNameByModelIndex(const QModelIndex &idx);
	QModelIndex GetModelIndexByItem(class C4Def *def) const;

public:
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
