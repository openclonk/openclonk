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
#include <map>
#endif

struct _SDL_GameController;
typedef struct _SDL_GameController SDL_GameController;

struct _SDL_Haptic;
typedef struct _SDL_Haptic SDL_Haptic;

union SDL_Event;
typedef union SDL_Event SDL_Event;

class C4GamePadControl
{
#ifdef HAVE_SDL
public:
	enum {
		FEED_BUTTONS = 1,
		FEED_MOVED   = 2,
	};
	// Called from C4AppSDL
	void FeedEvent(const SDL_Event& e, int feed);
private:
	std::set<C4KeyCode> PressedAxis; // for button emulation
	std::map<C4KeyCode, SDL_Event> AxisEvents; // for analog movement events
#endif
public:
	C4GamePadControl();
	~C4GamePadControl();
	void Clear();
	int GetGamePadCount();
	void Execute();
	void DoAxisInput(); // period axis strength update controls sent on each control frame creation
};

class C4GamePadOpener
{
public:
	C4GamePadOpener(int iGamePad);
	~C4GamePadOpener();

	// Force feedback: simple rumbling
	void PlayRumble(float strength, uint32_t length); // strength: 0-1, length: milliseconds
	void StopRumble();

#ifdef HAVE_SDL
	SDL_GameController *controller;
	SDL_Haptic *haptic;
#endif
};

#endif
