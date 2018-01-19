/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2016, The OpenClonk Team and contributors
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
// Keyboard input mapping to engine functions

#include "C4Include.h"
#include "gui/C4KeyboardInput.h"

#include "gui/C4MouseControl.h"
#include "c4group/C4Components.h"
#include "platform/C4Window.h"

#include <unordered_map>

#ifdef HAVE_SDL
#include <SDL.h>
#endif

#ifdef USE_SDL_MAINLOOP
// Required for KeycodeToString translation table.
#include "platform/C4App.h"
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
	{ KEYS_Undefined, nullptr }
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

#if defined(USE_COCOA)
#include "platform/CocoaKeycodeMap.h"
#else
const C4KeyCodeMapEntry KeyCodeMap[] = {
	{K_ESCAPE,        "Escape",       "Esc"},
	{K_1,             "1",            nullptr},
	{K_2,             "2",            nullptr},
	{K_3,             "3",            nullptr},
	{K_4,             "4",            nullptr},
	{K_5,             "5",            nullptr},
	{K_6,             "6",            nullptr},
	{K_7,             "7",            nullptr},
	{K_8,             "8",            nullptr},
	{K_9,             "9",            nullptr},
	{K_0,             "0",            nullptr},
	{K_MINUS,         "Minus",        "-"},
	{K_EQUAL,         "Equal",        "="},
	{K_BACK,          "BackSpace",    nullptr},
	{K_TAB,           "Tab",          nullptr},
	{K_Q,             "Q",            nullptr},
	{K_W,             "W",            nullptr},
	{K_E,             "E",            nullptr},
	{K_R,             "R",            nullptr},
	{K_T,             "T",            nullptr},
	{K_Y,             "Y",            nullptr},
	{K_U,             "U",            nullptr},
	{K_I,             "I",            nullptr},
	{K_O,             "O",            nullptr},
	{K_P,             "P",            nullptr},
	{K_LEFT_BRACKET,  "LeftBracket",  "["},
	{K_RIGHT_BRACKET, "RightBracket", "]"},
	{K_RETURN,        "Return",       "Ret"},
	{K_CONTROL_L,     "LeftControl",  "LCtrl"},
	{K_A,             "A",            nullptr},
	{K_S,             "S",            nullptr},
	{K_D,             "D",            nullptr},
	{K_F,             "F",            nullptr},
	{K_G,             "G",            nullptr},
	{K_H,             "H",            nullptr},
	{K_J,             "J",            nullptr},
	{K_K,             "K",            nullptr},
	{K_L,             "L",            nullptr},
	{K_SEMICOLON,     "Semicolon",    ";"},
	{K_APOSTROPHE,    "Apostrophe",   "'"},
	{K_GRAVE_ACCENT,  "GraveAccent",  "`"},
	{K_SHIFT_L,       "LeftShift",    "LShift"},
	{K_BACKSLASH,     "Backslash",    R"(\)"},
	{K_Z,             "Z",            nullptr},
	{K_X,             "X",            nullptr},
	{K_C,             "C",            nullptr},
	{K_V,             "V",            nullptr},
	{K_B,             "B",            nullptr},
	{K_N,             "N",            nullptr},
	{K_M,             "M",            nullptr},
	{K_COMMA,         "Comma",        ","},
	{K_PERIOD,        "Period",       "."},
	{K_SLASH,         "Slash",        "/"},
	{K_SHIFT_R,       "RightShift",   "RShift"},
	{K_MULTIPLY,      "Multiply",     "N*"},
	{K_ALT_L,         "LeftAlt",      "LAlt"},
	{K_SPACE,         "Space",        "Sp"},
	{K_CAPS,          "Capslock",     nullptr},
	{K_F1,            "F1",           nullptr},
	{K_F2,            "F2",           nullptr},
	{K_F3,            "F3",           nullptr},
	{K_F4,            "F4",           nullptr},
	{K_F5,            "F5",           nullptr},
	{K_F6,            "F6",           nullptr},
	{K_F7,            "F7",           nullptr},
	{K_F8,            "F8",           nullptr},
	{K_F9,            "F9",           nullptr},
	{K_F10,           "F10",          nullptr},
	{K_NUM,           "NumLock",      "NLock"},
	{K_SCROLL,        "ScrollLock",   "SLock"},
	{K_NUM7,          "Num7",         "N7"},
	{K_NUM8,          "Num8",         "N8"},
	{K_NUM9,          "Num9",         "N9"},
	{K_SUBTRACT,      "Subtract",     "N-"},
	{K_NUM4,          "Num4",         "N4"},
	{K_NUM5,          "Num5",         "N5"},
	{K_NUM6,          "Num6",         "N6"},
	{K_ADD,           "Add",          "N+"},
	{K_NUM1,          "Num1",         "N1"},
	{K_NUM2,          "Num2",         "N2"},
	{K_NUM3,          "Num3",         "N3"},
	{K_NUM0,          "Num0",         "N0"},
	{K_DECIMAL,       "Decimal",      "N,"},
	{K_86,            "|<>",          nullptr},
	{K_F11,           "F11",          nullptr},
	{K_F12,           "F12",          nullptr},
	{K_NUM_RETURN,    "NumReturn",    "NRet"},
	{K_CONTROL_R,     "RightControl", "RCtrl"},
	{K_DIVIDE,        "Divide",       "N/"},
	{K_ALT_R,         "RightAlt",     "RAlt"},
	{K_HOME,          "Home",         nullptr},
	{K_UP,            "Up",           nullptr},
	{K_PAGEUP,        "PageUp",       nullptr},
	{K_LEFT,          "Left",         nullptr},
	{K_RIGHT,         "Right",        nullptr},
	{K_END,           "End",          nullptr},
	{K_DOWN,          "Down",         nullptr},
	{K_PAGEDOWN,      "PageDown",     nullptr},
	{K_INSERT,        "Insert",       "Ins"},
	{K_DELETE,        "Delete",       "Del"},
	{K_PAUSE,         "Pause",        nullptr},
	{K_WIN_L,         "LeftWin",      "LWin"},
	{K_WIN_R,         "RightWin",     "RWin"},
	{K_MENU,          "Menu",         nullptr},
	{K_PRINT,         "Print",        nullptr},
	{0x00,            nullptr,           nullptr}
};
#endif

