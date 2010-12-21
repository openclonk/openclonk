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
#include <C4Fullscreen.h>
#include <C4GraphicsSystem.h>
#include <C4Viewport.h>
#include <C4ViewportWindow.h>
#include <C4Console.h>
#include <C4Game.h>
#include <C4Landscape.h>

#import <StdGL.h>

#import "ClonkWindowController.h"
#import "ClonkOpenGLView.h"
#import "ConsoleWindowController.h"
#import "ClonkAppDelegate.h"
#import "AppKit/NSOpenGL.h"

#ifdef USE_COCOA

// Turns out, it is necessary to derive a NSWindow class after all - or else the screen-filling window won't be able to become the key window

@interface ClonkScreenfillingWindow: NSWindow
{}
- (BOOL) canBecomeKeyWindow;
@end

@implementation ClonkScreenfillingWindow
- (BOOL) canBecomeKeyWindow;
{
	return YES; // a resounding one
}
@end


@implementation ClonkWindowController

@synthesize stdWindow, openGLView, scrollView;

- (void) fadeOut:(CGDisplayFadeReservationToken*)token
{
	if (CGAcquireDisplayFadeReservation(15, token) == 0)
		CGDisplayFade(*token, 0.2, 0.0, 1.0, 0, 0, 0, YES);
}

- (void) fadeIn:(CGDisplayFadeReservationToken)token
{
	CGDisplayFade(token, 0.2, 1.0, 0.0, 0, 0, 0, YES);
	CGReleaseDisplayFadeReservation(token);
}

- (BOOL) isFullscreen
{
	return fullscreenWindow != nil;
}

- (void) setFullscreen:(BOOL)fullscreen
{
	if (fullscreen != [self isFullscreen])
	{
		// fade out
		CGDisplayFadeReservationToken token;
		[self fadeOut:&token];
		if (![self isFullscreen])
		{
			NSRect fullscreenRect = NSScreen.mainScreen.frame;
			fullscreenWindow = [[ClonkScreenfillingWindow alloc] initWithContentRect:fullscreenRect styleMask:NSBorderlessWindowMask backing:NSBackingStoreBuffered defer:YES];
			[fullscreenWindow setLevel:NSMainMenuWindowLevel+1];
			[fullscreenWindow setOpaque:YES];
			[fullscreenWindow setHidesOnDeactivate:YES];
			[fullscreenWindow setContentView:openGLView];
			[fullscreenWindow setReleasedWhenClosed:YES];
			[fullscreenWindow setDelegate:self];
			[self.window orderOut:self];
			[fullscreenWindow setInitialFirstResponder:openGLView];
			[fullscreenWindow makeKeyAndOrderFront:self];
			[openGLView enableEvents];
			// hide cursor completely
			[NSCursor hide];
		}
		else
		{
			// unhide and rely on cursor rects again
			[NSCursor unhide];
		
			[openGLView retain];
			[fullscreenWindow close];
			fullscreenWindow = nil;
			[self.window.contentView addSubview:openGLView];
			[self.window orderFront:self];
			[openGLView setFrame:[self.window.contentView frame]];
			[openGLView enableEvents];
			[openGLView display];
			[self.window makeKeyAndOrderFront:self];
		}
		// fade in again
		[self fadeIn:token];
	}
}

- (void) setStdWindow:(CStdWindow*)window
{
	stdWindow = window;
}

- (BOOL) windowShouldClose:(id)sender
{
	if (sender == self.window && self.stdWindow)
	{
		[self.openGLView showCursor];
		self.stdWindow->Close();
	}
	return YES;
}

- (C4Viewport*) viewport
{
	for (C4Viewport* v = ::Viewports.GetFirstViewport(); v; v = v->GetNext())
		if (v->GetWindow() == stdWindow || v->GetWindow() == NULL && stdWindow == &FullScreen)
			return v;
	return NULL;
}

- (IBAction) scroll:(id)sender
{
	C4Viewport* viewport = self.viewport;
	if (viewport)
	{
		viewport->ViewPositionByScrollBars();
		viewport->Execute();
	}
}

- (void) windowDidResize:(NSNotification *)notification
{
	C4Viewport* viewport = self.viewport;
	if (viewport && Application.isEditor)
	{
		viewport->ScrollBarsByViewPosition();
	}
}

- (BOOL) isLiveResizing
{
	return self.window.inLiveResize;
}

@end

// C4Fullscreen

void C4FullScreen::HandleMessage (void* event)
{
	[NSApp sendEvent:(NSEvent*)event];
}

// C4ViewportWindow

CStdWindow * C4ViewportWindow::Init(CStdWindow::WindowKind windowKind, CStdApp * pApp, const char * Title, CStdWindow * pParent, bool b)
{
	CStdWindow* result = CStdWindow::Init(windowKind, pApp, Title, pParent, b);
	return result;
}

bool C4Viewport::ScrollBarsByViewPosition()
{
	if (PlayerLock) return false;
	NSScrollView* scrollView = ((ConsoleWindowController*)pWindow->GetController()).scrollView;
	[scrollView.horizontalScroller setFloatValue:ViewX/(GBackWdt-ViewWdt)*GetZoom()];
	[scrollView.verticalScroller   setFloatValue:ViewY/(GBackHgt-ViewHgt)*GetZoom()];
	[scrollView.horizontalScroller setKnobProportion:(float)ViewWdt/(float)GBackWdt/GetZoom()];
	[scrollView.verticalScroller setKnobProportion:(float)ViewHgt/(float)GBackHgt/GetZoom()];
	return true;
}

bool C4Viewport::ViewPositionByScrollBars()
{
	NSScrollView* scrollView = ((ConsoleWindowController*)pWindow->GetController()).scrollView;
	ViewX = [scrollView.horizontalScroller floatValue] * (GBackWdt-ViewWdt) / GetZoom();
	ViewY = [scrollView.verticalScroller floatValue] * (GBackHgt-ViewHgt) / GetZoom();
	return true;
}

bool C4Viewport::TogglePlayerLock()
{
	NSScrollView* scrollView = ((ConsoleWindowController*)pWindow->GetController()).scrollView;
	if (PlayerLock)
	{
		PlayerLock = false;
		if (scrollView)
		{
			[scrollView.verticalScroller setEnabled:YES];
			[scrollView.horizontalScroller setEnabled:YES];
			[scrollView setAutohidesScrollers:NO];
		}
		ScrollBarsByViewPosition();
	}
	else
	{
		PlayerLock = true;
		if (scrollView)
		{
			[scrollView.verticalScroller setEnabled:NO];
			[scrollView.horizontalScroller setEnabled:NO];
			[scrollView setAutohidesScrollers:YES];
		}
	}
	return true;
}

#endif
