/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2010-2013, The OpenClonk Team and contributors
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
- (void) setInputFunctions:(std::list<const char*>)functions;
@end

#endif
