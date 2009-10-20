/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005-2006, 2008  Sven Eberhardt
 * Copyright (c) 2005  Peter Wortmann
 * Copyright (c) 2005-2006  GÃ¼nther Brammer
 * Copyright (c) 2008  Julian Raschke
 * Copyright (c) 2008  Matthes Bender
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

#include <C4Game.h>

#ifndef _WIN32
#include <X11/Xlib.h>
#endif

#ifdef USE_SDL_MAINLOOP
#include <SDL/SDL.h>
#include <string>
#include <vector>

#include <SDL/SDL_keysym.h>

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

const C4KeyShiftMapEntry KeyShiftMap [] = {
	{ KEYS_Alt,     "Alt" },
	{ KEYS_Control, "Ctrl" },
	{ KEYS_Shift,   "Shift" },
	{ KEYS_Undefined, NULL } };

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

#ifdef _WIN32
const C4KeyCodeMapEntry KeyCodeMap [] = {
	{ VK_CANCEL         , "Cancel"        , NULL },

	{ VK_BACK           , "Back"          , NULL },
	{ VK_TAB            , "Tab"           , NULL },
	{ VK_CLEAR          , "Clear"         , NULL },
	{ VK_RETURN         , "Return"        , NULL },

	{ VK_SHIFT          , "KeyShift"      , "Shift" },
	{ VK_CONTROL        , "KeyControl"    , "Control" },
	{ VK_MENU           , "Menu"          , NULL },
	{ VK_PAUSE          , "Pause"         , NULL },

	{ VK_CAPITAL        , "Capital"       , NULL },
	{ VK_KANA           , "Kana"          , NULL },
	{ VK_HANGEUL        , "Hangeul"       , NULL },
	{ VK_HANGUL         , "Hangul"        , NULL },
	{ VK_JUNJA          , "Junja"         , NULL },
	{ VK_FINAL          , "Final"         , NULL },
	{ VK_HANJA          , "Hanja"         , NULL },
	{ VK_KANJI          , "Kanji"         , NULL },
	{ VK_ESCAPE         , "Escape"        , "Esc" },
	{ VK_ESCAPE         , "Esc"           ,NULL },
	{ VK_CONVERT        , "Convert"       , NULL },
	{ VK_NONCONVERT     , "Noconvert"     , NULL },
	{ VK_ACCEPT         , "Accept"        , NULL },
	{ VK_MODECHANGE     , "Modechange"    , NULL },

	{ VK_SPACE          , "Space"         , "Sp" },

	{ VK_PRIOR          , "Prior"         , NULL },
	{ VK_NEXT           , "Next"          , NULL },
	{ VK_END            , "End"           , NULL },
	{ VK_HOME           , "Home"          , NULL },
	{ VK_LEFT           , "Left"          , NULL },
	{ VK_UP             , "Up"            , NULL },
	{ VK_RIGHT          , "Right"         , NULL },
	{ VK_DOWN           , "Down"          , NULL },
	{ VK_SELECT         , "Select"        , NULL },
	{ VK_PRINT          , "Print"         , NULL },
	{ VK_EXECUTE        , "Execute"       , NULL },
	{ VK_SNAPSHOT       , "Snapshot"      , NULL },
	{ VK_INSERT         , "Insert"        , "Ins" },
	{ VK_DELETE         , "Delete"        , "Del" },
	{ VK_HELP           , "Help"          , NULL },

	{ '0'               , "0"         , NULL },
	{ '1'               , "1"         , NULL },
	{ '2'               , "2"         , NULL },
	{ '3'               , "3"         , NULL },
	{ '4'               , "4"         , NULL },
	{ '5'               , "5"         , NULL },
	{ '6'               , "6"         , NULL },
	{ '7'               , "7"         , NULL },
	{ '8'               , "8"         , NULL },
	{ '9'               , "9"         , NULL },

	{ 'A'               , "A"         , NULL },
	{ 'B'               , "B"         , NULL },
	{ 'C'               , "C"         , NULL },
	{ 'D'               , "D"         , NULL },
	{ 'E'               , "E"         , NULL },
	{ 'F'               , "F"         , NULL },
	{ 'G'               , "G"         , NULL },
	{ 'H'               , "H"         , NULL },
	{ 'I'               , "I"         , NULL },
	{ 'J'               , "J"         , NULL },
	{ 'K'               , "K"         , NULL },
	{ 'L'               , "L"         , NULL },
	{ 'M'               , "M"         , NULL },
	{ 'N'               , "N"         , NULL },
	{ 'O'               , "O"         , NULL },
	{ 'P'               , "P"         , NULL },
	{ 'Q'               , "Q"         , NULL },
	{ 'R'               , "R"         , NULL },
	{ 'S'               , "S"         , NULL },
	{ 'T'               , "T"         , NULL },
	{ 'U'               , "U"         , NULL },
	{ 'V'               , "V"         , NULL },
	{ 'W'               , "W"         , NULL },
	{ 'X'               , "X"         , NULL },
	{ 'Y'               , "Y"         , NULL },
	{ 'Z'               , "Z"         , NULL },

	{ VK_LWIN           , "WinLeft"      , NULL },
	{ VK_RWIN           , "WinRight"     , NULL },
	{ VK_APPS           , "Apps"         , NULL },

	{ VK_NUMPAD0        , "Num0"         , "N0" },
	{ VK_NUMPAD1        , "Num1"         , "N1" },
	{ VK_NUMPAD2        , "Num2"         , "N2" },
	{ VK_NUMPAD3        , "Num3"         , "N3" },
	{ VK_NUMPAD4        , "Num4"         , "N4" },
	{ VK_NUMPAD5        , "Num5"         , "N5" },
	{ VK_NUMPAD6        , "Num6"         , "N6" },
	{ VK_NUMPAD7        , "Num7"         , "N7" },
	{ VK_NUMPAD8        , "Num8"         , "N8" },
	{ VK_NUMPAD9        , "Num9"         , "N9" },
	{ VK_MULTIPLY       , "Multiply"     , "N*" },
	{ VK_ADD            , "Add"          , "N+" },
	{ VK_SEPARATOR      , "Seperator"    , "NSep" },
	{ VK_SUBTRACT       , "Subtract"     , "N-" },
	{ VK_DECIMAL        , "Decimal"      , "N," },
	{ VK_DIVIDE         , "Divide"       , "N/" },
	{ VK_F1             , "F1"           , NULL },
	{ VK_F2             , "F2"           , NULL },
	{ VK_F3             , "F3"           , NULL },
	{ VK_F4             , "F4"           , NULL },
	{ VK_F5             , "F5"           , NULL },
	{ VK_F6             , "F6"           , NULL },
	{ VK_F7             , "F7"           , NULL },
	{ VK_F8             , "F8"           , NULL },
	{ VK_F9             , "F9"           , NULL },
	{ VK_F10            , "F10"          , NULL },
	{ VK_F11            , "F11"          , NULL },
	{ VK_F12            , "F12"          , NULL },
	{ VK_F13            , "F13"          , NULL },
	{ VK_F14            , "F14"          , NULL },
	{ VK_F15            , "F15"          , NULL },
	{ VK_F16            , "F16"          , NULL },
	{ VK_F17            , "F17"          , NULL },
	{ VK_F18            , "F18"          , NULL },
	{ VK_F19            , "F19"          , NULL },
	{ VK_F20            , "F20"          , NULL },
	{ VK_F21            , "F21"          , NULL },
	{ VK_F22            , "F22"          , NULL },
	{ VK_F23            , "F23"          , NULL },
	{ VK_F24            , "F24"          , NULL },
	{ VK_NUMLOCK        , "NumLock"      , "NLock" },
	{ K_SCROLL         , "Scroll"        , NULL },

	{ VK_PROCESSKEY     , "PROCESSKEY"   , NULL },

#if defined VK_SLEEP && defined VK_OEM_NEC_EQUAL
	{ VK_SLEEP          , "Sleep"        , NULL },

	{ VK_OEM_NEC_EQUAL  , "OEM_NEC_EQUAL"     , NULL },

	{ VK_OEM_FJ_JISHO   , "OEM_FJ_JISHO"      , NULL },
	{ VK_OEM_FJ_MASSHOU , "OEM_FJ_MASSHOU"    , NULL },
	{ VK_OEM_FJ_TOUROKU , "OEM_FJ_TOUROKU"    , NULL },
	{ VK_OEM_FJ_LOYA    , "OEM_FJ_LOYA"       , NULL },
	{ VK_OEM_FJ_ROYA    , "OEM_FJ_ROYA"       , NULL },

	{ VK_BROWSER_BACK        , "BROWSER_BACK"         , NULL },
	{ VK_BROWSER_FORWARD     , "BROWSER_FORWARD"      , NULL },
	{ VK_BROWSER_REFRESH     , "BROWSER_REFRESH"      , NULL },
	{ VK_BROWSER_STOP        , "BROWSER_STOP"         , NULL },
	{ VK_BROWSER_SEARCH      , "BROWSER_SEARCH"       , NULL },
	{ VK_BROWSER_FAVORITES   , "BROWSER_FAVORITES"    , NULL },
	{ VK_BROWSER_HOME        , "BROWSER_HOME"         , NULL },

	{ VK_VOLUME_MUTE         , "VOLUME_MUTE"          , NULL },
	{ VK_VOLUME_DOWN         , "VOLUME_DOWN"          , NULL },
	{ VK_VOLUME_UP           , "VOLUME_UP"            , NULL },
	{ VK_MEDIA_NEXT_TRACK    , "MEDIA_NEXT_TRACK"     , NULL },
	{ VK_MEDIA_PREV_TRACK    , "MEDIA_PREV_TRACK"     , NULL },
	{ VK_MEDIA_STOP          , "MEDIA_STOP"           , NULL },
	{ VK_MEDIA_PLAY_PAUSE    , "MEDIA_PLAY_PAUSE"     , NULL },
	{ VK_LAUNCH_MAIL         , "LAUNCH_MAIL"          , NULL },
	{ VK_LAUNCH_MEDIA_SELECT , "LAUNCH_MEDIA_SELECT"  , NULL },
	{ VK_LAUNCH_APP1         , "LAUNCH_APP1"          , NULL },
	{ VK_LAUNCH_APP2         , "LAUNCH_APP2"          , NULL },

	{ VK_OEM_1          , "OEM Ü"    , "Ü" }, // German hax
	{ VK_OEM_PLUS       , "OEM +"		, "+" },
	{ VK_OEM_COMMA      , "OEM ,"		, "," },
	{ VK_OEM_MINUS      , "OEM -"		, "-" },
	{ VK_OEM_PERIOD     , "OEM ."		, "." },
	{ VK_OEM_2          , "OEM 2"    , "2" },
	{ VK_OEM_3          , "OEM Ö"    , "Ö" }, // German hax
	{ VK_OEM_4          , "OEM 4"    , "4" },
	{ VK_OEM_5          , "OEM 5"    , "5" },
	{ VK_OEM_6          , "OEM 6"    , "6" },
	{ VK_OEM_7          , "OEM Ä"    , "Ä" }, // German hax
	{ VK_OEM_8          , "OEM 8"		, "8" },
	{ VK_OEM_AX         , "AX"			, "AX" },
	{ VK_OEM_102        , "< > |"    , "<" }, // German hax
	{ VK_ICO_HELP       , "Help"		, "Help" },
	{ VK_ICO_00         , "ICO_00"   , "00" },

	{ VK_ICO_CLEAR      , "ICO_CLEAR"     , NULL },

	{ VK_PACKET         , "PACKET"        , NULL },

	{ VK_OEM_RESET      , "OEM_RESET"     , NULL },
	{ VK_OEM_JUMP       , "OEM_JUMP"      , NULL },
	{ VK_OEM_PA1        , "OEM_PA1"       , NULL },
	{ VK_OEM_PA2        , "OEM_PA2"       , NULL },
	{ VK_OEM_PA3        , "OEM_PA3"       , NULL },
	{ VK_OEM_WSCTRL     , "OEM_WSCTRL"    , NULL },
	{ VK_OEM_CUSEL      , "OEM_CUSEL"     , NULL },
	{ VK_OEM_ATTN       , "OEM_ATTN"      , NULL },
	{ VK_OEM_FINISH     , "OEM_FINISH"    , NULL },
	{ VK_OEM_COPY       , "OEM_COPY"      , NULL },
	{ VK_OEM_AUTO       , "OEM_AUTO"      , NULL },
	{ VK_OEM_ENLW       , "OEM_ENLW"      , NULL },
	{ VK_OEM_BACKTAB    , "OEM_BACKTAB"   , NULL },
#endif

	{ VK_ATTN           , "ATTN"          , NULL },
	{ VK_CRSEL          , "CRSEL"         , NULL },
	{ VK_EXSEL          , "EXSEL"         , NULL },
	{ VK_EREOF          , "EREOF"         , NULL },
	{ VK_PLAY           , "PLAY"          , NULL },
	{ VK_ZOOM           , "ZOOM"          , NULL },
	{ VK_NONAME         , "NONAME"        , NULL },
	{ VK_PA1            , "PA1"           , NULL },
	{ VK_OEM_CLEAR      , "OEM_CLEAR"     , NULL },

	{ KEY_Any, "Any"     , NULL},
	{ KEY_Default, "None", NULL},
	{ KEY_Undefined, NULL, NULL } };
