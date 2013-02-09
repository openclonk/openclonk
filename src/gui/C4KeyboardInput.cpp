/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005-2006, 2008-2012  Sven Eberhardt
 * Copyright (c) 2005-2006, 2011  Günther Brammer
 * Copyright (c) 2005  Peter Wortmann
 * Copyright (c) 2005-2006, 2008-2010  Asmageddon
 * Copyright (c) 2008  Julian Raschke
 * Copyright (c) 2008  Matthes Bender
 * Copyright (c) 2009-2010, 2012  Armin Burgmeier
 * Copyright (c) 2010  Benjamin Herr
 * Copyright (c) 2010  Martin Plicht
 * Copyright (c) 2005-2009, RedWolf Design GmbH, http://www.clonk.de
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
// Keyboard input mapping to engine functions

#include <C4Include.h>
#include <C4KeyboardInput.h>

#include <C4Components.h>
#include <C4Game.h>
#include <C4Window.h>

#ifdef USE_X11
#include <X11/Xlib.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <X11/XKBlib.h>
#endif

#include <algorithm>

#ifdef USE_SDL_MAINLOOP
#include <SDL.h>
#include <string>
#include <vector>

#include <SDL_keysym.h>

namespace
{
	std::string getKeyName(C4KeyCode k)
	{
		std::string result = SDL_GetKeyName(static_cast<SDLKey>(k));
		// unknown key
		if (result == "unknown key")
			result = FormatString("\\x%x", (DWORD) k).getData();
		// some special cases
		if (result == "world 0") result = "´";
		if (result == "world 1") result = "ß";
		if (result == "world 2") result = "Ü";
		if (result == "world 3") result = "Ä";
		if (result == "world 4") result = "Ö";
		// capitalize first letter
		result[0] = toupper(result[0]);
		// return key name
		return result;
	}
}
#endif

/* ----------------- Key maps ------------------ */

struct C4KeyShiftMapEntry
{
	C4KeyShiftState eShift;
	const char *szName;
};

const C4KeyShiftMapEntry KeyShiftMap [] =
{
	{ KEYS_Alt,     "Alt" },
	{ KEYS_Control, "Ctrl" },
	{ KEYS_Shift,   "Shift" },
	{ KEYS_Undefined, NULL }
};

C4KeyShiftState C4KeyCodeEx::String2KeyShift(const StdStrBuf &sName)
{
	// query map
	const C4KeyShiftMapEntry *pCheck = KeyShiftMap;
	while (pCheck->szName)
			if (SEqualNoCase(sName.getData(), pCheck->szName)) break; else ++pCheck;
	return pCheck->eShift;
}

StdStrBuf C4KeyCodeEx::KeyShift2String(C4KeyShiftState eShift)
{
	// query map
	const C4KeyShiftMapEntry *pCheck = KeyShiftMap;
	while (pCheck->szName)
			if (eShift == pCheck->eShift) break; else ++pCheck;
	return StdStrBuf(pCheck->szName);
}

struct C4KeyCodeMapEntry
{
	C4KeyCode wCode;
	const char *szName;
	const char *szShortName;
};

