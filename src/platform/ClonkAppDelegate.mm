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
// Roughly adapted from the original ClonkAppDelegate.m; haxxed to death by teh Gurkendoktor.
// Look at main() to get an idea for what happens here.

#include <C4Include.h>
#include <C4Application.h>
#include <C4Game.h>

#import "ClonkAppDelegate.h"
#ifdef USE_SDL_MAINLOOP
#import "SDL/SDL.h"
#endif

/* The main class of the application, the appl¤ication's delegate */
@implementation ClonkAppDelegate

+ (ClonkAppDelegate*) instance;
{
	return (ClonkAppDelegate*)[[NSApplication sharedApplication] delegate];
}

+ (BOOL) isConsoleAndGameRunning
{
	return Application.isEditor && Game.IsRunning;
}

#ifdef USE_COCOA
@synthesize newViewportForPlayerMenuItem, consoleController, kickPlayerMenuItem, recordMenuItem, netMenu, gameWindowController;
#endif

- (id) init
{
	self = [super init];
	if (self)
	{
		NSArray* args = [[NSProcessInfo processInfo] arguments];
		gatheredArguments = [args copy];
	}
	return self;
}

- (void)awakeFromNib
{
	[[NSAppleEventManager sharedAppleEventManager] setEventHandler:self
												       andSelector:@selector(getUrl:withReplyEvent:)
													 forEventClass:kInternetEventClass andEventID:kAEGetURL];
}

- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename
{
	NSString* pathExtension = [[filename pathExtension] lowercaseString];
    
	NSArray* clonkFileNameExtensions = [NSArray arrayWithObjects:@"c4d", @"c4s", @"c4f", @"c4g", @"c4s", nil];
	if ([clonkFileNameExtensions containsObject:pathExtension])
	{
		// later decide whether to install or run
		addonSupplied = filename;
		if (running)
		{
			// if application is already running install immediately
			[self installAddOn];
		}
	}
	return YES;
}

- (void)getUrl:(NSAppleEventDescriptor *)event withReplyEvent:(NSAppleEventDescriptor *)replyEvent
{
	NSString *url = [[event paramDescriptorForKeyword:keyDirectObject] stringValue];
	[gatheredArguments addObject:url];
}

- (void) quitAndMaybeRestart
{
	// free app stuff
	Application.Clear();
	if (Application.restartAtEnd)
	{
		NSString* filename = [[NSBundle mainBundle] bundlePath];
		NSString* cmd = [@"open " stringByAppendingString: filename];
		system([cmd UTF8String]);
	}
}

- (void) applicationDidFinishLaunching: (NSNotification *) note
{
	if (!([self argsLookLikeItShouldBeInstallation] && [self installAddOn]))
	{
		[[NSFileManager defaultManager] changeCurrentDirectoryPath:[self clonkDirectory]];
		[NSApp activateIgnoringOtherApps:YES];

		[self makeFakeArgs];

#ifdef USE_SDL_MAINLOOP
		running = true;
		SDL_main(newArgc, newArgv);
		running = NO;
		[self quitAndMaybeRestart];
		[NSApp terminate:self];
#endif

#ifdef USE_COCOA
		// Init application
		if (!Application.Init(argc, argv))
		{
			Application.Clear();
			[NSApp terminate:self];
		}
		[[NSRunLoop currentRunLoop] performSelector:@selector(delayedRun:) target:self argument:self order:0 modes:[NSArray arrayWithObject:NSDefaultRunLoopMode]];
#endif
	}
}

- (void) delayedRun:(id)sender
{
	running = YES;
	while (!Application.fQuitMsgReceived)
		Application.ScheduleProcs();
	running = NO;
	[self quitAndMaybeRestart];
	[NSApp terminate:self];
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)application
{
	if (running)
	{
		[self terminate:application];
		return NSTerminateCancel;
	}
	return NSTerminateNow;
}

- (void)terminate:(NSApplication*)sender
{
#ifdef USE_SDL_MAINLOOP
    // Post an SDL_QUIT event
	SDL_Event event;
	event.type = SDL_QUIT;
	SDL_PushEvent(&event);
#endif
#ifdef USE_COCOA
	Application.Quit();
#endif
}

// arguments that should be converted to a c char* array and then passed on to SDL_main
- (NSMutableArray*)gatheredArguments
{
	return gatheredArguments;
}

// return the directory where Clonk.app lives
- (NSString*)clonkDirectory
{
	if (!clonkDirectory) {
		clonkDirectory = [[[NSBundle mainBundle] bundlePath] stringByDeletingLastPathComponent];
	}
	return clonkDirectory;
}

// Look for -psn argument which generally is a clue that the application should open a file (double-clicking, calling /usr/bin/open and such)
- (BOOL) argsLookLikeItShouldBeInstallation
{
	// not having this check leads to deletion of Clonk folder -.-
	if (!addonSupplied)
		return NO;
	for (int i = 0; i < [gatheredArguments count]; i++)
	{
		NSString* arg = [gatheredArguments objectAtIndex:i];
		if ([arg hasPrefix:@"-psn"])
			return YES;
	}
	return NO;
}

// Copies the add-on to the clonk directory
- (BOOL) installAddOn
{
	
	if (!addonSupplied)
		return NO;
	
	// Build destination path.
	NSString* justFileName = [addonSupplied lastPathComponent];
	NSString* destPath = [[self clonkDirectory] stringByAppendingPathComponent:justFileName];
	
	NSString* formatString;
	
	// Already installed?
	if ([destPath isEqualToString:addonSupplied])
	{
		return NO; // run scenarios when they are already in the clonk directory
	}
	
	NSFileManager* fileManager = [NSFileManager defaultManager];
	if ([fileManager fileExistsAtPath:destPath])
		// better to throw it into the trash. everything else seems so dangerously destructive
		[[NSWorkspace sharedWorkspace] performFileOperation:NSWorkspaceRecycleOperation source:[self clonkDirectory] destination:@"" files:[NSArray arrayWithObject:justFileName] tag:0];
	if ([fileManager copyItemAtPath:addonSupplied toPath:destPath error:NULL])
		formatString = NSLocalizedString(@"AddOnInstallationSuccess", nil);
	else
		formatString = NSLocalizedString(@"AddOnInstallationFailure", nil);
	
	NSRunInformationalAlertPanel(NSLocalizedString(@"AddOnInstallationTitle", nil),
								 [NSString stringWithFormat: formatString, [justFileName cStringUsingEncoding:NSASCIIStringEncoding]],
								 @"OK", nil, nil);
								 
	return YES; // only return NO when the scenario should be run rather than installed
}

// convert gatheredArguments to c array
- (void)makeFakeArgs
{
	int argCount = [gatheredArguments count];
	argv = (char**)malloc(sizeof(char*) * argCount);
	for (int i = 0; i < argCount; i++)
	{
		argv[i] = strdup([[gatheredArguments objectAtIndex:i] cStringUsingEncoding:NSUTF8StringEncoding]);
	}
	argc = argCount;
}

@end

#ifdef main
#  undef main
#endif

/* Main entry point to executable - should *not* be SDL_main! */
int main (int argc, const char **argv)
{
	return NSApplicationMain(argc, argv);
}