#endif

C4KeyCode C4KeyCodeEx::String2KeyCode(const StdStrBuf &sName)
	{
	// direct key code?
	if (sName.getLength() > 2)
		{
		DWORD dwRVal;
		if (sscanf(sName.getData(), "\\x%x", &dwRVal) == 1) return dwRVal;
		// direct gamepad code
#ifdef _WIN32
		if (!strnicmp(sName.getData(), "Joy", 3))
#else
		if (!strncasecmp(sName.getData(), "Joy", 3))
#endif
			{
			int iGamepad; char cGamepadButton; int iAxis;
			if (sscanf(sName.getData(), "Joy%dLeft",  &iGamepad) == 1) return KEY_Gamepad(iGamepad-1, KEY_JOY_Left);
			if (sscanf(sName.getData(), "Joy%dUp",    &iGamepad) == 1) return KEY_Gamepad(iGamepad-1, KEY_JOY_Up);
			if (sscanf(sName.getData(), "Joy%dDown",  &iGamepad) == 1) return KEY_Gamepad(iGamepad-1, KEY_JOY_Down);
			if (sscanf(sName.getData(), "Joy%dRight", &iGamepad) == 1) return KEY_Gamepad(iGamepad-1, KEY_JOY_Right);
			if (sscanf(sName.getData(), "Joy%d%c", &iGamepad, &cGamepadButton) == 2) return KEY_Gamepad(iGamepad-1, cGamepadButton-'A');
			if (sscanf(sName.getData(), "Joy%dAxis%dMin", &iGamepad, &iAxis) == 1) return KEY_Gamepad(iGamepad-1, KEY_JOY_Axis(iAxis, false));
			if (sscanf(sName.getData(), "Joy%dAxis%dMax", &iGamepad, &iAxis) == 1) return KEY_Gamepad(iGamepad-1, KEY_JOY_Axis(iAxis, true));
			}
		}
#ifdef _WIN32
	// query map
	const C4KeyCodeMapEntry *pCheck = KeyCodeMap;
	while (pCheck->szName)
		if (SEqualNoCase(sName.getData(), pCheck->szName)) break; else ++pCheck;
	return pCheck->wCode;
#elif defined(USE_X11)
	return XStringToKeysym(sName.getData());
#elif defined(USE_SDL_MAINLOOP)
  for (C4KeyCode k = 0; k < SDLK_LAST; ++k)
		if (getKeyName(k) == sName.getData())
			return k;
	return KEY_Default;
#else
	return KEY_Default;
#endif
	}

