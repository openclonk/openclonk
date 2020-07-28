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

#ifndef INC_C4ConsoleQtNewScenario
#define INC_C4ConsoleQtNewScenario
#ifdef WITH_QT_EDITOR

#include "C4Include.h" // needed for automoc
#include "editor/C4ConsoleGUI.h" // for OpenGL
#include "editor/C4ConsoleQt.h"
#include "ui_C4ConsoleQtNewScenario.h"
#include "landscape/C4Scenario.h"

// Definition file view for selection in New Scenario dialogue
class C4ConsoleQtDefinitionFileListModel : public QAbstractItemModel
{
	Q_OBJECT

public:
	C4ConsoleQtDefinitionFileListModel();
	~C4ConsoleQtDefinitionFileListModel() override;
	void AddExtraDef(const char *def);
	std::list<const char *> GetUserSelectedDefinitions() const;
	std::list<const char *> GetSelectedDefinitions() const;
	void SetForcedSelection(const std::list<const char *> &defs);

private:

	// Cached def file info: Children loaded on demand (if user expands into tree)
	class DefFileInfo
	{
		DefFileInfo *parent{nullptr};
		C4Group grp;
		std::vector< std::unique_ptr<DefFileInfo> > children;
		StdCopyStrBuf filename, root_path, full_filename;
		bool was_opened{true}, is_root{true};
		bool user_selected{false}, force_selected{false};

		bool OpenGroup();
	public:
		DefFileInfo(DefFileInfo *parent, const char *filename, const char *root_path);
		DefFileInfo(); // init as root
		int32_t GetChildCount();
		DefFileInfo *GetParent() const { return parent; }
		DefFileInfo *GetChild(int32_t index);
		int32_t GetChildIndex(const DefFileInfo *child);
		const char *GetName() const { return filename.getData(); }
		bool IsRoot() const { return is_root; }
		void SetSelected(bool to_val, bool forced);
		bool IsUserSelected() const { return user_selected; }
		bool IsForceSelected() const { return force_selected; }
		bool IsSelected() const { return user_selected || force_selected; }
		bool IsDisabled() const { return force_selected || (parent && parent->IsSelected()); }
		void AddUserSelectedDefinitions(std::list<const char *> *result) const;
		void AddSelectedDefinitions(std::list<const char *> *result) const;
		void SetForcedSelection(const char *selected_def_filepath);
		void AddExtraDef(const char *def);
	};

	mutable DefFileInfo root;

protected:
	int rowCount(const QModelIndex & parent = QModelIndex()) const override;
	int columnCount(const QModelIndex & parent = QModelIndex()) const override;
	QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
	QModelIndex index(int row, int column, const QModelIndex &parent) const override;
	QModelIndex parent(const QModelIndex &index) const override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;
	bool setData(const QModelIndex& index, const QVariant& value, int role) override;
};

class C4ConsoleQtNewScenarioDlg : public QDialog
{
	Q_OBJECT

	Ui::NewScenarioDialog ui;
	StdCopyStrBuf filename;
	bool has_custom_filename;
	C4ConsoleQtDefinitionFileListModel def_file_model;
	std::vector<C4Scenario> all_template_c4s;

public:
	C4ConsoleQtNewScenarioDlg(class QMainWindow *parent_window);
	const char *GetFilename() const { return filename.getData(); }
	bool IsHostAsNetwork() const;

private:
	void InitScenarioTemplateList();
	void AddScenarioTemplate(C4Group &parent, const char *filename, bool is_default);
	bool CreateScenario();
	void DoError(const char *msg);

protected slots:
	void CreatePressed();
	void BrowsePressed();
	void TitleChanged(const QString &new_title);
	void SelectedTemplateChanged(int new_selection);
};

#endif // WITH_QT_EDITOR
#endif // INC_C4ConsoleQtNewScenario