#if defined(USE_WIN32_WINDOWS) || defined(USE_X11)
const C4KeyCodeMapEntry KeyCodeMap[] = {
	{1,		"Escape",	"Esc"},
    {2,		"1",			NULL},
    {3,		"2",			NULL},
    {4,		"3",			NULL},
    {5,		"4",			NULL},
    {6,		"5",			NULL},
    {7,		"6",			NULL},
    {8,		"7",			NULL},
    {9,		"8",			NULL},
    {10,	"9",			NULL},
    {11,	"0",			NULL},
    {12,	"Minus",		"-"},
    {13,	"Equal",		"="},
	{14,	"BackSpace",	NULL},
	{15,	"Tab",			NULL},
    {16,	"Q",			NULL},
    {17,	"W",			NULL},
    {18,	"E",			NULL},
    {19,	"R",			NULL},
    {20,	"T",			NULL},
    {21,	"Y",			NULL},
    {22,	"U",			NULL},
    {23,	"I",			NULL},
    {24,	"O",			NULL},
    {25,	"P",			NULL},
    {26,	"LeftBracket",	"["},
    {27,	"RightBracket",	"]"},
	{28,	"Return",		"Ret"},
	{29,	"LeftControl",	"LCtrl"},
    {30,	"A",			NULL},
    {31,	"S",			NULL},
    {32,	"D",			NULL},
    {33,	"F",			NULL},
    {34,	"G",			NULL},
    {35,	"H",			NULL},
    {36,	"J",			NULL},
    {37,	"K",			NULL},
    {38,	"L",			NULL},
    {39,	"Semicolon",	";"},
    {40,	"Apostrophe",	"'"},
	{42,	"LeftShift",	"LShift"},
    {43,	"Backslash",	"\\"},
    {44,	"Z",			NULL},
    {45,	"X",			NULL},
    {46,	"C",			NULL},
    {47,	"V",			NULL},
    {48,	"B",			NULL},
    {49,	"N",			NULL},
    {50,	"M",			NULL},
    {51,	"Comma",		","},
    {52,	"Period",		"."},
    {53,	"Slash",		"/"},
	{54,	"RightShift",	"RShift"},
	{55,	"Multiply",		"N*"},
	{56,	"LeftAlt",		"LAlt"},
	{57,	"Space",		"Sp"},
	{58,	"Capslock",		NULL},
	{59,	"F1",			NULL},
	{60,	"F2",			NULL},
	{61,	"F3",			NULL},
	{62,	"F4",			NULL},
	{63,	"F5",			NULL},
	{64,	"F6",			NULL},
	{65,	"F7",			NULL},
	{66,	"F8",			NULL},
	{67,	"F9",			NULL},
	{68,	"F10",			NULL},
	{69,	"NumLock",		"NLock"},
	{71,	"Num7", 		"N7"},
	{72,	"Num8", 		"N8"},
	{73,	"Num9", 		"N9"},
	{74,	"Subtract",		"N-"},
	{75,	"Num4", 		"N4"},
	{76,	"Num5", 		"N5"},
	{77,	"Num6", 		"N6"},
	{78,	"Add",			"N+"},
	{79,	"Num1", 		"N1"},
	{80,	"Num2", 		"N2"},
	{81,	"Num3", 		"N3"},
	{82,	"Num0",			"N0"},
	{83,	"Decimal",		"N,"},
	{86,	"|<>",			NULL},
	{87,	"F11",			NULL},
	{88,	"F12",			NULL},
	{96,	"NumReturn",	"NRet"},
	{97,	"RightControl",	"RCtrl"},
	{98,	"Divide",		"N/"},
	{100,	"RightAlt",		"RAlt"},
	{102,	"Home",			NULL},
	{103,	"Up",			NULL},
	{104,	"PageUp",		NULL},
	{105,	"Left",			NULL},
	{106,	"Right", 		NULL},
	{107,	"End",			NULL},
	{108,	"Down",			NULL},
	{109,	"PageDown",		NULL},
	{110,	"Insert",		"Ins"},
	{111,	"Delete",		"Del"},
	{119,	"Pause",		NULL},
	{125,	"LeftWin",		"LWin"},
	{127,	"Menu",			NULL},
    {0x00,	NULL, 			NULL}
};
#elif defined(USE_COCOA)
#include "CocoaKeycodeMap.h"
#endif

C4KeyCode C4KeyCodeEx::GetKeyByScanCode(const char *scan_code)
{
	// scan code is in hex format
	unsigned int scan_code_int;
	if (sscanf(scan_code, "$%x", &scan_code_int) != 1) return KEY_Undefined;
	return scan_code_int;
}

