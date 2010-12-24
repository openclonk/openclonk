/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2001, 2006  Sven Eberhardt
 * Copyright (c) 2006  Günther Brammer
 * Copyright (c) 2007  Julian Raschke
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de
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

/* Gamepad control - forwards gamepad events of opened gamepads to Game.KeyboardInput */

#ifndef INC_C4GamePadCon
#define INC_C4GamePadCon

#ifdef _WIN32
#include <StdJoystick.h>
#endif

#ifdef HAVE_SDL
#include <C4KeyboardInput.h>
#include <set>
#endif

struct _SDL_Joystick;
typedef struct _SDL_Joystick SDL_Joystick;

union SDL_Event;
typedef union SDL_Event SDL_Event;

class C4GamePadControl
{
#ifdef _WIN32
private:
	struct Pad
	{
		CStdGamePad *pGamepad;
		int iRefCount;
		uint32_t Buttons;
		CStdGamePad::AxisPos AxisPosis[CStdGamepad_MaxAxis];
		int32_t AxisStrengths[CStdGamepad_MaxAxis];
	};
	Pad Gamepads[CStdGamepad_MaxGamePad];
	int iNumGamepads;

	enum { AxisStrengthChangeThreshold = 2 }; // if axis strength change > this value, a new control is issued

public:
	void OpenGamepad(int id);  // add gamepad ref
	void CloseGamepad(int id); // del gamepad ref
	static C4GamePadControl *pInstance; // singleton
#elif defined(HAVE_SDL)
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
#ifdef _WIN32
	int iGamePad;
	int GetGamePadIndex() const { return iGamePad; }
#endif
public:
	C4GamePadOpener(int iGamePad);
	~C4GamePadOpener();
	void SetGamePad(int iNewGamePad);
#ifdef HAVE_SDL
	SDL_Joystick *Joy;
#endif
};

#endif
