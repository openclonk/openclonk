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
#import "ClonkMainMenuActions.h"
#ifdef USE_SDL_MAINLOOP
#import "SDL/SDL.h"
#endif

/* The main class of the application, the application's delegate */
@implementation ClonkAppDelegate

+ (ClonkAppDelegate*) instance;
{
	return (ClonkAppDelegate*)[[NSApplication sharedApplication] delegate];
}

+ (BOOL) isEditorAndGameRunning
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
		gatheredArguments = [[NSMutableArray arrayWithCapacity:[args count]+2] retain];
		[gatheredArguments addObjectsFromArray:args];
	}
	return self;
}

- (void)awakeFromNib
{
	NSAppleEventManager* appleEvents = [NSAppleEventManager sharedAppleEventManager];
	[appleEvents setEventHandler:self
					 andSelector:@selector(getUrl:withReplyEvent:)
				   forEventClass:kInternetEventClass andEventID:kAEGetURL];
	[appleEvents setEventHandler:self
					 andSelector:@selector(handleQuitEvent:withReplyEvent:)
				   forEventClass:kCoreEventClass andEventID:kAEQuitApplication];
}

//handler for the quit apple event
- (void)handleQuitEvent:(NSAppleEventDescriptor*)event withReplyEvent:(NSAppleEventDescriptor*)replyEvent
{
	NSLog(@"Quit message via Dock or Tab-Switcher");
	[self suggestQuitting:self];
}

- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename
{
	NSString* pathExtension = [[filename pathExtension] lowercaseString];
    
	NSArray* clonkFileNameExtensions = [NSArray arrayWithObjects:@"ocd", @"ocs", @"ocf", @"ocg", nil];
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
	Application.Quit();
	if (Application.restartAtEnd)
	{
		NSString* filename = [[NSBundle mainBundle] bundlePath];
		NSString* cmd = [@"open " stringByAppendingString: filename];
		system([cmd UTF8String]);
	}
}

- (void) applicationDidFinishLaunching: (NSNotification *) note
{
	[[NSFileManager defaultManager] changeCurrentDirectoryPath:[self clonkDirectory]];
	if (!([self argsLookLikeItShouldBeInstallation] && [self installAddOn]))
	{
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
	else
	{
		[NSApp terminate:self];
	}
}

#ifdef USE_COCOA
- (void) delayedRun:(id)sender
{
	running = YES;
	while (!Application.fQuitMsgReceived)
		Application.ScheduleProcs();
	[NSApp replyToApplicationShouldTerminate:YES];
	running = NO;
	[self quitAndMaybeRestart];
	[NSApp terminate:self];
}
#endif

- (void) simulateKeyPressed:(C4KeyCode)key
{
	Game.DoKeyboardInput(
						 key,
						 KEYEV_Down,
						 false, false, false,
						 false, NULL
						 );
	Game.DoKeyboardInput(
						 key,
						 KEYEV_Up,
						 false, false, false,
						 false, NULL
						 );
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)application
{
	[self suggestQuitting:self];
	return running ? NSTerminateCancel : NSTerminateNow;
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
	Application.fQuitMsgReceived = true;
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
	for (unsigned int i = 0; i < [gatheredArguments count]; i++)
	{
		NSString* arg = [gatheredArguments objectAtIndex:i];
		if ([arg hasPrefix:@"-psn"])
			return YES;
	}
	return NO;
}

- (void) infoWithFormat:(NSString*) formatString andArgument:(const char*) arg
{
	NSRunInformationalAlertPanel([NSString stringWithCString:LoadResStr("IDS_ADDON_INSTALLTITLE") encoding:NSUTF8StringEncoding],
								 [NSString stringWithFormat: formatString, arg],
								 @"OK", nil, nil);
}

- (void) minimalConfigurationInitialization
{
	Config.Init();
	Config.Load();
	Reloc.Init();
	Languages.Init();
	Languages.LoadLanguage(Config.General.LanguageEx);
}

// Copies the add-on to the clonk directory
- (BOOL) installAddOn
{
	if (!addonSupplied)
		return NO;
	
	// load configuration + localization so LoadResStr can be used
	[self minimalConfigurationInitialization];
	
	// Build destination path.
	NSString* justFileName = [addonSupplied lastPathComponent];
	NSString* destPath = [self clonkDirectory];
	NSString* formatString;
	
	// Already installed?
	for (C4Reloc::iterator it = Reloc.begin(); it != Reloc.end(); it++)
	{
		if ([addonSupplied hasPrefix:[NSString stringWithCString:(*it).strBuf.getData() encoding:NSUTF8StringEncoding]])
		{
			[gatheredArguments addObject:addonSupplied];
			return NO; // run scenarios when they are already containd in one of the Reloc directories
		}
		else if (it->pathType == C4Reloc::PATH_PreferredInstallationLocation)
			destPath = [NSString stringWithCString:it->strBuf.getData() encoding:NSUTF8StringEncoding];
	}
	destPath = [destPath stringByAppendingPathComponent:justFileName];
	
	NSFileManager* fileManager = [NSFileManager defaultManager];
	if ([fileManager fileExistsAtPath:destPath])
		// better to throw it into the trash. everything else seems so dangerously destructive
		[[NSWorkspace sharedWorkspace] performFileOperation:NSWorkspaceRecycleOperation source:[self clonkDirectory] destination:@"" files:[NSArray arrayWithObject:justFileName] tag:0];
	if ([fileManager copyItemAtPath:addonSupplied toPath:destPath error:NULL])
	{
		formatString = [NSString stringWithCString:LoadResStr("IDS_ADDON_INSTALLSUCCESS") encoding:NSUTF8StringEncoding];
	}
	else
	{
		formatString = [NSString stringWithCString:LoadResStr("IDS_ADDON_INSTALLFAILURE") encoding:NSUTF8StringEncoding];
	}
	[self infoWithFormat:formatString andArgument:[justFileName cStringUsingEncoding:NSUTF8StringEncoding]];
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

static void _ExceptionHandler(NSException* exception)
{
	NSAlert* alert = [NSAlert alertWithMessageText:[exception description] defaultButton:nil alternateButton:nil otherButton:nil informativeTextWithFormat:@""];
	[alert runModal];
}

/* Main entry point to executable - should *not* be SDL_main! */
int main (int argc, const char **argv)
{
	NSSetUncaughtExceptionHandler(&_ExceptionHandler);
	return NSApplicationMain(argc, argv);
}