C4KeyCode C4KeyCodeEx::String2KeyCode(const StdStrBuf &sName)
{
	// direct key code?
	if (sName.getLength() > 2)
	{
		unsigned int dwRVal;
		if (sscanf(sName.getData(), "\\x%x", &dwRVal) == 1) return dwRVal;
		// scan code
		if (*sName.getData() == '$') return GetKeyByScanCode(sName.getData());
		// direct gamepad code
#ifdef _WIN32
		if (!strnicmp(sName.getData(), "Joy", 3))
#else
		if (!strncasecmp(sName.getData(), "Joy", 3))
#endif
		{
			int iGamepad;
			if (sscanf(sName.getData(), "Joy%d",  &iGamepad) == 1)
			{
				// skip Joy[number]
				const char *key_str = sName.getData()+4;
				while (isdigit(*key_str)) ++key_str;
				// check for button (single, uppercase letter) (e.g. Joy1A)
				if (*key_str && !key_str[1])
				{
					char cGamepadButton = toupper(*key_str);
					if (Inside(cGamepadButton, 'A', 'Z'))
					{
						cGamepadButton = cGamepadButton - 'A';
						return KEY_Gamepad(iGamepad-1, KEY_JOY_Button(cGamepadButton));
					}
				}
				else
				{
					// check for standard axis (e.g. Joy1Left)
					if (!stricmp(key_str, "Left")) return KEY_Gamepad(iGamepad-1, KEY_JOY_Left);
					if (!stricmp(key_str, "Up")) return KEY_Gamepad(iGamepad-1, KEY_JOY_Up);
					if (!stricmp(key_str, "Down")) return KEY_Gamepad(iGamepad-1, KEY_JOY_Down);
					if (!stricmp(key_str, "Right")) return KEY_Gamepad(iGamepad-1, KEY_JOY_Right);
					// check for specific axis (e.g. Joy1Axis1Min)
					int iAxis;
					if (sscanf(key_str, "Axis%d", &iAxis) == 1 && iAxis>0)
					{
						--iAxis; // axis is 0-based internally but written 1-based in config
						key_str += 5;
						while (isdigit(*key_str)) ++key_str;
						if (!stricmp(key_str, "Min")) return KEY_Gamepad(iGamepad-1, KEY_JOY_Axis(iAxis, false));
						if (!stricmp(key_str, "Max")) return KEY_Gamepad(iGamepad-1, KEY_JOY_Axis(iAxis, true));
					}
				}
			}
		}
		bool is_mouse_key;
#ifdef _WIN32
		is_mouse_key = !strnicmp(sName.getData(), "Mouse", 5);
#else
		is_mouse_key = !strncasecmp(sName.getData(), "Mouse", 5);
#endif
		if (is_mouse_key)
		{
			// skip Mouse/GameMouse
			const char *key_str = sName.getData()+5;
			int mouse_id;
			if (sscanf(key_str, "%d",  &mouse_id) == 1)
			{
				// skip number
				while (isdigit(*key_str)) ++key_str;
				// check for known mouse events (e.g. Mouse1Move or GameMouse1Wheel)
				if (!stricmp(key_str, "Move")) return KEY_Mouse(mouse_id-1, KEY_MOUSE_Move);
				if (!stricmp(key_str, "Wheel1Up")) return KEY_Mouse(mouse_id-1, KEY_MOUSE_Wheel1Up);
				if (!stricmp(key_str, "Wheel1Down")) return KEY_Mouse(mouse_id-1, KEY_MOUSE_Wheel1Down);
				if (SEqualNoCase(key_str, "Button", 6)) // e.g. Mouse1ButtonLeft or GameMouse1ButtonRightDouble
				{
					// check for known mouse button events
					uint8_t mouseevent_id = 0;
					key_str += 6;
					if (SEqualNoCase(key_str, "Left",4)) { mouseevent_id=KEY_MOUSE_ButtonLeft; key_str += 4; }
					else if (SEqualNoCase(key_str, "Right",5)) { mouseevent_id=KEY_MOUSE_ButtonRight; key_str += 5; }
					else if (SEqualNoCase(key_str, "Middle",6)) { mouseevent_id=KEY_MOUSE_ButtonMiddle; key_str += 6; }
					else if (isdigit(*key_str))
					{
						// indexed mouse button (e.g. Mouse1Button4 or Mouse1Button4Double)
						int button_index;
						if (sscanf(key_str, "%d",  &button_index) == 1)
						{
							mouseevent_id=static_cast<uint8_t>(KEY_MOUSE_Button1+button_index-1);
							while (isdigit(*key_str)) ++key_str;
						}
					}
					if (mouseevent_id)
					{
						// valid event if finished or followed by "Double"
						if (!*key_str) return KEY_Mouse(mouse_id-1, mouseevent_id);
						if (!stricmp(key_str, "Double")) return KEY_Mouse(mouse_id-1, mouseevent_id+(KEY_MOUSE_Button1Double-KEY_MOUSE_Button1));
						// invalid mouse key...
					}
				}
			}
		}

	}
#if defined(USE_WIN32_WINDOWS) || defined(USE_COCOA) || defined(USE_X11)
	// query map
	const C4KeyCodeMapEntry *pCheck = KeyCodeMap;
	while (pCheck->szName) {
		if (SEqualNoCase(sName.getData(), pCheck->szName)) {
			return(pCheck->wCode);
		}
		++pCheck;
	}
	return KEY_Undefined;
#elif defined(USE_SDL_MAINLOOP)
	for (C4KeyCode k = 0; k < SDLK_LAST; ++k)
	{
		if (SEqualNoCase(sName.getData(), getKeyName(k).c_str()))
			return k;
	}
	return KEY_Undefined;
#else
	return KEY_Undefined;
#endif
}

