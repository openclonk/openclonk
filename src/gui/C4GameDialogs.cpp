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

#include "C4Include.h"
#include "gui/C4GameDialogs.h"

#include "game/C4Game.h"
#include "network/C4Network2.h"

bool C4AbortGameDialog::is_shown = false;

// ---------------------------------------------------
// C4GameAbortDlg

C4AbortGameDialog::C4AbortGameDialog()
		: C4GUI::ConfirmationDialog(LoadResStr("IDS_HOLD_ABORT"),
		                            LoadResStr("IDS_DLG_ABORT"),
		                            nullptr,
		                            MessageDialog::btnYesNo,
		                            true,
		                            C4GUI::Ico_Exit),
		fGameHalted(false)
{
	is_shown = true; // assume dlg will be shown, soon
}

C4AbortGameDialog::~C4AbortGameDialog()
{
	is_shown = false;
}

void C4AbortGameDialog::OnShown()
{
	if (!::Network.isEnabled())
	{
		fGameHalted = true;
		Game.HaltCount++;
	}
}

void C4AbortGameDialog::OnClosed(bool fOK)
{
	if (fGameHalted)
		Game.HaltCount--;
	// inherited
	typedef C4GUI::ConfirmationDialog Base;
	Base::OnClosed(fOK);
	// abort
	if (fOK)
		Game.Abort();
}