C4KeyCodeEx::C4KeyCodeEx(C4KeyCode key, DWORD Shift, bool fIsRepeated, int32_t deviceId)
: Key(key), dwShift(Shift), fRepeated(fIsRepeated), deviceId(deviceId)
{
}

C4KeyCodeEx C4KeyCodeEx::FromC4MC(int8_t mouse_id, int32_t iButton, DWORD dwKeyParam, bool *is_down)
{
	bool dummy;
	if (!is_down)
		is_down = &dummy;
	*is_down = true;
	C4KeyCode mouseevent_code;
	int wheel_dir = 0;
	if (iButton == C4MC_Button_Wheel) wheel_dir = (short)(dwKeyParam >> 16);
	switch (iButton)
	{
	case C4MC_Button_None: mouseevent_code = KEY_MOUSE_Move; break;
	case C4MC_Button_LeftDown: mouseevent_code = KEY_MOUSE_ButtonLeft; break;
	case C4MC_Button_LeftUp: mouseevent_code = KEY_MOUSE_ButtonLeft; *is_down = false; break;
	case C4MC_Button_LeftDouble: mouseevent_code = KEY_MOUSE_ButtonLeftDouble; break;
	case C4MC_Button_RightDown: mouseevent_code = KEY_MOUSE_ButtonRight; break;
	case C4MC_Button_RightDouble: mouseevent_code = KEY_MOUSE_ButtonRightDouble; break;
	case C4MC_Button_RightUp: mouseevent_code = KEY_MOUSE_ButtonRight; *is_down = false; break;
	case C4MC_Button_MiddleDown: mouseevent_code = KEY_MOUSE_ButtonMiddle; break;
	case C4MC_Button_MiddleUp: mouseevent_code = KEY_MOUSE_ButtonMiddle; *is_down = false; break;
	case C4MC_Button_MiddleDouble: mouseevent_code = KEY_MOUSE_ButtonMiddleDouble; break;
	case C4MC_Button_X1Down: mouseevent_code = KEY_MOUSE_ButtonX1; break;
	case C4MC_Button_X1Up: mouseevent_code = KEY_MOUSE_ButtonX1; *is_down = false; break;
	case C4MC_Button_X1Double: mouseevent_code = KEY_MOUSE_ButtonX1Double; break;
	case C4MC_Button_X2Down: mouseevent_code = KEY_MOUSE_ButtonX2; break;
	case C4MC_Button_X2Up: mouseevent_code = KEY_MOUSE_ButtonX2; *is_down = false; break;
	case C4MC_Button_X2Double: mouseevent_code = KEY_MOUSE_ButtonX2Double; break;
	case C4MC_Button_Wheel:
		if (!wheel_dir) assert("Attempted to record mouse wheel movement without a direction");
		mouseevent_code = (wheel_dir > 0) ? KEY_MOUSE_Wheel1Up : KEY_MOUSE_Wheel1Down; break;
	}
	C4KeyCodeEx key{KEY_Mouse(mouse_id, mouseevent_code), KEYS_None};
	if (dwKeyParam & MK_CONTROL) key.dwShift |= KEYS_Control;
	if (dwKeyParam & MK_SHIFT) key.dwShift |= KEYS_Shift;
	if (dwKeyParam & MK_ALT) key.dwShift |= KEYS_Alt;
	return key;
}

