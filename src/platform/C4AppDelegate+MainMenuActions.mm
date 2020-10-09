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

#define GL_SILENCE_DEPRECATION
#include <epoxy/gl.h>

#include "C4Include.h"
#include "editor/C4Console.h"
#include "game/C4Viewport.h"
#include "game/C4GraphicsSystem.h"

#import "platform/C4AppDelegate+MainMenuActions.h"
#import "graphics/C4DrawGLMac.h"
#import "editor/C4EditorWindowController.h"

@implementation C4AppDelegate (MainMenuActions)

- (IBAction) openScenario:(id)sender
{
	Console.FileOpen();
}

- (IBAction) openScenarioWithPlayers:(id)sender
{
	Console.FileOpenWPlrs();
}

- (IBAction) closeScenario:(id)sender
{
	Console.FileClose();
	[editorWindowController.window setRepresentedFilename:@""];
}


- (IBAction) saveGameAs:(id)sender
{
	Console.FileSaveAs(true);
}

- (IBAction) saveScenario:(id)sender
{
	Console.FileSave();
}

- (IBAction) saveScenarioAs:(id)sender
{
	Console.FileSaveAs(false);
}

- (IBAction) record:(id)sender
{
	Console.FileRecord();
}

- (IBAction) newViewport:(id)sender
{
	Console.ViewportNew();
}

- (IBAction) newViewportForPlayer:(id)sender
{
	::Viewports.CreateViewport([sender tag]);
}

- (IBAction) joinPlayer:(id)sender
{
	Console.PlayerJoin();
}

- (IBAction) openPropTools:(id)sender
{
	Console.EditCursor.OpenPropTools();
}

- (IBAction) showAbout:(id)sender
{
	Console.HelpAbout();
}

- (IBAction) toggleFullScreen:(id)sender
{
	if (Application.isEditor)
	{
		NSBeep();
		return;
	}
	[gameWindowController setFullscreen:Config.Graphics.Windowed];
	Config.Graphics.Windowed = !Config.Graphics.Windowed;
	
}

- (IBAction) togglePause:(id)sender
{
	[self simulateKeyPressed:K_PAUSE];
}

- (IBAction) setConsoleMode:(id)sender
{
	[editorWindowController selectMode:sender];
	[editorWindowController.modeSelector selectSegmentWithTag:[sender tag]];
}

- (IBAction) setDrawingTool:(id)sender
{
	[editorWindowController selectTool:sender];
	[editorWindowController.toolSelector selectSegmentWithTag:[sender tag]];
}

- (IBAction) suggestQuitting:(id)sender
{
	if (!Application.isEditor && Game.IsRunning)
	{
		NSLog(@"Game running, only simulating Esc key");
		[self simulateKeyPressed:K_ESCAPE];
	}
	else
	{
		if (Application.isEditor)
			Console.FileClose();
		Application.fQuitMsgReceived = true;
		Application.ScheduleProcs();
		Application.Clear();
		Application.Quit();
	}
}

- (BOOL)validateUserInterfaceItem:(id<NSValidatedUserInterfaceItem>)item
{
	// enabled when running in fullscreen
	if ([item action] == @selector(toggleFullScreen:))
		return !Application.isEditor;
	
	SEL s;
	int i;
	
	SEL gameRunningSelectors[] =
	{
		@selector(togglePause:),
		@selector(makeScreenshot:),
		@selector(makeScreenshotOfWholeMap:),
		nil
	};
	for (i = 0; (s = gameRunningSelectors[i]) != nil; i++)
		if ([item action] == s)
			return Game.IsRunning;

	// enabled when game running and console mode
	SEL gameRunningInConsoleModeSelectors[] =
	{
		@selector(saveScenario:),
		@selector(saveScenarioAs:),
		@selector(saveGameAs:),
		@selector(record:),
		@selector(closeScenario:),
		@selector(newViewport:),
		@selector(newViewportForPlayer:),
		@selector(joinPlayer:),
		@selector(openPropTools:),
		@selector(setConsoleMode:),
		@selector(setDrawingTool:),
		nil
	};
	for (i = 0; (s = gameRunningInConsoleModeSelectors[i]) != nil; i++)
	{
		if (s == [item action])
			return Application.isEditor && Game.IsRunning;
	}
	
	// always enabled
	return YES;
}

- (IBAction) visitWebsite:(id)sender
{
	OpenURL("http://wiki.openclonk.org");
}

- (void) simulateKeyPressed:(C4KeyCode)key
{
	Game.DoKeyboardInput(key, KEYEV_Down, false, false, false, false, NULL);
	Game.DoKeyboardInput(key, KEYEV_Up,   false, false, false, false, NULL);
}

- (IBAction) makeScreenshot:(id)sender
{
	::GraphicsSystem.SaveScreenshotKey(false);
}

- (IBAction) makeScreenshotOfWholeMap:(id)sender
{
	::GraphicsSystem.SaveScreenshotKey(true);
}

@end
