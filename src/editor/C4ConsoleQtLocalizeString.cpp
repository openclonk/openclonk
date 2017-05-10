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
#include "editor/C4ConsoleQtLocalizeString.h"
#include "c4group/C4Language.h"


/* Single string editor */

C4ConsoleQtLocalizeStringDlg::C4ConsoleQtLocalizeStringDlg(class QMainWindow *parent_window, const C4Value &translations)
	: QDialog(parent_window, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint)
	, translations(translations)
{
	ui.setupUi(this);
	// Add language editors
	int32_t lang_index = 0;
	C4LanguageInfo *lang_info;
	while (lang_info = ::Languages.GetInfo(lang_index++))
	{
		AddEditor(lang_info->Code, lang_info->Name);
	}
	// Fill in values
	C4PropList *translations_proplist = translations.getPropList();
	assert(translations_proplist);
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
					QLineEdit *editor = GetEditorByLanguage(lang_str->GetCStr());
					if (!editor)
					{
						// Unknown language. Just add an editor without language name.
						editor = AddEditor(lang_str->GetCStr(), nullptr);
					}
					editor->setText(QString(text->GetCStr()));
				}
			}
		}
	}
	// Size
	adjustSize();
	setMinimumSize(size());
	// Focus on first empty editor
	if (edited_languages.size())
	{
		edited_languages.front().value_editor->setFocus(); // fallback to first editor
		for (const auto & langs : edited_languages)
		{
			if (!langs.value_editor->text().length())
			{
				langs.value_editor->setFocus();
				break;
			}
		}
	}
}

void C4ConsoleQtLocalizeStringDlg::DoError(const char *msg)
{
	QMessageBox::critical(this, ::LoadResStr("IDS_ERR_TITLE"), QString(msg));
}

QLineEdit *C4ConsoleQtLocalizeStringDlg::AddEditor(const char *language, const char *language_name)
{
	assert(!GetEditorByLanguage(language));
	// Add editor widgets
	int32_t row = edited_languages.size();
	QString language_label_text(language);
	if (language_name) language_label_text.append(FormatString(" (%s)", language_name).getData());
	QLabel *language_label = new QLabel(language_label_text, this);
	ui.mainGrid->addWidget(language_label, row, 0);
	QLineEdit *value_editor = new QLineEdit(this);
	ui.mainGrid->addWidget(value_editor, row, 1);
	// Add to list
	EditedLanguage new_editor;
	SCopy(language, new_editor.language, 2);
	new_editor.value_editor = value_editor;
	edited_languages.push_back(new_editor);
	return value_editor;
}

QLineEdit *C4ConsoleQtLocalizeStringDlg::GetEditorByLanguage(const char *language)
{
	// Search text editor by language ID
	for (const auto & langs : edited_languages)
	{
		if (!strcmp(langs.language, language))
		{
			return langs.value_editor;
		}
	}
	// Not found
	return nullptr;
}

void C4ConsoleQtLocalizeStringDlg::done(int r)
{
	if (QDialog::Accepted == r)  // ok was pressed
	{
		C4PropList *translations_proplist = translations.getPropList();
		assert(translations_proplist);
		// Set all translations
		for (const auto & langs : edited_languages)
		{
			// Empty strings are set to nil, because that allows the user to set it to fallback
			QString text = langs.value_editor->text();
			if (text.length())
			{
				C4Value text_val = C4VString(text.toUtf8());
				translations_proplist->SetPropertyByS(::Strings.RegString(langs.language), text_val);
			}
			else
			{
				translations_proplist->ResetProperty(::Strings.RegString(langs.language));
			}
		}
	}
	// Close
	QDialog::done(r);
}

void C4ConsoleQtLocalizeStringDlg::AddLanguagePressed()
{
	bool lang_ok = false;
	QRegExpValidator validator(QRegExp("^[a-zA-Z][a-zA-Z]$"), this);
	QString lang_id;
	while (!lang_ok)
	{
		bool ok; int q = 0;
		lang_id = QInputDialog::getText(this, LoadResStr("IDS_CNS_ADDLANGUAGE"), LoadResStr("IDS_CNS_ADDLANGUAGEID"), QLineEdit::Normal, QString(), &ok);
		if (!ok) return;
		lang_ok = (validator.validate(lang_id, q) == QValidator::Acceptable);
		if (!lang_ok)
		{
			DoError(LoadResStr("IDS_ERR_INVALIDLANGUAGEID"));
		}
	}
	// Either add or just focus existing editor
	QLineEdit *editor = GetEditorByLanguage(lang_id.toUtf8());
	if (!editor)
	{
		editor = AddEditor(lang_id.toUtf8(), nullptr);
		adjustSize();
		setMinimumSize(size());
	}
	editor->setFocus();
}
