/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2006  Julian Raschke
 * Copyright (c) 2008-2009  Günther Brammer
 * Copyright (c) 2005-2009, RedWolf Design GmbH, http://www.clonk.de
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

// based on SDL implementation

#import <AppKit/AppKit.h>

#include <C4Include.h>
#include <StdWindow.h>
#include <string>

bool CStdApp::Copy(const StdStrBuf & text, bool fClipboard) {
	NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
	[pasteboard declareTypes:[NSArray arrayWithObject:NSStringPboardType] owner:nil];
	NSString* string = [NSString stringWithCString:text.getData() encoding:NSUTF8StringEncoding];
	if (![pasteboard setString:string forType:NSStringPboardType]) {
		NSLog(@"Writing to Cocoa pasteboard failed");
		return false;
	}
	return true;
}

StdStrBuf CStdApp::Paste(bool fClipboard) {
	if (fClipboard) {
		NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
		const char* chars = [[pasteboard stringForType:NSStringPboardType] cStringUsingEncoding:NSUTF8StringEncoding];
		return StdStrBuf(chars);
	}
	return StdStrBuf(0);
}

bool CStdApp::IsClipboardFull(bool fClipboard) {
	return [[NSPasteboard generalPasteboard] availableTypeFromArray:[NSArray arrayWithObject:NSStringPboardType]];
}

void CStdApp::ClearClipboard(bool fClipboard) {
	[[NSPasteboard generalPasteboard] declareTypes:[NSArray array] owner:nil];
}

void CStdApp::MessageDialog(const char * message)
{
	NSAlert* alert = [NSAlert alertWithMessageText:@"Fatal Error" defaultButton:nil alternateButton:nil otherButton:nil informativeTextWithFormat:[NSString stringWithUTF8String:message]];
	[alert runModal];
}

void CStdWindow::FlashWindow() {
	[NSApp requestUserAttention:NSCriticalRequest];
}

// Event-pipe-whatever stuff I do not understand.

bool CStdApp::ReadStdInCommand()
{
	char c;
	if(read(0, &c, 1) != 1)
		return false;
	if(c == '\n') {
		if(!CmdBuf.isNull()) {
			OnCommand(CmdBuf.getData()); CmdBuf.Clear();
		}
	} else if(isprint((unsigned char)c))
		CmdBuf.AppendChar(c);
	return true;
}

bool IsGermanSystem()
{
	id languages = [[NSUserDefaults standardUserDefaults] valueForKey:@"AppleLanguages"];
	return languages && [[languages objectAtIndex:0] isEqualToString:@"de"];
}

bool OpenURL(const char* szURL)
{
	std::string command = std::string("open ") + '"' + szURL + '"';
	std::system(command.c_str());
	return true;
}

bool EraseItemSafe(const char* szFilename)
{
	NSString* filename = [NSString stringWithUTF8String: szFilename];
	return [[NSWorkspace sharedWorkspace]
		performFileOperation: NSWorkspaceRecycleOperation
		source: [filename stringByDeletingLastPathComponent]
		destination: @""
		files: [NSArray arrayWithObject: [filename lastPathComponent]]
		tag: 0];
}
