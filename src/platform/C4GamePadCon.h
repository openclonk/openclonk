/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
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

/* Gamepad control - forwards gamepad events of opened gamepads to Game.KeyboardInput */

#ifndef INC_C4GamePadCon
#define INC_C4GamePadCon

#ifdef HAVE_SDL
#include <C4KeyboardInput.h>
#include <set>
#endif

struct _SDL_GameController;
typedef struct _SDL_GameController SDL_GameController;

union SDL_Event;
typedef union SDL_Event SDL_Event;

class C4GamePadControl
{
#ifdef HAVE_SDL
public:
	void FeedEvent(SDL_Event& e);
private:
	std::set<C4KeyCode> PressedAxis;
#endif
public:
	C4GamePadControl();
	~C4GamePadControl();
	void Clear();
	int GetGamePadCount();
	void Execute(bool send_axis_strength_changes=false);
	void DoAxisInput(); // period axis strength update controls sent on each control frame creation
	static bool AnyButtonDown();
};

class C4GamePadOpener
{
public:
	C4GamePadOpener(int iGamePad);
	~C4GamePadOpener();
	void SetGamePad(int iNewGamePad);
#ifdef HAVE_SDL
	SDL_GameController *controller;
#endif
};

#endif