StdStrBuf C4KeyCodeEx::KeyCode2String(C4KeyCode wCode, bool fHumanReadable, bool fShort)
	{
	// Gamepad keys
	if (Key_IsGamepad(wCode))
		{
		int iGamepad = Key_GetGamepad(wCode);
		int iGamepadButton = Key_GetGamepadButton(wCode);
		switch (iGamepadButton)
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
						return FormatString("Joy%dAxis%d%s", iGamepad+1, static_cast<char>(Key_GetGamepadAxisIndex(wCode)), Key_IsGamepadAxisHigh(wCode) ? "Max" : "Min");
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
#ifdef _WIN32

//	TODO: Works?
//	StdStrBuf Name; Name.SetLength(1000);
//	int res = GetKeyNameText(wCode, Name.getMData(), Name.getSize());
//	if(!res)
//		// not found: Compose as direct code
//		return FormatString("\\x%x", (DWORD) wCode);
//	// Set size
//	Name.SetLength(res);
//	return Name;

	// query map
	const C4KeyCodeMapEntry *pCheck = KeyCodeMap;
	while (pCheck->szName)
		if (wCode == pCheck->wCode) return StdStrBuf((pCheck->szShortName && fShort) ? pCheck->szShortName : pCheck->szName); else ++pCheck;
	// not found: Compose as direct code
	return FormatString("\\x%x", (DWORD) wCode);
#elif defined(USE_X11)
	return StdStrBuf(XKeysymToString(wCode));
#elif defined(USE_SDL_MAINLOOP)
	return StdStrBuf(getKeyName(wCode).c_str());
#else
	return StdStrBuf("unknown");
#endif
	}

StdStrBuf C4KeyCodeEx::ToString(bool fHumanReadable, bool fShort)
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

void C4KeyCodeEx::CompileFunc(StdCompiler *pComp)
	{
	if (pComp->isCompiler())
		{
		// reading from file
		StdStrBuf sCode;
		DWORD dwSetShift = 0;
		for(;;)
			{
			pComp->Value(mkParAdapt(sCode, StdCompiler::RCT_Idtf));
			if (!pComp->Seperator(StdCompiler::SEP_PLUS)) break; // no more seperator: Parse this as keyboard code
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
				pComp->excCorrupt("undefined key code: %s", sCode.getData());
			dwShift = dwSetShift;
			Key = eCode;
			}
		}
	else
		{
		// write shift states
		for (DWORD dwShiftCheck = KEYS_First; dwShiftCheck <= KEYS_Max; dwShiftCheck <<= 1)
			if (dwShiftCheck & dwShift)
				{
				pComp->Value(mkDecompileAdapt(KeyShift2String((C4KeyShiftState) dwShiftCheck)));
				pComp->Seperator(StdCompiler::SEP_PLUS);
				}
		// write key
		pComp->Value(mkDecompileAdapt(KeyCode2String(Key, false, false)));
		}
	}

/* ----------------- C4CustomKey------------------ */

C4CustomKey::C4CustomKey(C4KeyCodeEx DefCode, const char *szName, C4KeyScope Scope, C4KeyboardCallbackInterface *pCallback, unsigned int uiPriority)
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

C4CustomKey::C4CustomKey(C4KeyCodeEx Code, const StdStrBuf &rName)
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
		for(CBVec::const_iterator i = rCpy.vecCallbacks.begin(); i != rCpy.vecCallbacks.end(); ++i)
			{
			(*i)->Ref();
			vecCallbacks.push_back(*i);
			}
		}
	}

