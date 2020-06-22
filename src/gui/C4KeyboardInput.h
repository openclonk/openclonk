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

#ifndef INC_C4KeyboardInput
#define INC_C4KeyboardInput

// key context classifications
enum C4KeyScope
{
	KEYSCOPE_None       = 0,
	KEYSCOPE_Control    = 1,  // player control (e.g. NUM1 to move left on keypad control)
	KEYSCOPE_Gui        = 2,  // keys used to manipulate GUI elements (e.g. Tab to cycle through controls)
	KEYSCOPE_Fullscreen = 4,  // generic fullscreen-only keys (e.g. F9 for screenshot)
	KEYSCOPE_Console    = 8,  // generic console-mode only keys (e.g. Space to switch edit cursor)
	KEYSCOPE_Generic    = 16, // generic keys available in fullscreen and console mode outside GUI (e.g. F1 for music on/off)
	KEYSCOPE_FullSMenu  = 32, // fullscreen menu control. If fullscreen menu is active, this disables viewport controls (e.g. Return to close player join menu)
	KEYSCOPE_FilmView   = 64, // ownerless viewport scrolling in film mode, player switching, etc. (e.g. Enter to switch to next player)
	KEYSCOPE_FreeView   = 128, // ownerless viewport scrolling, player switching, etc. (e.g. arrow left to scroll left in view)
	KEYSCOPE_FullSView  = 256 // player fullscreen viewport
};

// what can happen to keys
enum C4KeyEventType
{
	KEYEV_None    =  0, // no event
	KEYEV_Down    =  1, // in response to WM_KEYDOWN or joypad button pressed
	KEYEV_Up      =  2, // in response to WM_KEYUP or joypad button released
	KEYEV_Pressed =  3, // in response to WM_KEYPRESSED
	KEYEV_Moved   =  4, // when moving a gamepad stick
};

// keyboard code
typedef unsigned long C4KeyCode;

// Gamepad codes (KEY_CONTROLLER_*): Masked as 0x420000; bit 8-15 used for gamepad index
const C4KeyCode KEY_CONTROLLER_Mask = 0x420000;

// Mouse codes (KEY_MOUSE_*): Masked as 0x430000; bit 8-15 used for mouse index
const C4KeyCode KEY_MOUSE_Mask = 0x430000;

