/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2004, 2007  Matthes Bender
 * Copyright (c) 2002, 2005, 2007  Sven Eberhardt
 * Copyright (c) 2004, 2006, 2008  Peter Wortmann
 * Copyright (c) 2004-2005, 2007, 2009, 2011  Günther Brammer
 * Copyright (c) 2011  Nicolas Hake
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

/* Handles group files */

#ifndef INC_C4Group
#define INC_C4Group

#ifdef HAVE_IO_H
#include <io.h>
#endif
#include <CStdFile.h>
#include <StdBuf.h>
#include <StdCompiler.h>

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
bool C4Group_SetOriginal(const char *szFilename, bool fOriginal);
bool C4Group_ReadFile(const char *szFilename, char **pData, size_t *iSize);

extern const char *C4CFN_FLS[];

extern time_t C4Group_AssumeTimeOffset;

#pragma pack (push, 1)

class C4GroupHeader
{
public:
	C4GroupHeader();
public:
	char id[24+4];
	int Ver1,Ver2;
	int Entries;
	char reserved[164];
public:
	void Init();
};

class C4GroupEntryCore
{
public:
	C4GroupEntryCore();
public:
	char FileName[260];
	int32_t Packed,ChildGroup;
	int32_t Size, reserved1, Offset;
	int32_t reserved2;
	char reserved3; unsigned int reserved4;
	char Executable;
	BYTE fbuf[26];
};

#pragma pack (pop)

const int C4GRES_InGroup  = 0,
          C4GRES_OnDisk   = 1,
          C4GRES_InMemory = 2,
          C4GRES_Deleted  = 3;

class C4GroupEntry: public C4GroupEntryCore
{
public:
	C4GroupEntry();
	~C4GroupEntry();
public:
	char DiskPath[_MAX_PATH + 1];
	int Status;
	bool DeleteOnDisk;
	bool HoldBuffer;
	bool BufferIsStdbuf;
	bool NoSort;
	BYTE *bpMemBuf;
	C4GroupEntry *Next;
public:
	void Set(const DirectoryIterator & iter, const char * szPath);
};

const int GRPF_Inactive=0,
          GRPF_File=1,
          GRPF_Folder=2;

class C4Group: public CStdStream
{
public:
	C4Group();
	~C4Group();

protected:
	int Status;
	char FileName[_MAX_PATH+1];
	// Parent status
	C4Group *Mother;
	bool ExclusiveChild;
	// File & Folder
	C4GroupEntry *SearchPtr;
	CStdFile StdFile;
	size_t iCurrFileSize; // size of last accessed file
	// File only
	int FilePtr;
	int MotherOffset;
	int EntryOffset;
	bool Modified;
	C4GroupHeader Head;
	C4GroupEntry *FirstEntry;
	// Folder only
	//struct _finddata_t Fdt;
	//long hFdt;
	DirectoryIterator FolderSearch;
	C4GroupEntry FolderSearchEntry;
	C4GroupEntry LastFolderSearchEntry;

	bool StdOutput;
	bool (*fnProcessCallback)(const char *, int);
	char ErrorString[C4GroupMaxError+1];

