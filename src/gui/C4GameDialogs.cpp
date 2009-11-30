/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005  Sven Eberhardt
 * Copyright (c) 2006  Günther Brammer
 * Copyright (c) 2007-2008  Matthes Bender
 * Copyright (c) 2007  Peter Wortmann
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

#include <C4Include.h>
#include <C4GameDialogs.h>

#include <C4Viewport.h>
#include <C4Network2Dialogs.h>
#include <C4Game.h>
#include <C4Player.h>
#include <C4Network2.h>

bool C4AbortGameDialog::is_shown = false;

// ---------------------------------------------------
// C4GameAbortDlg

C4AbortGameDialog::C4AbortGameDialog()
: fGameHalted(false),
  C4GUI::ConfirmationDialog(LoadResStr("IDS_HOLD_ABORT"),
														LoadResStr("IDS_DLG_ABORT"),
														NULL,
														MessageDialog::btnYesNo,
														true,
														C4GUI::Ico_Exit)
	{
	is_shown = true; // assume dlg will be shown, soon
	}

C4AbortGameDialog::~C4AbortGameDialog()
	{
	is_shown = false;
	}

void C4AbortGameDialog::OnShown()
	{
	if(!::Network.isEnabled())
		{
		fGameHalted = true;
		Game.HaltCount++;
		}
	}

void C4AbortGameDialog::OnClosed(bool fOK)
	{
	if(fGameHalted)
		Game.HaltCount--;
	// inherited
	typedef C4GUI::ConfirmationDialog Base;
	Base::OnClosed(fOK);
	// abort
	if(fOK)
		Game.Abort();
	}
