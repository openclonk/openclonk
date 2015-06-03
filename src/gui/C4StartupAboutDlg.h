/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2013-2014, The OpenClonk Team and contributors
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
// Credits screen

#ifndef INC_C4StartupAboutDlg
#define INC_C4StartupAboutDlg

#include "C4Startup.h"

// startup dialog: credits
class C4StartupAboutDlg : public C4StartupDlg
{
public:
	C4StartupAboutDlg();
	~C4StartupAboutDlg(); 

protected:
	virtual bool OnEnter() { DoBack(); return true; }
	virtual bool OnEscape() { DoBack(); return true; }
	virtual void DrawElement(C4TargetFacet &cgo);
	bool KeyBack() { DoBack(); return true; }
	void OnBackBtn(C4GUI::Control *btn) { DoBack(); }
#ifdef WITH_AUTOMATIC_UPDATE
	void OnUpdateBtn(C4GUI::Control *btn);
#endif

public:

	void DoBack(); // back to main menu
};


#endif // INC_C4StartupAboutDlg
