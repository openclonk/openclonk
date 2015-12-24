/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2006-2007, Julian Raschke
 * Copyright (c) 2010-2013, The OpenClonk Team and contributors
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
/*   SDLMain.m - main entry point for our Cocoa-ized SDL app
       Initial Version: Darrell Walisser <dwaliss1@purdue.edu>
       Non-NIB-Code & other changes: Max Horn <max@quendi.de>

    Feel free to customize this file to suit your needs
*/

#include <vector>

#include <C4Include.h>
#include <C4KeyboardInput.h>

#import <Cocoa/Cocoa.h>
#ifdef USE_COCOA
#import "C4EditorWindowController.h"
#endif

@interface C4AppDelegate: NSObject
{
	NSMutableArray *gatheredArguments;
	NSString *addonSupplied;
#ifdef USE_COCOA
	C4EditorWindowController *editorWindowController;
	C4WindowController *gameWindowController;
#endif
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