C4CustomKey::~C4CustomKey()
	{
	// free callback handles
	for(CBVec::const_iterator i = vecCallbacks.begin(); i != vecCallbacks.end(); ++i)
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
	for(CBVec::const_iterator i = pByKey->vecCallbacks.begin(); i != pByKey->vecCallbacks.end(); ++i)
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
	for(CBVec::iterator i = vecCallbacks.begin(); i != vecCallbacks.end(); ++i)
		if ((*i)->OnKeyEvent(key, eEv))
			return true;
	// no event processed it
	return false;
	}



/* ----------------- C4KeyBinding ------------------ */

C4KeyBinding::C4KeyBinding(C4KeyCodeEx DefCode, const char *szName, C4KeyScope Scope, C4KeyboardCallbackInterface *pCallback, unsigned int uiPriority)
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
		std::pair<KeyCodeMap::iterator, KeyCodeMap::iterator> KeyRange = KeysByCode.equal_range(*iCode);
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
		KeysByCode.insert(std::make_pair(*iCode, pKey));
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
		// new unique key: Insert into both maps
		KeysByName[pRegKey->GetName().getData()] = pRegKey;
		for (C4CustomKey::CodeList::const_iterator i = pRegKey->GetCodes().begin(); i != pRegKey->GetCodes().end(); ++i)
			KeysByCode.insert(std::make_pair(*i, pRegKey));
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
		std::pair<KeyCodeMap::iterator, KeyCodeMap::iterator> KeyRange = KeysByCode.equal_range(*iCode);
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

