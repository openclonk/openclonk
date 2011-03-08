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

#include <C4Include.h>
#include <C4Console.h>
#include <C4Player.h>
#include <C4PlayerList.h>
#include <C4Game.h>

#import <Cocoa/Cocoa.h>
#import <ConsoleWindowController.h>
#import <ClonkOpenGLView.h>
#import <ClonkAppDelegate.h>

#ifdef USE_COCOA

@implementation ConsoleWindowController

@synthesize
	frameLabel, scriptLabel, timeLabel, outputTextView, objectPropertiesText,
	materialsPopup, texturesPopup, outputScrollView, previewView,
	objectsPanel, toolsPanel, toolSelector, modeSelector, objectCombo, consoleCombo;

- (void) awakeFromNib
{
	[super awakeFromNib];
	ClonkAppDelegate.instance.consoleController = self;
	NSWindow* window = self.window;
	[window makeKeyAndOrderFront:self];
	[window makeMainWindow];
	[toolsPanel setBecomesKeyOnlyIfNeeded:YES];
	[objectsPanel setBecomesKeyOnlyIfNeeded:YES];
}

- (void) windowWillClose:(NSNotification*)notification
{
	if (notification.object == objectsPanel)
	{
		Console.PropertyDlgClose();
	}
	else if (notification.object == toolsPanel)
	{
		Console.ToolsDlg.Clear();
	}
}

int indexFromSender(id sender)
{
	if ([sender respondsToSelector:@selector(selectedSegment)])
		return [sender selectedSegment];
	else if ([sender respondsToSelector:@selector(tag)])
		return [sender tag];
	else
		return -1;
}

- (IBAction) selectMode:(id)sender
{
	Console.EditCursor.SetMode(indexFromSender(sender));
	for (NSWindow* w in [[NSApplication sharedApplication] windows])
	{
		if ([[w windowController] isKindOfClass:[ClonkWindowController class]])
		{
			[w invalidateCursorRectsForView:[[w windowController] openGLView]];
		}
	}
}

- (IBAction) play:(id)sender
{
	Console.DoPlay();
}

- (IBAction) halt:(id)sender
{
	Console.DoHalt();
}

- (IBAction) selectMaterial:(id)sender
{
	Console.ToolsDlg.SetMaterial([[(NSPopUpButton*)sender titleOfSelectedItem] cStringUsingEncoding:NSUTF8StringEncoding]);
}

- (IBAction) selectTexture:(id)sender
{
	Console.ToolsDlg.SetTexture([[(NSPopUpButton*)sender titleOfSelectedItem] cStringUsingEncoding:NSUTF8StringEncoding]);
}

- (IBAction) selectTool:(id)sender
{
	Console.ToolsDlg.SetTool(indexFromSender(sender), NO);
}

- (IBAction) selectIFT:(id)sender
{
	Console.ToolsDlg.SetIFT([sender selectedSegment] == 1);
}

- (IBAction) selectLandscapeMode:(id)sender
{
	// add one since 0 is "undefined"
	Console.ToolsDlg.SetLandscapeMode([sender selectedSegment]+1, NO);
}

- (IBAction) setGrade:(id)sender
{
	Console.ToolsDlg.SetGrade([sender intValue]);
}

// manually catch case of game not running since button validation would require key value binding -.-

- (IBAction) consoleIn:(id)sender
{
	if (![ClonkAppDelegate isEditorAndGameRunning])
		return;
	Console.In([[consoleCombo stringValue] cStringUsingEncoding:NSUTF8StringEncoding]);
}

- (IBAction) objectIn:(id)sender
{
	if (![ClonkAppDelegate isEditorAndGameRunning])
		return;
	Console.EditCursor.In([[objectCombo stringValue] cStringUsingEncoding:NSUTF8StringEncoding]);
}

- (IBAction) kickPlayer:(id)sender
{
	if (!::Control.isCtrlHost())
		return;
	::Game.Clients.CtrlRemove(::Game.Clients.getClientByID([sender tag]), LoadResStr("IDS_MSG_KICKBYMENU"));
}

- (BOOL)validateUserInterfaceItem:(id<NSValidatedUserInterfaceItem>)item
{
	// enabled when game running and in editor mode
	SEL gameRunningInConsoleModeSelectors[] =
	{
		@selector(play:),
		@selector(halt:),
		@selector(selectMode:),
		nil
	};
	int i = 0;
	SEL s;
	while (s = gameRunningInConsoleModeSelectors[i++])
	{
		if (s == [item action])
			return [ClonkAppDelegate isEditorAndGameRunning];
	}
	
	// always enabled
	return YES;
}

@end

#endif