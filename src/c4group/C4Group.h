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
#include "lib/StdBuf.h"

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

const int C4GroupFileVer1=1, C4GroupFileVer2=2;

const int C4GroupMaxError = 100;

const int32_t C4GroupSwapThreshold = 10 * 1024 * 1024;

#define C4GroupFileID "RedWolf Design GrpFolder"

bool C4Group_TestIgnore(const char *szFilename);
void C4Group_SetTempPath(const char *szPath);
const char* C4Group_GetTempPath();
void C4Group_SetSortList(const char **ppSortList);
void C4Group_SetProcessCallback(bool (*fnCallback)(const char *, int));
bool C4Group_IsGroup(const char *szFilename);
bool C4Group_CopyItem(const char *szSource, const char *szTarget, bool fNoSort=false, bool fResetAttributes=false);
bool C4Group_MoveItem(const char *szSource, const char *szTarget, bool fNoSort=false);
bool C4Group_DeleteItem(const char *szItem, bool fRecycle=false);
bool C4Group_PackDirectoryTo(const char *szFilename, const char *szFilenameTo);
bool C4Group_PackDirectory(const char *szFilename);
bool C4Group_UnpackDirectory(const char *szFilename);
bool C4Group_ExplodeDirectory(const char *szFilename);
bool C4Group_ReadFile(const char *szFilename, char **pData, size_t *iSize);

extern const char *C4CFN_FLS[];

#pragma pack (push, 1)

struct C4GroupHeader
{
	char id[24+4] = C4GroupFileID;
	int Ver1 = C4GroupFileVer1;
	int Ver2 = C4GroupFileVer2;
	int Entries = 0;
	char reserved[164] = { 0 };
};

struct C4GroupEntryCore
{
	char FileName[260] = { 0 };
	int32_t Packed = 0, ChildGroup = 0;
	int32_t Size = 0, reserved1 = 0, Offset = 0;
	int32_t reserved2 = 0;
	char reserved3 = '\0';
	unsigned int reserved4 = 0;
	char Executable = '\0';
	BYTE fbuf[26] = { 0 };
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
	char DiskPath[_MAX_PATH + 1] = { 0 };
	EntryStatus Status = C4GRES_InGroup;
	bool DeleteOnDisk = false;
	bool HoldBuffer = false;
	bool BufferIsStdbuf = false;
	bool NoSort = false;
	BYTE *bpMemBuf = 0;
	C4GroupEntry *Next = 0;
public:
	void Set(const DirectoryIterator & iter, const char * szPath);
};

