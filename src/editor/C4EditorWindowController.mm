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

#include "C4Include.h"
#include "editor/C4Console.h"
#include "player/C4Player.h"
#include "player/C4PlayerList.h"

#include <epoxy/gl.h>

#import <Cocoa/Cocoa.h>
#import "editor/C4EditorWindowController.h"
#import "graphics/C4DrawGLMac.h"
#import "platform/C4AppDelegate.h"

#ifdef USE_COCOA

@interface InputFunctions : NSObject<NSComboBoxDataSource> {
	NSArray* items;
}
- (void) setFunctions:(std::list<const char*>) functions;
@end
@implementation InputFunctions
- (void) setFunctions:(std::list<const char *>)functions {
	NSMutableArray* _items = [NSMutableArray arrayWithCapacity:functions.size()];
	for (auto f : functions)
		if (f != NULL)
			[_items addObject:[NSString stringWithUTF8String:f]];;
	[_items sortUsingSelector:@selector(compare:)];
	items = _items;
}
- (NSInteger)numberOfItemsInComboBox:(NSComboBox *)aComboBox {
	return [items count];
}
- (id)comboBox:(NSComboBox *)aComboBox objectValueForItemAtIndex:(NSInteger)index {
	return [items objectAtIndex:index];
}
- (NSUInteger)comboBox:(NSComboBox *)aComboBox indexOfItemWithStringValue:(NSString *)string {
	return [items indexOfObject:string];
}
- (NSString *)comboBox:(NSComboBox *)aComboBox completedString:(NSString *)string {
	int x;
	for (x = [string length]-1;
		x >= 0 && [[NSCharacterSet letterCharacterSet] characterIsMember:[string characterAtIndex:x]]; x--);
	x++;
	auto pfx = [string substringWithRange:NSMakeRange(0, x)];
	auto sub = [string substringFromIndex:x];
	auto ndx = [items indexOfObjectPassingTest:^(NSString* item, NSUInteger x, BOOL* stop) { return [item hasPrefix:sub]; }];
	return ndx != NSNotFound ? [pfx stringByAppendingString:[items objectAtIndex:ndx]] : nil;
}
@end

@implementation C4EditorWindowController
{
	InputFunctions* inputFunctions;
}

@synthesize
	frameLabel, timeLabel, outputTextView, objectPropertiesText,
	materialsPopup, texturesPopup, outputScrollView, previewView,
	objectsPanel, toolsPanel, toolSelector, modeSelector, objectCombo, consoleCombo;

- (void) awakeFromNib
{
	[super awakeFromNib];
	C4AppDelegate.instance.editorWindowController = self;
	NSWindow* window = self.window;
	[window makeKeyAndOrderFront:self];
	[window makeMainWindow];
	[toolsPanel setBecomesKeyOnlyIfNeeded:YES];
	[objectsPanel setBecomesKeyOnlyIfNeeded:YES];
	inputFunctions = [InputFunctions new];
	[consoleCombo setUsesDataSource:YES];
	[consoleCombo setDataSource:inputFunctions];
	[consoleCombo setCompletes:YES];
}

- (void) setInputFunctions:(std::list<const char*>)functions {
	[inputFunctions setFunctions:functions];
	[consoleCombo reloadData];
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
		if ([[w windowController] isKindOfClass:[C4WindowController class]])
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
	Console.ToolsDlg.SetLandscapeMode((LandscapeMode)([sender selectedSegment]+1), NO, NO);
}

- (IBAction) setGrade:(id)sender
{
	Console.ToolsDlg.SetGrade([sender intValue]);
}

// manually catch case of game not running since button validation would require key value binding -.-

- (IBAction) consoleIn:(id)sender
{
	if (![C4AppDelegate isEditorAndGameRunning])
		return;
	Console.In([[consoleCombo stringValue] cStringUsingEncoding:NSUTF8StringEncoding]);
}

- (IBAction) objectIn:(id)sender
{
	if (![C4AppDelegate isEditorAndGameRunning])
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
	while ((s = gameRunningInConsoleModeSelectors[i++]) != nil)
	{
		if (s == [item action])
			return [C4AppDelegate isEditorAndGameRunning];
	}
	
	// always enabled
	return YES;
}

@end

#endif