const C4KeyCode
	KEY_Default                  = 0,  // no key
	KEY_Any                      = ~0, // used for default key processing
	KEY_Undefined                = (~0)^1, // used to indicate an unknown key
	KEY_CONTROLLER_ButtonMin           = 0x10, // first button
	KEY_CONTROLLER_ButtonA             = 0x10,
	KEY_CONTROLLER_ButtonB             = 0x11,
	KEY_CONTROLLER_ButtonX             = 0x12,
	KEY_CONTROLLER_ButtonY             = 0x13,
	KEY_CONTROLLER_ButtonBack          = 0x14,
	KEY_CONTROLLER_ButtonGuide         = 0x15,
	KEY_CONTROLLER_ButtonStart         = 0x16,
	KEY_CONTROLLER_ButtonLeftStick     = 0x17,
	KEY_CONTROLLER_ButtonRightStick    = 0x18,
	KEY_CONTROLLER_ButtonLeftShoulder  = 0x19,
	KEY_CONTROLLER_ButtonRightShoulder = 0x1a,
	KEY_CONTROLLER_ButtonDpadUp        = 0x1b,
	KEY_CONTROLLER_ButtonDpadDown      = 0x1c,
	KEY_CONTROLLER_ButtonDpadLeft      = 0x1d,
	KEY_CONTROLLER_ButtonDpadRight     = 0x1e,
	KEY_CONTROLLER_ButtonMax           = 0x1e, // last button
	KEY_CONTROLLER_AnyButton           = 0xff, // any of the buttons above
	KEY_CONTROLLER_AxisMin             = 0x30, // first axis
	KEY_CONTROLLER_AxisLeftXLeft       = 0x30,
	KEY_CONTROLLER_AxisLeftXRight      = 0x31,
	KEY_CONTROLLER_AxisLeftYUp         = 0x32,
	KEY_CONTROLLER_AxisLeftYDown       = 0x33,
	KEY_CONTROLLER_AxisRightXLeft      = 0x34,
	KEY_CONTROLLER_AxisRightXRight     = 0x35,
	KEY_CONTROLLER_AxisRightYUp        = 0x36,
	KEY_CONTROLLER_AxisRightYDown      = 0x37,
	KEY_CONTROLLER_AxisTriggerLeft     = 0x39, // triggers are only positive
	KEY_CONTROLLER_AxisTriggerRight    = 0x3b,
	KEY_CONTROLLER_AxisMax             = 0x3b, // last axis
	KEY_MOUSE_Move               = 1,    // mouse control: mouse movement
	KEY_MOUSE_Button1            = 0x10, // key index of mouse buttons + button index for more buttons
	KEY_MOUSE_ButtonLeft         = KEY_MOUSE_Button1 + 0,
	KEY_MOUSE_ButtonRight        = KEY_MOUSE_Button1 + 1,
	KEY_MOUSE_ButtonMiddle       = KEY_MOUSE_Button1 + 2,
	KEY_MOUSE_ButtonX1           = KEY_MOUSE_Button1 + 3,
	KEY_MOUSE_ButtonX2           = KEY_MOUSE_Button1 + 4,
	KEY_MOUSE_ButtonMax          = KEY_MOUSE_Button1 + 0x1f, // max number of supported mouse buttons
	KEY_MOUSE_Button1Double      = 0x30, // double clicks have special events because double click speed is issued by OS
	KEY_MOUSE_ButtonLeftDouble   = KEY_MOUSE_Button1Double + 0,
	KEY_MOUSE_ButtonRightDouble  = KEY_MOUSE_Button1Double + 1,
	KEY_MOUSE_ButtonMiddleDouble = KEY_MOUSE_Button1Double + 2,
	KEY_MOUSE_ButtonX1Double     = KEY_MOUSE_Button1Double + 3,
	KEY_MOUSE_ButtonX2Double     = KEY_MOUSE_Button1Double + 4,
	KEY_MOUSE_ButtonMaxDouble    = KEY_MOUSE_Button1Double + 0x1f, // max number of supported mouse buttons
	KEY_MOUSE_Wheel1Up           = 0x40,    // mouse control: wheel up
	KEY_MOUSE_Wheel1Down         = 0x41;    // mouse control: wheel down

inline uint8_t KEY_CONTROLLER_Button(uint8_t idx) { return KEY_CONTROLLER_ButtonMin+idx; }
inline uint8_t KEY_CONTROLLER_Axis(uint8_t idx, bool fMax) { return KEY_CONTROLLER_AxisMin+2*idx+fMax; }

inline C4KeyCode KEY_Gamepad(uint8_t idButton) // convert gamepad key to Clonk-gamepad-keycode
{
	// mask key as 0x004200bb, where 00 used to be the gamepad ID and bb is button ID.
	return KEY_CONTROLLER_Mask + idButton;
}

inline bool Key_IsGamepad(C4KeyCode key)
{
	return (0xff0000 & key) == KEY_CONTROLLER_Mask;
}

inline uint8_t Key_GetGamepad(C4KeyCode key)
{
	return ((uint32_t)key >> 8) & 0xff;
}

inline uint8_t Key_GetGamepadEvent(C4KeyCode key)
{
	return ((uint32_t)key) & 0xff;
}

inline bool Key_IsGamepadButton(C4KeyCode key)
{
	// whether this is a unique button event (AnyButton not included)
	return Key_IsGamepad(key) && Inside<uint8_t>(Key_GetGamepadEvent(key), KEY_CONTROLLER_ButtonMin, KEY_CONTROLLER_ButtonMax);
}