StdStrBuf C4KeyCodeEx::KeyCode2String(C4KeyCode wCode, bool fHumanReadable, bool fShort)
{
	// Gamepad keys
	if (Key_IsGamepad(wCode))
	{
		int iGamepad = Key_GetGamepad(wCode);
		int gamepad_event = Key_GetGamepadEvent(wCode);
		switch (gamepad_event)
		{
		case KEY_JOY_Left:  return FormatString("Joy%dLeft", iGamepad+1);
		case KEY_JOY_Up:    return FormatString("Joy%dUp", iGamepad+1);
		case KEY_JOY_Down:  return FormatString("Joy%dDown", iGamepad+1);
		case KEY_JOY_Right: return FormatString("Joy%dRight", iGamepad+1);
		default:
			if (Key_IsGamepadAxis(wCode))
			{
				if (fHumanReadable)
					// This is still not great, but it is not really possible to assign unknown axes to "left/right" "up/down"...
					return FormatString("[%d] %s", int(1 + Key_GetGamepadAxisIndex(wCode)), Key_IsGamepadAxisHigh(wCode) ? "Max" : "Min");
				else
					return FormatString("Joy%dAxis%d%s", iGamepad+1, static_cast<int>(Key_GetGamepadAxisIndex(wCode)+1), Key_IsGamepadAxisHigh(wCode) ? "Max" : "Min");
			}
			else
			{
				// button
				if (fHumanReadable)
					// If there should be gamepads around with A B C D... on the buttons, we might create a display option to show letters instead...
					return FormatString("< %d >", int(1 + Key_GetGamepadButtonIndex(wCode)));
				else
					return FormatString("Joy%d%c", iGamepad+1, static_cast<char>(Key_GetGamepadButtonIndex(wCode) + 'A'));
			}
		}
	}
	// Mouse keys
	if (Key_IsMouse(wCode))
	{
		int mouse_id = Key_GetMouse(wCode);
		int mouse_event = Key_GetMouseEvent(wCode);
		const char *mouse_str = "Mouse";
		switch (mouse_event)
		{
		case KEY_MOUSE_Move:              return FormatString("%s%dMove", mouse_str, mouse_id);
		case KEY_MOUSE_Wheel1Up:          return FormatString("%s%dWheel1Up", mouse_str, mouse_id);
		case KEY_MOUSE_Wheel1Down:        return FormatString("%s%dWheel1Down", mouse_str, mouse_id);
		case KEY_MOUSE_ButtonLeft:        return FormatString("%s%dLeft", mouse_str, mouse_id);
		case KEY_MOUSE_ButtonRight:       return FormatString("%s%dRight", mouse_str, mouse_id);
		case KEY_MOUSE_ButtonMiddle:      return FormatString("%s%dMiddle", mouse_str, mouse_id);
		case KEY_MOUSE_ButtonLeftDouble:  return FormatString("%s%dLeftDouble", mouse_str, mouse_id);
		case KEY_MOUSE_ButtonRightDouble: return FormatString("%s%dRightDouble", mouse_str, mouse_id);
		case KEY_MOUSE_ButtonMiddleDouble:return FormatString("%s%dMiddleDouble", mouse_str, mouse_id);
		default:
			// extended mouse button
		{
			uint8_t btn = Key_GetMouseEvent(wCode);
			if (btn >= KEY_MOUSE_Button1Double)
				return FormatString("%s%dButton%dDouble", mouse_str, mouse_id, int(btn-KEY_MOUSE_Button1Double));
			else
				return FormatString("%s%dButton%d", mouse_str, mouse_id, int(btn-KEY_MOUSE_Button1));
		}
		}
	}

	// it's a keyboard key
	if (!fHumanReadable) {
		// for config files and such: dump scancode
		return FormatString("$%x", static_cast<unsigned int>(wCode));
	}
#if defined(_WIN32) || defined(USE_COCOA)

//  TODO: Works?
//  StdStrBuf Name; Name.SetLength(1000);
//  int res = GetKeyNameText(wCode, Name.getMData(), Name.getSize());
//  if(!res)
//    // not found: Compose as direct code
//    return FormatString("\\x%x", (DWORD) wCode);
//  // Set size
//  Name.SetLength(res);
//  return Name;

	wchar_t buf[100];
	int len = GetKeyNameText(wCode<<16, buf, 100);
	if (len > 0) {
		// buf is nullterminated name
		return StdStrBuf(buf);
	}
#elif defined(USE_X11)
	Display * const dpy = gdk_x11_display_get_xdisplay(gdk_display_get_default());
	KeySym keysym = (KeySym)XkbKeycodeToKeysym(dpy,wCode+8,0,0);
	char* name = NULL;
	if (keysym != NoSymbol) { // is the keycode without shift modifiers mapped to a symbol?
		#if defined(USE_GTK3)
		name = gtk_accelerator_get_label_with_keycode(dpy, keysym, wCode+8, (GdkModifierType)0);
		#else
		name = gtk_accelerator_get_label(keysym, (GdkModifierType)0);
		#endif
	}
	if (name) { // is there a string representation of the keysym?
		// prevent memleak
		StdStrBuf buf;
		buf.Take(name);
		return buf;
	}
#elif defined(USE_SDL_MAINLOOP)
	return StdStrBuf(getKeyName(wCode).c_str());
#endif
	return FormatString("$%x", static_cast<unsigned int>(wCode));
}

StdStrBuf C4KeyCodeEx::ToString(bool fHumanReadable, bool fShort) const
{
	static StdStrBuf sResult;
	sResult.Clear();
	// Add shift
	for (DWORD dwShiftCheck = KEYS_First; dwShiftCheck <= KEYS_Max; dwShiftCheck <<= 1)
		if (dwShiftCheck & dwShift)
		{
			sResult.Append(KeyShift2String((C4KeyShiftState) dwShiftCheck));
			sResult.AppendChar('+');
		}
	// Add key
	if (sResult.getLength())
	{
		sResult.Append(KeyCode2String(Key, fHumanReadable, fShort));
		return sResult;
	}
	else
	{
		return KeyCode2String(Key, fHumanReadable, fShort);
	}
}



/* ----------------- C4KeyCodeEx ------------------ */

