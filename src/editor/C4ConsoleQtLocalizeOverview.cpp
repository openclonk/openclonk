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

/* String localization editors */

#include "C4Include.h"
#include "script/C4Value.h"
#include "config/C4Config.h"
#include "editor/C4ConsoleQtLocalizeOverview.h"
#include "c4group/C4Language.h"


/* Single string editor */

C4ConsoleQtLocalizeOverviewDlg::C4ConsoleQtLocalizeOverviewDlg(class QMainWindow *parent_window)
	: QDialog(parent_window, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint)
{
	ui.setupUi(this);
	// Size
	adjustSize();
	setMinimumSize(size());
}

int32_t C4ConsoleQtLocalizeOverviewDlg::GetColumnByLanguage(const char *lang) const
{
	// Language column by ID
	auto iter = lang2col.find(QString(lang));
	if (iter != lang2col.end())
	{
		return iter->second;
	}
	// No column matches
	return -1;
}

int32_t C4ConsoleQtLocalizeOverviewDlg::AddLanguageColumn(const char *lang_id, const char *lang_name)
{
	// Add column
	int32_t col = ui.translationTable->columnCount();
	ui.translationTable->setColumnCount(col + 1);
	ui.translationTable->horizontalHeader()->setSectionResizeMode(col, QHeaderView::Stretch);
	// Set header
	QString text(lang_id);
	if (lang_name)
	{
		text.append(" - ").append(lang_name);
	}
	SetTableItem(0, col, TIT_Header, text);
	// Remember language to resolve index
	lang2col[QString(lang_id)] = col;
	col2lang.emplace_back(QString(lang_id));
	return col;
}

void C4ConsoleQtLocalizeOverviewDlg::SetTableItem(int32_t row, int32_t col, TableItemType item_type, const QString &text)
{
	// Set entry in translation table
	auto item = new QTableWidgetItem(QString(text));
	// Headers and info columns cannot be edited
	if (item_type != TIT_Entry)
	{
		item->setFlags(item->flags() & ~(Qt::ItemIsEditable)); // | Qt::ItemIsSelectable
	}
	// Set the entry
	if (item_type == TIT_Header)
	{
		ui.translationTable->setHorizontalHeaderItem(col, item);
	}
	else
	{
		ui.translationTable->setItem(row, col, item);
	}
}

void C4ConsoleQtLocalizeOverviewDlg::reject()
{
	// Cleanup on dialogue close to avoid hanging proplists in C4Value
	ClearTable();
	QDialog::reject();
}

void C4ConsoleQtLocalizeOverviewDlg::ClearTable()
{
	ui.translationTable->clearContents();
	lang_strings.clear();
	lang2col.clear();
	col2lang.clear();
}

void C4ConsoleQtLocalizeOverviewDlg::Refresh()
{
	// Re-fill 
	is_refreshing = true;
	// This may take a while; show a dialogue
	QProgressDialog progress(QString(LoadResStr("IDS_CNS_COLLECTINGLOCALIZATIONS")), QString(), 0, 0, this);
	progress.setCancelButton(nullptr); // Can't cancel
	progress.setWindowModality(Qt::WindowModal);

	// Clear previous
	ClearTable();

	// Collect localizable strings
	C4PropertyCollection lang_string_collector;
	lang_string_collector.CollectPropLists(P_Function, C4VString(&::Strings.P[P_Translate]));
	lang_strings = lang_string_collector.GetEntries();

	// Set up headers
	ui.translationTable->setRowCount(lang_strings.size());
	ui.translationTable->setColumnCount(2);
	SetTableItem(0, 0, TIT_Header, QString(LoadResStr("IDS_CNS_OBJECT")));
	SetTableItem(0, 1, TIT_Header, QString(LoadResStr("IDS_CNS_PATH")));
	ui.translationTable->setColumnWidth(0, 100);
	ui.translationTable->setColumnWidth(1, 200);
	col2lang.resize(2);

	// Add default language columns
	int32_t lang_index = 0;
	C4LanguageInfo *lang_info;
	while (lang_info = ::Languages.GetInfo(lang_index++))
	{
		AddLanguageColumn(lang_info->Code, lang_info->Name);
	}

	// Add them to the table
	int32_t row = 0;
	for (auto &entry : lang_strings)
	{
		assert(entry.value.GetType() == C4V_PropList);
		C4PropList *translations_proplist = entry.value._getPropList();
		assert(translations_proplist);
		
		// Add name and path
		SetTableItem(row, 0, TIT_Info, QString(entry.name.getData()));
		SetTableItem(row, 1, TIT_Info, QString(entry.path.GetGetPath()));

		// Add each language
		for (C4String *lang_str : translations_proplist->GetSortedLocalProperties(false))
		{
			if (lang_str->GetData().getLength() == 2)
			{
				C4Value text_val;
				if (translations_proplist->GetPropertyByS(lang_str, &text_val))
				{
					C4String *text = text_val.getStr();
					if (text)
					{
						int32_t col = GetColumnByLanguage(lang_str->GetCStr());
						if (col < 0)
						{
							// This is a non-default language. Add a column.
							col = AddLanguageColumn(lang_str->GetCStr(), nullptr);
						}
						// Set text for this translation
						SetTableItem(row, col, TIT_Entry, QString(text->GetCStr()));
					}
				}
			}
		}
		++row;
	}

	// Done!
	progress.close();
	is_refreshing = false;
}

void C4ConsoleQtLocalizeOverviewDlg::OnTableItemChanged(QTableWidgetItem *item)
{
	// User edits only
	if (is_refreshing)
	{
		return;
	}
	// Find path to proplist to edit
	const C4PropertyPath &prop_path = lang_strings[item->row()].path;
	// Find language to edit
	QString lang_id = col2lang[item->column()];
	// Set to new value through control queue
	QString new_value = item->text();
	if (new_value.length())
	{
		// TODO: Would be better to handle escaping in the C4Value-to-string code
		new_value = new_value.replace(R"(\)", R"(\\)").replace(R"(")", R"(\")");
		// Update in script
		C4PropertyPath set_path(prop_path, lang_id.toUtf8().data());
		set_path.SetProperty((R"(")" + new_value + R"(")").toUtf8().data());
	}
	else
	{
		// Empty string: Delete this language entry
		prop_path.DoCall(FormatString(R""(ResetProperty("%s", %%s))"", lang_id.toUtf8().data()).getData());
		
	}
}