inline bool Key_IsGamepadAxis(C4KeyCode key)
{
	// whether this is a unique button event (AnyButton not included)
	return Key_IsGamepad(key) && Inside<uint8_t>(Key_GetGamepadEvent(key), KEY_CONTROLLER_AxisMin, KEY_CONTROLLER_AxisMax);
}

inline uint8_t Key_GetGamepadButtonIndex(C4KeyCode key)
{
	// get zero-based button index
	return Key_GetGamepadEvent(key) - KEY_CONTROLLER_ButtonMin;
}

inline uint8_t Key_GetGamepadAxisIndex(C4KeyCode key)
{
	// get zero-based axis index
	return (Key_GetGamepadEvent(key) - KEY_CONTROLLER_AxisMin) / 2;
}

inline bool Key_IsGamepadAxisHigh(C4KeyCode key)
{
	return !!(key & 1);
}

inline C4KeyCode KEY_Mouse(uint8_t mouse_id, uint8_t mouseevent)
{
	// mask key as 0x0043ggbb, where mm is mouse ID and bb is mouse event ID.
	return KEY_MOUSE_Mask + (mouse_id<<8) + mouseevent;
}

inline bool Key_IsMouse(C4KeyCode key)
{
	return (0xff0000 & key) == KEY_MOUSE_Mask;
}

inline uint8_t Key_GetMouse(C4KeyCode key)
{
	return ((uint32_t)key >> 8) & 0xff;
}

inline uint8_t Key_GetMouseEvent(C4KeyCode key)
{
	return ((uint32_t)key) & uint8_t(0xff);
}

bool KEY_IsModifier(C4KeyCode k);

#ifdef _WIN32
#define TOUPPERIFX11(key) (key)
#else
#define TOUPPERIFX11(key) toupper(key)
#endif

enum C4KeyShiftState
{
	KEYS_None    =  0,
	KEYS_First   =  1,
	KEYS_Alt     =  1,
	KEYS_Control =  2,
	KEYS_Shift   =  4,
	KEYS_Max     = KEYS_Shift,
	KEYS_Undefined = 0xffff
};

// extended key information containing shift state
struct C4KeyCodeEx
{
	C4KeyCode Key; // the key
	DWORD dwShift; // the status of Alt, Shift, Control

	int32_t deviceId;

	// if set, the keycode was generated by a key that has been held down
	// this flag is ignored in comparison operations
	bool fRepeated;

	// helpers
	static C4KeyShiftState String2KeyShift(const StdStrBuf &sName);
	static C4KeyCode String2KeyCode(const StdStrBuf &sName);
	static StdStrBuf KeyCode2String(C4KeyCode wCode, bool fHumanReadable, bool fShort);
	StdStrBuf ToString(bool fHumanReadable, bool fShort) const;
	static StdStrBuf KeyShift2String(C4KeyShiftState eShift);

	// comparison operator for map access
	inline bool operator <(const C4KeyCodeEx &v2) const
	{
		return Key < v2.Key || (Key == v2.Key && dwShift < v2.dwShift);
	}

	inline bool operator ==(const C4KeyCodeEx &v2) const
	{
		return Key == v2.Key && dwShift == v2.dwShift;
	}

	void CompileFunc(StdCompiler *pComp, StdStrBuf *pOutBuf=nullptr);

	C4KeyCodeEx(C4KeyCode Key = KEY_Default, DWORD Shift = KEYS_None, bool fIsRepeated = false, int32_t deviceId = -1);
	static C4KeyCodeEx FromC4MC(int8_t mouse_id, int32_t button, DWORD param, bool * is_down = nullptr);

	bool IsRepeated() const { return fRepeated; }