void C4KeyCodeEx::CompileFunc(StdCompiler *pComp, StdStrBuf *pOutBuf)
{
	if (pComp->isCompiler())
	{
		// reading from file
		StdStrBuf sCode;
		bool is_scan_code;
		// read shifts
		DWORD dwSetShift = 0;
		for (;;)
		{
			is_scan_code = pComp->Separator(StdCompiler::SEP_DOLLAR);
			if (!is_scan_code) pComp->NoSeparator();
			pComp->Value(mkParAdapt(sCode, StdCompiler::RCT_Idtf));
			if (is_scan_code) // scan codes start with $. Reassamble the two tokens that were split by StdCompiler
			{
				sCode.Take(FormatString("$%s", sCode.getData()));
				break;
			}
			if (!pComp->Separator(StdCompiler::SEP_PLUS)) break; // no more separator: Parse this as keyboard code
			// try to convert to shift state
			C4KeyShiftState eAddState = String2KeyShift(sCode);
			if (eAddState == KEYS_Undefined)
				pComp->excCorrupt("undefined key shift state: %s", sCode.getData());
			dwSetShift |= eAddState;
		}
		// any code given? Otherwise, keep default
		if (sCode.getLength())
		{
			// last section: convert to key code
			C4KeyCode eCode = String2KeyCode(sCode);
			if (eCode == KEY_Undefined)
			{
				if (pOutBuf)
				{
					// unknown key, but an output buffer for unknown keys was provided. No failure; caller might resolve key.
					eCode = KEY_Default;
				}
				else
				{
					pComp->excCorrupt("undefined key code: %s", sCode.getData());
				}
			}
			dwShift = dwSetShift;
			Key = eCode;
			if (pOutBuf) pOutBuf->Take(std::move(sCode));
		}
	}
	else
	{
		// write shift states
		for (DWORD dwShiftCheck = KEYS_First; dwShiftCheck <= KEYS_Max; dwShiftCheck <<= 1)
			if (dwShiftCheck & dwShift)
			{
				pComp->Value(mkDecompileAdapt(KeyShift2String((C4KeyShiftState) dwShiftCheck)));
				pComp->Separator(StdCompiler::SEP_PLUS);
			}
		// write key
		pComp->Value(mkDecompileAdapt(KeyCode2String(Key, false, false)));
	}
}

void C4KeyEventData::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(iStrength);
	pComp->Separator();
	pComp->Value(game_x);
	pComp->Separator();
	pComp->Value(game_y);
	pComp->Separator();
	pComp->Value(vp_x);
	pComp->Separator();
	pComp->Value(vp_y);
}

bool C4KeyEventData::operator ==(const struct C4KeyEventData &cmp) const
{
	return iStrength == cmp.iStrength
	       && game_x == cmp.game_x && game_y == cmp.game_y
	       && vp_x == cmp.vp_x && vp_y == cmp.vp_y;

}

/* ----------------- C4CustomKey------------------ */

C4CustomKey::C4CustomKey(const C4KeyCodeEx &DefCode, const char *szName, C4KeyScope Scope, C4KeyboardCallbackInterface *pCallback, unsigned int uiPriority)
		: Scope(Scope), Name(), uiPriority(uiPriority), iRef(0)
{
	// generate code
	if (DefCode.Key != KEY_Default) DefaultCodes.push_back(DefCode);
	// ctor for default key
	Name.Copy(szName);
	if (pCallback)
	{
		pCallback->Ref();
		vecCallbacks.push_back(pCallback);
		pCallback->pOriginalKey = this;
	}
}

C4CustomKey::C4CustomKey(const CodeList &rDefCodes, const char *szName, C4KeyScope Scope, C4KeyboardCallbackInterface *pCallback, unsigned int uiPriority)
		: DefaultCodes(rDefCodes), Scope(Scope), Name(), uiPriority(uiPriority), iRef(0)
{
	// ctor for default key
	Name.Copy(szName);
	if (pCallback)
	{
		pCallback->Ref();
		vecCallbacks.push_back(pCallback);
		pCallback->pOriginalKey = this;
	}
}

C4CustomKey::C4CustomKey(const C4KeyCodeEx &Code, const StdStrBuf &rName)
		: Codes(), DefaultCodes(), Scope(KEYSCOPE_None), Name(), uiPriority(PRIO_None), iRef(0)
{
	// ctor for custom key override
	if (Code.Key != KEY_Default) Codes.push_back(Code);
	Name.Copy(rName);
}

C4CustomKey::C4CustomKey(const C4CustomKey &rCpy, bool fCopyCallbacks)
		: Codes(rCpy.Codes), DefaultCodes(rCpy.DefaultCodes), Scope(rCpy.Scope), Name(), uiPriority(rCpy.uiPriority), iRef(0)
{
	Name.Copy(rCpy.GetName());
	if (fCopyCallbacks)
	{
		for (CBVec::const_iterator i = rCpy.vecCallbacks.begin(); i != rCpy.vecCallbacks.end(); ++i)
		{
			(*i)->Ref();
			vecCallbacks.push_back(*i);
		}
	}
}

C4CustomKey::~C4CustomKey()
{
	// free callback handles
	for (CBVec::const_iterator i = vecCallbacks.begin(); i != vecCallbacks.end(); ++i)
		(*i)->Deref();
}

void C4CustomKey::Update(const C4CustomKey *pByKey)
{
	assert(pByKey);
	assert(Name == pByKey->Name);
	// transfer any assigned data, except name which should be equal anyway
	if (pByKey->DefaultCodes.size()) DefaultCodes = pByKey->DefaultCodes;
	if (pByKey->Codes.size()) Codes = pByKey->Codes;
	if (pByKey->Scope != KEYSCOPE_None) Scope = pByKey->Scope;
	if (pByKey->uiPriority != PRIO_None) uiPriority = pByKey->uiPriority;
	for (CBVec::const_iterator i = pByKey->vecCallbacks.begin(); i != pByKey->vecCallbacks.end(); ++i)
	{
		(*i)->Ref();
		vecCallbacks.push_back(*i);
	}
}

