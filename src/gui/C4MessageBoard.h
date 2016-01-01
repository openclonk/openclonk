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

/* Fullscreen startup log and chat type-in */

#ifndef INC_C4MessageBoard
#define INC_C4MessageBoard

const int C4MSGB_MaxMsgFading   = 6;

#include <C4Facet.h>
#include <C4LogBuf.h>

class C4CustomKey;
class C4MessageBoard
{
public:
	C4MessageBoard();
	~C4MessageBoard();

	C4Facet Output;
private:
	float ScreenFader;
	bool Startup;
	// mode 0:
	int Delay;  // how long the curr msg will stay
	int Fader;  // =0: hold curr msg until Delay == 0
	// >0: fade curr msg in
	int Speed;  // fade/delay speed
	int iBackScroll; // how many lines scrolled back?
	int iLineHgt; // line height

	std::unique_ptr<C4KeyBinding> ScrollUpBinding, ScrollDownBinding;

	C4LogBuffer LogBuffer; // backbuffer for log
public:
	void Init(C4Facet &cgo, bool fStartup);
	void Execute();
	void Draw(C4Facet &cgo);
	void AddLog(const char *szMessage);
	void ClearLog();
	void LogNotify();
	void EnsureLastMessage();
	bool ControlScrollUp();
	bool ControlScrollDown();
	C4Player* GetMessagePlayer(const char *szMessage);

	friend class C4MessageInput;
};


#endif
