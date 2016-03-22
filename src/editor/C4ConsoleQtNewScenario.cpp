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

/* "New scenario" editor dialogue */

#include <C4Include.h>
#include <C4Value.h>
#include <C4Config.h>
#include <C4ConsoleQtNewScenario.h>


/* Definition file list model for new scenario definition selection */

C4ConsoleQtDefinitionFileListModel::DefFileInfo::DefFileInfo(C4ConsoleQtDefinitionFileListModel::DefFileInfo *parent, const char *filename, const char *root_path)
	: parent(parent), filename(filename), root_path(root_path), was_opened(false), is_root(false), selected(parent->IsSelected()), disabled(parent->IsSelected() /*not a bug*/)
{
	// Delay opening of groups until information is actually requested
	// Full names into child groups in C4S always delimeted with backslashes
	if (parent->full_filename.getLength())
		full_filename = parent->full_filename + "\\" + filename;
	else
		full_filename = filename;
}

C4ConsoleQtDefinitionFileListModel::DefFileInfo::DefFileInfo()
	: parent(NULL), was_opened(true), is_root(true), selected(false), disabled(false)
{
	// Init as root: List definitions in root paths
	// Objects.ocd is always there (even if not actually found) and always first
	DefFileInfo *main_objects_def = new DefFileInfo(this, C4CFN_Objects, "");
	children.emplace_back(main_objects_def);
	main_objects_def->SetSelected(true);
	main_objects_def->SetDisabled(true);
	bool has_default_objects_found = false;
	for (auto & root_iter : ::Reloc)
	{
		const char *root = root_iter.strBuf.getData();
		for (DirectoryIterator def_file_iter(root); *def_file_iter; ++def_file_iter)
		{
			const char *def_file = ::GetFilename(*def_file_iter);
			if (WildcardMatch(C4CFN_DefFiles, def_file))
			{
				// Set path of main objects if found
				if (!has_default_objects_found && !strcmp(C4CFN_Objects, def_file))
				{
					main_objects_def->root_path.Copy(root);
					continue;
				}
				// Avoid duplicates on top level
				bool dup = false;
				for (auto & child : children)
					if (!strcmp(child->GetName(), def_file))
					{
						dup = true; break;
					}
				if (dup) continue;
				children.emplace_back(new DefFileInfo(this, def_file, root));
			}
		}
	}
}

void C4ConsoleQtDefinitionFileListModel::DefFileInfo::SetSelected(bool to_val)
{
	selected = to_val;
	// Selection propagates to children; children of selected are disabled because they cannot be un-selected
	for (auto & child : children)
	{
		child->SetSelected(selected);
		child->SetDisabled(selected);
	}
}

bool C4ConsoleQtDefinitionFileListModel::DefFileInfo::OpenGroup()
{
	children.clear();
	was_opened = true; // mark as opened even if fails to prevent permanent re-loading of broken groups
	if (parent->IsRoot())
	{
		if (!grp.Open((root_path + DirectorySeparator + filename).getData())) return false;
	}
	else
	{
		if (!grp.OpenAsChild(&parent->grp, filename.getData())) return false;
	}
	// Init child array (without loading full groups)
	StdStrBuf child_filename;
	children.reserve(grp.EntryCount(C4CFN_DefFiles));
	grp.ResetSearch();
	while (grp.FindNextEntry(C4CFN_DefFiles, &child_filename))
		children.emplace_back(new DefFileInfo(this, child_filename.getData(), NULL));
	return true;
}

int32_t C4ConsoleQtDefinitionFileListModel::DefFileInfo::GetChildCount()
{
	if (!was_opened) OpenGroup();
	return children.size();
}

C4ConsoleQtDefinitionFileListModel::DefFileInfo *C4ConsoleQtDefinitionFileListModel::DefFileInfo::GetChild(int32_t index)
{
	if (!was_opened) OpenGroup();
	if (index >= children.size()) return NULL;
	return children[index].get();
}

int32_t C4ConsoleQtDefinitionFileListModel::DefFileInfo::GetChildIndex(const DefFileInfo *child)
{
	auto iter = std::find_if(children.begin(), children.end(),
		[child](std::unique_ptr<DefFileInfo> & item)->bool { return item.get() == child; });
	if (iter == children.end()) return -1; // not found
	return int32_t(iter - children.begin());
}

