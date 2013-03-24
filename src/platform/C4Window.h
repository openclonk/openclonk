/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005-2007, 2009-2010  Günther Brammer
 * Copyright (c) 2005  Sven Eberhardt
 * Copyright (c) 2005, 2009  Peter Wortmann
 * Copyright (c) 2006  Julian Raschke
 * Copyright (c) 2006, 2008, 2010  Armin Burgmeier
 * Copyright (c) 2007  Alexander Post
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

/* A wrapper class to OS dependent event and window interfaces */

#ifndef INC_STDWINDOW
#define INC_STDWINDOW

#include <StdBuf.h>

#if defined(USE_WIN32_WINDOWS) || defined(USE_X11)
#define K_SHIFT_L 42
#define K_SHIFT_R 54
#define K_ALT_L 56
#define K_ALT_R 100
#define K_F1 59
#define K_F2 60
#define K_F3 61
#define K_F4 62
#define K_F5 63
#define K_F6 64
#define K_F7 65
#define K_F8 66
#define K_F9 67
#define K_F10 68
#define K_F11 87
#define K_F12 88
#define K_ADD 78
#define K_SUBTRACT 74
#define K_MULTIPLY 55
#define K_ESCAPE 1
#define K_PAUSE 119
#define K_TAB 15
#define K_RETURN 28
#define K_DELETE 111
#define K_INSERT 110
#define K_BACK 14
#define K_SPACE 57
#define K_UP 103
#define K_DOWN 108
#define K_LEFT 105
#define K_RIGHT 106
#define K_HOME 102
#define K_END 107
#define K_SCROLL 70
#define K_MENU 127
#define K_PAGEUP 104
#define K_PAGEDOWN 109
#define KEY_A 30 // select all in GUI-editbox
#define KEY_C 46 // copy in GUI-editbox
#define KEY_I 23 // console mode control key
#define KEY_M 50 // console mode control key
#define KEY_T 20 // console mode control key
#define KEY_V 47 // paste in GUI-editbox
#define KEY_W 17 // console mode control key
#define KEY_X 45 // cut from GUI-editbox
#elif defined(USE_SDL_MAINLOOP)
#include <SDL.h>
#define K_SHIFT_L SDLK_LSHIFT
#define K_SHIFT_R SDLK_RSHIFT
#define K_ALT_L SDLK_LALT
#define K_ALT_R SDLK_RALT
#define K_F1 SDLK_F1
#define K_F2 SDLK_F2
#define K_F3 SDLK_F3
#define K_F4 SDLK_F4
#define K_F5 SDLK_F5
#define K_F6 SDLK_F6
#define K_F7 SDLK_F7
#define K_F8 SDLK_F8
#define K_F9 SDLK_F9
#define K_F10 SDLK_F10
#define K_F11 SDLK_F11
#define K_F12 SDLK_F12
#define K_ADD SDLK_KP_PLUS
#define K_SUBTRACT SDLK_KP_MINUS
#define K_MULTIPLY SDLK_KP_MULTIPLY
#define K_ESCAPE SDLK_ESCAPE
#define K_PAUSE SDLK_PAUSE
#define K_TAB SDLK_TAB
#define K_RETURN SDLK_RETURN
#define K_DELETE SDLK_DELETE
#define K_INSERT SDLK_INSERT
#define K_BACK SDLK_BACKSPACE
#define K_SPACE SDLK_SPACE
#define K_UP SDLK_UP
#define K_DOWN SDLK_DOWN
#define K_LEFT SDLK_LEFT
#define K_RIGHT SDLK_RIGHT
#define K_HOME SDLK_HOME
#define K_END SDLK_END
#define K_SCROLL SDLK_SCROLLOCK
#define K_MENU SDLK_MENU
#define K_PAGEUP SDLK_PAGEUP
#define K_PAGEDOWN SDLK_PAGEDOWN
#define KEY_M SDLK_m
#define KEY_T SDLK_t
#define KEY_W SDLK_w
#define KEY_I SDLK_i
#define KEY_C SDLK_c
#define KEY_V SDLK_v
#define KEY_X SDLK_x
#define KEY_A SDLK_a
#elif defined(USE_CONSOLE)
#define K_SHIFT_L 0
#define K_SHIFT_R 0
#define K_ALT_L 0
#define K_ALT_R 0
#define K_F1 0
#define K_F2 0
#define K_F3 0
#define K_F4 0
#define K_F5 0
#define K_F6 0
#define K_F7 0
#define K_F8 0
#define K_F9 0
#define K_F10 0
#define K_F11 0
#define K_F12 0
#define K_ADD 0
#define K_SUBTRACT 0
#define K_MULTIPLY 0
#define K_ESCAPE 0
#define K_PAUSE 0
#define K_TAB 0
#define K_RETURN 0
#define K_DELETE 0
#define K_INSERT 0
#define K_BACK 0
#define K_SPACE 0
#define K_UP 0
#define K_DOWN 0
#define K_LEFT 0
#define K_RIGHT 0
#define K_HOME 0
#define K_END 0
#define K_SCROLL 0
#define K_MENU 0
#define K_PAGEUP 0
#define K_PAGEDOWN 0
#define KEY_M 0
#define KEY_T 0
#define KEY_W 0
#define KEY_I 0
#define KEY_C 0
#define KEY_V 0
#define KEY_X 0
#define KEY_A 0
#elif defined(USE_COCOA)
#import "ObjectiveCAssociated.h"
// declare as extern variables and initialize them in StdMacWindow.mm so as to not include objc headers
const int CocoaKeycodeOffset = 300;
extern int K_SHIFT_L;
extern int K_SHIFT_R;
extern int K_ALT_L;
extern int K_ALT_R;
extern int K_F1;
extern int K_F2;
extern int K_F3;
extern int K_F4;
extern int K_F5;
extern int K_F6;
extern int K_F7;
extern int K_F8;
extern int K_F9;
extern int K_F10;
extern int K_F11;
extern int K_F12;
extern int K_ADD;
extern int K_SUBTRACT;
extern int K_MULTIPLY;
extern int K_ESCAPE;
extern int K_PAUSE;
extern int K_TAB;
extern int K_RETURN;
extern int K_DELETE;
extern int K_INSERT;
extern int K_BACK;
extern int K_SPACE;
extern int K_UP;
extern int K_DOWN;
extern int K_LEFT;
extern int K_RIGHT;
extern int K_HOME;
extern int K_END;
extern int K_SCROLL;
extern int K_MENU;
extern int K_PAGEUP;
extern int K_PAGEDOWN;
extern int KEY_M;
extern int KEY_T;
extern int KEY_W;
extern int KEY_I;
extern int KEY_C;
extern int KEY_V;
extern int KEY_X;
extern int KEY_A;
#endif