bool C4KeyboardCallbackInterfaceHasOriginalKey(C4KeyboardCallbackInterface *pIntfc, const C4CustomKey *pCheckKey)
{
	return pIntfc->IsOriginalKey(pCheckKey);
}

void C4CustomKey::KillCallbacks(const C4CustomKey *pOfKey)
{
	// remove all instances from list
	CBVec::iterator i;
	while ((i = std::find_if(vecCallbacks.begin(), vecCallbacks.end(), std::bind2nd(std::ptr_fun(&C4KeyboardCallbackInterfaceHasOriginalKey), pOfKey))) != vecCallbacks.end())
	{
		C4KeyboardCallbackInterface *pItfc = *i;
		vecCallbacks.erase(i);
		pItfc->Deref();
	}
}

void C4CustomKey::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(mkSTLContainerAdapt(Codes), Name.getData(), DefaultCodes));
}

bool C4CustomKey::Execute(C4KeyEventType eEv, C4KeyCodeEx key)
{
	// execute all callbacks
	for (CBVec::iterator i = vecCallbacks.begin(); i != vecCallbacks.end(); ++i)
		if ((*i)->OnKeyEvent(key, eEv))
			return true;
	// no event processed it
	return false;
}



/* ----------------- C4KeyBinding ------------------ */

C4KeyBinding::C4KeyBinding(const C4KeyCodeEx &DefCode, const char *szName, C4KeyScope Scope, C4KeyboardCallbackInterface *pCallback, unsigned int uiPriority)
		: C4CustomKey(DefCode, szName, Scope, pCallback, uiPriority)
{
	// self holds a ref
	Ref();
	// register into keyboard input class
	C4KeyboardInput_Init().RegisterKey(this);
}

C4KeyBinding::C4KeyBinding(const CodeList &rDefCodes, const char *szName, C4KeyScope Scope, C4KeyboardCallbackInterface *pCallback, unsigned int uiPriority)
		: C4CustomKey(rDefCodes, szName, Scope, pCallback, uiPriority)
{
	// self holds a ref
	Ref();
	// register into keyboard input class
	C4KeyboardInput_Init().RegisterKey(this);
}

C4KeyBinding::~C4KeyBinding()
{
	// deregister from keyboard input class, if that class still exists
	if (C4KeyboardInput::IsValid)
		Game.KeyboardInput.UnregisterKeyBinding(this);
	// shouldn't be refed now
	assert(iRef==1);
	iRef = 0;
}


/* ----------------- C4KeyboardInput ------------------ */

bool C4KeyboardInput::IsValid = false;

void C4KeyboardInput::Clear()
{
	LastKeyExtraData = C4KeyEventData();
	// release all keys - name map is guarantueed to contain them all
	for (KeyNameMap::const_iterator i = KeysByName.begin(); i != KeysByName.end(); ++i)
		i->second->Deref();
	// clear maps
	KeysByCode.clear();
	KeysByName.clear();
}

void C4KeyboardInput::UpdateKeyCodes(C4CustomKey *pKey, const C4CustomKey::CodeList &rOldCodes, const C4CustomKey::CodeList &rNewCodes)
{
	// new key codes must be the new current key codes
	assert(pKey->GetCodes() == rNewCodes);
	// kill from old list
	C4CustomKey::CodeList::const_iterator iCode;
	for (iCode = rOldCodes.begin(); iCode != rOldCodes.end(); ++iCode)
	{
		// no need to kill if code stayed
		if (std::find(rNewCodes.begin(), rNewCodes.end(), *iCode) != rNewCodes.end()) continue;
		std::pair<KeyCodeMap::iterator, KeyCodeMap::iterator> KeyRange = KeysByCode.equal_range((*iCode).Key);
		for (KeyCodeMap::iterator i = KeyRange.first; i != KeyRange.second; ++i)
			if (i->second == pKey)
			{
				KeysByCode.erase(i);
				break;
			}
	}
	// readd new codes
	for (iCode = rNewCodes.begin(); iCode != rNewCodes.end(); ++iCode)
	{
		// no double-add if it was in old list already
		if (std::find(rOldCodes.begin(), rOldCodes.end(), *iCode) != rOldCodes.end()) continue;
		KeysByCode.insert(std::make_pair((*iCode).Key, pKey));
	}
}