void C4KeyCodeEx::FixShiftKeys()
{
	// reduce stuff like Ctrl+RightCtrl to simply RightCtrl
	if ((dwShift & KEYS_Alt) && (Key == K_ALT_L || Key == K_ALT_R)) dwShift &= ~KEYS_Alt;
	if ((dwShift & KEYS_Control) && (Key == K_CONTROL_L || Key == K_CONTROL_R)) dwShift &= ~KEYS_Control;
	if ((dwShift & KEYS_Shift) && (Key == K_SHIFT_L || Key == K_SHIFT_R)) dwShift &= ~KEYS_Shift;
}

C4KeyCode C4KeyCodeEx::GetKeyByScanCode(const char *scan_code)
{
	// scan code is in hex format
	unsigned int scan_code_int;
	if (sscanf(scan_code, "$%x", &scan_code_int) != 1) return KEY_Undefined;
	return scan_code_int;
}

static const std::unordered_map<std::string, C4KeyCode> controllercodes =
{
		{ "ButtonA",               KEY_CONTROLLER_ButtonA             },
		{ "ButtonB",               KEY_CONTROLLER_ButtonB             },
		{ "ButtonX",               KEY_CONTROLLER_ButtonX             },
		{ "ButtonY",               KEY_CONTROLLER_ButtonY             },
		{ "ButtonBack",            KEY_CONTROLLER_ButtonBack          },
		{ "ButtonGuide",           KEY_CONTROLLER_ButtonGuide         },
		{ "ButtonStart",           KEY_CONTROLLER_ButtonStart         },
		{ "ButtonLeftStick",       KEY_CONTROLLER_ButtonLeftStick     },
		{ "ButtonRightStick",      KEY_CONTROLLER_ButtonRightStick    },
		{ "ButtonLeftShoulder",    KEY_CONTROLLER_ButtonLeftShoulder  },
		{ "ButtonRightShoulder",   KEY_CONTROLLER_ButtonRightShoulder },
		{ "ButtonDpadUp",          KEY_CONTROLLER_ButtonDpadUp        },
		{ "ButtonDpadDown",        KEY_CONTROLLER_ButtonDpadDown      },
		{ "ButtonDpadLeft",        KEY_CONTROLLER_ButtonDpadLeft      },
		{ "ButtonDpadRight",       KEY_CONTROLLER_ButtonDpadRight     },
		{ "AnyButton",             KEY_CONTROLLER_AnyButton           },
		{ "LeftStickLeft",         KEY_CONTROLLER_AxisLeftXLeft       },
		{ "LeftStickRight",        KEY_CONTROLLER_AxisLeftXRight      },
		{ "LeftStickUp",           KEY_CONTROLLER_AxisLeftYUp         },
		{ "LeftStickDown",         KEY_CONTROLLER_AxisLeftYDown       },
		{ "RightStickLeft",        KEY_CONTROLLER_AxisRightXLeft      },
		{ "RightStickRight",       KEY_CONTROLLER_AxisRightXRight     },
		{ "RightStickUp",          KEY_CONTROLLER_AxisRightYUp        },
		{ "RightStickDown",        KEY_CONTROLLER_AxisRightYDown      },
		{ "LeftTrigger",           KEY_CONTROLLER_AxisTriggerLeft     },
		{ "RightTrigger",          KEY_CONTROLLER_AxisTriggerRight    },
};