	bool NoSort; // If this flag is set, all entries will be marked NoSort in AddEntry

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
	bool Extract(const char *szFiles, const char *szExtractTo=NULL, const char *szExclude=NULL);
	bool ExtractEntry(const char *szFilename, const char *szExtractTo=NULL);
	bool Delete(const char *szFiles, bool fRecursive = false);
	bool DeleteEntry(const char *szFilename, bool fRecycle=false);
	bool Rename(const char *szFile, const char *szNewName);
	bool Sort(const char *szSortList);
	bool SortByList(const char **ppSortList, const char *szFilename=NULL);
	bool View(const char *szFiles);
	bool AccessEntry(const char *szWildCard,
	                 size_t *iSize=NULL, char *sFileName=NULL,
	                 bool NeedsToBeAGroup = false);
	bool AccessNextEntry(const char *szWildCard,
	                     size_t *iSize=NULL, char *sFileName=NULL,
	                     bool fStartAtFilename=false);
	bool LoadEntry(const char *szEntryName, char **lpbpBuf,
	               size_t *ipSize=NULL, int iAppendZeros=0);
	bool LoadEntry(const char *szEntryName, StdBuf * Buf);
	bool LoadEntry(const StdStrBuf & name, StdBuf * Buf) { return LoadEntry(name.getData(), Buf); }
	bool LoadEntryString(const char *szEntryName, StdStrBuf * Buf);
	bool LoadEntryString(const StdStrBuf & name, StdStrBuf * Buf) { return LoadEntryString(name.getData(), Buf); }
	bool FindEntry(const char *szWildCard,
	               StdStrBuf *sFileName=NULL,
	               size_t *iSize=NULL);
	bool FindEntry(const char *szWildCard,
	               char *sFileName)
	{
		StdStrBuf name;
		bool r = FindEntry(szWildCard, &name);
		if(sFileName) SCopy(name.getData(),sFileName);
		return r;
	}
	bool FindNextEntry(const char *szWildCard,
	                   StdStrBuf *sFileName=NULL,
	                   size_t *iSize=NULL,
	                   bool fStartAtFilename=false);
	bool FindNextEntry(const char *szWildCard,
	                   char *sFileName,
	                   size_t *iSize=NULL,
	                   bool fStartAtFilename=false)
	{
		StdStrBuf name(fStartAtFilename ? sFileName : "");
		bool r = FindNextEntry(szWildCard, &name, iSize, fStartAtFilename);
		if (r && sFileName) SCopy(name.getData(),sFileName);
		return r;
	}
	bool Read(void *pBuffer, size_t iSize);
	bool Advance(int iOffset);
	void SetStdOutput(bool fStatus);
	void ResetSearch(bool reload_contents=false); // reset search pointer so calls to FindNextEntry find first entry again. if reload_contents is set, the file list for directories is also refreshed.
	const char *GetError();
	const char *GetName();
	StdStrBuf GetFullName() const;
	int EntryCount(const char *szWildCard=NULL);
	size_t EntrySize(const char *szWildCard=NULL);
	size_t AccessedEntrySize() { return iCurrFileSize; } // retrieve size of last accessed entry
	unsigned int EntryCRC32(const char *szWildCard=NULL);
	int GetStatus();
	inline bool IsOpen() { return Status != GRPF_Inactive; }
	C4Group *GetMother();
	inline bool IsPacked() { return Status == GRPF_File; }
	inline bool HasPackedMother() { if (!Mother) return false; return Mother->IsPacked(); }
	inline bool SetNoSort(bool fNoSort) { NoSort = fNoSort; return true; }
	void PrintInternals(const char *szIndent=NULL);

protected:
	void Init();
	void Default();
	void Clear();
	void ProcessOut(const char *szMessage, int iProcess=0);
	bool EnsureChildFilePtr(C4Group *pChild);
	bool CloseExclusiveMother();
	bool Error(const char *szStatus);
	bool OpenReal(const char *szGroupName);
	bool OpenRealGrpFile();
	bool SetFilePtr(int iOffset);
	bool RewindFilePtr();
	bool AdvanceFilePtr(int iOffset, C4Group *pByChild=NULL);
	bool AddEntry(int status,
	              bool childgroup,
	              const char *fname,
	              long size,
	              const char *entryname = NULL,
	              BYTE *membuf = NULL,
	              bool fDeleteOnDisk = false,
	              bool fHoldBuffer = false,
	              bool fExecutable = false,
	              bool fBufferIsStdbuf = false);
	bool AddEntryOnDisk(const char *szFilename, const char *szAddAs=NULL, bool fMove=false);
	bool SetFilePtr2Entry(const char *szName, bool NeedsToBeAGroup = false);
	bool AppendEntry2StdFile(C4GroupEntry *centry, CStdFile &stdfile);
	C4GroupEntry *GetEntry(const char *szName);
	C4GroupEntry *SearchNextEntry(const char *szName);
	C4GroupEntry *GetNextFolderEntry();
	uint32_t CalcCRC32(C4GroupEntry *pEntry);
};

#endif
