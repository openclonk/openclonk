/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2008  Matthes Bender
 * Copyright (c) 2008  Peter Wortmann
 * Copyright (c) 2008-2009  Günther Brammer
 * Copyright (c) 2008-2009, RedWolf Design GmbH, http://www.clonk.de
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

// An inotify wrapper

#include <C4Include.h>
#include <C4FileMonitor.h>
#include <C4Application.h>

#include <StdFile.h>

#ifdef HAVE_SYS_INOTIFY_H
#include <sys/inotify.h>

#elif defined(HAVE_SYSCALL_INOTIFY)

#define _BSD_SOURCE        /* or _GNU_SOURCE or _SVID_SOURCE */
#include <unistd.h>
#include <sys/syscall.h>   /* For SYS_xxx definitions */


/* Structure describing an inotify event.  */
struct inotify_event
{
	int wd;   /* Watch descriptor.  */
	uint32_t mask;  /* Watch mask.  */
	uint32_t cookie;  /* Cookie to synchronize two events.  */
	uint32_t len;   /* Length (including NULs) of name.  */
	char name __flexarr;  /* Name.  */
};


/* Supported events suitable for MASK parameter of INOTIFY_ADD_WATCH.  */
#define IN_ACCESS  0x00000001 /* File was accessed.  */
#define IN_MODIFY  0x00000002 /* File was modified.  */
#define IN_ATTRIB  0x00000004 /* Metadata changed.  */
#define IN_CLOSE_WRITE   0x00000008 /* Writtable file was closed.  */
#define IN_CLOSE_NOWRITE 0x00000010 /* Unwrittable file closed.  */
#define IN_CLOSE   (IN_CLOSE_WRITE | IN_CLOSE_NOWRITE) /* Close.  */
#define IN_OPEN    0x00000020 /* File was opened.  */
#define IN_MOVED_FROM  0x00000040 /* File was moved from X.  */
#define IN_MOVED_TO      0x00000080 /* File was moved to Y.  */
#define IN_MOVE    (IN_MOVED_FROM | IN_MOVED_TO) /* Moves.  */
#define IN_CREATE  0x00000100 /* Subfile was created.  */
#define IN_DELETE  0x00000200 /* Subfile was deleted.  */
#define IN_DELETE_SELF   0x00000400 /* Self was deleted.  */
#define IN_MOVE_SELF   0x00000800 /* Self was moved.  */

/* Events sent by the kernel.  */
#define IN_UNMOUNT   0x00002000 /* Backing fs was unmounted.  */
#define IN_Q_OVERFLOW  0x00004000 /* Event queued overflowed.  */
#define IN_IGNORED   0x00008000 /* File was ignored.  */

/* Helper events.  */
#define IN_CLOSE   (IN_CLOSE_WRITE | IN_CLOSE_NOWRITE)  /* Close.  */
#define IN_MOVE    (IN_MOVED_FROM | IN_MOVED_TO)    /* Moves.  */

/* Special flags.  */
#define IN_ONLYDIR   0x01000000 /* Only watch the path if it is a
directory.  */
#define IN_DONT_FOLLOW   0x02000000 /* Do not follow a sym link.  */
#define IN_MASK_ADD  0x20000000 /* Add to the mask of an already
existing watch.  */
#define IN_ISDIR   0x40000000 /* Event occurred against dir.  */
#define IN_ONESHOT   0x80000000 /* Only send event once.  */

/* All events which a program can wait on.  */
#define IN_ALL_EVENTS  (IN_ACCESS | IN_MODIFY | IN_ATTRIB | IN_CLOSE_WRITE  \
        | IN_CLOSE_NOWRITE | IN_OPEN | IN_MOVED_FROM        \
        | IN_MOVED_TO | IN_CREATE | IN_DELETE         \
        | IN_DELETE_SELF | IN_MOVE_SELF)


/* Create and initialize inotify instance.  */
int inotify_init (void) { return syscall(__NR_inotify_init); }

/* Add watch of object NAME to inotify instance FD.  Notify about
   events specified by MASK.  */
int inotify_add_watch (int __fd, const char *__name, uint32_t __mask)
{
	return syscall(__NR_inotify_add_watch, __fd, __name, __mask);
}

/* Remove the watch specified by WD from the inotify instance FD.  */
int inotify_rm_watch (int __fd, uint32_t __wd)
{
	return syscall(__NR_inotify_rm_watch, __fd, __wd);
}

#endif

#if defined(HAVE_SYS_INOTIFY_H) || defined(HAVE_SYSCALL_INOTIFY)
#include <errno.h>

C4FileMonitor::C4FileMonitor(ChangeNotify pCallback): fStarted(false), pCallback(pCallback)
{
	fd = inotify_init();
	if (fd == -1) LogF("inotify_init %s", strerror(errno));
}

