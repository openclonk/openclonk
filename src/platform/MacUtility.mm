/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2004, 2007  GÃƒÂ¼nther Brammer
 * Copyright (c) 2008  Peter Wortmann
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de
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

#import <AppKit/AppKit.h>

namespace MacUtility {
	
	bool sendFileToTrash(const char* szFilename)
	{
		NSString* filename = [NSString stringWithCString: szFilename];
		return [[NSWorkspace sharedWorkspace]
				performFileOperation: NSWorkspaceRecycleOperation
				source: [filename stringByDeletingLastPathComponent]
				destination: @""
				files: [NSArray arrayWithObject: [filename lastPathComponent]]
				tag: 0];
	}
	
	bool isGerman()
	{
		id languages = [[NSUserDefaults standardUserDefaults] valueForKey:@"AppleLanguages"];
		return languages && [[languages objectAtIndex:0] isEqualToString:@"de"];
	}
	
	void restart(char*[])
	{
		NSString* filename = [[NSBundle mainBundle] bundlePath];
		NSString* cmd = [@"open " stringByAppendingString: filename];
		system([cmd UTF8String]);
	}
	
	namespace {
		int repeatDelayCfgValue(NSString* key, int defaultValue) {
			id value = [[NSUserDefaults standardUserDefaults] valueForKey:key];
			int result = value != nil ? [value intValue] * 15 : defaultValue;
			return result;
		}
	}
	
	int keyRepeatDelay(int defaultValue) {
		return repeatDelayCfgValue(@"InitialKeyRepeat", defaultValue);
	}
	
	int keyRepeatInterval(int defaultValue) {
		return repeatDelayCfgValue(@"KeyRepeat", defaultValue);
	}
	
	void ensureWindowInFront() {
		if ([NSApp isActive]) {
			NSWindow* w = [NSApp mainWindow];
			[w makeKeyAndOrderFront:NSApp];
		}
	}
	
}