// Roughly adapted from the original SDLMain.m; haxxed to death by teh Gurkendoktor.
// Look at main() to get an idea for what happens here.

#import "SDLMain.h"
#import "SDL/SDL.h"

/* The main class of the application, the appl¤ication's delegate */
@implementation SDLMain

- (id) init {
	if (self = [super init]) {
		// first argument is path to executable (for authenticity)
		gatheredArguments = [NSMutableArray arrayWithCapacity:10];
		[gatheredArguments addObject:[[NSBundle mainBundle] executablePath]];
	}
	return self;
}

- (void) release {
	if (terminateRequested)
		[NSApp replyToApplicationShouldTerminate:YES];
	[super release];
}

- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename
{
	NSString* pathExtension = [[filename pathExtension] lowercaseString];
    
	NSArray* clonkFileNameExtensions = [NSArray arrayWithObjects:@"c4d", @"c4s", @"c4f", @"c4g", @"c4s", nil];
	if ([clonkFileNameExtensions containsObject:pathExtension])
	 {
		 // later decide whether to install or run
		 addonSupplied = filename;
		 if (hasFinished) {
			 // if application is already running install immediately
			 [self installAddOn];
			 return YES;
		 }
		 // still add to gatheredArguments
		 // return YES;
	 }
	
	// Key/Update files or simply arguments: Just pass to engine, will install
	[gatheredArguments addObject:filename];
	fflush(0);
	return YES;
}

- (void)getUrl:(NSAppleEventDescriptor *)event withReplyEvent:(NSAppleEventDescriptor *)replyEvent
{
	NSString *url = [[event paramDescriptorForKeyword:keyDirectObject] stringValue];
	[gatheredArguments addObject:url];
}

/* Called when the internal event loop has just started running */
- (void) applicationDidFinishLaunching: (NSNotification *) note
{
/*	NSDictionary* args = [[NSUserDefaults standardUserDefaults] volatileDomainForName:NSArgumentDomain];
	for (NSString* key in args) {
		[gatheredArguments addObject:[NSString stringWithFormat:@"/%@:%@", key, [args valueForKey:key]]];
	}*/
	hasFinished = YES;
}

- (NSApplicationTerminateReply) applicationShouldTerminate:(NSApplication*)application {
	[self terminate:application];
	return NSTerminateCancel; // cancels logoff but it's the only way that does not interrupt the lifecycle of the application
}

- (void)terminate:(NSApplication*)sender
{
    // Post an SDL_QUIT event
	SDL_Event event;
	event.type = SDL_QUIT;
	SDL_PushEvent(&event);
}

- (BOOL)hasFinished {
	return hasFinished;
}

// arguments that should be converted to a c char* array and then passed on to SDL_main
- (NSMutableArray*)gatheredArguments {
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
- (BOOL) argsLookLikeItShouldBeInstallation:(char**)argv argc:(int)argc {
	// not having this check leads to deletion of Clonk foler -.-
	if (!addonSupplied)
		return NO;
	for (int i = 0; i < argc; i++) {
		if ([[NSString stringWithCString:argv[i]] hasPrefix:@"-psn"])
			return YES;
	}
	return NO;
}

// Copies the add-on to the clonk directory
- (BOOL) installAddOn {
	
	if (!addonSupplied)
		return NO;
	
	// Build destination path.
	NSString* justFileName = [addonSupplied lastPathComponent];
	NSString* destPath = [[self clonkDirectory] stringByAppendingPathComponent:justFileName];
	
	NSString* formatString;
	
	// Already installed?
	if ([destPath isEqualToString:addonSupplied])
	{
		[gatheredArguments addObject:@"/fullscreen"];
		return NO; // run scenarios when they are already in the clonk directory
	}
	
	NSFileManager* fileManager = [NSFileManager defaultManager];
	if ([fileManager fileExistsAtPath:destPath])
		// better to throw it into the trash. everything else seems so dangerously destructive
		[[NSWorkspace sharedWorkspace] performFileOperation:NSWorkspaceRecycleOperation source:[self clonkDirectory] destination:@"" files:[NSArray arrayWithObject:justFileName] tag:0];
	if ([fileManager copyPath:addonSupplied toPath:destPath handler:nil])
		formatString = NSLocalizedString(@"AddOnInstallationSuccess", nil);
	else
		formatString = NSLocalizedString(@"AddOnInstallationFailure", nil);
	
	NSRunInformationalAlertPanel(NSLocalizedString(@"AddOnInstallationTitle", nil),
								 [NSString stringWithFormat: formatString, [justFileName cStringUsingEncoding:NSASCIIStringEncoding]],
								 @"OK", nil, nil);
								 
	return YES; // only return NO when the scenario should be run rather than installed
}

// convert gatheredArguments to c array
- (void)makeFakeArgs:(char***)argv argc:(int*)argc {
	int argCount = [gatheredArguments count];
	char** args = (char**)malloc(sizeof(char*) * argCount);
	for (int i = 0; i < argCount; i++) {
		args[i] = strdup([[gatheredArguments objectAtIndex:i] cStringUsingEncoding:NSASCIIStringEncoding]);
	}
	*argv = args;
	*argc = argCount;
}

@end

#ifdef main
#  undef main
#endif

/* Main entry point to executable - should *not* be SDL_main! */
int main (int argc, char **argv)
{
    // Catches whatever would be leaked otherwise.
    // Gets rid of warnings from hax0red SDL framework.
    [[NSAutoreleasePool alloc] init];
	
	SDLMain* sdlMain = [[SDLMain alloc] init];
	[[NSFileManager defaultManager] changeCurrentDirectoryPath:[sdlMain clonkDirectory]];
	
    // Set up application & delegate.
    [NSApplication sharedApplication];
    [NSApp setDelegate:sdlMain];
	[[NSAppleEventManager sharedAppleEventManager] setEventHandler:sdlMain
													   andSelector:@selector(getUrl:withReplyEvent:) forEventClass:kInternetEventClass andEventID:kAEGetURL];
    [NSBundle loadNibNamed:@"SDLMain" owner:NSApp];
    [NSApp finishLaunching];
	
	[NSApp activateIgnoringOtherApps:YES];
    
    // Now there's openFile calls and all that waiting for us in the event queue.
    // Fetch events until applicationHasFinishedLaunching was called.
    while (![sdlMain hasFinished]) {
        NSEvent *event = [NSApp nextEventMatchingMask:NSAnyEventMask
											untilDate:nil
											   inMode:NSDefaultRunLoopMode
											  dequeue:YES ];
        if ( event == nil ) {
            break;
        }
        [NSApp sendEvent:event];
    }
	
	if ([sdlMain argsLookLikeItShouldBeInstallation:argv argc:argc]) {
		if ([sdlMain installAddOn])
			return 0;
	}
		
    // Hand off to Clonk code
	char** newArgv;
	int newArgc;
	[sdlMain makeFakeArgs:&newArgv argc:&newArgc];
	int status = SDL_main(newArgc, newArgv);
	
	// I don't think this is even necessary since the OS will just wipe it all out when the process ends
	for (int i = newArgc-1; i >= 0; i--) {
		free (newArgv[i]);
	}
	free(newArgv);
	
	[sdlMain release];
	
    return status;
}