C4FileMonitor::~C4FileMonitor()
{
	if (fStarted)
	{
		C4InteractiveThread &Thread = Application.InteractiveThread;
		Thread.RemoveProc(this);
		Thread.ClearCallback(Ev_FileChange, this);
	}
	while (close(fd) == -1 && errno == EINTR) { }
}

void C4FileMonitor::StartMonitoring()
{
	if (fStarted) return;
	C4InteractiveThread &Thread = Application.InteractiveThread;
	Thread.AddProc(this);
	Thread.SetCallback(Ev_FileChange, this);
	fStarted = true;
}

void C4FileMonitor::AddDirectory(const char * file)
{
	// Add IN_CLOSE_WRITE?
	int wd = inotify_add_watch(fd, file, IN_CREATE | IN_MODIFY | IN_MOVED_TO | IN_MOVE_SELF | IN_ONLYDIR);
	if (wd == -1)
		LogF("inotify_add_watch %s", strerror(errno));
	watch_descriptors[wd] = file;
}

bool C4FileMonitor::Execute(int iTimeout, pollfd * pfd) // some other thread
{
	// pfd is NULL here since it is not passed from StdScheduler::ScheduleProcs
	// Note that simply changing that breaks other ScheduleProcs which rely
	// on it being NULL (e.g. C4NetIOTCP).

	//if ((pfd->revents & pfd->events) != POLLIN || pfd->fd != fd)
	//  LogF("C4FileMonitor::Execute unexpectedly called %d %d %hd %hd", fd, pfd->fd, pfd->events, pfd->revents);
	char buf[sizeof(inotify_event) + _MAX_FNAME + 1];
	inotify_event* event = new (buf) inotify_event;
	if (read(fd, buf, sizeof(buf)) > 0)
	{
		const char * file = watch_descriptors[event->wd];
		uint32_t mask = event->mask;
		C4InteractiveThread &Thread = Application.InteractiveThread;
		if (mask & IN_CREATE)
			Thread.PushEvent(Ev_FileChange, (void*)file);
		if (mask & IN_MODIFY)
			Thread.PushEvent(Ev_FileChange, (void*)file);
		if (mask & IN_MOVED_TO)
			Thread.PushEvent(Ev_FileChange, (void*)file);
		if (mask & IN_MOVE_SELF)
			Thread.PushEvent(Ev_FileChange, (void*)file);
		// FIXME: (*(inotify_event*)buf).name);
	}
	else
	{
		Log("inotify buffer too small");
	}
	return true;
}

void C4FileMonitor::OnThreadEvent(C4InteractiveEventType eEvent, void *pEventData) // main thread
{
	if (eEvent != Ev_FileChange) return;
	pCallback((const char *)pEventData, 0);
}

void C4FileMonitor::GetFDs(std::vector<struct pollfd> & fds)
{
	pollfd pfd = { fd, POLLIN, 0 };
	fds.push_back(pfd);
}

#elif defined(_WIN32)

C4FileMonitor::C4FileMonitor(ChangeNotify pCallback)
		: pCallback(pCallback), pWatches(NULL), fStarted(false)
{
	hEvent = CreateEvent(NULL, true, false, NULL);
}

C4FileMonitor::~C4FileMonitor()
{
	if (fStarted)
	{
		C4InteractiveThread &Thread = Application.InteractiveThread;
		Thread.RemoveProc(this);
		Thread.ClearCallback(Ev_FileChange, this);
	}
	while (pWatches)
	{
		CloseHandle(pWatches->hDir);
		TreeWatch *pDelete = pWatches;
		pWatches = pWatches->Next;
		delete pDelete;
	}
	CloseHandle(hEvent);
}

void C4FileMonitor::StartMonitoring()
{
	C4InteractiveThread &Thread = Application.InteractiveThread;
	Thread.AddProc(this);
	Thread.SetCallback(Ev_FileChange, this);
	fStarted = true;
}

const DWORD C4FileMonitorNotifies = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE;

void C4FileMonitor::AddDirectory(const char *szDir)
{
	// Create file handle
	HANDLE hDir = CreateFile(szDir, FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, 0);
	if (hDir == INVALID_HANDLE_VALUE) return;
	// Create tree watch structure
	TreeWatch *pWatch = new TreeWatch();
	pWatch->hDir = hDir;
	pWatch->DirName = szDir;
	// Build description of async operation
	ZeroMem(&pWatch->ov, sizeof(pWatch->ov));
	pWatch->ov.hEvent = hEvent;
	// Add to list
	pWatch->Next = pWatches;
	pWatches = pWatch;
	// Start async directory change notification
	if (!ReadDirectoryChangesW(hDir, pWatch->Buffer, sizeof(pWatch->Buffer), false, C4FileMonitorNotifies, NULL, &pWatch->ov, NULL))
		if (GetLastError() != ERROR_IO_PENDING)
		{
			delete pWatch;
			return;
		}
}