	void FixShiftKeys(); // reduce stuff like Ctrl+RightCtrl to simply RightCtrl
private:
	static C4KeyCode GetKeyByScanCode(const char *scan_code);
};

// extra data associated with a key event
struct C4KeyEventData
{
	enum { KeyPos_None = 0x7fffff }; // value used to denote invalid/none key positions
	int32_t iStrength{0}; // pressure between 0 and 100 (100 for nomal keypress)
	int32_t game_x{KeyPos_None},game_y{KeyPos_None}, vp_x{KeyPos_None},vp_y{KeyPos_None};       // position for mouse event, landscape+viewport coordinates
	C4KeyEventData() = default;
	C4KeyEventData(int32_t iStrength, int32_t game_x, int32_t game_y, int32_t vp_x, int32_t vp_y) : iStrength(iStrength), game_x(game_x), game_y(game_y),vp_x(vp_x),vp_y(vp_y) {}
	void CompileFunc(StdCompiler *pComp);
	bool operator ==(const struct C4KeyEventData &cmp) const;
};

// Helper functions for high-level GUI control mappings.
namespace ControllerKeys {
template<class T> void Any(T &keys)    { keys.push_back(C4KeyCodeEx(KEY_Gamepad(KEY_CONTROLLER_AnyButton))); }
template<class T> void Cancel(T &keys) { keys.push_back(C4KeyCodeEx(KEY_Gamepad(KEY_CONTROLLER_ButtonB))); }
template<class T> void Ok(T &keys)     { keys.push_back(C4KeyCodeEx(KEY_Gamepad(KEY_CONTROLLER_ButtonA))); }
template<class T> void Left(T &keys)   { keys.push_back(C4KeyCodeEx(KEY_Gamepad(KEY_CONTROLLER_AxisLeftXLeft)));
                                         keys.push_back(C4KeyCodeEx(KEY_Gamepad(KEY_CONTROLLER_ButtonDpadLeft))); }
template<class T> void Right(T &keys)  { keys.push_back(C4KeyCodeEx(KEY_Gamepad(KEY_CONTROLLER_AxisLeftXRight)));
                                         keys.push_back(C4KeyCodeEx(KEY_Gamepad(KEY_CONTROLLER_ButtonDpadRight))); }
template<class T> void Up(T &keys)     { keys.push_back(C4KeyCodeEx(KEY_Gamepad(KEY_CONTROLLER_AxisLeftYUp)));
                                         keys.push_back(C4KeyCodeEx(KEY_Gamepad(KEY_CONTROLLER_ButtonDpadUp))); }
template<class T> void Down(T &keys)   { keys.push_back(C4KeyCodeEx(KEY_Gamepad(KEY_CONTROLLER_AxisLeftYDown)));
                                         keys.push_back(C4KeyCodeEx(KEY_Gamepad(KEY_CONTROLLER_ButtonDpadDown))); }
}

// callback interface
class C4KeyboardCallbackInterface
{
private:
	int iRef{0};
public:
	class C4CustomKey *pOriginalKey{nullptr};

public:
	virtual bool OnKeyEvent(const C4KeyCodeEx &key, C4KeyEventType eEv) = 0; // return true if processed

	friend class C4KeyboardMapping;

	// reference counter
	inline void Ref() { ++iRef; }
	inline void Deref() { if (!--iRef) delete this; }

	C4KeyboardCallbackInterface() = default;
	virtual ~C4KeyboardCallbackInterface() = default;

	bool IsOriginalKey(const class C4CustomKey *pCheckKey) const { return pCheckKey == pOriginalKey; }
};

