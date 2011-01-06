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
/*   SDLMain.m - main entry point for our Cocoa-ized SDL app
       Initial Version: Darrell Walisser <dwaliss1@purdue.edu>
       Non-NIB-Code & other changes: Max Horn <max@quendi.de>

    Feel free to customize this file to suit your needs
*/

#import <Cocoa/Cocoa.h>
#ifdef USE_COCOA
#import "ConsoleWindowController.h"
#endif

@interface ClonkAppDelegate: NSObject
{
	NSMutableArray *gatheredArguments;
	NSString *clonkDirectory;
	NSString *addonSupplied;
	ConsoleWindowController *consoleController;
	// declared here since ConsoleWindow.xib can't refer to objects in MainMenu.xib -.-
	IBOutlet NSMenuItem *newViewportForPlayerMenuItem;
	IBOutlet NSMenuItem *kickPlayerMenuItem;
	IBOutlet NSMenuItem *recordMenuItem;
	IBOutlet NSMenuItem *netMenu;
	
	BOOL running;
	char** argv;
	int argc;
}
- (NSString*) clonkDirectory;
- (BOOL) argsLookLikeItShouldBeInstallation;
- (void)makeFakeArgs:(char***)argv argc:(int*)argc;
- (BOOL)installAddOn;
- (void)terminate:(NSApplication*)sender;

+ (ClonkAppDelegate*) instance;
+ (BOOL) isConsoleAndGameRunning;

#ifdef USE_COCOA
@property(readonly) NSMenuItem* newViewportForPlayerMenuItem;
@property(readonly) NSMenuItem* kickPlayerMenuItem;
@property(readwrite, retain) ConsoleWindowController* consoleController;
@property(readonly) NSMenuItem* recordMenuItem;
@property(readonly) NSMenuItem* netMenu;
#endif
@end
