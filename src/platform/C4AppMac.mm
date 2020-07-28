/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005-2009, RedWolf Design GmbH, http://www.clonk.de
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

// based on SDL implementation

#include "C4ForbidLibraryCompilation.h"
#define GL_SILENCE_DEPRECATION
#include <epoxy/gl.h>

#include "C4Include.h"
#include "platform/C4Window.h"
#include "graphics/C4Draw.h"

#include "platform/C4App.h"
#import <Cocoa/Cocoa.h>

#ifndef USE_CONSOLE
#import "platform/C4WindowController.h"
#import "graphics/C4DrawGLMac.h"

bool C4AbstractApp::Copy(const std::string &text, bool fClipboard)
{
	NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
	[pasteboard declareTypes:[NSArray arrayWithObject:NSStringPboardType] owner:nil];
	NSString* string = [NSString stringWithCString:text.c_str() encoding:NSUTF8StringEncoding];
	if (![pasteboard setString:string forType:NSStringPboardType])
	{
		Log("Writing to Cocoa pasteboard failed");
		return false;
	}
	return true;
}

std::string C4AbstractApp::Paste(bool fClipboard)
{
	if (fClipboard)
	{
		NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
		const char* chars = [[pasteboard stringForType:NSStringPboardType] cStringUsingEncoding:NSUTF8StringEncoding];
		return chars;
	}
	return std::string();
}

bool C4AbstractApp::IsClipboardFull(bool fClipboard)
{
	return [[NSPasteboard generalPasteboard] availableTypeFromArray:[NSArray arrayWithObject:NSStringPboardType]];
}

void C4AbstractApp::MessageDialog(const char * message)
{
	NSAlert* alert = [NSAlert alertWithMessageText:@"Fatal Error"
		defaultButton:nil
		alternateButton:nil
		otherButton:nil
		informativeTextWithFormat:@"%@",
		[NSString stringWithUTF8String:message]
	];
	[alert runModal];
}

void C4Window::FlashWindow()
{
	[NSApp requestUserAttention:NSCriticalRequest];
}

#ifdef USE_COCOA

C4AbstractApp::C4AbstractApp(): Active(false), fQuitMsgReceived(false), MainThread(pthread_self()), fDspModeSet(false)
{
}
 
C4AbstractApp::~C4AbstractApp() {}

bool C4AbstractApp::Init(int argc, char * argv[])
{
	// Set locale
	setlocale(LC_ALL,"");

	// Custom initialization
	return DoInit (argc, argv);
}


void C4AbstractApp::Clear() {}

void C4AbstractApp::Quit()
{
	[NSApp terminate:[NSApp delegate]];
}

bool C4AbstractApp::FlushMessages()
{
	// Always fail after quit message
	if(fQuitMsgReceived)
		return false;

	while (CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, TRUE) == kCFRunLoopRunHandledSource);
	NSEvent* event;
	while ((event = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate distantPast] inMode:NSEventTrackingRunLoopMode dequeue:YES]) != nil)
	{
		[NSApp sendEvent:event];
		[NSApp updateWindows];
	}
	return true;
}

static int32_t bitDepthFromPixelEncoding(CFStringRef encoding)
{
	// copy-pasta: http://gitorious.org/ogre3d/mainlinemirror/commit/722dbd024aa91a6401850788db76af89c364d6e7
	if (CFStringCompare(encoding, CFSTR(IO32BitDirectPixels), kCFCompareCaseInsensitive) == kCFCompareEqualTo)
		return 32;
	else if(CFStringCompare(encoding, CFSTR(IO16BitDirectPixels), kCFCompareCaseInsensitive) == kCFCompareEqualTo)
		return 16;
	else if(CFStringCompare(encoding, CFSTR(IO8BitIndexedPixels), kCFCompareCaseInsensitive) == kCFCompareEqualTo)
		return 8;
	else
		return -1; // fail
}

bool C4AbstractApp::GetIndexedDisplayMode(int32_t iIndex, int32_t *piXRes, int32_t *piYRes, int32_t *piBitDepth, int32_t *piRefreshRate, uint32_t iMonitor)
{
	// No support for multiple monitors.
	CFArrayRef array = CGDisplayCopyAllDisplayModes(iMonitor, NULL);
	bool good_index = iIndex >= 0 && iIndex < (int32_t)CFArrayGetCount(array);
	if (good_index)
	{
		CGDisplayModeRef displayMode = (CGDisplayModeRef)CFArrayGetValueAtIndex(array, iIndex);
		*piXRes = CGDisplayModeGetWidth(displayMode);
		*piYRes = CGDisplayModeGetHeight(displayMode);
		CFStringRef pixelEncoding = CGDisplayModeCopyPixelEncoding(displayMode);
		*piBitDepth = bitDepthFromPixelEncoding(pixelEncoding);
		CFRelease(pixelEncoding);
	}
	CFRelease(array);
	return good_index;
}

void C4AbstractApp::RestoreVideoMode()
{
}

bool C4AbstractApp::SetVideoMode(int iXRes, int iYRes, unsigned int iRefreshRate, unsigned int iMonitor, bool fFullScreen)
{
	fFullScreen &= !lionAndBeyond(); // Always false for Lion since then Lion's true(tm) Fullscreen is used
	C4WindowController* controller = pWindow->objectiveCObject<C4WindowController>();
	NSWindow* window = controller.window;

	size_t dw = CGDisplayPixelsWide(C4OpenGLView.displayID);
	size_t dh = CGDisplayPixelsHigh(C4OpenGLView.displayID);
	if (iXRes == -1)
		iXRes = dw;
	if (iYRes == -1)
		iYRes = dh;
	ActualFullscreenX = iXRes;
	ActualFullscreenY = iYRes;
	[C4OpenGLView setSurfaceBackingSizeOf:[C4OpenGLView mainContext] width:ActualFullscreenX height:ActualFullscreenY];
	if ((window.styleMask & NSWindowStyleMaskFullScreen) == 0)
	{
		[window setResizeIncrements:NSMakeSize(1.0, 1.0)];
		pWindow->SetSize(iXRes, iYRes);
		[controller setFullscreen:fFullScreen];
		[window setAspectRatio:[[window contentView] frame].size];
		[window center];
	}
	else
	{
		[window toggleFullScreen:window];
		pWindow->SetSize(dw, dh);
	}
	if (!fFullScreen)
		[window makeKeyAndOrderFront:nil];
	OnResolutionChanged(iXRes, iYRes);
	return true;
}

#endif // USE_COCOA
#endif // USE_CONSOLE

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

std::string C4AbstractApp::GetGameDataPath()
{
	return [[[NSBundle mainBundle] resourcePath] fileSystemRepresentation];
}