// callback interface
template <class TargetClass> class C4KeyCB : public C4KeyboardCallbackInterface
{
public:
	typedef bool(TargetClass::*CallbackFunc)();

protected:
	TargetClass &rTarget;
	CallbackFunc pFuncDown, pFuncUp, pFuncPressed, pFuncMoved;

protected:
	bool OnKeyEvent(const C4KeyCodeEx &key, C4KeyEventType eEv) override
	{
		if (!CheckCondition()) return false;
		switch (eEv)
		{
		case KEYEV_Down: return pFuncDown ? (rTarget.*pFuncDown)() : false;
		case KEYEV_Up: return pFuncUp ? (rTarget.*pFuncUp)() : false;
		case KEYEV_Pressed: return pFuncPressed ? (rTarget.*pFuncPressed)() : false;
		case KEYEV_Moved: return pFuncMoved ? (rTarget.*pFuncMoved)() : false;
		default: return false;
		}
	}

	virtual bool CheckCondition() { return true; }

public:
	C4KeyCB(TargetClass &rTarget, CallbackFunc pFuncDown, CallbackFunc pFuncUp=nullptr, CallbackFunc pFuncPressed=nullptr, CallbackFunc pFuncMoved=nullptr)
			: rTarget(rTarget), pFuncDown(pFuncDown), pFuncUp(pFuncUp), pFuncPressed(pFuncPressed), pFuncMoved(pFuncMoved) {}
};

// callback interface that passes the pressed key as a parameter
template <class TargetClass> class C4KeyCBPassKey : public C4KeyboardCallbackInterface
{
public:
	typedef bool(TargetClass::*CallbackFunc)(const C4KeyCodeEx &key);

protected:
	TargetClass &rTarget;
	CallbackFunc pFuncDown, pFuncUp, pFuncPressed, pFuncMoved;

protected:
	bool OnKeyEvent(const C4KeyCodeEx &key, C4KeyEventType eEv) override
	{
		if (!CheckCondition()) return false;
		switch (eEv)
		{
		case KEYEV_Down: return pFuncDown ? (rTarget.*pFuncDown)(key) : false;
		case KEYEV_Up: return pFuncUp ? (rTarget.*pFuncUp)(key) : false;
		case KEYEV_Pressed: return pFuncPressed ? (rTarget.*pFuncPressed)(key) : false;
		case KEYEV_Moved: return pFuncMoved ? (rTarget.*pFuncMoved)(key) : false;
		default: return false;
		}
	}

	virtual bool CheckCondition() { return true; }

public:
	C4KeyCBPassKey(TargetClass &rTarget, CallbackFunc pFuncDown, CallbackFunc pFuncUp=nullptr, CallbackFunc pFuncPressed=nullptr, CallbackFunc pFuncMoved=nullptr)
			: rTarget(rTarget), pFuncDown(pFuncDown), pFuncUp(pFuncUp), pFuncPressed(pFuncPressed), pFuncMoved(pFuncMoved) {}
};

// parameterized callback interface
template <class TargetClass, class ParameterType> class C4KeyCBEx : public C4KeyboardCallbackInterface
{
public:
	typedef bool(TargetClass::*CallbackFunc)(ParameterType par);

protected:
	TargetClass &rTarget;
	CallbackFunc pFuncDown, pFuncUp, pFuncPressed, pFuncMoved;
	ParameterType par;

protected:
	bool OnKeyEvent(const C4KeyCodeEx &key, C4KeyEventType eEv) override
	{
		if (!CheckCondition()) return false;
		switch (eEv)
		{
		case KEYEV_Down: return pFuncDown ? (rTarget.*pFuncDown)(par) : false;
		case KEYEV_Up: return pFuncUp ? (rTarget.*pFuncUp)(par) : false;
		case KEYEV_Pressed: return pFuncPressed ? (rTarget.*pFuncPressed)(par) : false;
		case KEYEV_Moved: return pFuncMoved ? (rTarget.*pFuncMoved)(par) : false;
		default: return false;
		}
	}

	virtual bool CheckCondition() { return true; }

public:
	C4KeyCBEx(TargetClass &rTarget, const ParameterType &par, CallbackFunc pFuncDown, CallbackFunc pFuncUp=nullptr, CallbackFunc pFuncPressed=nullptr, CallbackFunc pFuncMoved=nullptr)
			: rTarget(rTarget), pFuncDown(pFuncDown), pFuncUp(pFuncUp), pFuncPressed(pFuncPressed), pFuncMoved(pFuncMoved), par(par) {}
};