void C4ConsoleQtDefinitionFileListModel::DefFileInfo::AddSelectedDefinitions(std::list<const char *> *result) const
{
	// Add parent-most selected
	if (IsSelected())
		result->push_back(full_filename.getData());
	else
		for (auto &iter : children) iter->AddSelectedDefinitions(result);
}

C4ConsoleQtDefinitionFileListModel::C4ConsoleQtDefinitionFileListModel()
{
}

C4ConsoleQtDefinitionFileListModel::~C4ConsoleQtDefinitionFileListModel()
{
}

std::list<const char *> C4ConsoleQtDefinitionFileListModel::GetSelectedDefinitions() const
{
	std::list<const char *> result;
	root.AddSelectedDefinitions(&result);
	return result;
}

int C4ConsoleQtDefinitionFileListModel::rowCount(const QModelIndex & parent) const
{
	if (!parent.isValid()) return root.GetChildCount();
	DefFileInfo *parent_def = static_cast<DefFileInfo *>(parent.internalPointer());
	if (!parent_def) return 0;
	return parent_def->GetChildCount();
}

int C4ConsoleQtDefinitionFileListModel::columnCount(const QModelIndex & parent) const
{
	return 1; // Name
}

QVariant C4ConsoleQtDefinitionFileListModel::data(const QModelIndex & index, int role) const
{
	DefFileInfo *def = static_cast<DefFileInfo *>(index.internalPointer());
	if (!def) return QVariant();
	// Query latest data from prop list
	if (role == Qt::DisplayRole)
	{
		return QString(def->GetName());
	}
	else if (role == Qt::CheckStateRole)
	{
		return def->IsSelected() ? Qt::Checked : Qt::Unchecked;
	}
	// Nothing to show
	return QVariant();
}

QModelIndex C4ConsoleQtDefinitionFileListModel::index(int row, int column, const QModelIndex &parent) const
{
	if (column) return QModelIndex();
	DefFileInfo *parent_def = &root;
	if (parent.isValid()) parent_def = static_cast<DefFileInfo *>(parent.internalPointer());
	if (!parent_def) return QModelIndex();
	return createIndex(row, column, parent_def->GetChild(row));
}

QModelIndex C4ConsoleQtDefinitionFileListModel::parent(const QModelIndex &index) const
{
	DefFileInfo *def = static_cast<DefFileInfo *>(index.internalPointer());
	if (!def) return QModelIndex();
	DefFileInfo *parent_def = def->GetParent();
	if (!parent_def) return QModelIndex();
	int32_t def_index = parent_def->GetChildIndex(def);
	if (def_index < 0) return QModelIndex(); // can't happen
	return createIndex(def_index, 0, parent_def);
}

Qt::ItemFlags C4ConsoleQtDefinitionFileListModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
	DefFileInfo *def = static_cast<DefFileInfo *>(index.internalPointer());
	if (def && !def->IsDisabled()) flags |= Qt::ItemIsEnabled;
	return flags;
}

bool C4ConsoleQtDefinitionFileListModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	// Adjust check-state
	if (role == Qt::CheckStateRole)
	{
		DefFileInfo *def = static_cast<DefFileInfo *>(index.internalPointer());
		if (def && !def->IsDisabled())
		{
			def->SetSelected(value.toBool());
			// Update changed index and all children
			int32_t child_count = def->GetChildCount();
			QModelIndex end_changed = index;
			if (child_count) end_changed = createIndex(child_count - 1, 0, def->GetChild(child_count - 1));
			emit dataChanged(index, end_changed);
		}
	}
	return true;
}


/* New scenario dialogue */

C4ConsoleQtNewScenarioDlg::C4ConsoleQtNewScenarioDlg(class QMainWindow *parent_window)
	: QDialog(parent_window, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint)
	, has_custom_filename(false)
{
	ui.setupUi(this);
	adjustSize();
	setMinimumSize(size());
	ui.filenameEdit->setText(::Config.General.UserDataPath);
	ui.definitionTreeView->setModel(&def_file_model);
}

bool C4ConsoleQtNewScenarioDlg::SaveScenario(C4Group &grp)
{
	// Save c4s
	if (!c4s.Save(grp)) return false;
	//return grp.Save(false); -- not needed because group is unpacked
	return true;
}