bool C4KeyboardInput::DoInput(const C4KeyCodeEx &InKey, C4KeyEventType InEvent, DWORD InScope)
	{
	// check all key events generated by this key: First the keycode itself, then any more generic key events like KEY_Any
	const int32_t iKeyRangeMax = 5;
	int32_t iKeyRangeCnt=0, j;
	std::pair<KeyCodeMap::iterator, KeyCodeMap::iterator> KeyRanges[iKeyRangeMax];
	KeyRanges[iKeyRangeCnt++] = KeysByCode.equal_range(InKey);
	if (Key_IsGamepadButton(InKey.Key))
		{
		uint8_t byGamepad = Key_GetGamepad(InKey.Key);
		uint8_t byBtnIndex = Key_GetGamepadButtonIndex(InKey.Key);
		// even/odd button events: Add even button indices as odd events, because byBtnIndex is zero-based and the event naming scheme is for one-based button indices
		if (byBtnIndex % 2) KeyRanges[iKeyRangeCnt++] = KeysByCode.equal_range(C4KeyCodeEx(KEY_Gamepad(byGamepad, KEY_JOY_AnyEvenButton)));
		               else KeyRanges[iKeyRangeCnt++] = KeysByCode.equal_range(C4KeyCodeEx(KEY_Gamepad(byGamepad, KEY_JOY_AnyOddButton)));
		// high/low button events
		if (byBtnIndex < 4) KeyRanges[iKeyRangeCnt++] = KeysByCode.equal_range(C4KeyCodeEx(KEY_Gamepad(byGamepad, KEY_JOY_AnyLowButton)));
		               else KeyRanges[iKeyRangeCnt++] = KeysByCode.equal_range(C4KeyCodeEx(KEY_Gamepad(byGamepad, KEY_JOY_AnyHighButton)));
		// "any gamepad button"-event
		KeyRanges[iKeyRangeCnt++] = KeysByCode.equal_range(C4KeyCodeEx(KEY_Gamepad(byGamepad, KEY_JOY_AnyButton)));
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
		else
			if (fHigh) keyAxisDir = KEY_JOY_Right; else keyAxisDir = KEY_JOY_Left;
		KeyRanges[iKeyRangeCnt++] = KeysByCode.equal_range(C4KeyCodeEx(KEY_Gamepad(byGamepad, (uint8_t)keyAxisDir)));
		}
	if (InKey.Key != KEY_Any) KeyRanges[iKeyRangeCnt++] = KeysByCode.equal_range(C4KeyCodeEx(KEY_Any, C4KeyShiftState(InKey.dwShift)));
	assert(iKeyRangeCnt <= iKeyRangeMax);
	// check all assigned keys
	// exec from highest to lowest priority
	unsigned int uiLastPrio = C4CustomKey::PRIO_MoreThanMax;
	for(;;)
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
	catch(StdCompiler::Exception *pEx)
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
	if (!GrpExtra.LoadEntry(C4CFN_KeyConfig, sFileContents)) return false;
	StdStrBuf sFileContentsString((const char *) sFileContents.getData());
  if(!CompileFromBuf_LogWarn<StdCompilerINIRead>(*this, sFileContentsString, "Custom keys from" C4CFN_Extra DirSep C4CFN_KeyConfig))
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