template <class TargetClass, class ParameterType> class C4KeyCBExPassKey : public C4KeyboardCallbackInterface
{
public:
	typedef bool(TargetClass::*CallbackFunc)(const C4KeyCodeEx &key, const ParameterType &par);

protected:
	TargetClass &rTarget;
	CallbackFunc pFuncDown, pFuncUp, pFuncPressed, pFuncMoved;
	ParameterType par;

protected:
	bool OnKeyEvent(const C4KeyCodeEx &key, C4KeyEventType eEv) override
	{
		if (!CheckCondition()) return false;
		switch (eEv)
		{
		case KEYEV_Down: return pFuncDown ? (rTarget.*pFuncDown)(key, par) : false;
		case KEYEV_Up: return pFuncUp ? (rTarget.*pFuncUp)(key, par) : false;
		case KEYEV_Pressed: return pFuncPressed ? (rTarget.*pFuncPressed)(key, par) : false;
		case KEYEV_Moved: return pFuncMoved ? (rTarget.*pFuncMoved)(key, par) : false;
		default: return false;
		}
	}

	virtual bool CheckCondition() { return true; }

public:
	C4KeyCBExPassKey(TargetClass &rTarget, const ParameterType &par, CallbackFunc pFuncDown, CallbackFunc pFuncUp=nullptr, CallbackFunc pFuncPressed=nullptr, CallbackFunc pFuncMoved=nullptr)
			: rTarget(rTarget), pFuncDown(pFuncDown), pFuncUp(pFuncUp), pFuncPressed(pFuncPressed), pFuncMoved(pFuncMoved), par(par) {}
};

// one mapped keyboard entry
class C4CustomKey
{
public:
	typedef std::vector<C4KeyCodeEx> CodeList;
private:
	CodeList Codes, DefaultCodes; // keyboard scancodes of OS plus shift state
	C4KeyScope Scope;              // scope in which key is processed
	StdStrBuf Name;                // custom key name; used for association in config files
	typedef std::vector<C4KeyboardCallbackInterface *> CBVec;
	unsigned int uiPriority;       // key priority: If multiple keys of same code are defined, high prio overwrites low prio keys
	bool is_down;                  // down-callbacks have been executed but up-callbacks have not (not compiled)

public:
	CBVec vecCallbacks; // a list of all callbacks assigned to that key

	enum Priority
	{
		PRIO_None = 0u,
		PRIO_Base = 1u,
		PRIO_Dlg  = 2u,
		PRIO_Ctrl = 3u, // controls have higher priority than dialogs in GUI
		PRIO_CtrlOverride = 4u, // dialog handlings of keys that overwrite regular control handlings
		PRIO_FocusCtrl = 5u,  // controls override special dialog handling keys (e.g., RenameEdit)
		PRIO_Context = 6u, // context menus above controls
		PRIO_PlrControl = 7u, // player controls overwrite any other controls
		PRIO_MoreThanMax  = 100u // must be larger than otherwise largest used priority
	};

protected:
	int iRef;
	C4CustomKey(const C4KeyCodeEx &DefCode, const char *szName, C4KeyScope Scope, C4KeyboardCallbackInterface *pCallback, unsigned int uiPriority = PRIO_Base); // ctor for default key
	C4CustomKey(CodeList rDefCodes, const char *szName, C4KeyScope Scope, C4KeyboardCallbackInterface *pCallback, unsigned int uiPriority = PRIO_Base); // ctor for default key with multiple possible keys assigned
	friend class C4Game;

public:
	C4CustomKey(const C4CustomKey &rCpy, bool fCopyCallbacks);
	virtual ~C4CustomKey(); // dtor

