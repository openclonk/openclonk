/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
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

/* Handles group files */

#ifndef INC_C4Group
#define INC_C4Group

#ifdef HAVE_IO_H
#include <io.h>
#endif
#include "c4group/CStdFile.h"

// C4Group-Rewind-warning:
// The current C4Group-implementation cannot handle random file access very well,
// because all files are written within a single zlib-stream.
// For every out-of-order-file accessed a group-rewind must be performed, and every
// single file up to the accessed file unpacked. As a workaround, all C4Groups are
// packed in a file order matching the reading order of the engine.
// If the reading order doesn't match the packing order, and a rewind has to be performed,
// a warning is issued in Debug-builds of the engine. But since some components require
// random access because they are loaded on-demand at runtime (e.g. global sounds), the
// warning may be temp disabled for those files using C4GRP_DISABLE_REWINDWARN and
// C4GRP_ENABLE_REWINDWARN. A ref counter keeps track of nested calls to those functions.
//
// If you add any new components to scenario or definition files, remember to adjust the
// sort order lists in C4Components.h accordingly, and enforce a reading order for that
// component.
//
// Maybe some day, someone will write a C4Group-implementation that is probably capable of
// random access...
#ifdef _DEBUG
extern int iC4GroupRewindFilePtrNoWarn;
#define C4GRP_DISABLE_REWINDWARN ++iC4GroupRewindFilePtrNoWarn;
#define C4GRP_ENABLE_REWINDWARN --iC4GroupRewindFilePtrNoWarn;
#else
#define C4GRP_DISABLE_REWINDWARN ;
#define C4GRP_ENABLE_REWINDWARN ;
#endif

const int C4GroupFileVer1 = 1;
const int C4GroupFileVer2 = 2;

const int C4GroupMaxError = 100;

const int32_t C4GroupSwapThreshold = 10 * 1024 * 1024;

#define C4GroupFileID "RedWolf Design GrpFolder"

bool C4Group_TestIgnore(const char *filename);
void C4Group_SetTempPath(const char *path);
const char* C4Group_GetTempPath();
void C4Group_SetSortList(const char **sort_list);
void C4Group_SetProcessCallback(bool (*callback)(const char *, int));
bool C4Group_IsGroup(const char *filename);
bool C4Group_CopyItem(const char *source, const char *target, bool no_sorting = false, bool reset_attributes = false);
bool C4Group_MoveItem(const char *source, const char *target, bool no_sorting = false);
bool C4Group_DeleteItem(const char *item_name, bool do_recycle = false);
bool C4Group_PackDirectoryTo(const char *filename, const char *to_filename);
bool C4Group_PackDirectory(const char *filename);
bool C4Group_UnpackDirectory(const char *filename);
bool C4Group_ExplodeDirectory(const char *filename);
bool C4Group_ReadFile(const char *filename, char **data, size_t *size);

extern const char *C4CFN_FLS[];

#pragma pack (push, 1)

struct C4GroupHeader
{
	char Id[24+4] = C4GroupFileID;
	int Ver1 = C4GroupFileVer1;
	int Ver2 = C4GroupFileVer2;
	int Entries = 0;
	char Reserved[164] = { 0 };
};

struct C4GroupEntryCore
{
	char FileName[260] = { 0 };
	int32_t Packed = 0;
	int32_t ChildGroup = 0;
	int32_t Size = 0;
	int32_t Offset = 0;
	int32_t Reserved1 = 0;
	int32_t Reserved2 = 0;
	char Reserved3 = '\0';
	unsigned int Reserved4 = 0;
	char Executable = '\0';
	BYTE Buffer[26] = { 0 };
};

#pragma pack (pop)

class C4GroupEntry: public C4GroupEntryCore
{
public:
	~C4GroupEntry();

	enum EntryStatus
	{
		C4GRES_InGroup,
		C4GRES_OnDisk,
		C4GRES_InMemory,
		C4GRES_Deleted
	};

public:
	char DiskPath[_MAX_PATH_LEN] = { 0 };
	EntryStatus Status = C4GRES_InGroup;
	bool DeleteOnDisk = false;
	bool HoldBuffer = false;
	bool BufferIsStdbuf = false;
	bool NoSort = false;
	BYTE *MemoryBuffer = nullptr;
	C4GroupEntry *Next = nullptr;
public:
	void Set(const DirectoryIterator & directories, const char * path);
};

