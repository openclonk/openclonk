/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2009-2016, The OpenClonk Team and contributors
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

// Roughly adapted from the original C4AppDelegate.m; haxxed to death by teh Gurkendoktor.
// Look at main() to get an idea for what happens here.

#include "C4Include.h"
#include "game/C4Application.h"
#include "game/C4Game.h"

#import "platform/C4AppDelegate.h"
#import "platform/C4AppDelegate+MainMenuActions.h"
#ifdef USE_SDL_MAINLOOP
#import "SDL/SDL.h"
#endif

/* The main class of the application, the application's delegate */
@implementation C4AppDelegate

+ (C4AppDelegate*) instance
{
	return (C4AppDelegate*)[[NSApplication sharedApplication] delegate];
}

+ (BOOL) isEditorAndGameRunning
{
	return Application.isEditor && Game.IsRunning;
}

#ifdef USE_COCOA
@synthesize addViewportForPlayerMenuItem, editorWindowController, kickPlayerMenuItem, recordMenuItem, netMenu, gameWindowController, toggleFullScreen;
#endif

- (id) init
{
	self = [super init];
	if (self)
	{
		NSArray* processArguments = [[NSProcessInfo processInfo] arguments];
		gatheredArguments = [NSMutableArray arrayWithCapacity:[processArguments count]+2];
		[gatheredArguments addObjectsFromArray:processArguments];
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
    
	NSArray* clonkFileNameExtensions = @[@"ocd", @"ocs", @"ocf", @"ocg"];
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

// return the directory where Clonk.app lives
- (NSString*)clonkDirectory { return [NSBundle.mainBundle.bundlePath stringByDeletingLastPathComponent]; }

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
		if (!Application.Init(args.size(), &args[0]))
		{
			Application.Clear();
			[NSApp terminate:self];
		}
		[[NSRunLoop currentRunLoop] performSelector:@selector(delayedRun:) target:self argument:self order:0 modes:[NSArray arrayWithObject:NSDefaultRunLoopMode]];
		if (!lionAndBeyond())
			[toggleFullScreen setTarget:self]; // revert to old pre-Lion fullscreen
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
	Application.StartOnCurrentThread();
	running = YES;
	//while (!Application.fQuitMsgReceived)
	//	Application.ScheduleProcs();
	//[NSApp replyToApplicationShouldTerminate:YES];
	//running = NO;
	//[self quitAndMaybeRestart];
	//[NSApp terminate:self];
}
#endif

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)application
{
	if (!Application.fQuitMsgReceived)
		[self suggestQuitting:self];
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
	Application.fQuitMsgReceived = true;
#endif
}

// arguments that should be converted to a c char* array and then passed on to SDL_main
- (NSMutableArray*)gatheredArguments
{
	return gatheredArguments;
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
	NSArray* nonCocoaArgs = [gatheredArguments filteredArrayUsingPredicate:[NSPredicate predicateWithBlock:^BOOL(NSString* arg, NSDictionary *bindings)
		{
			return
				!(
					[arg hasPrefix:@"-NS"] ||
					[arg isEqualToString:@"YES"]
				);
		}
	]];
	int argCount = [nonCocoaArgs count];
	args.resize(argCount);
	for (int i = 0; i < argCount; i++)
	{
		args[i] = strdup([[nonCocoaArgs objectAtIndex:i] cStringUsingEncoding:NSUTF8StringEncoding]);
	}
}

- (void) applicationDidBecomeActive:(NSNotification*)notification
{
#ifdef USE_COCOA
	if (gameWindowController)
		[gameWindowController.window makeKeyAndOrderFront:self];
#endif
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
