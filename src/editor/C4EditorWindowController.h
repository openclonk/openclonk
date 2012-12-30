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
#import <C4WindowController.h>

#ifdef USE_COCOA

@class C4OpenGLView;
@class C4AppDelegate;

@interface C4EditorWindowController : C4WindowController<NSUserInterfaceValidations> {}
@property NSTextField* frameLabel;
@property NSTextField* timeLabel;
@property NSTextView* outputTextView;
@property NSTextView* objectPropertiesText;
@property NSPopUpButton* materialsPopup;
@property NSPopUpButton* texturesPopup;
@property NSScrollView* outputScrollView;
@property IKImageView* previewView;
@property NSPanel* toolsPanel;
@property NSPanel* objectsPanel;
@property NSSegmentedControl* toolSelector;
@property NSSegmentedControl* modeSelector;
@property NSComboBox* objectCombo;
@property NSComboBox* consoleCombo;
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
