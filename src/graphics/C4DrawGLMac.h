/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2009-2011  Martin Plicht
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

#import <Cocoa/Cocoa.h>
#include <C4Window.h>

#ifdef USE_COCOA

@class C4WindowController;

extern int ActualFullscreenX, ActualFullscreenY;

@interface C4OpenGLView : NSView
{
@private
	NSOpenGLContext* context;
	CGPoint savedMouse;
}
- (C4WindowController*) controller;
- (void)update;
- (void) enableEvents;
- (void) showCursor;
- (void) hideCursor;
- (BOOL) shouldHideMouseCursor;
- (void) setContextSurfaceBackingSizeToOwnDimensions;
- (void) centerMouse;

+ (CGDirectDisplayID) displayID;
+ (NSOpenGLContext*) mainContext;
+ (void) setSurfaceBackingSizeOf:(NSOpenGLContext*) context width:(int)wdt height:(int)hgt;
+ (NSOpenGLContext*) createContext:(CStdGLCtx*) pMainCtx;

@property(readwrite, strong) NSOpenGLContext* context;
@end

@interface C4EditorOpenGLView: C4OpenGLView
{
}
- (IBAction) grabContents:(id) sender;
- (IBAction) copy:(id) sender;
- (IBAction) delete:(id) sender;
- (IBAction) resetZoom:(id) sender;
- (IBAction) increaseZoom:(id)sender;
- (IBAction) decreaseZoom:(id)sender;
@end

#endif
