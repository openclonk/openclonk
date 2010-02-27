// Roughly adapted from the original ClonkAppDelegate.m; haxxed to death by teh Gurkendoktor.
// Look at main() to get an idea for what happens here.

#import "ClonkAppDelegate.h"
#import "SDL/SDL.h"

/* The main class of the application, the appl¤ication's delegate */
@implementation ClonkAppDelegate

- (id) init
{
	if (self = [super init]) {
		NSArray* args = [[NSProcessInfo processInfo] arguments];
		gatheredArguments = [args copy];
		gameState = GS_NotYetStarted;
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
		if (gameState == GS_Running)
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

- (void)gameLoop
{
	[[NSFileManager defaultManager] changeCurrentDirectoryPath:[self clonkDirectory]];

	[NSApp activateIgnoringOtherApps:YES];

    // Hand off to Clonk code
	char** newArgv;
	int newArgc;
	[self makeFakeArgs:&newArgv argc:&newArgc];
	int status = SDL_main(newArgc, newArgv);
	for (int i = newArgc-1; i >= 0; i--) {free (newArgv[i]);}
	free(newArgv);
}

/* Called when the internal event loop has just started running */
- (void) applicationDidFinishLaunching: (NSNotification *) note
{
	if (!([self argsLookLikeItShouldBeInstallation] && [self installAddOn]))
	{
		gameState = GS_Running;
		[self gameLoop];
		gameState = GS_Finished;
	}
	[NSApp terminate:self];
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)application
{
	if (gameState == GS_Running)
	{
		[self terminate:application];
		return NSTerminateCancel; // cancels logoff but it's the only way that does not interrupt the lifecycle of the application
	}
	return NSTerminateNow;
}

- (void)terminate:(NSApplication*)sender
{
    // Post an SDL_QUIT event
	SDL_Event event;
	event.type = SDL_QUIT;
	SDL_PushEvent(&event);
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
- (void)makeFakeArgs:(char***)argv argc:(int*)argc
{
	int argCount = [gatheredArguments count];
	char** args = (char**)malloc(sizeof(char*) * argCount);
	for (int i = 0; i < argCount; i++)
	{
		args[i] = strdup([[gatheredArguments objectAtIndex:i] cStringUsingEncoding:NSUTF8StringEncoding]);
	}
	*argv = args;
	*argc = argCount;
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
