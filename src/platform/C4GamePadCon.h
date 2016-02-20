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

#include <memory>

#ifdef HAVE_SDL
#include <C4KeyboardInput.h>
#include <set>
#include <map>

#include <SDL.h>
#endif

class C4GamePadOpener;

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
	void CheckGamePad(const SDL_Event& e);
private:
	std::set<C4KeyCode> PressedAxis; // for button emulation
	std::map<C4KeyCode, SDL_Event> AxisEvents; // for analog movement events
	std::map<int32_t, std::shared_ptr<C4GamePadOpener> > Gamepads; // gamepad instance id -> gamepad
#endif
public:
	C4GamePadControl();
	~C4GamePadControl();
	void Clear();
	int GetGamePadCount();
	void Execute();
	void DoAxisInput(); // period axis strength update controls sent on each control frame creation

	std::shared_ptr<C4GamePadOpener> GetGamePad(int gamepad); // Gets the nth gamepad.
	std::shared_ptr<C4GamePadOpener> GetGamePadByID(int32_t id); // Gets a gamepad by its instance id.
	std::shared_ptr<C4GamePadOpener> GetAvailableGamePad(); // Looks for a gamepad that doesn't have an assigned player.
};

class C4GamePadOpener
{
	int32_t player = -1;

public:
	C4GamePadOpener(int iGamePad);
	~C4GamePadOpener();

	// A gamepad can be assigned to a player.
	int32_t GetPlayer() const { return player; }
	void SetPlayer(int32_t plr) { player = plr; }

	int32_t GetID(); // Returns the gamepad's instance id.
	bool IsAttached(); // Returns whether the gamepad is currently attached.

	// Force feedback: simple rumbling
	void PlayRumble(float strength, uint32_t length); // strength: 0-1, length: milliseconds
	void StopRumble();

#ifdef HAVE_SDL
	SDL_GameController *controller;
	SDL_Haptic *haptic;
#endif
};

#endif
