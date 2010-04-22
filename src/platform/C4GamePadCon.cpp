/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2006  Armin Burgmeier
 * Copyright (c) 2006  Günther Brammer
 * Copyright (c) 2006  Peter Wortmann
 * Copyright (c) 2006  Sven Eberhardt
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

/* Gamepad control */

#include <C4Include.h>
#include <C4GamePadCon.h>

#include <C4ObjectCom.h>
#include <C4Log.h>
#include <C4Game.h>

// regardless of WIN32 or SDL
void C4GamePadControl::DoAxisInput()
{
	// Send axis strength changes
	Execute(true);
}

#ifdef _WIN32
//#include <StdJoystick.h>

C4GamePadControl *C4GamePadControl::pInstance = NULL;

C4GamePadControl::C4GamePadControl()
{
	for (int i=0; i<CStdGamepad_MaxGamePad; ++i)
	{
		Gamepads[i].pGamepad = NULL;
		Gamepads[i].iRefCount = 0;
	}
	iNumGamepads = 0;
	// singleton
	if (!pInstance) pInstance = this;
}

C4GamePadControl::~C4GamePadControl()
{
	if (pInstance == this) pInstance = NULL;
	Clear();
}

void C4GamePadControl::Clear()
{
	for (int i=0; i<CStdGamepad_MaxGamePad; ++i)
		while (Gamepads[i].iRefCount) CloseGamepad(i);
}

void C4GamePadControl::OpenGamepad(int id)
{
	if (!Inside(id, 0, CStdGamepad_MaxGamePad-1)) return;
	// add gamepad ref
	if (!(Gamepads[id].iRefCount++))
	{
		// this is the first gamepad opening: Init it
		Pad &rPad = Gamepads[id];
		rPad.pGamepad = new CStdGamePad(id);
		rPad.Buttons= 0;
		for (int i=0; i< CStdGamepad_MaxAxis; ++i)
		{
			rPad.AxisPosis[i] = CStdGamePad::Mid;
			rPad.AxisStrengths[i] = 0;
		}
		rPad.pGamepad->SetCalibration(&(Config.Gamepads[id].AxisMin[0]), &(Config.Gamepads[id].AxisMax[0]), &(Config.Gamepads[id].AxisCalibrated[0]));
		++iNumGamepads;
	}
}

void C4GamePadControl::CloseGamepad(int id)
{
	if (!Inside(id, 0, CStdGamepad_MaxGamePad-1)) return;
	// del gamepad ref
	if (!--Gamepads[id].iRefCount)
	{
		Gamepads[id].pGamepad->GetCalibration(&(Config.Gamepads[id].AxisMin[0]), &(Config.Gamepads[id].AxisMax[0]), &(Config.Gamepads[id].AxisCalibrated[0]));
		delete Gamepads[id].pGamepad; Gamepads[id].pGamepad = NULL;
		--iNumGamepads;
	}
}

int C4GamePadControl::GetGamePadCount()
{
	JOYINFOEX joy;
	ZeroMem(&joy, sizeof(JOYINFOEX)); joy.dwSize = sizeof(JOYINFOEX); joy.dwFlags = JOY_RETURNALL;
	int iCnt=0;
	while (iCnt<CStdGamepad_MaxGamePad && ::joyGetPosEx(iCnt, &joy) == JOYERR_NOERROR) ++iCnt;
	return iCnt;
}

const int MaxGamePadButton=10;

