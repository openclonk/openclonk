#include <C4Include.h>
#include <StdScheduler.h>
#ifdef USE_COCOA
#import <Cocoa/Cocoa.h>

using namespace std;

@class SCHAdditions;

@interface SCHAddition : NSObject
{
@protected
	__weak SCHAdditions* schedulerAdditions;
	StdSchedulerProc* proc;
}
- (id) initWithProc:(StdSchedulerProc*)_proc;
- (void) registerAt:(SCHAdditions*) _additions;
- (void) unregisterFrom:(SCHAdditions*) _additions;
- (bool) shouldExecuteProc;
- (void) changed;
@end

@interface SCHNotify : SCHAddition
{
	list<CFRunLoopSourceRef> socketSources;
}
- (void) registerAt:(SCHAdditions*) _additions;
- (void) unregisterFrom:(SCHAdditions*) _additions;
@end

@interface SCHTimer : SCHAddition
{
@private
	NSTimer* timer;
}
- (void) registerAt:(SCHAdditions*) _additions;
- (void) unregisterFrom:(SCHAdditions*) _additions;
@end

@interface SCHAdditions : NSObject
{
	NSMutableDictionary* procAdditions;
}
- (id) initWithScheduler:(StdScheduler*) scheduler;
- (SCHAddition*) additionForProc:(StdSchedulerProc*) proc;
- (SCHAddition*) assignAdditionForProc:(StdSchedulerProc*) proc;
+ (SCHAdditions*) requestAdditionForScheduler:(StdScheduler*) scheduler;
- (BOOL) removeAdditionForProc:(StdSchedulerProc*) proc;
@property(readonly) NSRunLoop* runLoop;
@property(readonly) StdScheduler* scheduler;
@end

@implementation SCHAdditions

static NSMutableDictionary* additionsDictionary;

- (id) initWithScheduler:(StdScheduler*) scheduler
{
	if (self = [super init])
	{
		_scheduler = scheduler;
		_runLoop = [NSRunLoop currentRunLoop];
		procAdditions = [NSMutableDictionary new];
		return self;
	} else
		return nil;
}

- (SCHAddition*) additionForProc:(StdSchedulerProc*) proc
{
	return [procAdditions objectForKey:[NSNumber valueWithPointer:proc]];
}

- (BOOL) removeAdditionForProc:(StdSchedulerProc*) proc
{
	auto key = [NSNumber valueWithPointer:proc];
	SCHAddition* x = [procAdditions objectForKey:key];
	if (x)
	{
		[x unregisterFrom:self];
		[procAdditions removeObjectForKey:key];
		return YES;
	}
	else
		return NO;
}

- (SCHAddition*) assignAdditionForProc:(StdSchedulerProc*) proc
{
	auto timerInterval = proc->TimerInterval();
	auto addition =
		timerInterval ? [[SCHTimer alloc] initWithProc:proc] :
		proc->IsNotify() ? [[SCHNotify alloc] initWithProc:proc] :
		nullptr;
	if (addition)
	{
		[procAdditions setObject:addition forKey:[NSNumber valueWithPointer:proc]];
		return addition;
	} else
		return nullptr;
}

+ (SCHAdditions*) requestAdditionForScheduler:(StdScheduler *)scheduler
{
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken,
		^{ additionsDictionary = [NSMutableDictionary new]; });
	auto key = [NSNumber valueWithPointer:scheduler];
	@synchronized (additionsDictionary)
	{
		SCHAdditions* additions = [additionsDictionary objectForKey:key];
		if (!additions)
		{
			additions = [[SCHAdditions alloc] initWithScheduler:scheduler];
			[additionsDictionary setObject:additions forKey:key];
		}
		return additions;
	}
}

@end

@implementation SCHAddition
- (id) initWithProc:(StdSchedulerProc *) _proc
{
	if (self = [super init])
	{
		proc = _proc;
		return self;
	} else
		return nil;
}
- (bool) shouldExecuteProc
{
	auto s = schedulerAdditions;
	return s && !s.scheduler->IsInManualLoop();
}
- (void) registerAt:(SCHAdditions*) _additions
{
	schedulerAdditions = _additions;
}
- (void) unregisterFrom:(SCHAdditions*) _additions
{
	schedulerAdditions = nil;
}
- (void) changed
{
	auto s = schedulerAdditions;
	[self unregisterFrom:s];
	[self registerAt:s];
}
@end

@implementation SCHTimer
- (id) initWithProc:(StdSchedulerProc *) _proc
{
	if (self = [super init])
	{
		proc = _proc;
		return self;
	} else
		return nil;
}
- (void) run:(id) sender
{
	auto i = timer;
	if (i && [self shouldExecuteProc])
		proc->Execute();
}
- (void) registerAt:(SCHAdditions*) _additions
{
	[super registerAt:_additions];
	auto loop = _additions.runLoop;
	timer = [NSTimer timerWithTimeInterval:proc->TimerInterval()/1000.0 target:self selector:@selector(run:) userInfo:nil repeats:YES];
	if ([timer respondsToSelector:@selector(setTolerance:)])
		[timer setTolerance:0.0];
	[loop addTimer:timer forMode:NSDefaultRunLoopMode];
}
- (void) unregisterFrom:(SCHAdditions*) _additions
{
	[timer invalidate];
	timer = nil;
	[super unregisterFrom:_additions];
}
@end

@implementation SCHNotify
void callback (CFSocketRef s, CFSocketCallBackType type, CFDataRef address, const void *data, void *info)
{
	auto notify = (__bridge SCHNotify*)info;
	pollfd p = {.fd=CFSocketGetNative(s)};
	if ([notify shouldExecuteProc])
		notify->proc->Execute(-1, &p);
}
- (void) registerAt:(SCHAdditions*) _additions
{
	[super registerAt:_additions];
	vector<struct pollfd> vecs;
	proc->GetFDs(vecs);
	CFSocketContext ctx = {};
	ctx.info = (__bridge void*)self;
	for (auto p : vecs)
	{
		auto socket = CFSocketCreateWithNative(NULL,
			p.fd, kCFSocketReadCallBack,
			callback, &ctx
		);
		auto runLoopSource = CFSocketCreateRunLoopSource(NULL, socket, 0);
		CFRunLoopAddSource([_additions.runLoop getCFRunLoop], runLoopSource, kCFRunLoopDefaultMode);
		socketSources.push_back(runLoopSource);
	}
}
- (void) unregisterFrom:(SCHAdditions*) _additions
{
	for (auto r : socketSources)
	{
		CFRunLoopSourceInvalidate(r);
		CFRelease(r);
	}
	socketSources.clear();
	[super unregisterFrom:_additions];
}
@end

void StdScheduler::Added(StdSchedulerProc *pProc)
{
	auto x = [SCHAdditions requestAdditionForScheduler:this];
	auto addition = [x assignAdditionForProc:pProc];
	if (addition)
		[addition registerAt:x];
}

void StdScheduler::Removing(StdSchedulerProc *pProc)
{
	auto x = [SCHAdditions requestAdditionForScheduler:this];
	[x removeAdditionForProc:pProc];
}

void StdScheduler::Changed(StdSchedulerProc* pProc)
{
	auto x = [SCHAdditions requestAdditionForScheduler:this];
	auto addition = [x additionForProc:pProc];
	if (addition)
		[addition changed];
}
#endif