void C4KeyboardInput::RegisterKey(C4CustomKey *pRegKey)
{
	assert(pRegKey); if (!pRegKey) return;
	// key will be added: ref it
	pRegKey->Ref();
	// search key of same name first
	C4CustomKey *pDupKey = KeysByName[pRegKey->GetName().getData()];
	if (pDupKey)
	{
		// key of this name exists: Merge them (old codes copied cuz they'll be overwritten)
		C4CustomKey::CodeList OldCodes = pDupKey->GetCodes();
		const C4CustomKey::CodeList &rNewCodes = pRegKey->GetCodes();
		pDupKey->Update(pRegKey);
		// update access map if key changed
		if (!(OldCodes == rNewCodes)) UpdateKeyCodes(pDupKey, OldCodes, rNewCodes);
		// key to be registered no longer used
		pRegKey->Deref();
	}
	else
	{
		// new unique key: Insert into map
		KeysByName[pRegKey->GetName().getData()] = pRegKey;
		for (C4CustomKey::CodeList::const_iterator i = pRegKey->GetCodes().begin(); i != pRegKey->GetCodes().end(); ++i)
		{
			KeysByCode.insert(std::make_pair((*i).Key, pRegKey));
		}
	}
}

void C4KeyboardInput::UnregisterKey(const StdStrBuf &rsName)
{
	// kill from name map
	KeyNameMap::iterator in = KeysByName.find(rsName.getData());
	if (in == KeysByName.end()) return;
	C4CustomKey *pKey = in->second;
	KeysByName.erase(in);
	// kill all key bindings from key map
	for (C4CustomKey::CodeList::const_iterator iCode = pKey->GetCodes().begin(); iCode != pKey->GetCodes().end(); ++iCode)
	{
		std::pair<KeyCodeMap::iterator, KeyCodeMap::iterator> KeyRange = KeysByCode.equal_range((*iCode).Key);
		for (KeyCodeMap::iterator i = KeyRange.first; i != KeyRange.second; ++i)
			if (i->second == pKey)
			{
				KeysByCode.erase(i);
				break;
			}
	}
	// release reference to key
	pKey->Deref();
}

void C4KeyboardInput::UnregisterKeyBinding(C4CustomKey *pUnregKey)
{
	// find key in name map
	KeyNameMap::iterator in = KeysByName.find(pUnregKey->GetName().getData());
	if (in == KeysByName.end()) return;
	C4CustomKey *pKey = in->second;
	// is this key in the map?
	if (pKey != pUnregKey)
	{
		// Other key is in the list: Just remove the callbacks
		pKey->KillCallbacks(pUnregKey);
		return;
	}
	// this key is in the list: Replace by a duplicate...
	C4CustomKey *pNewKey = new C4CustomKey(*pUnregKey, true);
	// ...without the own callbacks
	pNewKey->KillCallbacks(pUnregKey);
	// and replace current key by duplicate
	UnregisterKey(pUnregKey->GetName());
	RegisterKey(pNewKey);
}

bool C4KeyboardInput::DoInput(const C4KeyCodeEx &InKey, C4KeyEventType InEvent, DWORD InScope, int32_t iStrength)
{
	// store last-key-info
	LastKeyExtraData.iStrength = (iStrength >= 0) ? iStrength : ((InEvent != KEYEV_Up) * 100);
	LastKeyExtraData.game_x = LastKeyExtraData.game_y = LastKeyExtraData.vp_x = LastKeyExtraData.vp_y = 0;
	// check all key events generated by this key: First the keycode itself, then any more generic key events like KEY_Any
	const int32_t iKeyRangeMax = 5;
	int32_t iKeyRangeCnt=0, j;
	C4KeyCode FallbackKeys[iKeyRangeMax];
	FallbackKeys[iKeyRangeCnt++] = InKey.Key;
	if (Key_IsGamepadButton(InKey.Key))
	{
		uint8_t byGamepad = Key_GetGamepad(InKey.Key);
		uint8_t byBtnIndex = Key_GetGamepadButtonIndex(InKey.Key);
		// even/odd button events: Add even button indices as odd events, because byBtnIndex is zero-based and the event naming scheme is for one-based button indices
		if (byBtnIndex % 2) FallbackKeys[iKeyRangeCnt++] = KEY_Gamepad(byGamepad, KEY_JOY_AnyEvenButton);
		else FallbackKeys[iKeyRangeCnt++] = KEY_Gamepad(byGamepad, KEY_JOY_AnyOddButton);
		// high/low button events
		if (byBtnIndex < 4) FallbackKeys[iKeyRangeCnt++] = KEY_Gamepad(byGamepad, KEY_JOY_AnyLowButton);
		else FallbackKeys[iKeyRangeCnt++] = KEY_Gamepad(byGamepad, KEY_JOY_AnyHighButton);
		// "any gamepad button"-event
		FallbackKeys[iKeyRangeCnt++] = KEY_Gamepad(byGamepad, KEY_JOY_AnyButton);
	}
	else if (Key_IsGamepadAxis(InKey.Key))
	{
		// xy-axis-events for all even/odd axises
		uint8_t byGamepad = Key_GetGamepad(InKey.Key);
		uint8_t byAxis = Key_GetGamepadAxisIndex(InKey.Key);
		bool fHigh = Key_IsGamepadAxisHigh(InKey.Key);
		C4KeyCode keyAxisDir;
		if (byAxis % 2)
				if (fHigh) keyAxisDir = KEY_JOY_Down; else keyAxisDir = KEY_JOY_Up;
		else if (fHigh) keyAxisDir = KEY_JOY_Right; else keyAxisDir = KEY_JOY_Left;
		FallbackKeys[iKeyRangeCnt++] = KEY_Gamepad(byGamepad, (uint8_t)keyAxisDir);
	}
	if (InKey.Key != KEY_Any) FallbackKeys[iKeyRangeCnt++] = KEY_Any;
	// now get key ranges for fallback chain
	std::pair<KeyCodeMap::iterator, KeyCodeMap::iterator> KeyRanges[iKeyRangeMax];
	assert(iKeyRangeCnt <= iKeyRangeMax);
	for (int32_t i = 0; i<iKeyRangeCnt; ++i)
	{
		KeyRanges[i] = KeysByCode.equal_range(FallbackKeys[i]);
	}
	// check all assigned keys
	// exec from highest to lowest priority
	unsigned int uiLastPrio = C4CustomKey::PRIO_MoreThanMax;
	for (;;)
	{
		KeyCodeMap::const_iterator i;
		// get priority to exec
		unsigned int uiExecPrio = C4CustomKey::PRIO_None, uiCurr;
		for (j = 0; j < iKeyRangeCnt; ++j)
			for (i = KeyRanges[j].first; i != KeyRanges[j].second; ++i)
			{
				uiCurr = i->second->GetPriority();
				if (uiCurr > uiExecPrio && uiCurr < uiLastPrio) uiExecPrio = uiCurr;
			}
		// nothing with correct priority set left?
		if (uiExecPrio == C4CustomKey::PRIO_None) break;
		// exec all of this priority
		for (j = 0; j < iKeyRangeCnt; ++j)
			for (i = KeyRanges[j].first; i != KeyRanges[j].second; ++i)
			{
				C4CustomKey *pKey = i->second;
				assert(pKey);
				// check priority
				if (pKey->GetPriority() == uiExecPrio)
					// check scope
					if (pKey->GetScope() & InScope)
						// check shift modifier (not on release, because a key release might happen with a different modifier than its pressing!)
						if (InEvent == KEYEV_Up || pKey->IsCodeMatched(C4KeyCodeEx(FallbackKeys[j], C4KeyShiftState(InKey.dwShift))))
							// exec it
							if (pKey->Execute(InEvent, InKey))
								return true;
			}
		// nothing found in this priority: exec next
		uiLastPrio = uiExecPrio;
	}
	// no key matched or all returned false in Execute: Not processed
	return false;
}