void C4GamePadControl::Execute(bool send_axis_strength_changes)
{
	// Get gamepad inputs
	int iNum = iNumGamepads;
	for (int idGamepad=0; iNum && idGamepad<CStdGamepad_MaxGamePad; ++idGamepad)
	{
		Pad &rPad = Gamepads[idGamepad];
		if (!rPad.iRefCount) continue;
		--iNum;
		if (!rPad.pGamepad->Update()) continue;
		bool fAnyAxis = false;
		for (int iAxis = 0; iAxis < CStdGamepad_MaxAxis; ++iAxis)
		{
			int32_t iStrength = 100;
			CStdGamePad::AxisPos eAxisPos = rPad.pGamepad->GetAxisPos(iAxis, &iStrength), ePrevAxisPos = rPad.AxisPosis[iAxis];
			int32_t iPrevStrength = rPad.AxisStrengths[iAxis];
			// Evaluate changes and pass single controls
			// this is a generic Gamepad-control: Create events
			if (eAxisPos != ePrevAxisPos || (send_axis_strength_changes && Abs(iPrevStrength-iStrength) > AxisStrengthChangeThreshold))
			{
				rPad.AxisPosis[iAxis] = eAxisPos;
				rPad.AxisStrengths[iAxis] = iStrength;
				if (ePrevAxisPos != CStdGamePad::Mid && eAxisPos != ePrevAxisPos)
					Game.DoKeyboardInput(KEY_Gamepad(idGamepad, KEY_JOY_Axis(iAxis, (ePrevAxisPos==CStdGamePad::High))), KEYEV_Up, false, false, false, false);
				// it's tempting to send fRepeated here for eAxisPos == ePrevAxisPos, but it would cause the key to be ignored for sync controls
				// might improve the check in sync controls so they accept repeated keys if strength is updated?
				if (eAxisPos != CStdGamePad::Mid)
					Game.DoKeyboardInput(KEY_Gamepad(idGamepad, KEY_JOY_Axis(iAxis, (eAxisPos==CStdGamePad::High))), KEYEV_Down, false, false, false, false, NULL, false, iStrength);
			}
		}
		uint32_t Buttons = rPad.pGamepad->GetButtons();
		uint32_t PrevButtons = rPad.Buttons;
		if (Buttons != PrevButtons)
		{
			rPad.Buttons = Buttons;
			for (int iButton = 0; iButton < MaxGamePadButton; ++iButton)
				if ((Buttons & (1 << iButton)) != (PrevButtons & (1 << iButton)))
				{
					bool fRelease = ((Buttons & (1 << iButton)) == 0);
					Game.DoKeyboardInput(KEY_Gamepad(idGamepad, KEY_JOY_Button(iButton)), fRelease ? KEYEV_Up : KEYEV_Down, false, false, false, false);
				}
		}
	}
}

bool C4GamePadControl::AnyButtonDown()
{
	return false;
}

C4GamePadOpener::C4GamePadOpener(int iGamepad)
{
	assert(C4GamePadControl::pInstance);
	this->iGamePad = iGamepad;
	C4GamePadControl::pInstance->OpenGamepad(iGamePad);
}

C4GamePadOpener::~C4GamePadOpener()
{
	if (C4GamePadControl::pInstance)
		C4GamePadControl::pInstance->CloseGamepad(iGamePad);
}

void C4GamePadOpener::SetGamePad(int iNewGamePad)
{
	if (iNewGamePad == iGamePad) return;
	assert(C4GamePadControl::pInstance);
	C4GamePadControl::pInstance->CloseGamepad(iGamePad);
	C4GamePadControl::pInstance->OpenGamepad(iGamePad = iNewGamePad);
}

#elif defined(HAVE_SDL)

#include <SDL.h>

bool C4GamePadControl::AnyButtonDown()
{
	return false;
}

C4GamePadControl::C4GamePadControl()
{
	// Initialize SDL, if necessary.
	if (!SDL_WasInit(SDL_INIT_JOYSTICK)
	    && SDL_Init(SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE))
		LogF("SDL: %s", SDL_GetError());
	SDL_JoystickEventState(SDL_ENABLE);
	if (!SDL_NumJoysticks()) Log("No Gamepad found");
}

C4GamePadControl::~C4GamePadControl()
{
}

