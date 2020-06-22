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

#ifndef INC_C4ConsoleQtLocalizeString
#define INC_C4ConsoleQtLocalizeString
#ifdef WITH_QT_EDITOR

#include "C4Include.h" // needed for automoc
#include "editor/C4ConsoleGUI.h" // for OpenGL
#include "editor/C4ConsoleQt.h"
#include "ui_C4ConsoleQtLocalizeString.h"

class C4ConsoleQtLocalizeStringDlg : public QDialog
{
	Q_OBJECT

	Ui::LocalizeStringDialog ui;
	C4Value translations;

	struct EditedLanguage
	{
		char language[3];
		QLineEdit *value_editor;
	};
	std::list<EditedLanguage> edited_languages;

public:
	C4ConsoleQtLocalizeStringDlg(class QMainWindow *parent_window, const C4Value &translations);
	C4PropList *GetTranslations() const { return translations.getPropList(); }

private:
	void DoError(const char *msg);
	QLineEdit *AddEditor(const char *language, const char *language_name);
	QLineEdit *GetEditorByLanguage(const char *language);
	void done(int r) override;

protected slots:
	void AddLanguagePressed();
};

#endif // WITH_QT_EDITOR
#endif // INC_C4ConsoleQtLocalizeString