	inline void Ref() { ++iRef; }
	inline void Deref() { if (!--iRef) delete this; }

	const CodeList &GetCodes() const { return Codes.size() ? Codes : DefaultCodes; } // return assigned codes; default if no custom has been assigned
	const StdStrBuf &GetName() const { return Name; }
	C4KeyScope GetScope() const { return Scope; }
	unsigned int GetPriority() const { return uiPriority; }
	bool IsCodeMatched(const C4KeyCodeEx &key) const;

	void Update(const C4CustomKey *pByKey); // merge given key into this
	bool Execute(C4KeyEventType eEv, C4KeyCodeEx key);

	bool IsDown() const { return is_down; }

	void KillCallbacks(const C4CustomKey *pOfKey); // remove any callbacks that were created by given key

	void CompileFunc(StdCompiler *pComp);
};

// a key that auto-registers itself into main game keyboard input class and does dereg when deleted
class C4KeyBinding : protected C4CustomKey
{
	// Stuffing these into an std::vector ends badly, so I've marked them non-copyable.
	C4KeyBinding(const C4KeyBinding&) = delete;
	C4KeyBinding& operator=(const C4KeyBinding&) = delete;
public:
	C4KeyBinding(const C4KeyCodeEx &DefCode, const char *szName, C4KeyScope Scope, C4KeyboardCallbackInterface *pCallback, unsigned int uiPriority = PRIO_Base); // ctor for default key
	C4KeyBinding(const CodeList &rDefCodes, const char *szName, C4KeyScope Scope, C4KeyboardCallbackInterface *pCallback, unsigned int uiPriority = PRIO_Base); // ctor for default key
	~C4KeyBinding() override;
};

// main keyboard mapping class
class C4KeyboardInput
{
private:
	// comparison fn for map
	struct szLess
	{
		bool operator()(const char *p, const char *q) const { return p && q && (strcmp(p,q)<0); }
	};

	typedef std::multimap<C4KeyCode, C4CustomKey *> KeyCodeMap;
	typedef std::map<const char *, C4CustomKey *, szLess> KeyNameMap;
	// mapping of all keys by code and name
	KeyCodeMap KeysByCode;
	KeyNameMap KeysByName;
	C4KeyEventData LastKeyExtraData;

public:
	static bool IsValid; // global var to fix any deinitialization orders of key map and static keys

	C4KeyboardInput() { IsValid = true; }
	~C4KeyboardInput() { Clear(); IsValid = false; }

	void Clear(); // empty keyboard maps

private:
	// assign keycodes changed for a key: Update codemap
	void UpdateKeyCodes(C4CustomKey *pKey, const C4CustomKey::CodeList &rOldCodes, const C4CustomKey::CodeList &rNewCodes);

public:
	void RegisterKey(C4CustomKey *pRegKey); // register key into code and name maps, or update specific key
	void UnregisterKey(const StdStrBuf &rsName); // remove key from all maps
	void UnregisterKeyBinding(C4CustomKey *pKey); // just remove callbacks from a key

	bool DoInput(const C4KeyCodeEx &InKey, C4KeyEventType InEvent, DWORD InScope, int32_t iStrength);

	void CompileFunc(StdCompiler *pComp);
	bool LoadCustomConfig(); // load keyboard customization file

	C4CustomKey *GetKeyByName(const char *szKeyName);
	StdStrBuf GetKeyCodeNameByKeyName(const char *szKeyName, bool fShort = false, int32_t iIndex = 0);
	const C4KeyEventData &GetLastKeyExtraData() const { return LastKeyExtraData; }
	void SetLastKeyExtraData(const C4KeyEventData &data) { LastKeyExtraData=data; }
};

// keyboardinput-initializer-helper
C4KeyboardInput &C4KeyboardInput_Init();

#endif // INC_C4KeyboardInput

