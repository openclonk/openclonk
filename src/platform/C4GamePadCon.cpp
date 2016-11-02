/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2010-2016, The OpenClonk Team and contributors
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

#include "C4Include.h"
#include "C4ForbidLibraryCompilation.h"
#include "platform/C4GamePadCon.h"

#include "config/C4Config.h"
#include "object/C4ObjectCom.h"
#include "lib/C4Log.h"
#include "game/C4Application.h"
#include "game/C4Game.h"

#if defined(HAVE_SDL) && !defined(USE_CONSOLE)

#include <SDL.h>

C4GamePadControl::C4GamePadControl()
{
	// SDL2 will only report events when the window has focus, so set
	// this hint as we don't have a window
	SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");

	if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC | SDL_INIT_EVENTS) != 0)
		LogF("SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER): %s", SDL_GetError());
	SDL_GameControllerEventState(SDL_ENABLE);
	if (!GetGamePadCount()) Log("No Gamepad found");
}

C4GamePadControl::~C4GamePadControl()
{
	// All gamepads have to be released before quitting SDL.
	Gamepads.clear();
	SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS);
}

void C4GamePadControl::Execute()
{
#ifdef USE_SDL_MAINLOOP
	if (!Application.isEditor) return;
#endif
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_CONTROLLERAXISMOTION:
		case SDL_CONTROLLERBUTTONDOWN:
		case SDL_CONTROLLERBUTTONUP:
			FeedEvent(event, FEED_BUTTONS);
			break;
		case SDL_JOYDEVICEADDED:
		case SDL_CONTROLLERDEVICEADDED:
		case SDL_CONTROLLERDEVICEREMOVED:
			CheckGamePad(event);
			break;
		}
	}
}

namespace
{
	const int deadZone = 16000;

	// Axis strength uses the full signed 16 bit integer range. As we're
	// splitting axes in left/right and up/down, it's preferable to have
	// symmetrical ranges [0, 2^15 - 1] in both directions.
	inline int32_t abs_strength(int32_t strength)
	{
		return strength >= 0 ? strength : -(strength + 1);
	}
}

void C4GamePadControl::FeedEvent(const SDL_Event& event, int feed)
{
	switch (event.type)
	{
	case SDL_CONTROLLERAXISMOTION:
	{
		C4KeyCode minCode = KEY_Gamepad(KEY_CONTROLLER_Axis(event.caxis.axis, false));
		C4KeyCode maxCode = KEY_Gamepad(KEY_CONTROLLER_Axis(event.caxis.axis, true));
		int32_t value = abs_strength(event.caxis.value);
		uint8_t which = event.caxis.which;
		C4KeyCode keyCode = event.caxis.value >= 0 ? maxCode : minCode;

		auto doInput = [&](C4KeyEventType event, int32_t strength)
		{
			Game.DoKeyboardInput(
			  C4KeyCodeEx(KEY_Gamepad(keyCode), KEYS_None, false, which),
			  event, nullptr, false, strength);
		};

		if (feed & FEED_BUTTONS)
		{
			// Also emulate button presses.
			if (PressedAxis.count(keyCode) && value <= deadZone)
			{
				PressedAxis.erase(keyCode);
				doInput(KEYEV_Up, -1);
			}
			else if (!PressedAxis.count(keyCode) && value > deadZone)
			{
				PressedAxis.insert(keyCode);
				doInput(KEYEV_Down, -1);
			}
		}
		if (feed & FEED_MOVED)
			doInput(KEYEV_Moved, value);

		AxisEvents[keyCode] = event;

		break;
	}
	case SDL_CONTROLLERBUTTONDOWN:
		if (feed & FEED_BUTTONS)
			Game.DoKeyboardInput(
			  C4KeyCodeEx(KEY_Gamepad(KEY_CONTROLLER_Button(event.cbutton.button)), KEYS_None, false, event.cbutton.which),
			  KEYEV_Down);
		break;
	case SDL_CONTROLLERBUTTONUP:
		if (feed & FEED_BUTTONS)
			Game.DoKeyboardInput(
			  C4KeyCodeEx(KEY_Gamepad(KEY_CONTROLLER_Button(event.cbutton.button)), KEYS_None, false, event.cbutton.which),
			  KEYEV_Up);
		break;
	}
}

