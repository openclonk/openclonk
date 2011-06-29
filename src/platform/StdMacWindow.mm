/*
 * OpenClonk, http://www.openclonk.org
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
 
#include <C4Include.h>
#include <Standard.h>
#include <StdRegistry.h>
#include <StdGL.h>
#include <StdWindow.h>
#include <C4Version.h>
#include <C4Application.h>
#include <C4Rect.h>

#import <Appkit/AppKit.h>
#import <ClonkWindowController.h>

#ifdef USE_COCOA

#define ctrler ((ClonkWindowController*)this->controller)

CStdWindow::CStdWindow ():
	Active(false),
	pSurface(0),
	controller(nil) 
{}

CStdWindow::~CStdWindow () {}

static NSString* windowNibNameForWindowKind(CStdWindow::WindowKind kind)
{
	switch (kind)
	{
	case CStdWindow::W_GuiWindow:
		return @"ConsoleGUIWindow";
	case CStdWindow::W_Fullscreen:
		return @"FullscreenWindow";
	case CStdWindow::W_Viewport:
		return @"ClonkWindow";
	default:
		return nil;
	}
}

CStdWindow * CStdWindow::Init(CStdWindow::WindowKind windowKind, CStdApp * pApp, const char * Title, CStdWindow * pParent, bool HideCursor)
{
	Active = true;

	// Create window
	ClonkWindowController* controller = [ClonkWindowController new];
	this->controller = controller;
	[NSBundle loadNibNamed:windowNibNameForWindowKind(windowKind) owner:controller];
	[controller setStdWindow:this];
	SetTitle(Title);
	return this;
}

void CStdWindow::Clear()
{
	// Destroy window
	ClonkWindowController* controller;
	if (controller = ctrler)
	{
		[controller.openGLView setNeedsDisplay:NO];
		[controller.openGLView removeFromSuperview];
		[controller setStdWindow:NULL];
		[controller close];
		[controller release];
		this->controller = nil;
	}
}

bool CStdWindow::StorePosition(const char *szWindowName, const char *szSubKey, bool fStoreSize)
{
	[ctrler setWindowFrameAutosaveName:[NSString stringWithFormat:@"%s_%s", szWindowName, szSubKey]];
	return true;
}

bool CStdWindow::RestorePosition(const char *szWindowName, const char *szSubKey, bool fHidden)
{
	StorePosition(szWindowName, szSubKey, true);
	if (fHidden)
		[ctrler.window orderOut:ctrler];
	else
		[ctrler.window orderFront:ctrler];
	return true;
}

void CStdWindow::SetTitle(const char *szToTitle)
{
	ClonkWindowController* controller;
	if ((controller = ctrler) && controller.window)
		[controller.window setTitle:[NSString stringWithUTF8String:szToTitle ? szToTitle : ""]];
}

bool CStdWindow::GetSize(C4Rect * pRect)
{
	ClonkWindowController* controller = ctrler;
	NSView* view = controller.openGLView ? controller.openGLView : controller.window.contentView;
	NSRect r = [view frame];
	pRect->x = 0;
	pRect->y = 0;
	pRect->Wdt = r.size.width;
	pRect->Hgt = r.size.height;
	return true;
}

void CStdWindow::SetSize(unsigned int cx, unsigned int cy)
{
	ClonkWindowController* controller = ctrler;
	if (controller.window)
	{
		[controller.window setContentSize:NSMakeSize(cx, cy)];
		//[controller.window center];
	}
}

void CStdWindow::HandleMessage(void*)
{
}

void CStdWindow::RequestUpdate()
{
	[ctrler.openGLView display];
}

bool CStdWindow::ReInit(CStdApp* pApp)
{
	return true;
}

int K_F1 = 122 + CocoaKeycodeOffset;
int K_F2 = 120 + CocoaKeycodeOffset;
int K_F3 = 99 + CocoaKeycodeOffset;
int K_F4 = 118 + CocoaKeycodeOffset;
int K_F5 = 96 + CocoaKeycodeOffset;
int K_F6 = 97 + CocoaKeycodeOffset;
int K_F7 = 98 + CocoaKeycodeOffset;
int K_F8 = 100 + CocoaKeycodeOffset;
int K_F9 = 101 + CocoaKeycodeOffset;
int K_F10 = 109 + CocoaKeycodeOffset;
int K_F11 = 103 + CocoaKeycodeOffset;
int K_F12 = 111 + CocoaKeycodeOffset;
int K_ADD = 69 + CocoaKeycodeOffset;
int K_SUBTRACT = 78 + CocoaKeycodeOffset;
int K_MULTIPLY = 67 + CocoaKeycodeOffset;
int K_ESCAPE = 53 + CocoaKeycodeOffset;
int K_PAUSE = NSPauseFunctionKey + CocoaKeycodeOffset;
int K_TAB = 48 + CocoaKeycodeOffset;
int K_RETURN = 36 + CocoaKeycodeOffset;
int K_DELETE = 117 + CocoaKeycodeOffset;
int K_INSERT = 125125125 + CocoaKeycodeOffset;
int K_BACK = 51 + CocoaKeycodeOffset;
int K_SPACE = 49 + CocoaKeycodeOffset;
int K_UP = 126 + CocoaKeycodeOffset;
int K_DOWN = 125 + CocoaKeycodeOffset;
int K_LEFT = 123 + CocoaKeycodeOffset;
int K_RIGHT = 124 + CocoaKeycodeOffset;
int K_HOME = 115 + CocoaKeycodeOffset;
int K_END = 119 + CocoaKeycodeOffset;
int K_SCROLL = 1000 + CocoaKeycodeOffset;
int K_MENU = 1000 + CocoaKeycodeOffset;
int K_PAGEUP = 116 + CocoaKeycodeOffset;
int K_PAGEDOWN = 121 + CocoaKeycodeOffset;
int KEY_M = 46 + CocoaKeycodeOffset;
int KEY_T = 17 + CocoaKeycodeOffset;
int KEY_W = 13 + CocoaKeycodeOffset;
int KEY_I = 34 + CocoaKeycodeOffset;
int KEY_C = 8 + CocoaKeycodeOffset;
int KEY_V = 9 + CocoaKeycodeOffset;
int KEY_X = 7 + CocoaKeycodeOffset;
int KEY_A = 0 + CocoaKeycodeOffset;
int MK_SHIFT = NSShiftKeyMask;
int MK_CONTROL = NSControlKeyMask;

#endif