#ifdef USE_X11
// Forward declarations because xlib.h is evil
typedef union _XEvent XEvent;
typedef struct _XDisplay Display;
#endif

class C4Window
#ifdef USE_COCOA
	: public ObjectiveCAssociated
#endif
{
public:
	enum WindowKind
	{
		W_GuiWindow,
		W_Console,
		W_Viewport,
		W_Fullscreen
	};
public:
	C4Window ();
	virtual ~C4Window ();
	bool Active;
	C4Surface * pSurface;
	virtual void Clear();
	// Only when the wm requests a close
	// For example, when the user clicks the little x in the corner or uses Alt-F4
	virtual void Close() = 0;
	// Keypress(es) translated to a char
	virtual void CharIn(const char *) { }

	// Reinitialize the window with updated configuration settings.
	// Keep window kind, title and size as they are. Currently the only point
	// at which it makes sense for this function to be called is when the
	// multisampling configuration option changes, since, for the change to
	// take effect, we need to choose another visual or pixel format, respectively.
	virtual bool ReInit(C4AbstractApp* pApp);

	// Creates a list of available samples for multisampling
	virtual void EnumerateMultiSamples(std::vector<int>& samples) const;

	bool StorePosition(const char *szWindowName, const char *szSubKey, bool fStoreSize = true);
	bool RestorePosition(const char *szWindowName, const char *szSubKey, bool fHidden = false);
	bool GetSize(C4Rect * pRect);
	void SetSize(unsigned int cx, unsigned int cy); // resize
	void SetTitle(const char * Title);
	void FlashWindow();
	// request that this window be redrawn in the near future (including immediately)
	virtual void RequestUpdate();
	// Invokes actual drawing code - should not be called directly
	virtual void PerformUpdate();

#ifdef USE_WIN32_WINDOWS
public:
	HWND hWindow;
	HWND hRenderWindow;
	virtual bool Win32DialogMessageHandling(MSG * msg) { return false; };
#elif defined(WITH_GLIB)
public:
	/*GtkWidget*/void * window;
	// Set by Init to the widget which is used as a
	// render target, which can be the whole window.
	/*GtkWidget*/void * render_widget;
protected:
	bool FindInfo(int samples, void** info);

	unsigned long wnd;
	unsigned long renderwnd;
	// The XVisualInfo the window was created with
	void * Info;
	unsigned long handlerDestroy;

	friend class C4X11AppImpl;
#endif
protected:
	virtual C4Window * Init(WindowKind windowKind, C4AbstractApp * pApp, const char * Title, const C4Rect * size);
	friend class C4Draw;
	friend class CStdGL;
	friend class CStdGLCtx;
	friend class C4AbstractApp;
};

#endif // INC_STDWINDOW
