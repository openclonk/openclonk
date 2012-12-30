/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2006-2007  Julian Raschke
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
/*   SDLMain.m - main entry point for our Cocoa-ized SDL app
       Initial Version: Darrell Walisser <dwaliss1@purdue.edu>
       Non-NIB-Code & other changes: Max Horn <max@quendi.de>

    Feel free to customize this file to suit your needs
*/

#import <Cocoa/Cocoa.h>
#ifdef USE_COCOA
#import "C4EditorWindowController.h"
#endif

@interface C4AppDelegate: NSObject
{
	NSMutableArray *gatheredArguments;
	NSString *addonSupplied;
	C4EditorWindowController *editorWindowController;
	C4WindowController *gameWindowController;
	BOOL running;
	std::vector<char*> args;
}
- (BOOL) argsLookLikeItShouldBeInstallation;
- (void)makeFakeArgs;
- (BOOL)installAddOn;
- (void)terminate:(NSApplication*)sender;

+ (C4AppDelegate*) instance;
+ (BOOL) isEditorAndGameRunning;

#ifdef USE_COCOA
@property(weak, readonly) NSMenuItem* addViewportForPlayerMenuItem;
@property(weak, readonly) NSMenuItem* kickPlayerMenuItem;
@property(readwrite, strong) C4EditorWindowController* editorWindowController;
@property(readwrite, strong) C4WindowController* gameWindowController;
@property(weak, readonly) NSMenuItem* recordMenuItem;
@property(weak, readonly) NSMenuItem* netMenu;
@property(weak) NSMenuItem* toggleFullScreen;
#endif
@end