class C4Group : public CStdStream
{
	struct P;
	std::unique_ptr<P> p;
public:
	C4Group();
	~C4Group() override;
	C4Group(C4Group &&) = default;
	C4Group &operator=(C4Group &&) = default;

protected:
	// C4Update requires these to be available by a subclass (C4GroupEx)
	C4GroupHeader Head;
	C4GroupEntry *GetEntry(const char *entry_name);
	void Clear();

public:
	bool Open(const char *group_name, bool do_create = false);
	bool Close();
	bool Save(bool reopen);
	bool OpenAsChild(C4Group *mother, const char *entry_name, bool is_exclusive = false, bool do_create = false);
	bool OpenChild(const char* entry_name);
	bool OpenMother();
	bool Add(const char *filename, const char *entry_name);
	bool Add(const char *entry_name, void *buffer, int size, bool add_as_child = false, bool hold_buffer = false, bool is_executable = false);
	bool Add(const char *entry_name, StdBuf &buffer, bool add_as_child = false, bool hold_buffer = false, bool is_executable = false);
	bool Add(const char *entry_name, StdStrBuf &buffer, bool add_as_child = false, bool hold_buffer = false, bool is_executable = false);
	bool Merge(const char *folders);
	bool Move(const char *filename, const char *entry_name);
	bool Extract(const char *files, const char *destination = nullptr, const char *exclude = nullptr);
	bool ExtractEntry(const char *filename, const char *destination = nullptr);
	bool Delete(const char *files, bool recursive = false);
	bool DeleteEntry(const char *filename, bool do_recycle = false);
	bool Rename(const char *filename, const char *new_name);
	bool Sort(const char *list);
	bool SortByList(const char **list, const char *filename = nullptr);
	bool AccessEntry(const char *wildcard,
	                 size_t *size = nullptr,
					 char *filename = nullptr,
	                 bool needs_to_be_a_group = false);
	bool AccessNextEntry(const char *wildcard,
	                     size_t *size = nullptr,
						 char *filename = nullptr,
	                     bool start_at_filename = false);
	bool LoadEntry(const char *entry_name,
			       char **buffer,
	               size_t *size_info = nullptr,
				   int zeros_to_append = 0);
	bool LoadEntry(const char *entry_name, StdBuf * buffer);
	bool LoadEntry(const StdStrBuf & name, StdBuf * buffer) { return LoadEntry(name.getData(), buffer); }
	bool LoadEntryString(const char *entry_name, StdStrBuf * buffer);
	bool LoadEntryString(const StdStrBuf & name, StdStrBuf * buffer) { return LoadEntryString(name.getData(), buffer); }
	bool FindEntry(const char *wildcard,
	               StdStrBuf *filename = nullptr,
	               size_t *size = nullptr);
	bool FindEntry(const char *wildcard, char *filename)
	{
		StdStrBuf name;
		bool found_entry = FindEntry(wildcard, &name);
		if (filename)
		{
			SCopy(name.getData(), filename);
		}
		return found_entry;
	}
	bool FindNextEntry(const char *wildcard,
	                   StdStrBuf *filename = nullptr,
	                   size_t *size = nullptr,
	                   bool start_at_filename = false);
	bool FindNextEntry(const char *wildcard,
	                   char *filename,
	                   size_t *size = nullptr,
	                   bool start_at_filename = false)
	{
		StdStrBuf name(start_at_filename ? filename : "");
		bool found_entry = FindNextEntry(wildcard, &name, size, start_at_filename);
		if (found_entry && filename)
		{
			SCopy(name.getData(),filename);
		}
		return found_entry;
	}
	bool Read(void *buffer, size_t size) override;
	bool Advance(int offset) override;
	void SetStdOutput(bool log_status);
	void ResetSearch(bool reload_contents = false); // reset search pointer so calls to FindNextEntry find first entry again. if reload_contents is set, the file list for directories is also refreshed.
	const char *GetError();
	const char *GetName() const;
	StdStrBuf GetFullName() const;
	int EntryCount(const char *wildcard = nullptr);
	size_t EntrySize(const char *wildcard = nullptr);
	size_t AccessedEntrySize() const override; // retrieve size of last accessed entry
	unsigned int EntryCRC32(const char *wildcard = nullptr);
	bool IsOpen() const;
	C4Group *GetMother();
	bool IsPacked() const;
	bool HasPackedMother() const;
	bool SetNoSort(bool no_sorting);
	int PreCacheEntries(const char *search_pattern, bool cache_previous = false); // pre-load entries to memory. return number of loaded entries.

	const C4GroupHeader &GetHeader() const;
	const C4GroupEntry *GetFirstEntry() const;

private:
	void Init();
	bool EnsureChildFilePtr(C4Group *child);
	bool CloseExclusiveMother();
	bool Error(const char *status_message);
	bool OpenReal(const char *group_name);
	bool OpenRealGrpFile();
	bool SetFilePtr(int offset);
	bool RewindFilePtr();
	bool AdvanceFilePtr(int offset);
	bool AddEntry(C4GroupEntry::EntryStatus status,
	              bool add_as_child,
	              const char *fname,
	              long size,
	              const char *entry_name = nullptr,
	              BYTE *buffer = nullptr,
	              bool delete_on_disk = false,
	              bool hold_buffer = false,
	              bool is_executable = false,
	              bool buffer_is_stdbuf = false);
	bool AddEntryOnDisk(const char *filename, const char *entry_name = nullptr, bool move = false);
	bool SetFilePtr2Entry(const char *entry_name, bool needs_to_be_a_group = false);
	bool AppendEntry2StdFile(C4GroupEntry *entry, CStdFile &target);
	C4GroupEntry *SearchNextEntry(const char *entry_name);
	C4GroupEntry *GetNextFolderEntry();
	uint32_t CalcCRC32(C4GroupEntry *entry);
	void PreCacheEntry(C4GroupEntry *entry);
};

#endif
