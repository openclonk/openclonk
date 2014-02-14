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

#ifdef USE_WIN32_WINDOWS
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
#ifdef USE_WIN32_WINDOWS
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
#ifdef USE_WIN32_WINDOWS
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
