/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2009-2013, The OpenClonk Team and contributors
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

#import <Cocoa/Cocoa.h>
#include <C4Window.h>

#ifdef USE_COCOA

@class C4WindowController;

extern int ActualFullscreenX, ActualFullscreenY;

@interface C4OpenGLView : NSView
{
@private
	NSOpenGLContext* context;
}
- (C4WindowController*) controller;
- (void)update;
- (void) enableEvents;
- (void) showCursor;
- (void) hideCursor;
- (BOOL) shouldHideMouseCursor;
- (void) setContextSurfaceBackingSizeToOwnDimensions;
- (IBAction) increaseZoom:(id)sender;
- (IBAction) decreaseZoom:(id)sender;

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
@end

#endif
