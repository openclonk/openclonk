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

/* Gamepad control */

#include <C4Include.h>
#include <C4GamePadCon.h>

#include <C4Config.h>
#include <C4ObjectCom.h>
#include <C4Log.h>
#include <C4Game.h>

// regardless of WIN32 or SDL
void C4GamePadControl::DoAxisInput()
{
	// Send axis strength changes
	Execute(true);
}

#if defined(HAVE_SDL) && !defined(USE_CONSOLE)

#include <SDL.h>

bool C4GamePadControl::AnyButtonDown()
{
	return false;
}

C4GamePadControl::C4GamePadControl()
{
	if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS) != 0)
		LogF("SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER): %s", SDL_GetError());
	SDL_GameControllerEventState(SDL_ENABLE);
	if (!GetGamePadCount()) Log("No Gamepad found");
}

C4GamePadControl::~C4GamePadControl()
{
	SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS);
}

void C4GamePadControl::Execute(bool)
{
#ifndef USE_SDL_MAINLOOP
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_CONTROLLERAXISMOTION:
		case SDL_CONTROLLERBUTTONDOWN:
		case SDL_CONTROLLERBUTTONUP:
			FeedEvent(event);
			break;
		}
	}
#endif
}

namespace
{
	const int deadZone = 13337;

	int amplify(int i)
	{
		if (i < 0)
			return -(deadZone + 1);
		if (i > 0)
			return deadZone + 1;
		return 0;
	}
}

void C4GamePadControl::FeedEvent(SDL_Event& event)
{
	switch (event.type)
	{
	case SDL_CONTROLLERAXISMOTION:
	{
		C4KeyCode minCode = KEY_Gamepad(event.caxis.which, KEY_JOY_Axis(event.caxis.axis, false));
		C4KeyCode maxCode = KEY_Gamepad(event.caxis.which, KEY_JOY_Axis(event.caxis.axis, true));

		// FIXME: This assumes that the axis really rests around (0, 0) if it is not used, which is not always true.
		if (event.caxis.value < -deadZone)
		{
			if (PressedAxis.count(minCode) == 0)
			{
				Game.DoKeyboardInput(
				  KEY_Gamepad(event.caxis.which, minCode),
				  KEYEV_Down, false, false, false, false);
				PressedAxis.insert(minCode);
			}
		}
		else
		{
			if (PressedAxis.count(minCode) != 0)
			{
				Game.DoKeyboardInput(
				  KEY_Gamepad(event.caxis.which, minCode),
				  KEYEV_Up, false, false, false, false);
				PressedAxis.erase(minCode);
			}
		}
		if (event.caxis.value > +deadZone)
		{
			if (PressedAxis.count(maxCode) == 0)
			{
				Game.DoKeyboardInput(
				  KEY_Gamepad(event.caxis.which, maxCode),
				  KEYEV_Down, false, false, false, false);
				PressedAxis.insert(maxCode);
			}
		}
		else
		{
			if (PressedAxis.count(maxCode) != 0)
			{
				Game.DoKeyboardInput(
				  KEY_Gamepad(event.caxis.which, maxCode),
				  KEYEV_Up, false, false, false, false);
				PressedAxis.erase(maxCode);
			}
		}
		break;
	}
	case SDL_CONTROLLERBUTTONDOWN:
		Game.DoKeyboardInput(
		  KEY_Gamepad(event.cbutton.which, KEY_JOY_Button(event.cbutton.button)),
		  KEYEV_Down, false, false, false, false);
		break;
	case SDL_CONTROLLERBUTTONUP:
		Game.DoKeyboardInput(
		  KEY_Gamepad(event.cbutton.which, KEY_JOY_Button(event.cbutton.button)),
		  KEYEV_Up, false, false, false, false);
		break;
	}
}

int C4GamePadControl::GetGamePadCount()
{
	// Not all Joysticks are game controllers.
	int count = 0;
	for (int i = 0; i < SDL_NumJoysticks(); i++)
		if (SDL_IsGameController(i))
			count++;
	return count;
}

C4GamePadOpener::C4GamePadOpener(int iGamepad)
{
	int n = iGamepad;
	for (int i = 0; i < SDL_NumJoysticks(); i++)
		if (SDL_IsGameController(i) && n-- == 0)
		{
			controller = SDL_GameControllerOpen(i);
			if (!controller) LogF("SDL: %s", SDL_GetError());
			break;
		}

	if (!controller) LogF("Gamepad %d not available", iGamepad);
}

C4GamePadOpener::~C4GamePadOpener()
{
	if (controller) SDL_GameControllerClose(controller);
}

void C4GamePadOpener::SetGamePad(int iGamepad)
{
	// TODO: why do we need this?
	LogF("SetGamePad: Not implemented yet");
}

#else

// Dedicated server and everything else with neither Win32 nor SDL.

C4GamePadControl::C4GamePadControl() { Log("WARNING: Engine without Gamepad support"); }
C4GamePadControl::~C4GamePadControl() { }
void C4GamePadControl::Execute(bool) { }
int C4GamePadControl::GetGamePadCount() { return 0; }
bool C4GamePadControl::AnyButtonDown() { return false; }

C4GamePadOpener::C4GamePadOpener(int iGamepad) { }
C4GamePadOpener::~C4GamePadOpener() {}
void C4GamePadOpener::SetGamePad(int iGamepad) { }

#endif
