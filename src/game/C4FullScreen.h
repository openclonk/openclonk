/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2013-2016, The OpenClonk Team and contributors
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

#include "platform/C4Window.h"

bool IsKeyDown(int iKey);

class C4FullScreen: public C4Window
{
public:
	C4MainMenu *MainMenu;
public:
	C4FullScreen();
	~C4FullScreen() override;
	void Execute();
	bool ViewportCheck();
	bool ShowAbortDlg(); // show game abort dialog (Escape pressed)
	bool ActivateMenuMain();
	void CloseMenu();
	bool MenuKeyControl(BYTE command); // direct keyboard callback
	using C4Window::Init;
	C4Window * Init(C4AbstractApp * application);
	// User requests close
	void Close() override;
	void Clear() override;
	void CharIn(const char * c) override;
	void PerformUpdate() override;
};

extern C4FullScreen   FullScreen;

#endif
