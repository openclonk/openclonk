/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2009-2015, The OpenClonk Team and contributors
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

#import <Cocoa/Cocoa.h>
#import "C4AppDelegate.h"

@interface C4AppDelegate (MainMenuActions)
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
- (IBAction) toggleFullScreen:(id)sender;
- (IBAction) togglePause:(id)sender;
- (IBAction) setConsoleMode:(id)sender;
- (IBAction) setDrawingTool:(id)sender;
- (IBAction) suggestQuitting:(id)sender;
- (IBAction) simulateKeyPressed:(C4KeyCode)key;
- (IBAction) visitWebsite:(id)sender;
- (IBAction) makeScreenshot:(id)sender;
- (IBAction) makeScreenshotOfWholeMap:(id)sender;
@end
