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

- (void) awakeFromNib
{
	[super awakeFromNib];
	if (!Application.isEditor)
		ClonkAppDelegate.instance.gameWindowController = self;
}

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
		// fade out
#ifndef _DEBUG
		CGDisplayFadeReservationToken token;
		[self fadeOut:&token];
#endif
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
			//[openGLView setContextSurfaceBackingSizeToOwnDimensions];
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
			[self.window setContentView:openGLView];
			[self.window orderFront:self];
			[openGLView setFrame:[self.window.contentView frame]];
			[openGLView enableEvents];
			[self.window makeKeyAndOrderFront:self];
		}
		[openGLView display];
#ifndef _DEBUG
		// fade in again
		[self fadeIn:token];
#endif
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
	if (Application.isEditor)
	{
		C4Viewport* viewport = self.viewport;
		if (viewport && Application.isEditor)
		{
			viewport->ScrollBarsByViewPosition();
		}
	}
	else
	{
	/*
		NSSize newRes = openGLView.frame.size;
		[ClonkOpenGLView setSurfaceBackingSizeOf:ClonkOpenGLView.mainContext width:newRes.width height:newRes.height];
		Config.Graphics.ResX = newRes.width;
		Config.Graphics.ResY = newRes.height; */
	}
}

- (BOOL) isLiveResizing
{
	return self.window.inLiveResize;
}

@end

@interface NSScroller (ClonkZoom)
- (void) setToLandscapeCoordinate:(float)lc
	size:(int) s
	viewportSize:(int) vs
	zoom: (float) z;
- (float) landscapeCoordinateForSize:(int) s
	viewportSize:(int) vs
	zoom:(float) z;
@end
@implementation NSScroller (ClonkZoom)
- (void) setToLandscapeCoordinate:(float)lc
	size:(int) s
	viewportSize:(int) vs
	zoom: (float) z
{
	self.doubleValue = (double)lc/((double)s - (double)vs/z);
	self.knobProportion = (CGFloat)std::min(1.0, (double)vs/(s*z));
}

- (float) landscapeCoordinateForSize:(int) s
	viewportSize:(int) vs
	zoom:(float) z
{	
	return self.doubleValue * ((double)s - (double)vs/z);
}
@end

// C4Fullscreen

void C4FullScreen::HandleMessage (void* event)
{
	[NSApp sendEvent:(NSEvent*)event];
}

// C4ViewportWindow

bool C4Viewport::ScrollBarsByViewPosition()
{
	if (PlayerLock) return false;
	NSScrollView* scrollView = ((ConsoleWindowController*)pWindow->GetController()).scrollView;
	[scrollView.horizontalScroller setToLandscapeCoordinate:ViewX size:GBackWdt viewportSize:ViewWdt zoom:GetZoom()];
	[scrollView.verticalScroller setToLandscapeCoordinate:ViewY size:GBackHgt viewportSize:ViewHgt zoom:GetZoom()];
	return true;
}

bool C4Viewport::ViewPositionByScrollBars()
{
	NSScrollView* scrollView = ((ConsoleWindowController*)pWindow->GetController()).scrollView;
	ViewX = [scrollView.horizontalScroller landscapeCoordinateForSize:GBackWdt viewportSize:ViewWdt zoom:GetZoom()];
	ViewY = [scrollView.verticalScroller landscapeCoordinateForSize:GBackHgt viewportSize:ViewHgt zoom:GetZoom()];
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
