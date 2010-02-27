/*   SDLMain.m - main entry point for our Cocoa-ized SDL app
       Initial Version: Darrell Walisser <dwaliss1@purdue.edu>
       Non-NIB-Code & other changes: Max Horn <max@quendi.de>

    Feel free to customize this file to suit your needs
*/

#import <Cocoa/Cocoa.h>

enum ClonkAppDelegateGameState {
	GS_NotYetStarted,
	GS_Running,
	GS_Finished
};

@interface ClonkAppDelegate: NSObject
{
	NSMutableArray* gatheredArguments;
	NSString* clonkDirectory;
	NSString* addonSupplied;
	ClonkAppDelegateGameState gameState;
}
- (NSString*) clonkDirectory;
- (BOOL) argsLookLikeItShouldBeInstallation;
- (void)makeFakeArgs:(char***)argv argc:(int*)argc;
- (BOOL)installAddOn;
- (void)terminate:(NSApplication*)sender;
@end
