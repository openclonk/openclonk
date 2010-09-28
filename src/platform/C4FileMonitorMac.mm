/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2010 Mortimer
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

#ifdef __APPLE__

#include <C4Include.h>
#include <C4FileMonitor.h>
#include <C4Application.h>

#include <StdFile.h>

#import <Foundation/Foundation.h>

namespace {const NSTimeInterval FileMonitor_Latency = 0.3;}

// Implementation using FSEvents

C4FileMonitor::C4FileMonitor(ChangeNotify pCallback): fStarted(false), pCallback(pCallback)
{
	eventStream = NULL;
	context.version = 0;
	context.info = this;
	context.retain = NULL;
	context.release = NULL;
	context.copyDescription = NULL;
	watchedDirectories = CFArrayCreateMutable(kCFAllocatorDefault, 10, &kCFTypeArrayCallBacks);
}

C4FileMonitor::~C4FileMonitor()
{
	CFRelease(watchedDirectories);
	watchedDirectories = NULL;
	if (fStarted)
		StopStream();
}

static void FSEvents_Callback(
    ConstFSEventStreamRef streamRef,
    void *clientCallBackInfo,
    size_t numEvents,
    void *eventPaths,
    const FSEventStreamEventFlags eventFlags[],
    const FSEventStreamEventId eventIds[])
{
	// FSEvents only tells us about directories in which some files were modified
	char** paths = (char**)eventPaths;
	C4FileMonitor* mon = (C4FileMonitor*)clientCallBackInfo;
	for (int i = 0; i < numEvents; i++)
	{
		NSString* dir = [NSString stringWithUTF8String:paths[i]];
		NSArray* filesInDir = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:dir error:NULL];
		for (NSString* str in filesInDir)
		{
			NSString* fullPath = [dir stringByAppendingPathComponent:str];
			NSDictionary* attribs = [[NSFileManager defaultManager] attributesOfItemAtPath:fullPath error:NULL];
			NSDate* modified = [attribs fileModificationDate];
			if (modified && abs([modified timeIntervalSinceNow]) <= FileMonitor_Latency)
				mon->OnThreadEvent(Ev_FileChange, (void*)[fullPath UTF8String]);
		}
	}
}

void C4FileMonitor::StartStream()
{
	eventStream = FSEventStreamCreate(kCFAllocatorDefault, &FSEvents_Callback, &context, watchedDirectories, kFSEventStreamEventIdSinceNow, FileMonitor_Latency,
		kFSEventStreamCreateFlagNone);
	FSEventStreamScheduleWithRunLoop(eventStream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
	FSEventStreamStart(eventStream);
}

void C4FileMonitor::StopStream()
{
	if (fStarted)
	{
		fStarted = false;
		FSEventStreamStop(eventStream);
		FSEventStreamInvalidate(eventStream);
		FSEventStreamRelease(eventStream);
	}
}

void C4FileMonitor::StartMonitoring()
{
	StartStream();
	fStarted = true;
}

void C4FileMonitor::AddDirectory(const char *szDir)
{
	NSString* path = [NSString stringWithUTF8String:szDir];
	NSString* fullPath = [path characterAtIndex:0] == '/'
		? path
		: [NSString stringWithFormat:@"%@/%@", [[NSFileManager defaultManager] currentDirectoryPath], path];
	CFArrayAppendValue(watchedDirectories, fullPath);
}

void C4FileMonitor::OnThreadEvent(C4InteractiveEventType eEvent, void* pEventData)
{
	if (eEvent != Ev_FileChange) return;
	pCallback((const char *)pEventData, 0);
}

void C4FileMonitor::GetFDs(std::vector<struct pollfd> & FDs) { }

bool C4FileMonitor::Execute(int iTimeout, pollfd *) { return true; }

#endif