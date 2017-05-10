/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2016, The OpenClonk Team and contributors
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
// main game dialogs (abort game dlg, observer dlg)

#ifndef INC_C4GameDialogs
#define INC_C4GameDialogs

#include "gui/C4Gui.h"

class C4AbortGameDialog : public C4GUI::ConfirmationDialog
{
public:
	C4AbortGameDialog();
	~C4AbortGameDialog() override;

protected:
	static bool is_shown;

	// callbacks to halt game
	void OnShown() override;    // inc game halt counter
	void OnClosed(bool fOK) override;    // dec game halt counter

	const char *GetID() override { return "AbortGameDialog"; }

	// align by screen, not viewport
	bool IsFreePlaceDialog() override { return true; }

	// true for dialogs that receive full keyboard and mouse input even in shared mode
	bool IsExclusiveDialog() override { return true; }

	bool fGameHalted{false};

public:
	static bool IsShown() { return is_shown; }

};

#endif // INC_C4GameDialogs
