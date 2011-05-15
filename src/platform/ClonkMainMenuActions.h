#import <Cocoa/Cocoa.h>
#import "ClonkAppDelegate.h"

@interface ClonkAppDelegate (ClonkMainMenuActions)
- (IBAction) openScenario:(id)sender;
- (IBAction) openScenarioWithPlayers:(id)sender;
- (IBAction) closeScenario:(id)sender;
- (IBAction) saveGameAs:(id)sender;
- (IBAction) saveScenario:(id)sender;
- (IBAction) saveScenarioAs:(id)sender;
- (IBAction) record:(id)sender;
- (IBAction) newViewport:(id)sender;
- (IBAction) openPropTools:(id)sender;
- (IBAction) newViewportForPlayer:(id)sender;
- (IBAction) joinPlayer:(id)sender;
- (IBAction) showAbout:(id)sender;
- (IBAction) toggleFullscreen:(id)sender;
- (IBAction) togglePause:(id)sender;
- (IBAction) setConsoleMode:(id)sender;
- (IBAction) setDrawingTool:(id)sender;
- (IBAction) suggestQuitting:(id)sender;
- (IBAction) simulateKeyPressed:(C4KeyCode)key;
- (IBAction) visitWebsite:(id)sender;
@end
