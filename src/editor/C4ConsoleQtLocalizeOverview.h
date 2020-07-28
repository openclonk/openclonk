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

/* String localization editor */

#ifndef INC_C4ConsoleQtLocalizeOverview
#define INC_C4ConsoleQtLocalizeOverview
#ifdef WITH_QT_EDITOR

#include "C4Include.h" // needed for automoc
#include "editor/C4ConsoleGUI.h" // for OpenGL
#include "editor/C4ConsoleQt.h"
#include "editor/C4PropertyPath.h"
#include "ui_C4ConsoleQtLocalizeOverview.h"

class C4ConsoleQtLocalizeOverviewDlg : public QDialog
{
	Q_OBJECT

	Ui::LocalizeOverviewDialog ui;
	std::vector<C4PropertyCollection::Entry> lang_strings;
	std::map<QString, int32_t> lang2col; // Language-to-column lookup
	std::vector<QString> col2lang; // Column-to-language loopup
	bool is_refreshing{ false };

	enum TableItemType
	{
		TIT_Header, // Top row
		TIT_Info,   // Name+Path columns
		TIT_Entry,  // Editable string entry
	};

public:
	C4ConsoleQtLocalizeOverviewDlg(class QMainWindow *parent_window);

private:
	int32_t GetColumnByLanguage(const char *lang) const;
	int32_t AddLanguageColumn(const char *lang_id, const char *lang_name);
	void SetTableItem(int32_t row, int32_t col, TableItemType item_type, const QString &text);
	void reject() override;
	void ClearTable();

public slots:
	void Refresh();
	void OnTableItemChanged(QTableWidgetItem *item);
};

#endif // WITH_QT_EDITOR
#endif // INC_C4ConsoleQtLocalizeOverview