void C4ConsoleQtNewScenarioDlg::CreatePressed()
{
	// Take over settings
	c4s.Landscape.MapWdt.SetConstant(ui.mapWidthSpinBox->value());
	c4s.Landscape.MapHgt.SetConstant(ui.mapHeightSpinBox->value());
	c4s.Landscape.MapZoom.SetConstant(ui.mapZoomSpinBox->value());
	SCopy(ui.titleEdit->text().toUtf8(), c4s.Head.Title, C4MaxTitle);
	c4s.Game.Mode.Copy(ui.gameModeComboBox->currentText().toUtf8());
	if (c4s.Game.Mode == "Undefined") c4s.Game.Mode.Clear();
	filename.Copy(ui.filenameEdit->text().toUtf8());
	std::list<const char *> definitions = def_file_model.GetSelectedDefinitions();
	if (definitions.size() > C4S_MaxDefinitions)
	{
		DoError(FormatString(::LoadResStr("IDS_ERR_TOOMANYDEFINITIONS"), (int)definitions.size(), (int)C4S_MaxDefinitions).getData());
		ui.definitionTreeView->setFocus();
		return;
	}
	std::ostringstream definitions_join("");
	if (definitions.size())
	{
		// definitions_join = definitions.join(";")
		auto iter_end = definitions.end();
		std::copy(definitions.begin(), --iter_end, std::ostream_iterator<std::string>(definitions_join, ";"));
		definitions_join << *iter_end;
	}
	c4s.Definitions.SetModules(definitions_join.str().c_str());
	// Check validity of settings
	if (!*c4s.Head.Title)
	{
		DoError(::LoadResStr("IDS_ERR_ENTERTITLE"));
		ui.titleEdit->setFocus();
		return;
	}
	if (ItemExists(filename.getData()))
	{
		DoError(::LoadResStr("IDS_ERR_NEWSCENARIOFILEEXISTS"));
		ui.titleEdit->setFocus();
		return;
	}
	// Try to create scenario
	if (!CreatePath(filename.getData()))
	{
		DoError(::LoadResStr("IDS_ERR_CREATESCENARIO"));
		ui.titleEdit->setFocus();
		return;
	}
	C4Group grp;
	if (!grp.Open(filename.getData(), false) || !SaveScenario(grp))
	{
		grp.Close();
		EraseDirectory(filename.getData());
		DoError(::LoadResStr("IDS_ERR_CREATESCENARIO"));
		ui.titleEdit->setFocus();
		return;
	}
	// Close dialogue with OK
	accept();
}

// Filter for allowed characters in filename
// (Also replace space, because spaces in filenames suk)
static char ReplaceSpecialFilenameChars(char c)
{
	const char *special_chars = "\\/:<>|$?\" ";
	return strchr(special_chars, c) ? '_' : c;
}

void C4ConsoleQtNewScenarioDlg::TitleChanged(const QString &new_title)
{
	if (!has_custom_filename)
	{
		// Default filename by title
		std::string filename = new_title.toStdString();
		std::transform(filename.begin(), filename.end(), filename.begin(), ReplaceSpecialFilenameChars);
		filename += (C4CFN_ScenarioFiles+1);
		ui.filenameEdit->setText(Config.AtUserDataPath(filename.c_str()));
	}
}

void C4ConsoleQtNewScenarioDlg::DoError(const char *msg)
{
	QMessageBox::critical(this, ::LoadResStr("IDS_ERR_TITLE"), QString(msg));
}

void C4ConsoleQtNewScenarioDlg::BrowsePressed()
{
	// Browse for new filename to be used instead of the filename generated from the title
	QString new_file;
	for (;;)
	{
		new_file = QFileDialog::getSaveFileName(this, LoadResStr("IDS_CNS_NEWSCENARIO"), Config.General.UserDataPath, QString("%1 (%2)").arg(LoadResStr("IDS_CNS_SCENARIOFILE")).arg(C4CFN_ScenarioFiles), NULL, QFileDialog::DontConfirmOverwrite);
		if (!new_file.size()) return;
		// Extension must be .ocs
		if (!new_file.endsWith(C4CFN_ScenarioFiles + 1)) new_file += (C4CFN_ScenarioFiles + 1);
		if (!ItemExists(new_file.toUtf8())) break;
		// Overwriting of existing scenarios not supported
		QMessageBox::critical(this, ::LoadResStr("IDS_ERR_TITLE"), ::LoadResStr("IDS_ERR_NEWSCENARIOFILEEXISTS"));
	}
	filename.Copy(new_file.toUtf8());
	ui.filenameEdit->setText(filename.getData()); // set from converted filename just in case weird stuff happened in toUtf8
	// After setting a new filename, it no longer changes when changing the title
	has_custom_filename = true;
}
