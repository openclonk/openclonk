/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2010, The OpenClonk Team and contributors
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

#ifdef __APPLE__

#include <C4Include.h>
#include <C4FileMonitor.h>
#include <C4Application.h>

#include <StdFile.h>

#import <Foundation/Foundation.h>

// Implementation using FSEvents

C4FileMonitor::C4FileMonitor(ChangeNotify pCallback): fStarted(false), pCallback(pCallback)
{
	eventStream = NULL;
	context.version = 0;
	context.info = this;
	context.retain = NULL;
	context.release = NULL;
	context.copyDescription = NULL;
	setObjectiveCObject([NSMutableArray arrayWithCapacity:10]);
}

C4FileMonitor::~C4FileMonitor()
{
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
	for (unsigned int i = 0; i < numEvents; i++)
	{
		NSString* dir = [NSString stringWithUTF8String:paths[i]];
		NSArray* filesInDir = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:dir error:NULL];
		for (NSString* str in filesInDir)
		{
			NSString* fullPath = [dir stringByAppendingPathComponent:str];
			NSDictionary* attribs = [[NSFileManager defaultManager] attributesOfItemAtPath:fullPath error:NULL];
			NSDate* modified = [attribs fileModificationDate];
			if (modified && -[modified timeIntervalSinceNow] <= 3.0)
				mon->OnThreadEvent(Ev_FileChange, (void*)[fullPath UTF8String]);
		}
	}
}

void C4FileMonitor::StartStream()
{
	eventStream = FSEventStreamCreate(kCFAllocatorDefault, &FSEvents_Callback, &context, (__bridge CFArrayRef)objectiveCObject<NSMutableArray>(), kFSEventStreamEventIdSinceNow, 0.3,
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
	[objectiveCObject<NSMutableArray>() addObject:fullPath];
}

void C4FileMonitor::OnThreadEvent(C4InteractiveEventType eEvent, void* pEventData)
{
	if (eEvent != Ev_FileChange) return;
	pCallback((const char *)pEventData, 0);
}

void C4FileMonitor::GetFDs(std::vector<struct pollfd> & FDs) { }

bool C4FileMonitor::Execute(int iTimeout, pollfd *) { return true; }

#endif