class C4Group : public CStdStream
{
	struct P;
	std::unique_ptr<P> p;
public:
	C4Group();
	~C4Group();
	C4Group(C4Group &&) = default;
	C4Group &operator=(C4Group &&) = default;

protected:
	// C4Update requires these to be available by a subclass (C4GroupEx)
	C4GroupHeader Head;
	C4GroupEntry *GetEntry(const char *szName);
	void Clear();

public:
	bool Open(const char *szGroupName, bool fCreate=false);
	bool Close();
	bool Save(bool fReOpen);
	bool OpenAsChild(C4Group *pMother, const char *szEntryName, bool fExclusive=false, bool fCreate=false);
	bool OpenChild(const char* strEntry);
	bool OpenMother();
	bool Add(const char *szFile, const char *szAddAs);
	bool Add(const char *szName, void *pBuffer, int iSize, bool fChild = false, bool fHoldBuffer = false, bool fExecutable = false);
	bool Add(const char *szName, StdBuf &pBuffer, bool fChild = false, bool fHoldBuffer = false, bool fExecutable = false);
	bool Add(const char *szName, StdStrBuf &pBuffer, bool fChild = false, bool fHoldBuffer = false, bool fExecutable = false);
	bool Merge(const char *szFolders);
	bool Move(const char *szFile, const char *szAddAs);
	bool Extract(const char *szFiles, const char *szExtractTo=nullptr, const char *szExclude=nullptr);
	bool ExtractEntry(const char *szFilename, const char *szExtractTo=nullptr);
	bool Delete(const char *szFiles, bool fRecursive = false);
	bool DeleteEntry(const char *szFilename, bool fRecycle=false);
	bool Rename(const char *szFile, const char *szNewName);
	bool Sort(const char *szSortList);
	bool SortByList(const char **ppSortList, const char *szFilename=nullptr);
	bool AccessEntry(const char *szWildCard,
	                 size_t *iSize=nullptr, char *sFileName=nullptr,
	                 bool NeedsToBeAGroup = false);
	bool AccessNextEntry(const char *szWildCard,
	                     size_t *iSize=nullptr, char *sFileName=nullptr,
	                     bool fStartAtFilename=false);
	bool LoadEntry(const char *szEntryName, char **lpbpBuf,
	               size_t *ipSize=nullptr, int iAppendZeros=0);
	bool LoadEntry(const char *szEntryName, StdBuf * Buf);
	bool LoadEntry(const StdStrBuf & name, StdBuf * Buf) { return LoadEntry(name.getData(), Buf); }
	bool LoadEntryString(const char *szEntryName, StdStrBuf * Buf);
	bool LoadEntryString(const StdStrBuf & name, StdStrBuf * Buf) { return LoadEntryString(name.getData(), Buf); }
	bool FindEntry(const char *szWildCard,
	               StdStrBuf *sFileName=nullptr,
	               size_t *iSize=nullptr);
	bool FindEntry(const char *szWildCard,
	               char *sFileName)
	{
		StdStrBuf name;
		bool r = FindEntry(szWildCard, &name);
		if(sFileName) SCopy(name.getData(),sFileName);
		return r;
	}
	bool FindNextEntry(const char *szWildCard,
	                   StdStrBuf *sFileName=nullptr,
	                   size_t *iSize=nullptr,
	                   bool fStartAtFilename=false);
	bool FindNextEntry(const char *szWildCard,
	                   char *sFileName,
	                   size_t *iSize=nullptr,
	                   bool fStartAtFilename=false)
	{
		StdStrBuf name(fStartAtFilename ? sFileName : "");
		bool r = FindNextEntry(szWildCard, &name, iSize, fStartAtFilename);
		if (r && sFileName) SCopy(name.getData(),sFileName);
		return r;
	}
	bool Read(void *pBuffer, size_t iSize) override;
	bool Advance(int iOffset) override;
	void SetStdOutput(bool fStatus);
	void ResetSearch(bool reload_contents=false); // reset search pointer so calls to FindNextEntry find first entry again. if reload_contents is set, the file list for directories is also refreshed.
	const char *GetError();
	const char *GetName() const;
	StdStrBuf GetFullName() const;
	int EntryCount(const char *szWildCard=nullptr);
	size_t EntrySize(const char *szWildCard=nullptr);
	size_t AccessedEntrySize() const override; // retrieve size of last accessed entry
	unsigned int EntryCRC32(const char *szWildCard=nullptr);
	bool IsOpen() const;
	C4Group *GetMother();
	bool IsPacked() const;
	bool HasPackedMother() const;
	bool SetNoSort(bool fNoSort);
	int PreCacheEntries(const char *szSearchPattern, bool cache_previous=false); // pre-load entries to memory. return number of loaded entries.

	const C4GroupHeader &GetHeader() const;
	const C4GroupEntry *GetFirstEntry() const;

private:
	void Init();
	bool EnsureChildFilePtr(C4Group *pChild);
	bool CloseExclusiveMother();
	bool Error(const char *szStatus);
	bool OpenReal(const char *szGroupName);
	bool OpenRealGrpFile();
	bool SetFilePtr(int iOffset);
	bool RewindFilePtr();
	bool AdvanceFilePtr(int iOffset);
	bool AddEntry(C4GroupEntry::EntryStatus status,
	              bool childgroup,
	              const char *fname,
	              long size,
	              const char *entryname = nullptr,
	              BYTE *membuf = nullptr,
	              bool fDeleteOnDisk = false,
	              bool fHoldBuffer = false,
	              bool fExecutable = false,
	              bool fBufferIsStdbuf = false);
	bool AddEntryOnDisk(const char *szFilename, const char *szAddAs=nullptr, bool fMove=false);
	bool SetFilePtr2Entry(const char *szName, bool NeedsToBeAGroup = false);
	bool AppendEntry2StdFile(C4GroupEntry *centry, CStdFile &stdfile);
	C4GroupEntry *SearchNextEntry(const char *szName);
	C4GroupEntry *GetNextFolderEntry();
	uint32_t CalcCRC32(C4GroupEntry *pEntry);
	void PreCacheEntry(C4GroupEntry * p);
};

#endif
