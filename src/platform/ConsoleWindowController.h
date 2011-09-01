/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2010  Martin Plicht
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

#import <AppKit/AppKit.h>
#import <Quartz/Quartz.h>
#import <ClonkWindowController.h>

#ifdef USE_COCOA

@class ClonkOpenGLView;
@class ClonkAppDelegate;

@interface ConsoleWindowController : ClonkWindowController<NSUserInterfaceValidations> {
	IBOutlet NSTextView* outputTextView;
	IBOutlet NSComboBox* consoleCombo;
	IBOutlet NSComboBox* objectCombo;
	IBOutlet NSPopUpButton* materialsPopup;
	IBOutlet NSPopUpButton* texturesPopup;
	IBOutlet NSTextField* frameLabel;
	IBOutlet NSTextField* scriptLabel;
	IBOutlet NSTextField* timeLabel;
	IBOutlet NSTextView* objectPropertiesText;
	IBOutlet NSScrollView* outputScrollView;
	IBOutlet IKImageView* previewView;
	IBOutlet NSPanel* toolsPanel;
	IBOutlet NSPanel* objectsPanel;
	IBOutlet NSSegmentedControl* toolSelector;
	IBOutlet NSSegmentedControl* modeSelector;
}
@property(readonly) NSTextField* frameLabel;
@property(readonly) NSTextField* scriptLabel;
@property(readonly) NSTextField* timeLabel;
@property(readonly) NSTextView* outputTextView;
@property(readonly) NSTextView* objectPropertiesText;
@property(readonly) NSPopUpButton* materialsPopup;
@property(readonly) NSPopUpButton* texturesPopup;
@property(readonly) NSScrollView* outputScrollView;
@property(readonly) IKImageView* previewView;
@property(readonly) NSWindow* toolsPanel;
@property(readonly) NSWindow* objectsPanel;
@property(readonly) NSSegmentedControl* toolSelector;
@property(readonly) NSSegmentedControl* modeSelector;
@property(readonly) NSComboBox* objectCombo;
@property(readonly) NSComboBox* consoleCombo;

- (IBAction) consoleIn:(id)sender;
- (IBAction) objectIn:(id)sender;
- (IBAction) selectMode:(id)sender;
- (IBAction) play:(id)sender;
- (IBAction) halt:(id)sender;
- (IBAction) selectMaterial:(id)sender;
- (IBAction) selectTexture:(id)sender;
- (IBAction) selectTool:(id)sender;
- (IBAction) selectIFT:(id)sender;
- (IBAction) selectMode:(id)sender;
- (IBAction) selectLandscapeMode:(id)sender;
- (IBAction) setGrade:(id)sender;
- (IBAction) kickPlayer:(id)sender;

@end

#endif