bool C4FileMonitor::Execute(int iTimeout, pollfd *)
{
	// Check event
	if (WaitForSingleObject(hEvent, iTimeout) != WAIT_OBJECT_0)
		return true;
	// Check handles
	for (TreeWatch *pWatch = pWatches; pWatch; pWatch = pWatch->Next)
	{
		DWORD dwBytes = 0;
		// Has a notification?
		if (GetOverlappedResult(pWatch->hDir, &pWatch->ov, &dwBytes, false))
		{
			// Read notifications
			const char *pPos = pWatch->Buffer;
			for (;;)
			{
				const _FILE_NOTIFY_INFORMATION *pNotify = reinterpret_cast<const _FILE_NOTIFY_INFORMATION *>(pPos);
				// Handle
				HandleNotify(pWatch->DirName.getData(), pNotify);
				// Get next entry
				if (!pNotify->NextEntryOffset) break;
				pPos += pNotify->NextEntryOffset;
				if (pPos >= pWatch->Buffer + Min<size_t>(sizeof(pWatch->Buffer), dwBytes))
					break;
				break;
			}
			// Restart directory change notification (flush queue)
			ReadDirectoryChangesW(pWatch->hDir, pWatch->Buffer, sizeof(pWatch->Buffer), false, C4FileMonitorNotifies, NULL, &pWatch->ov, NULL);
			dwBytes = 0;
			while (GetOverlappedResult(pWatch->hDir, &pWatch->ov, &dwBytes, false))
			{
				ReadDirectoryChangesW(pWatch->hDir, pWatch->Buffer, sizeof(pWatch->Buffer), false, C4FileMonitorNotifies, NULL, &pWatch->ov, NULL);
				dwBytes = 0;
			}
		}
	}
	ResetEvent(hEvent);
	return true;
}

void C4FileMonitor::OnThreadEvent(C4InteractiveEventType eEvent, void *pEventData) // main thread
{
	if (eEvent != Ev_FileChange) return;
	pCallback((const char *)pEventData, 0);
	delete pEventData;
}

HANDLE C4FileMonitor::GetEvent()
{
	return hEvent;
}

void C4FileMonitor::HandleNotify(const char *szDir, const _FILE_NOTIFY_INFORMATION *pNotify)
{
	// Get filename length
	UINT iCodePage = CP_ACP /* future: CP_UTF8 */;
	int iFileNameBytes = WideCharToMultiByte(iCodePage, 0,
	                     pNotify->FileName, pNotify->FileNameLength / 2, NULL, 0, NULL, NULL);
	// Set up filename buffer
	StdCopyStrBuf Path(szDir);
	Path.AppendChar(DirectorySeparator);
	Path.Grow(iFileNameBytes);
	char *pFilename = Path.getMPtr(SLen(Path.getData()));
	// Convert filename
	int iWritten = WideCharToMultiByte(iCodePage, 0,
	                                   pNotify->FileName, pNotify->FileNameLength / 2,
	                                   pFilename, iFileNameBytes,
	                                   NULL, NULL);
	if (iWritten != iFileNameBytes)
		Path.Shrink(iFileNameBytes+1);
	// Send notification
	Application.InteractiveThread.PushEvent(Ev_FileChange, Path.GrabPointer());
}

#else // !defined(HAVE_SYS_INOTIFY_H) && !defined(HAVE_SYS_SYSCALL_H)

// Stubs
C4FileMonitor::C4FileMonitor(ChangeNotify pCallback)
{
#if defined(HAVE_SYS_INOTIFY_H) || defined(HAVE_SYS_SYSCALL_H)
	C4FileMonitor::pCallback = pCallback;
#endif
}

C4FileMonitor::~C4FileMonitor() { }
bool C4FileMonitor::Execute(int iTimeout, pollfd *) { return false; /* blarg... function must return a value */ }
void C4FileMonitor::StartMonitoring() {}
void C4FileMonitor::OnThreadEvent(C4InteractiveEventType eEvent, void *pEventData) {}
void C4FileMonitor::AddDirectory(const char *szDir) {}

// Signal for calling Execute()
#ifdef STDSCHEDULER_USE_EVENTS
HANDLE C4FileMonitor::GetEvent() { return 0; }
#else
void C4FileMonitor::GetFDs(std::vector<struct pollfd> & FDs) { }
#endif

#endif // HAVE_SYS_INOTIFY_H