C4KeyCode C4KeyCodeEx::String2KeyCode(const StdStrBuf &sName)
{
	// direct key code, e.g. "$e" (Backspace)?
	if (sName.getLength() > 1)
	{
		unsigned int dwRVal;
		if (sscanf(sName.getData(), R"(\x%x)", &dwRVal) == 1) return dwRVal;
		// scan code
		if (*sName.getData() == '$') return GetKeyByScanCode(sName.getData());
		// direct gamepad code
		std::regex controller_re(R"/(^Controller(\w+)$)/");
		std::cmatch matches;
		if (std::regex_match(sName.getData(), matches, controller_re))
		{
			auto keycode_it = controllercodes.find(matches[1].str());
			if (keycode_it != controllercodes.end())
				return KEY_Gamepad(keycode_it->second);
			else
				return KEY_Undefined;

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
				// check for known mouse events (e.g. Mouse0Move or GameMouse0Wheel)
				if (!stricmp(key_str, "Move")) return KEY_Mouse(mouse_id, KEY_MOUSE_Move);
				if (!stricmp(key_str, "Wheel1Up")) return KEY_Mouse(mouse_id, KEY_MOUSE_Wheel1Up);
				if (!stricmp(key_str, "Wheel1Down")) return KEY_Mouse(mouse_id, KEY_MOUSE_Wheel1Down);
				// check for known mouse button events
				if (SEqualNoCase(key_str, "Button", 6)) // e.g. Mouse0ButtonLeft or GameMouse0ButtonRightDouble (This line is left here to not break anything, the buttons are now named Mouse0Left)
					key_str += 6;
				uint8_t mouseevent_id = 0;
				if (SEqualNoCase(key_str, "Left",4)) { mouseevent_id=KEY_MOUSE_ButtonLeft; key_str += 4; }
				else if (SEqualNoCase(key_str, "Right",5)) { mouseevent_id=KEY_MOUSE_ButtonRight; key_str += 5; }
				else if (SEqualNoCase(key_str, "Middle",6)) { mouseevent_id=KEY_MOUSE_ButtonMiddle; key_str += 6; }
				else if (SEqualNoCase(key_str, "X1",2)) { mouseevent_id=KEY_MOUSE_ButtonX1; key_str += 2; }
				else if (SEqualNoCase(key_str, "X2",2)) { mouseevent_id=KEY_MOUSE_ButtonX2; key_str += 2; }
				else if (isdigit(*key_str))
				{
					// indexed mouse button (e.g. Mouse0Button4 or Mouse0Button4Double)
					int button_index;
					if (sscanf(key_str, "%d",  &button_index) == 1)
					{
						mouseevent_id=static_cast<uint8_t>(KEY_MOUSE_Button1+button_index);
						while (isdigit(*key_str)) ++key_str;
					}
				}
				if (mouseevent_id)
				{
					// valid event if finished or followed by "Double"
					if (!*key_str) return KEY_Mouse(mouse_id, mouseevent_id);
					if (!stricmp(key_str, "Double")) return KEY_Mouse(mouse_id, mouseevent_id+(KEY_MOUSE_Button1Double-KEY_MOUSE_Button1));
					// invalid mouse key...
				}
			}
		}

	}
	// query map
	const C4KeyCodeMapEntry *pCheck = KeyCodeMap;
	while (pCheck->szName) {
		if (SEqualNoCase(sName.getData(), pCheck->szName)) {
			return(pCheck->wCode);
		}
		++pCheck;
	}
#if defined(USE_SDL_MAINLOOP)
	SDL_Scancode s = SDL_GetScancodeFromName(sName.getData());
	if (s != SDL_SCANCODE_UNKNOWN) return s;
#endif
	return KEY_Undefined;
}