void C4GamePadControl::Execute(bool)
{
#ifndef USE_SDL_MAINLOOP
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_JOYAXISMOTION:
		case SDL_JOYBALLMOTION:
		case SDL_JOYHATMOTION:
		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP:
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
	case SDL_JOYHATMOTION:
	{
		SDL_Event fakeX;
		fakeX.jaxis.type = SDL_JOYAXISMOTION;
		fakeX.jaxis.which = event.jhat.which;
		fakeX.jaxis.axis = event.jhat.hat * 2 + 6; /* *magic*number* */
		fakeX.jaxis.value = 0;
		SDL_Event fakeY = fakeX;
		fakeY.jaxis.axis += 1;
		switch (event.jhat.value)
		{
		case SDL_HAT_LEFTUP:    fakeX.jaxis.value = amplify(-1); fakeY.jaxis.value = amplify(-1); break;
		case SDL_HAT_LEFT:      fakeX.jaxis.value = amplify(-1); break;
		case SDL_HAT_LEFTDOWN:  fakeX.jaxis.value = amplify(-1); fakeY.jaxis.value = amplify(+1); break;
		case SDL_HAT_UP:        fakeY.jaxis.value = amplify(-1); break;
		case SDL_HAT_DOWN:      fakeY.jaxis.value = amplify(+1); break;
		case SDL_HAT_RIGHTUP:   fakeX.jaxis.value = amplify(+1); fakeY.jaxis.value = amplify(-1); break;
		case SDL_HAT_RIGHT:     fakeX.jaxis.value = amplify(+1); break;
		case SDL_HAT_RIGHTDOWN: fakeX.jaxis.value = amplify(+1); fakeY.jaxis.value = amplify(+1); break;
		}
		FeedEvent(fakeX);
		FeedEvent(fakeY);
		return;
	}
	case SDL_JOYBALLMOTION:
	{
		SDL_Event fake;
		fake.jaxis.type = SDL_JOYAXISMOTION;
		fake.jaxis.which = event.jball.which;
		fake.jaxis.axis = event.jball.ball * 2 + 12; /* *magic*number* */
		fake.jaxis.value = amplify(event.jball.xrel);
		FeedEvent(event);
		fake.jaxis.axis += 1;
		fake.jaxis.value = amplify(event.jball.yrel);
		FeedEvent(event);
		return;
	}
	case SDL_JOYAXISMOTION:
	{
		C4KeyCode minCode = KEY_Gamepad(event.jaxis.which, KEY_JOY_Axis(event.jaxis.axis, false));
		C4KeyCode maxCode = KEY_Gamepad(event.jaxis.which, KEY_JOY_Axis(event.jaxis.axis, true));

		// FIXME: This assumes that the axis really rests around (0, 0) if it is not used, which is not always true.
		if (event.jaxis.value < -deadZone)
		{
			if (PressedAxis.count(minCode) == 0)
			{
				Game.DoKeyboardInput(
				  KEY_Gamepad(event.jaxis.which, minCode),
				  KEYEV_Down, false, false, false, false);
				PressedAxis.insert(minCode);
			}
		}
		else
		{
			if (PressedAxis.count(minCode) != 0)
			{
				Game.DoKeyboardInput(
				  KEY_Gamepad(event.jaxis.which, minCode),
				  KEYEV_Up, false, false, false, false);
				PressedAxis.erase(minCode);
			}
		}
		if (event.jaxis.value > +deadZone)
		{
			if (PressedAxis.count(maxCode) == 0)
			{
				Game.DoKeyboardInput(
				  KEY_Gamepad(event.jaxis.which, maxCode),
				  KEYEV_Down, false, false, false, false);
				PressedAxis.insert(maxCode);
			}
		}
		else
		{
			if (PressedAxis.count(maxCode) != 0)
			{
				Game.DoKeyboardInput(
				  KEY_Gamepad(event.jaxis.which, maxCode),
				  KEYEV_Up, false, false, false, false);
				PressedAxis.erase(maxCode);
			}
		}
		break;
	}
	case SDL_JOYBUTTONDOWN:
		Game.DoKeyboardInput(
		  KEY_Gamepad(event.jbutton.which, KEY_JOY_Button(event.jbutton.button)),
		  KEYEV_Down, false, false, false, false);
		break;
	case SDL_JOYBUTTONUP:
		Game.DoKeyboardInput(
		  KEY_Gamepad(event.jbutton.which, KEY_JOY_Button(event.jbutton.button)),
		  KEYEV_Up, false, false, false, false);
		break;
	}
}

int C4GamePadControl::GetGamePadCount()
{
	return(SDL_NumJoysticks());
}

C4GamePadOpener::C4GamePadOpener(int iGamepad)
{
	Joy = SDL_JoystickOpen(iGamepad);
	if (!Joy) LogF("SDL: %s", SDL_GetError());
}

C4GamePadOpener::~C4GamePadOpener()
{
	if (Joy) SDL_JoystickClose(Joy);
}

void C4GamePadOpener::SetGamePad(int iGamepad)
{
	if (Joy)
		SDL_JoystickClose(Joy);
	Joy = SDL_JoystickOpen(iGamepad);
	if (!Joy)
		LogF("SDL: %s", SDL_GetError());
}

#else

// Dedicated server and everything else with neither Win32 nor SDL.

C4GamePadControl::C4GamePadControl() { Log("WARNING: Engine without Gamepad support"); }
C4GamePadControl::~C4GamePadControl(bool send_axis_strength_changes) { }
void C4GamePadControl::Execute() { }
int C4GamePadControl::GetGamePadCount() { return 0; }
bool C4GamePadControl::AnyButtonDown() { return false; }

C4GamePadOpener::C4GamePadOpener(int iGamepad) { }
C4GamePadOpener::~C4GamePadOpener() {}
void C4GamePadOpener::SetGamePad(int iGamepad) { }

#endif //_WIN32
