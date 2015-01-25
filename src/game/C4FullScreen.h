/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
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

/* Main class to execute the game fullscreen mode */

#ifndef INC_C4FullScreen
#define INC_C4FullScreen

#include <C4Window.h>

bool IsKeyDown(int iKey);

class C4FullScreen: public C4Window
{
public:
	C4MainMenu *pMenu;
public:
	C4FullScreen();
	~C4FullScreen();
	void Execute();
	bool Restart();
	bool ViewportCheck();
	bool ShowAbortDlg(); // show game abort dialog (Escape pressed)
	bool ActivateMenuMain();
	bool MenuCommand(const char *szCommand);
	void CloseMenu();
	bool MenuKeyControl(BYTE byCom); // direct keyboard callback
	using C4Window::Init;
	C4Window * Init(C4AbstractApp * pApp);
	// User requests close
	virtual void Close();
	virtual void Clear();
	virtual void CharIn(const char * c);
	virtual void PerformUpdate();
};

extern C4FullScreen   FullScreen;

#endif
