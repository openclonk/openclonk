/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2007-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2010-2013, The OpenClonk Team and contributors
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
// dialogs for update

#ifndef INC_C4UpdateDialogs
#define INC_C4UpdateDialogs

#ifdef WITH_AUTOMATIC_UPDATE

#include "C4Gui.h"
#include "C4GameVersion.h"
#include "C4Network2Reference.h"

// dialog showing info about a connected client
class C4UpdateDlg : public C4GUI::InfoDialog
{
protected:
	virtual const char *GetID() { return "UpdateDialog"; }
	virtual void UpdateText();
	virtual void UserClose(bool fOK);

	bool UpdateRunning;

	// Misc process variables which are shared between the static DoUpdate and the update dialog
	static int pid;
	static int c4group_output[2];
	static bool succeeded;

public:
	C4UpdateDlg(); // ctor

public:
	void Open(C4GUI::Screen *pScreen);
	void Write(const char *szText);

public:
	static bool IsValidUpdate(const char *szVersion); // Returns whether we can update to the specified version
	static bool CheckForUpdates(C4GUI::Screen *pScreen, bool fAutomatic = false); // Checks for available updates and prompts the user whether to apply
	static bool DoUpdate(const char *szUpdateURL, C4GUI::Screen *pScreen); // Static funtion for downloading and applying updates
	static bool ApplyUpdate(const char *strUpdateFile, bool fDeleteUpdate, C4GUI::Screen *pScreen); // Static funtion for applying updates
	static void RedirectToDownloadPage(); // open browser with download page
};

#endif // WITH_AUTOMATIC_UPDATE
#endif // INC_C4UpdateDialogs