StdStrBuf C4KeyCodeEx::KeyCode2String(C4KeyCode wCode, bool fHumanReadable, bool fShort)
{
	// Gamepad keys
	if (Key_IsGamepad(wCode))
	{
		if (fHumanReadable)
		{
			switch (Key_GetGamepadEvent(wCode))
			{
			case KEY_CONTROLLER_ButtonA             : return StdStrBuf("{{@Ico:A}}");
			case KEY_CONTROLLER_ButtonB             : return StdStrBuf("{{@Ico:B}}");
			case KEY_CONTROLLER_ButtonX             : return StdStrBuf("{{@Ico:X}}");
			case KEY_CONTROLLER_ButtonY             : return StdStrBuf("{{@Ico:Y}}");
			case KEY_CONTROLLER_ButtonBack          : return StdStrBuf("{{@Ico:Back}}");
			case KEY_CONTROLLER_ButtonGuide         : return StdStrBuf("Guide");
			case KEY_CONTROLLER_ButtonStart         : return StdStrBuf("{{@Ico:Start}}");
			case KEY_CONTROLLER_ButtonLeftStick     : return StdStrBuf("{{@Ico:LeftStick}}");
			case KEY_CONTROLLER_ButtonRightStick    : return StdStrBuf("{{@Ico:RightStick}}");
			case KEY_CONTROLLER_ButtonLeftShoulder  : return StdStrBuf("{{@Ico:LeftShoulder}}");
			case KEY_CONTROLLER_ButtonRightShoulder : return StdStrBuf("{{@Ico:RightShoulder}}");
			case KEY_CONTROLLER_ButtonDpadUp        : return StdStrBuf("{{@Ico:DpadUp}}");
			case KEY_CONTROLLER_ButtonDpadDown      : return StdStrBuf("{{@Ico:DpadDown}}");
			case KEY_CONTROLLER_ButtonDpadLeft      : return StdStrBuf("{{@Ico:DpadLeft}}");
			case KEY_CONTROLLER_ButtonDpadRight     : return StdStrBuf("{{@Ico:DpadRight}}");
			case KEY_CONTROLLER_AnyButton           : return StdStrBuf("Any Button");
			case KEY_CONTROLLER_AxisLeftXLeft       : return StdStrBuf("{{@Ico:LeftStick}} Left");
			case KEY_CONTROLLER_AxisLeftXRight      : return StdStrBuf("{{@Ico:LeftStick}} Right");
			case KEY_CONTROLLER_AxisLeftYUp         : return StdStrBuf("{{@Ico:LeftStick}} Up");
			case KEY_CONTROLLER_AxisLeftYDown       : return StdStrBuf("{{@Ico:LeftStick}} Down");
			case KEY_CONTROLLER_AxisRightXLeft      : return StdStrBuf("{{@Ico:RightStick}} Left");
			case KEY_CONTROLLER_AxisRightXRight     : return StdStrBuf("{{@Ico:RightStick}} Right");
			case KEY_CONTROLLER_AxisRightYUp        : return StdStrBuf("{{@Ico:RightStick}} Up");
			case KEY_CONTROLLER_AxisRightYDown      : return StdStrBuf("{{@Ico:RightStick}} Down");
			case KEY_CONTROLLER_AxisTriggerLeft     : return StdStrBuf("{{@Ico:LeftTrigger}}");
			case KEY_CONTROLLER_AxisTriggerRight    : return StdStrBuf("{{@Ico:RightTrigger}}");
			}
		}
		else
		{
			// A linear search in our small map is probably fast enough.
			auto it = std::find_if(controllercodes.begin(), controllercodes.end(), [wCode](const auto &p)
			{
				return p.second == Key_GetGamepadEvent(wCode);
			});
			if (it != controllercodes.end())
				return FormatString("Controller%s", it->first.c_str());
		}
		return StdStrBuf("Unknown");
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
		case KEY_MOUSE_ButtonX1:          return FormatString("%s%dX1", mouse_str, mouse_id);
		case KEY_MOUSE_ButtonX2:          return FormatString("%s%dX2", mouse_str, mouse_id);
		case KEY_MOUSE_ButtonLeftDouble:  return FormatString("%s%dLeftDouble", mouse_str, mouse_id);
		case KEY_MOUSE_ButtonRightDouble: return FormatString("%s%dRightDouble", mouse_str, mouse_id);
		case KEY_MOUSE_ButtonMiddleDouble:return FormatString("%s%dMiddleDouble", mouse_str, mouse_id);
		case KEY_MOUSE_ButtonX1Double:    return FormatString("%s%dX1Double", mouse_str, mouse_id);
		case KEY_MOUSE_ButtonX2Double:    return FormatString("%s%dX2Double", mouse_str, mouse_id);
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
#if defined(USE_WIN32_WINDOWS)

	// Query map
	const C4KeyCodeMapEntry *pCheck = KeyCodeMap;
	while (pCheck->szName)
		if (wCode == pCheck->wCode) return StdStrBuf((pCheck->szShortName && fShort) ? pCheck->szShortName : pCheck->szName); else ++pCheck;

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
#elif defined (USE_COCOA)
	// query map
	const C4KeyCodeMapEntry *pCheck = KeyCodeMap;
	while (pCheck->szName)
			if (wCode == pCheck->wCode) return StdStrBuf((pCheck->szShortName && fShort) ? pCheck->szShortName : pCheck->szName); else ++pCheck;
	// not found: Compose as direct code
	return FormatString("\\x%x", static_cast<unsigned int>(wCode));
#elif defined(USE_SDL_MAINLOOP)
	StdStrBuf buf;
	auto name = KeycodeToString(wCode);
	if (name) buf.Copy(name);
	if (!buf.getLength()) buf.Format("\\x%lx", wCode);
	return buf;
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
	if (pComp->isDeserializer())
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
			if (pOutBuf) {
				// FIXME: This function is used both, to deserialize things like CON_Right and Shift+$12
				// For CON_…, eCode and dwShift will be zero, and sCode will contain the key name.
				// For Shift+… sCode will only contain the last token. What is correct here?
				// Reading C4PlayerControlAssignment::KeyComboItem::CompileFunc suggests that setting not value for parsed combinations may be correct.
				if (eCode == 0)
					pOutBuf->Take(std::move(sCode));
				else
					pOutBuf->Copy(ToString(false, false));
			}
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

bool KEY_IsModifier(C4KeyCode k) {
	return k == K_CONTROL_L || k == K_SHIFT_L || k == K_ALT_L ||
	       k == K_CONTROL_R || k == K_SHIFT_R || k == K_ALT_R;
}


/* ----------------- C4CustomKey------------------ */

C4CustomKey::C4CustomKey(const C4KeyCodeEx &DefCode, const char *szName, C4KeyScope Scope, C4KeyboardCallbackInterface *pCallback, unsigned int uiPriority)
		: Scope(Scope), Name(), uiPriority(uiPriority), iRef(0), is_down(false)
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

C4CustomKey::C4CustomKey(CodeList rDefCodes, const char *szName, C4KeyScope Scope, C4KeyboardCallbackInterface *pCallback, unsigned int uiPriority)
		: DefaultCodes(std::move(rDefCodes)), Scope(Scope), Name(), uiPriority(uiPriority), iRef(0), is_down(false)
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

C4CustomKey::C4CustomKey(const C4CustomKey &rCpy, bool fCopyCallbacks)
		: Codes(rCpy.Codes), DefaultCodes(rCpy.DefaultCodes), Scope(rCpy.Scope), Name(), uiPriority(rCpy.uiPriority), iRef(0), is_down(false)
{
	Name.Copy(rCpy.GetName());
	if (fCopyCallbacks)
	{
		for (auto callback : rCpy.vecCallbacks)
		{
			callback->Ref();
			vecCallbacks.push_back(callback);
		}
	}
}

C4CustomKey::~C4CustomKey()
{
	// free callback handles
	for (CBVec::const_iterator i = vecCallbacks.begin(); i != vecCallbacks.end(); ++i)
		(*i)->Deref();
}

bool C4CustomKey::IsCodeMatched(const C4KeyCodeEx &key) const
{
	const CodeList &codes = GetCodes();
	for (const auto &code : codes)
		if (code == key)
			return true;
	return false;
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
	for (auto callback : pByKey->vecCallbacks)
	{
		callback->Ref();
		vecCallbacks.push_back(callback);
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
	// remember down-state
	is_down = (eEv == KEYEV_Down);
	// execute all callbacks
	for (auto & callback : vecCallbacks)
		if (callback->OnKeyEvent(key, eEv))
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
	LastKeyExtraData.game_x = LastKeyExtraData.game_y = LastKeyExtraData.vp_x = LastKeyExtraData.vp_y = C4KeyEventData::KeyPos_None;
	// check all key events generated by this key: First the keycode itself, then any more generic key events like KEY_Any
	const int32_t iKeyRangeMax = 5;
	int32_t iKeyRangeCnt=0, j;
	C4KeyCode FallbackKeys[iKeyRangeMax];
	FallbackKeys[iKeyRangeCnt++] = InKey.Key;
	if (Key_IsGamepadButton(InKey.Key))
	{
		// "any gamepad button"-event
		FallbackKeys[iKeyRangeCnt++] = KEY_Gamepad(KEY_CONTROLLER_AnyButton);
	}
	else if (Key_IsGamepadAxis(InKey.Key))
	{
		// TODO: do we need "any axis" events?
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
					// check scope and modifier
					// (not on release of a key that has been down, because a key release might happen with a different modifier or in different scope than its pressing!)
					if ((InEvent == KEYEV_Up && pKey->IsDown())
						|| ((pKey->GetScope() & InScope) && pKey->IsCodeMatched(C4KeyCodeEx(FallbackKeys[j], C4KeyShiftState(InKey.dwShift)))))
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
			if (pComp->isDeserializer())
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
	if (i == KeysByName.end()) return nullptr; else return (*i).second;
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
