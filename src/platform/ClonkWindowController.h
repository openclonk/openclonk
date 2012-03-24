/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2010  Martin Plicht
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

class C4Window;
class C4ViewportWindow;
class C4Viewport;

@class ClonkOpenGLView;
@class NSTextView;
@class ConsoleWindowController;

bool lionAndBeyond();

@interface ClonkWindowController : NSWindowController<NSWindowDelegate> {
	IBOutlet ClonkOpenGLView* openGLView;
	IBOutlet NSScrollView* scrollView;
	C4Window* stdWindow;
	NSWindow* fullscreenWindow;
	NSSize preferredContentSize;
}
- (C4Viewport*) viewport;
- (void) setFullscreen:(BOOL)fullscreen;
- (BOOL) isFullScreen;
- (BOOL) isFullScreenConsideringLionFullScreen;
- (void) setContentSize:(NSSize)size;

- (IBAction) scroll:(id)sender;

@property (readwrite) C4Window* stdWindow;
@property (readonly) ClonkOpenGLView* openGLView;
@property (readonly) NSScrollView* scrollView;
@property (readonly) BOOL isLiveResizing;
@end
