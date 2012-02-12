/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005  Sven Eberhardt
 * Copyright (c) 2008  Matthes Bender
 * Copyright (c) 2005-2009, RedWolf Design GmbH, http://www.clonk.de
 *
 * Portions might be copyrighted by other authors who have contributed
 * to OpenClonk.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * See isc_license.txt for full license and disclaimer.
 *
 * "Clonk" is a registered trademark of Matthes Bender.
 * See clonk_trademark_license.txt for full license.
 */
// main game dialogs (abort game dlg, observer dlg)

#ifndef INC_C4GameDialogs
#define INC_C4GameDialogs

#include <C4Gui.h>

class C4AbortGameDialog : public C4GUI::ConfirmationDialog
{
public:
	C4AbortGameDialog();
	~C4AbortGameDialog();

protected:
	static bool is_shown;

	// callbacks to halt game
	virtual void OnShown();    // inc game halt counter
	virtual void OnClosed(bool fOK);    // dec game halt counter

	virtual const char *GetID() { return "AbortGameDialog"; }

	// align by screen, not viewport
	virtual bool IsFreePlaceDialog() { return true; }

	// true for dialogs that receive full keyboard and mouse input even in shared mode
	virtual bool IsExclusiveDialog() { return true; }

	bool fGameHalted;

public:
	static bool IsShown() { return is_shown; }

};

#endif // INC_C4GameDialogs