void C4GamePadControl::CheckGamePad(const SDL_Event& e)
{
	switch (e.type)
	{
	case SDL_JOYDEVICEADDED:
		// Report that an unsupported joystick device has been detected, to help with controller issues.
		if (!SDL_IsGameController(e.jdevice.which))
			LogF("Gamepad %s isn't supported.", SDL_JoystickNameForIndex(e.jdevice.which));
		break;
	case SDL_CONTROLLERDEVICEADDED:
	{
		auto device = std::make_shared<C4GamePadOpener>(e.cdevice.which);
		Gamepads[device->GetID()] = device;
		LogF("Gamepad #%d connected: %s", device->GetID(), SDL_JoystickNameForIndex(e.cdevice.which));
		break;
	}
	case SDL_CONTROLLERDEVICEREMOVED:
		LogF("Gamepad #%d disconnected.", e.cdevice.which);
		Gamepads.erase(e.cdevice.which);
		break;
	}
}

void C4GamePadControl::DoAxisInput()
{
	for (auto const &e : AxisEvents)
	{
		FeedEvent(e.second, FEED_MOVED);
	}
	AxisEvents.clear();
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

std::shared_ptr<C4GamePadOpener> C4GamePadControl::GetGamePad(int gamepad)
{
	if (gamepad >= 0)
		for (const auto& p : Gamepads)
			if (gamepad-- == 0)
				return p.second;
	return nullptr;
}

std::shared_ptr<C4GamePadOpener> C4GamePadControl::GetGamePadByID(int32_t id)
{
	auto it = Gamepads.find(id);
	if (it != Gamepads.end())
		return it->second;
	return nullptr;
}

std::shared_ptr<C4GamePadOpener> C4GamePadControl::GetAvailableGamePad()
{
	for (const auto& p : Gamepads)
		if (p.second->GetPlayer() < 0)
			return p.second;
	return nullptr;
}

C4GamePadOpener::C4GamePadOpener(int iGamepad)
{
	int n = iGamepad;
	for (int i = 0; i < SDL_NumJoysticks(); i++)
		if (SDL_IsGameController(i) && n-- == 0)
		{
			controller = SDL_GameControllerOpen(i);
			if (!controller) LogF("SDL: %s", SDL_GetError());
			SDL_Joystick *joystick = SDL_GameControllerGetJoystick(controller);
			haptic = SDL_HapticOpenFromJoystick(joystick);
			if (haptic && SDL_HapticRumbleSupported(haptic))
				SDL_HapticRumbleInit(haptic);
			else
				LogF("Gamepad #%d %s does not support rumbling.", SDL_JoystickInstanceID(joystick), SDL_JoystickName(joystick));
			break;
		}

	if (!controller) LogF("Gamepad %d not available", iGamepad);
}

C4GamePadOpener::~C4GamePadOpener()
{
	if (haptic) SDL_HapticClose(haptic);
	if (controller) SDL_GameControllerClose(controller);
}

int32_t C4GamePadOpener::GetID()
{
	return SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller));
}

bool C4GamePadOpener::IsAttached()
{
	return !!SDL_GameControllerGetAttached(controller);
}

void C4GamePadOpener::PlayRumble(float strength, uint32_t length)
{
	if (SDL_HapticRumbleSupported(haptic))
		SDL_HapticRumblePlay(haptic, strength, length);
}

void C4GamePadOpener::StopRumble()
{
	if (SDL_HapticRumbleSupported(haptic))
		SDL_HapticRumbleStop(haptic);
}

#else

// Dedicated server and everything else with neither Win32 nor SDL.

C4GamePadControl::C4GamePadControl() { Log("WARNING: Engine without Gamepad support"); }
C4GamePadControl::~C4GamePadControl() { }
void C4GamePadControl::Execute() { }
void C4GamePadControl::DoAxisInput() { }
int C4GamePadControl::GetGamePadCount() { return 0; }
std::shared_ptr<C4GamePadOpener> C4GamePadControl::GetGamePad(int gamepad) { return nullptr; }
std::shared_ptr<C4GamePadOpener> C4GamePadControl::GetGamePadByID(int32_t id) { return nullptr; }
std::shared_ptr<C4GamePadOpener> C4GamePadControl::GetAvailableGamePad() { return nullptr; }

C4GamePadOpener::C4GamePadOpener(int iGamepad) { }
C4GamePadOpener::~C4GamePadOpener() {}
int32_t C4GamePadOpener::GetID() { return -1; }
bool C4GamePadOpener::IsAttached() { return false; }
void C4GamePadOpener::PlayRumble(float strength, uint32_t length) { }
void C4GamePadOpener::StopRumble() { }

#endif