void C4KeyboardInput::CompileFunc(StdCompiler *pComp)
{
	// compile all keys that are already defined
	// no definition of new keys with current compiler...
	pComp->Name("Keys");
	try
	{
		for (KeyNameMap::const_iterator i = KeysByName.begin(); i != KeysByName.end(); ++i)
		{
			// naming done in C4CustomKey, because default is determined by key only
			C4CustomKey::CodeList OldCodes = i->second->GetCodes();
			pComp->Value(*i->second);
			// resort in secondary map if key changed
			if (pComp->isCompiler())
			{
				const C4CustomKey::CodeList &rNewCodes = i->second->GetCodes();
				if (!(OldCodes == rNewCodes)) UpdateKeyCodes(i->second, OldCodes, rNewCodes);
			}
		}
	}
	catch (StdCompiler::Exception *pEx)
	{
		pComp->NameEnd(true);
		throw pEx;
	}
	pComp->NameEnd();
}

bool C4KeyboardInput::LoadCustomConfig()
{
	// load from INI file (2do: load from registry)
	C4Group GrpExtra;
	if (!GrpExtra.Open(C4CFN_Extra)) return false;
	StdBuf sFileContents;
	if (!GrpExtra.LoadEntry(C4CFN_KeyConfig, &sFileContents)) return false;
	StdStrBuf sFileContentsString((const char *) sFileContents.getData());
	if (!CompileFromBuf_LogWarn<StdCompilerINIRead>(*this, sFileContentsString, "Custom keys from" C4CFN_Extra DirSep C4CFN_KeyConfig))
		return false;
	LogF(LoadResStr("IDS_PRC_LOADEDKEYCONF"), C4CFN_Extra DirSep C4CFN_KeyConfig);
	return true;
}

C4CustomKey *C4KeyboardInput::GetKeyByName(const char *szKeyName)
{
	KeyNameMap::const_iterator i = KeysByName.find(szKeyName);
	if (i == KeysByName.end()) return NULL; else return (*i).second;
}

StdStrBuf C4KeyboardInput::GetKeyCodeNameByKeyName(const char *szKeyName, bool fShort, int32_t iIndex)
{
	C4CustomKey *pKey = GetKeyByName(szKeyName);
	if (pKey)
	{
		const C4CustomKey::CodeList &codes = pKey->GetCodes();
		if ((size_t)iIndex < codes.size())
		{
			C4KeyCodeEx code = codes[iIndex];
			return code.ToString(true, fShort);
		}
	}
	// Error
	return StdStrBuf();
}

C4KeyboardInput &C4KeyboardInput_Init()
{
	static C4KeyboardInput keyinp;
	return keyinp;
}
