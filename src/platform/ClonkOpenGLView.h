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

#import <Cocoa/Cocoa.h>
#include <StdWindow.h>

#ifdef USE_COCOA

@class ClonkWindowController;

@interface ClonkOpenGLView : NSView {
@private
	NSOpenGLContext* context;
}
- (ClonkWindowController*) controller;
- (void)update;
- (void) enableEvents;
- (void) showCursor;
- (void) hideCursor;
- (BOOL) shouldHideMouseCursor;

@property(readwrite, retain) NSOpenGLContext* context;
@end

#endif