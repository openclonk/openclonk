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

/* Needs to be compilable as Objective C++ on OS X */

#include "C4Include.h"
#include "c4group/C4Group.h"

#include "c4group/C4Components.h"
#include "lib/C4InputValidation.h"
#include <zlib.h>


//------------------------------ File Sort Lists -------------------------------------------

const char *C4CFN_FLS[] =
{
	C4CFN_System,           C4FLS_System,
	C4CFN_Material,         C4FLS_Material,
	C4CFN_Graphics,         C4FLS_Graphics,
	C4CFN_DefFiles,         C4FLS_Def,
	C4CFN_PlayerFiles,      C4FLS_Player,
	C4CFN_ObjectInfoFiles,  C4FLS_Object,
	C4CFN_ScenarioFiles,    C4FLS_Scenario,
	C4CFN_FolderFiles,      C4FLS_Folder,
	C4CFN_ScenarioSections, C4FLS_Section,
	C4CFN_Sound,            C4FLS_Sound,
	C4CFN_Music,            C4FLS_Music,
	nullptr, nullptr
};

#ifdef _DEBUG
const char *szCurrAccessedEntry = nullptr;
int iC4GroupRewindFilePtrNoWarn=0;
#endif

#ifdef _DEBUG
//#define C4GROUP_DUMP_ACCESS
#endif

//---------------------------- Global C4Group_Functions -------------------------------------------

char C4Group_TempPath[_MAX_PATH+1]="";
char C4Group_Ignore[_MAX_PATH+1]="cvs;CVS;Thumbs.db;.orig;.svn";
const char **C4Group_SortList = nullptr;
bool (*C4Group_ProcessCallback)(const char *, int)=nullptr;

void C4Group_SetProcessCallback(bool (*fnCallback)(const char *, int))
{
	C4Group_ProcessCallback = fnCallback;
}

void C4Group_SetSortList(const char **ppSortList)
{
	C4Group_SortList = ppSortList;
}

void C4Group_SetTempPath(const char *szPath)
{
	if (!szPath || !szPath[0]) C4Group_TempPath[0]=0;
	else { SCopy(szPath,C4Group_TempPath,_MAX_PATH); AppendBackslash(C4Group_TempPath); }
}

const char *C4Group_GetTempPath()
{
	return C4Group_TempPath;
}

bool C4Group_TestIgnore(const char *szFilename)
{
	if(!*szFilename) return true; //poke out empty strings
	const char* name = GetFilename(szFilename);
	return *name == '.' //no hidden files and the directory itself
		|| name[strlen(name) - 1] == '~' //no temp files
		|| SIsModule(C4Group_Ignore,name); //not on Blacklist
}

bool C4Group_IsGroup(const char *szFilename)
{
	C4Group hGroup; if (hGroup.Open(szFilename))  { hGroup.Close(); return true; }
	return false;
}

bool C4Group_CopyItem(const char *szSource, const char *szTarget1, bool fNoSort, bool fResetAttributes)
{
	// Parameter check
	if (!szSource || !szTarget1 || !szSource[0] || !szTarget1[0]) return false;
	char szTarget[_MAX_PATH+1]; SCopy(szTarget1,szTarget,_MAX_PATH);

	// Backslash terminator indicates target is a path only (append filename)
	if (szTarget[SLen(szTarget)-1]==DirectorySeparator) SAppend(GetFilename(szSource),szTarget);

	// Check for identical source and target
	// Note that attributes aren't reset here
	if (ItemIdentical(szSource,szTarget)) return true;

	// Source and target are simple items
	if (ItemExists(szSource) && CreateItem(szTarget)) return CopyItem(szSource,szTarget, fResetAttributes);

	// For items within groups, attribute resetting isn't needed, because packing/unpacking will kill all
	// attributes anyway

	// Source & target
	C4Group hSourceParent, hTargetParent;
	char szSourceParentPath[_MAX_PATH+1],szTargetParentPath[_MAX_PATH+1];
	GetParentPath(szSource,szSourceParentPath); GetParentPath(szTarget,szTargetParentPath);

	// Temp filename
	char szTempFilename[_MAX_PATH+1];
	SCopy(C4Group_TempPath,szTempFilename,_MAX_PATH);
	SAppend(GetFilename(szSource),szTempFilename);
	MakeTempFilename(szTempFilename);

	// Extract source to temp file
	if ( !hSourceParent.Open(szSourceParentPath)
	     || !hSourceParent.Extract(GetFilename(szSource),szTempFilename)
	     || !hSourceParent.Close() ) return false;

	// Move temp file to target
	if ( !hTargetParent.Open(szTargetParentPath)
	     || !hTargetParent.SetNoSort(fNoSort)
	     || !hTargetParent.Move(szTempFilename, GetFilename(szTarget))
	     || !hTargetParent.Close() ) { EraseItem(szTempFilename); return false; }

	return true;
}

bool C4Group_MoveItem(const char *szSource, const char *szTarget1, bool fNoSort)
{
	// Parameter check
	if (!szSource || !szTarget1 || !szSource[0] || !szTarget1[0]) return false;
	char szTarget[_MAX_PATH+1]; SCopy(szTarget1,szTarget,_MAX_PATH);

	// Backslash terminator indicates target is a path only (append filename)
	if (szTarget[SLen(szTarget)-1]==DirectorySeparator) SAppend(GetFilename(szSource),szTarget);

	// Check for identical source and target
	if (ItemIdentical(szSource,szTarget)) return true;

	// Source and target are simple items
	if (ItemExists(szSource) && CreateItem(szTarget))
	{
		// erase test file, because it may block moving a directory
		EraseItem(szTarget);
		return MoveItem(szSource,szTarget);
	}

	// Source & target
	C4Group hSourceParent, hTargetParent;
	char szSourceParentPath[_MAX_PATH+1],szTargetParentPath[_MAX_PATH+1];
	GetParentPath(szSource,szSourceParentPath); GetParentPath(szTarget,szTargetParentPath);

	// Temp filename
	char szTempFilename[_MAX_PATH+1];
	SCopy(C4Group_TempPath,szTempFilename,_MAX_PATH);
	SAppend(GetFilename(szSource),szTempFilename);
	MakeTempFilename(szTempFilename);

	// Extract source to temp file
	if ( !hSourceParent.Open(szSourceParentPath)
	     || !hSourceParent.Extract(GetFilename(szSource),szTempFilename)
	     || !hSourceParent.Close() ) return false;

	// Move temp file to target
	if ( !hTargetParent.Open(szTargetParentPath)
	     || !hTargetParent.SetNoSort(fNoSort)
	     || !hTargetParent.Move(szTempFilename, GetFilename(szTarget))
	     || !hTargetParent.Close() ) { EraseItem(szTempFilename); return false; }

	// Delete original file
	if ( !hSourceParent.Open(szSourceParentPath)
	     || !hSourceParent.DeleteEntry(GetFilename(szSource))
	     || !hSourceParent.Close() ) return false;

	return true;
}

bool C4Group_DeleteItem(const char *szItem, bool fRecycle)
{
	// Parameter check
	if (!szItem || !szItem[0]) return false;

	// simple item?
	if (ItemExists(szItem))
	{
		if (fRecycle)
			return EraseItemSafe(szItem);
		else
			return EraseItem(szItem);
	}

	// delete from parent
	C4Group hParent;
	char szParentPath[_MAX_PATH+1];
	GetParentPath(szItem,szParentPath);

	// Delete original file
	if ( !hParent.Open(szParentPath)
	     || !hParent.DeleteEntry(GetFilename(szItem), fRecycle)
	     || !hParent.Close() ) return false;

	return true;
}

bool C4Group_PackDirectoryTo(const char *szFilename, const char *szFilenameTo)
{
	// Check file type
	if (!DirectoryExists(szFilename)) return false;
	// Target mustn't exist
	if (FileExists(szFilenameTo)) return false;
	// Ignore
	if (C4Group_TestIgnore(szFilename))
		return true;
	// Process message
	if (C4Group_ProcessCallback)
		C4Group_ProcessCallback(szFilename,0);
	// Create group file
	C4Group hGroup;
	if (!hGroup.Open(szFilenameTo,true))
		return false;
	// Add folder contents to group
	DirectoryIterator i(szFilename);
	for (; *i; i++)
	{
		// Ignore
		if (C4Group_TestIgnore(*i))
			continue;
		// Must pack?
		if (DirectoryExists(*i))
		{
			// Find temporary filename
			char szTempFilename[_MAX_PATH+1];
			// At C4Group temp path
			SCopy(C4Group_TempPath, szTempFilename, _MAX_PATH);
			SAppend(GetFilename(*i), szTempFilename, _MAX_PATH);
			// Make temporary filename
			MakeTempFilename(szTempFilename);
			// Pack and move into group
			if ( !C4Group_PackDirectoryTo(*i, szTempFilename)) break;
			if (!hGroup.Move(szTempFilename, GetFilename(*i)))
			{
				EraseFile(szTempFilename);
				break;
			}
		}
		// Add normally otherwise
		else if (!hGroup.Add(*i, nullptr))
			break;
	}
	// Something went wrong?
	if (*i)
	{
		// Close group and remove temporary file
		hGroup.Close();
		EraseItem(szFilenameTo);
		return false;
	}
	// Reset iterator
	i.Reset();
	// Close group
	hGroup.SortByList(C4Group_SortList,szFilename);
	if (!hGroup.Close())
		return false;
	// Done
	return true;
}

bool C4Group_PackDirectory(const char *szFilename)
{
	// Make temporary filename
	char szTempFilename[_MAX_PATH+1];
	SCopy(szFilename, szTempFilename, _MAX_PATH);
	MakeTempFilename(szTempFilename);
	// Pack directory
	if (!C4Group_PackDirectoryTo(szFilename, szTempFilename))
		return false;
	// Rename folder
	char szTempFilename2[_MAX_PATH+1];
	SCopy(szFilename, szTempFilename2, _MAX_PATH);
	MakeTempFilename(szTempFilename2);
	if (!RenameFile(szFilename, szTempFilename2))
		return false;
	// Name group file
	if (!RenameFile(szTempFilename,szFilename))
		return false;
	// Last: Delete folder
	return EraseDirectory(szTempFilename2);
}

bool C4Group_UnpackDirectory(const char *szFilename)
{
	// Already unpacked: success
	if (DirectoryExists(szFilename)) return true;

	// Not a real file: unpack parent directory first
	char szParentFilename[_MAX_PATH+1];
	if (!FileExists(szFilename))
		if (GetParentPath(szFilename,szParentFilename))
			if (!C4Group_UnpackDirectory(szParentFilename))
				return false;

	// Open group
	C4Group hGroup;
	if (!hGroup.Open(szFilename)) return false;

	// Process message
	if (C4Group_ProcessCallback)
		C4Group_ProcessCallback(szFilename,0);

	// Create target directory
	char szFoldername[_MAX_PATH+1];
	SCopy(szFilename,szFoldername,_MAX_PATH);
	MakeTempFilename(szFoldername);
	if (!CreatePath(szFoldername)) { hGroup.Close(); return false; }

	// Extract files to folder
	if (!hGroup.Extract("*",szFoldername)) { hGroup.Close(); return false; }

	// Close group
	hGroup.Close();

	// Rename group file
	char szTempFilename[_MAX_PATH+1];
	SCopy(szFilename,szTempFilename,_MAX_PATH);
	MakeTempFilename(szTempFilename);
	if (!RenameFile(szFilename, szTempFilename)) return false;

	// Rename target directory
	if (!RenameFile(szFoldername,szFilename)) return false;

	// Delete renamed group file
	return EraseItem(szTempFilename);
}

bool C4Group_ExplodeDirectory(const char *szFilename)
{
	// Ignore
	if (C4Group_TestIgnore(szFilename)) return true;

	// Unpack this directory
	if (!C4Group_UnpackDirectory(szFilename)) return false;

	// Explode all children
	ForEachFile(szFilename,C4Group_ExplodeDirectory);

	// Success
	return true;
}

bool C4Group_ReadFile(const char *szFile, char **pData, size_t *iSize)
{
	// security
	if (!szFile || !pData) return false;
	// get mother path & file name
	char szPath[_MAX_PATH + 1];
	GetParentPath(szFile, szPath);
	const char *pFileName = GetFilename(szFile);
	// open mother group
	C4Group MotherGroup;
	if (!MotherGroup.Open(szPath)) return false;
	// access the file
	size_t iFileSize;
	if (!MotherGroup.AccessEntry(pFileName, &iFileSize)) return false;
	// create buffer
	*pData = new char [iFileSize];
	// read it
	if (!MotherGroup.Read(*pData, iFileSize)) { delete [] *pData; *pData = nullptr; return false; }
	// ok
	MotherGroup.Close();
	if (iSize) *iSize = iFileSize;
	return true;
}

void MemScramble(BYTE *bypBuffer, int iSize)
{
	int cnt; BYTE temp;
	// XOR deface
	for (cnt=0; cnt<iSize; cnt++)
		bypBuffer[cnt] ^= 237;
	// BYTE swap
	for (cnt=0; cnt+2<iSize; cnt+=3)
	{
		temp = bypBuffer[cnt];
		bypBuffer[cnt] = bypBuffer[cnt+2];
		bypBuffer[cnt+2] = temp;
	}
}

//---------------------------------- C4Group ---------------------------------------------

C4GroupEntry::~C4GroupEntry()
{
	if (HoldBuffer)
		if (bpMemBuf)
		{
			if (BufferIsStdbuf)
				StdBuf::DeletePointer(bpMemBuf);
			else
				delete [] bpMemBuf;
		}
}

void C4GroupEntry::Set(const DirectoryIterator &iter, const char * path)
{
	InplaceReconstruct(this);

	SCopy(GetFilename(*iter),FileName,_MAX_FNAME);
	SCopy(*iter, DiskPath, _MAX_PATH-1);
	Size = iter.GetFileSize();
	Status=C4GRES_OnDisk;
	Packed=false;
	ChildGroup=false;
	// Notice folder entries are not checked for ChildGroup status.
	// This would cause extreme performance loss and be good for
	// use in entry list display only.
}

C4Group::C4Group()
{
	Init();
	StdOutput=false;
	fnProcessCallback=nullptr;
	NoSort=false;
}

void C4Group::Init()
{
	// General
	Status=GRPF_Inactive;
	FileName[0]=0;
	// Child status
	Mother=nullptr;
	ExclusiveChild=false;
	// File only
	FilePtr=0;
	EntryOffset=0;
	Modified=false;
	InplaceReconstruct(&Head);
	FirstEntry=nullptr;
	SearchPtr=nullptr;
	pInMemEntry=nullptr; iInMemEntrySize=0u;
	// Folder only
	FolderSearch.Clear();
	// Error status
	SCopy("No Error",ErrorString,C4GroupMaxError);
}

C4Group::~C4Group()
{
	Clear();
}

bool C4Group::Error(const char *szStatus)
{
	SCopy(szStatus,ErrorString,C4GroupMaxError);
	return false;
}

const char *C4Group::GetError()
{
	return ErrorString;
}

void C4Group::SetStdOutput(bool fStatus)
{
	StdOutput=fStatus;
}

bool C4Group::Open(const char *szGroupName, bool fCreate)
{
	if (!szGroupName) return Error("Open: Null filename");
	if (!szGroupName[0]) return Error("Open: Empty filename");

	char szGroupNameN[_MAX_FNAME];
	SCopy(szGroupName,szGroupNameN,_MAX_FNAME);
	// Convert to native path
	SReplaceChar(szGroupNameN, '\\', DirectorySeparator);

	// Real reference
	if (FileExists(szGroupNameN))
	{
		// Init
		Init();
		// Open group or folder
		return OpenReal(szGroupNameN);
	}

	// If requested, try creating a new group file
	if (fCreate)
	{
		CStdFile temp;
		if (temp.Create(szGroupNameN,false))
		{
			// Temporary file has been created
			temp.Close();
			// Init
			Init();
			Status=GRPF_File; Modified=true;
			SCopy(szGroupNameN,FileName,_MAX_FNAME);
			return true;
		}
	}

	// While not a real reference (child group), trace back to mother group or folder.
	// Open mother and child in exclusive mode.
	char szRealGroup[_MAX_FNAME];
	SCopy(szGroupNameN,szRealGroup,_MAX_FNAME);
	do
		{ if (!TruncatePath(szRealGroup)) return Error(FormatString("Open(\"%s\"): File not found", szGroupNameN).getData()); }
	while (!FileExists(szRealGroup));

	// Open mother and child in exclusive mode
	C4Group *pMother = new C4Group;
	pMother->SetStdOutput(StdOutput);
	if (!pMother->Open(szRealGroup))
		{ Clear(); Error(pMother->ErrorString); delete pMother; return false; }
	if (!OpenAsChild(pMother,szGroupNameN+SLen(szRealGroup)+1,true))
		{ Clear(); return false; }

	// Success
	return true;

}

bool C4Group::OpenReal(const char *szFilename)
{
	// Get original filename
	if (!szFilename) return false;
	SCopy(szFilename,FileName,_MAX_FNAME);

	// Folder
	if (DirectoryExists(FileName))
	{
		// Ignore
		if (C4Group_TestIgnore(szFilename))
			return Error(FormatString("OpenReal: filename '%s' ignored", szFilename).getData());
		// OpenReal: Simply set status and return
		Status=GRPF_Folder;
		ResetSearch();
		// Success
		return true;
	}

	// File: Try reading header and entries
	if (OpenRealGrpFile())
	{
		Status=GRPF_File;
		ResetSearch();
		return true;
	}
	else
		return false;

	return Error("OpenReal: Not a valid group");
}

bool C4Group::OpenRealGrpFile()
{
	int cnt,file_entries;
	C4GroupEntryCore corebuf;

	// Open StdFile
	if (!StdFile.Open(FileName,true)) return Error("OpenRealGrpFile: Cannot open standard file");

	// Read header
	if (!StdFile.Read((BYTE*)&Head,sizeof(C4GroupHeader))) return Error("OpenRealGrpFile: Error reading header");
	MemScramble((BYTE*)&Head,sizeof(C4GroupHeader));
	EntryOffset+=sizeof(C4GroupHeader);

	// Check Header
	if (!SEqual(Head.id,C4GroupFileID)
	    || (Head.Ver1!=C4GroupFileVer1) || (Head.Ver2>C4GroupFileVer2))
		return Error("OpenRealGrpFile: Invalid header");

	// Read Entries
	file_entries=Head.Entries;
	Head.Entries=0; // Reset, will be recounted by AddEntry
	for (cnt=0; cnt<file_entries; cnt++)
	{
		if (!StdFile.Read((BYTE*)&corebuf,sizeof(C4GroupEntryCore))) return Error("OpenRealGrpFile: Error reading entries");
		// New C4Groups have filenames in UTF-8
		StdStrBuf entryname(corebuf.FileName);
		entryname.EnsureUnicode();
		// Prevent overwriting of user stuff by malicuous groups
		C4InVal::ValidateFilename(const_cast<char *>(entryname.getData()),entryname.getLength());
		EntryOffset+=sizeof(C4GroupEntryCore);
		if (!AddEntry(C4GroupEntry::C4GRES_InGroup,!!corebuf.ChildGroup,
		              corebuf.FileName,corebuf.Size,
		              entryname.getData(),
		              nullptr, false, false,
		              !!corebuf.Executable))
			return Error("OpenRealGrpFile: Cannot add entry");
	}

	return true;
}

bool C4Group::AddEntry(C4GroupEntry::EntryStatus status,
                       bool childgroup,
                       const char *fname,
                       long size,
                       const char *entryname,
                       BYTE *membuf,
                       bool fDeleteOnDisk,
                       bool fHoldBuffer,
                       bool fExecutable,
                       bool fBufferIsStdbuf)
{

	// Folder: add file to folder immediately
	if (Status==GRPF_Folder)
	{

		// Close open StdFile
		StdFile.Close();

		// Get path to target folder file
		char tfname[_MAX_FNAME];
		SCopy(FileName,tfname,_MAX_FNAME);
		AppendBackslash(tfname);
		if (entryname) SAppend(entryname,tfname);
		else SAppend(GetFilename(fname),tfname);

		switch (status)
		{

		case C4GroupEntry::C4GRES_OnDisk: // Copy/move file to folder
			if (!CopyItem(fname, tfname))
				return false;
			// Reset directory iterator to reflect new file
			ResetSearch(true);
			if (fDeleteOnDisk && !EraseItem(fname))
				return false;
			return true;

		case C4GroupEntry::C4GRES_InMemory: { // Save buffer to file in folder
			CStdFile hFile;
			bool fOkay = false;
			if (hFile.Create(tfname, !!childgroup))
				fOkay = !!hFile.Write(membuf,size);
			hFile.Close();
			ResetSearch(true);

			if (fHoldBuffer) { if (fBufferIsStdbuf) StdBuf::DeletePointer(membuf); else delete [] membuf; }

			return fOkay;
		}

		default: break; // InGrp & Deleted ignored
		}

		return Error("Add to folder: Invalid request");
	}


	// Group file: add to virtual entry list

	C4GroupEntry *nentry,*lentry,*centry;

	// Delete existing entries of same name
	centry=GetEntry(GetFilename(entryname ? entryname : fname));
	if (centry) { centry->Status = C4GroupEntry::C4GRES_Deleted; Head.Entries--; }

	// Allocate memory for new entry
	nentry = new C4GroupEntry;

	// Find end of list
	for (lentry=FirstEntry; lentry && lentry->Next; lentry=lentry->Next) {}

	// Init entry core data
	if (entryname) SCopy(entryname,nentry->FileName,_MAX_FNAME);
	else SCopy(GetFilename(fname),nentry->FileName,_MAX_FNAME);
	nentry->Size=size;
	nentry->ChildGroup=childgroup;
	nentry->Offset=0;
	nentry->Executable=fExecutable;
	nentry->DeleteOnDisk=fDeleteOnDisk;
	nentry->HoldBuffer=fHoldBuffer;
	nentry->BufferIsStdbuf=fBufferIsStdbuf;
	if (lentry) nentry->Offset=lentry->Offset+lentry->Size;

	// Init list entry data
	SCopy(fname,nentry->DiskPath,_MAX_FNAME);
	nentry->Status=status;
	nentry->bpMemBuf=membuf;
	nentry->Next=nullptr;
	nentry->NoSort = NoSort;

	// Append entry to list
	if (lentry) lentry->Next=nentry;
	else FirstEntry=nentry;

	// Increase virtual file count of group
	Head.Entries++;

	return true;
}

C4GroupEntry* C4Group::GetEntry(const char *szName)
{
	if (Status==GRPF_Folder) return nullptr;
	C4GroupEntry *centry;
	for (centry=FirstEntry; centry; centry=centry->Next)
		if (centry->Status != C4GroupEntry::C4GRES_Deleted)
			if (WildcardMatch(szName,centry->FileName))
				return centry;
	return nullptr;
}

bool C4Group::Close()
{
	C4GroupEntry *centry;
	bool fRewrite=false;

	if (Status==GRPF_Inactive) return false;

	// Folder: just close
	if (Status==GRPF_Folder)
		{ CloseExclusiveMother(); Clear(); return true; }

	// Rewrite check
	for (centry=FirstEntry; centry; centry=centry->Next)
		if (centry->Status != C4GroupEntry::C4GRES_InGroup)
			fRewrite=true;
	if (Modified) fRewrite=true;

	// No rewrite: just close
	if (!fRewrite)
		{ CloseExclusiveMother(); Clear(); return true; }

	if (StdOutput) printf("Writing group file...\n");

	// Set new version
	Head.Ver1=C4GroupFileVer1;
	Head.Ver2=C4GroupFileVer2;

	// Automatic sort
	SortByList(C4Group_SortList);

	// Save group contents to disk
	bool fSuccess = Save(false);

	// Close exclusive mother
	CloseExclusiveMother();

	// Close file
	Clear();

	return !!fSuccess;
}

bool C4Group::Save(bool fReOpen)
{

	int cscore;
	C4GroupEntryCore *save_core;
	C4GroupEntry *centry;
	char szTempFileName[_MAX_FNAME+1],szGrpFileName[_MAX_FNAME+1];

	// Create temporary core list with new actual offsets to be saved
	int32_t iContentsSize = 0;
	save_core = new C4GroupEntryCore [Head.Entries];
	cscore=0;
	for (centry=FirstEntry; centry; centry=centry->Next)
		if (centry->Status != C4GroupEntry::C4GRES_Deleted)
		{
			save_core[cscore]=(C4GroupEntryCore)*centry;
			// Make actual offset
			save_core[cscore].Offset = iContentsSize;
			iContentsSize += centry->Size;
			cscore++;
		}

	// Hold contents in memory?
	bool fToMemory = !fReOpen && Mother && iContentsSize < C4GroupSwapThreshold;
	if (!fToMemory)
	{
		// Create target temp file (in temp directory!)
		SCopy(FileName,szGrpFileName,_MAX_FNAME);
		if (C4Group_TempPath[0]) { SCopy(C4Group_TempPath,szTempFileName,_MAX_FNAME); SAppend(GetFilename(FileName),szTempFileName,_MAX_FNAME); }
		else SCopy(FileName,szTempFileName,_MAX_FNAME);
		MakeTempFilename(szTempFileName);
		// (Temp file must not have the same name as the group.)
		if (SEqual(szTempFileName,szGrpFileName))
		{
			SAppend(".tmp",szTempFileName); // Add a second temp extension
			MakeTempFilename(szTempFileName);
		}
	}

	// Create the new (temp) group file
	CStdFile tfile;
	if (!tfile.Create(szTempFileName,true,false,fToMemory))
		{  delete [] save_core; return Error("Close: ..."); }

	// Save header and core list
	C4GroupHeader headbuf = Head;
	MemScramble((BYTE*)&headbuf,sizeof(C4GroupHeader));
	if (!tfile.Write((BYTE*)&headbuf,sizeof(C4GroupHeader))
	    || !tfile.Write((BYTE*)save_core,Head.Entries*sizeof(C4GroupEntryCore)))
		{ tfile.Close(); delete [] save_core; return Error("Close: ..."); }
	delete [] save_core;

	// Save Entries to temp file
	int iTotalSize=0,iSizeDone=0;
	for (centry=FirstEntry; centry; centry=centry->Next) iTotalSize+=centry->Size;
	for (centry=FirstEntry; centry; centry=centry->Next)
		if (AppendEntry2StdFile(centry,tfile))
			{ iSizeDone+=centry->Size; if (iTotalSize && fnProcessCallback) fnProcessCallback(centry->FileName,100*iSizeDone/iTotalSize); }
		else
		{
			tfile.Close(); return false;
		}

	// Write
	StdBuf *pBuf;
	tfile.Close(fToMemory ? &pBuf : nullptr);

	// Child: move temp file to mother
	if (Mother)
	{
		if (fToMemory)
		{
			if (!Mother->Add(GetFilename(FileName), *pBuf, true, true))
				{ delete pBuf; CloseExclusiveMother(); Clear(); return Error("Close: Cannot move rewritten child data to mother"); }
			delete pBuf;
		}
		else
		{
			if (!Mother->Move(szTempFileName,GetFilename(FileName)))
				{ CloseExclusiveMother(); Clear(); return Error("Close: Cannot move rewritten child temp file to mother"); }
		}
		Clear();
		return true;
	}

	// Clear (close file)
	Clear();

	// Delete old group file, rename new file
	if (!EraseFile(szGrpFileName))
		return Error("Close: Cannot erase temp file");
	if (!RenameFile(szTempFileName,szGrpFileName))
		return Error("Close: Cannot rename group file");

	// Should reopen the file?
	if (fReOpen)
		OpenReal(szGrpFileName);

	return true;
}

void C4Group::Default()
{
	FirstEntry = nullptr;
	StdFile.Default();
	Mother = nullptr;
	ExclusiveChild = 0;
	Init();
}

void C4Group::Clear()
{
	// Delete entries
	C4GroupEntry *next;
	while (FirstEntry)
	{
		next=FirstEntry->Next;
		delete FirstEntry;
		FirstEntry=next;
	}
	// Close std file
	StdFile.Close();
	// Delete mother
	if (Mother && ExclusiveChild)
	{
		delete Mother;
		Mother=nullptr;
	}
	// Reset
	Init();
}

bool C4Group::AppendEntry2StdFile(C4GroupEntry *centry, CStdFile &hTarget)
{
	CStdFile hSource;
	long csize;
	BYTE fbuf;

	switch (centry->Status)
	{

	case C4GroupEntry::C4GRES_InGroup: // Copy from group to std file
		if (!SetFilePtr(centry->Offset))
			return Error("AE2S: Cannot set file pointer");
		for (csize=centry->Size; csize>0; csize--)
		{
			if (!Read(&fbuf,1))
				return Error("AE2S: Cannot read entry from group file");
			if (!hTarget.Write(&fbuf,1))
				return Error("AE2S: Cannot write to target file");
		}
		break;

	case C4GroupEntry::C4GRES_OnDisk: // Copy/move from disk item to std file
	{
		char szFileSource[_MAX_FNAME+1];
		SCopy(centry->DiskPath,szFileSource,_MAX_FNAME);

		// Disk item is a directory
		if (DirectoryExists(centry->DiskPath))
			return Error("AE2S: Cannot add directory to group file");

		// Resort group if neccessary
		// (The group might be renamed by adding, forcing a resort)
		bool fTempFile = false;
		if (centry->ChildGroup)
			if (!centry->NoSort)
				if (!SEqual(GetFilename(szFileSource), centry->FileName))
				{
					// copy group
					MakeTempFilename(szFileSource);
					if (!CopyItem(centry->DiskPath, szFileSource))
						return Error("AE2S: Cannot copy item");
					// open group and resort
					C4Group SortGrp;
					if (!SortGrp.Open(szFileSource))
						return Error("AE2S: Cannot open group");
					if (!SortGrp.SortByList(C4Group_SortList, centry->FileName))
						return Error("AE2S: Cannot resort group");
					fTempFile = true;
					// close group (won't be saved if the sort didn't change)
					SortGrp.Close();
				}

		// Append disk source to target file
		if (!hSource.Open(szFileSource, !!centry->ChildGroup))
			return Error("AE2S: Cannot open on-disk file");
		for (csize=centry->Size; csize>0; csize--)
		{
			if (!hSource.Read(&fbuf,1))
				{ hSource.Close(); return Error("AE2S: Cannot read on-disk file"); }
			if (!hTarget.Write(&fbuf,1))
				{ hSource.Close(); return Error("AE2S: Cannot write to target file"); }
		}
		hSource.Close();

		// Erase temp file
		if (fTempFile)
			EraseItem(szFileSource);
		// Erase disk source if requested
		if (centry->DeleteOnDisk)
			EraseItem(centry->DiskPath);

		break;
	}

	case C4GroupEntry::C4GRES_InMemory: // Copy from mem to std file
		if (!centry->bpMemBuf) return Error("AE2S: no buffer");
		if (!hTarget.Write(centry->bpMemBuf,centry->Size)) return Error("AE2S: writing error");
		break;

	case C4GroupEntry::C4GRES_Deleted: // Don't save
		break;

	default: // Unknown file status
		return Error("AE2S: Unknown file status");

	}

	return true;
}

void C4Group::ResetSearch(bool reload_contents)
{
	switch (Status)
	{
	case GRPF_Folder:
		SearchPtr=nullptr;
		FolderSearch.Reset(FileName, reload_contents);
		if (*FolderSearch)
		{
			FolderSearchEntry.Set(FolderSearch,FileName);
			SearchPtr=&FolderSearchEntry;
		}
		break;
	case GRPF_File:
		SearchPtr=FirstEntry;
		break;
	default: break; // InGrp & Deleted ignored
	}
}

C4GroupEntry* C4Group::GetNextFolderEntry()
{
	if (*++FolderSearch)
	{
		FolderSearchEntry.Set(FolderSearch,FileName);
		return &FolderSearchEntry;
	}
	else
	{
		return nullptr;
	}
}

C4GroupEntry* C4Group::SearchNextEntry(const char *szName)
{
	// Wildcard "*.*" is expected to find all files: substitute correct wildcard "*"
	if (SEqual(szName, "*.*"))
		szName = "*";
	// Search by group type
	C4GroupEntry *pEntry;
	switch (Status)
	{
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case GRPF_File:
		for (pEntry=SearchPtr; pEntry; pEntry=pEntry->Next)
			if (pEntry->Status != C4GroupEntry::C4GRES_Deleted)
				if (WildcardMatch(szName,pEntry->FileName))
				{
					SearchPtr=pEntry->Next;
					return pEntry;
				}
		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case GRPF_Folder:
		for (pEntry=SearchPtr; pEntry; pEntry=GetNextFolderEntry())
			if (WildcardMatch(szName,pEntry->FileName))
				if (!C4Group_TestIgnore(pEntry->FileName))
				{
					LastFolderSearchEntry=(*pEntry);
					pEntry=&LastFolderSearchEntry;
					SearchPtr=GetNextFolderEntry();
					return pEntry;
				}
		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	default: break; // InGrp & Deleted ignored
	}
	// No entry found: reset search pointer
	SearchPtr=nullptr;
	return nullptr;
}

bool C4Group::SetFilePtr(int iOffset)
{

	if (Status==GRPF_Folder)
		return Error("SetFilePtr not implemented for Folders");

	// ensure mother is at correct pos
	if (Mother) Mother->EnsureChildFilePtr(this);

	// Rewind if necessary
	if (FilePtr>iOffset)
		if (!RewindFilePtr()) return false;

	// Advance to target pointer
	if (FilePtr<iOffset)
		if (!AdvanceFilePtr(iOffset-FilePtr)) return false;

	return true;
}

bool C4Group::Advance(int iOffset)
{
	assert(iOffset >= 0);
	// cached advance
	if (pInMemEntry)
	{
		if (iInMemEntrySize < size_t(iOffset)) return false;
		iInMemEntrySize -= iOffset;
		pInMemEntry += iOffset;
		return true;
	}
	// uncached advance
	if (Status == GRPF_Folder) return !!StdFile.Advance(iOffset);
	// FIXME: reading the file one byte at a time sounds just slow.
	BYTE buf;
	for (; iOffset>0; iOffset--)
		if (!Read(&buf,1)) return false;
	return true;
}

bool C4Group::Read(void *pBuffer, size_t iSize)
{
	// Access cached entry from memory?
	if (pInMemEntry)
	{
		if (iInMemEntrySize < iSize) return Error("ReadCached:");
		memcpy(pBuffer, pInMemEntry, iSize);
		iInMemEntrySize -= iSize;
		pInMemEntry += iSize;
		return true;
	}
	// Not cached. Read from file.
	switch (Status)
	{
	case GRPF_File:
		// Child group: read from mother group
		if (Mother)
		{
			if (!Mother->Read(pBuffer,iSize))
				{ RewindFilePtr(); return Error("Read:"); }
		}
		// Regular group: read from standard file
		else
		{
			if (!StdFile.Read(pBuffer,iSize))
				{ RewindFilePtr(); return Error("Read:"); }
		}
		FilePtr+=iSize;
		break;
	case GRPF_Folder:
		if (!StdFile.Read(pBuffer,iSize)) return Error("Read: Error reading from folder contents");
		break;
	default: break; // InGrp & Deleted ignored
	}

	return true;
}

bool C4Group::AdvanceFilePtr(int iOffset, C4Group *pByChild)
{
	// Child group file: pass command to mother
	if ((Status==GRPF_File) && Mother)
	{

		// Ensure mother file ptr for it may have been moved by foreign access to mother
		if (!Mother->EnsureChildFilePtr(this))
			return false;

		if (!Mother->AdvanceFilePtr(iOffset,this))
			return false;

	}
	// Regular group
	else if (Status==GRPF_File)
	{
		if (!StdFile.Advance(iOffset))
			return false;
	}
	// Open folder
	else
	{
		if (!StdFile.Advance(iOffset))
			return false;
	}

	// Advanced
	FilePtr+=iOffset;

	return true;
}

bool C4Group::RewindFilePtr()
{

#ifdef _DEBUG
	if (szCurrAccessedEntry && !iC4GroupRewindFilePtrNoWarn)
	{
		LogF("C4Group::RewindFilePtr() for %s (%s) after %s", szCurrAccessedEntry ? szCurrAccessedEntry : "???", FileName, sPrevAccessedEntry.getLength() ? sPrevAccessedEntry.getData() : "???");
		szCurrAccessedEntry=nullptr;
	}
#endif

	// Child group file: pass command to mother
	if ((Status==GRPF_File) && Mother)
	{
		if (!Mother->SetFilePtr2Entry(FileName,true)) // Set to group file start
			return false;
		if (!Mother->AdvanceFilePtr(EntryOffset,this)) // Advance data offset
			return false;
	}
	// Regular group or open folder: rewind standard file
	else
	{
		if (!StdFile.Rewind()) // Set to group file start
			return false;
		if (!StdFile.Advance(EntryOffset)) // Advance data offset
			return false;
	}

	FilePtr=0;

	return true;
}

bool C4Group::Merge(const char *szFolders)
{
	bool fMove = true;

	if (StdOutput) printf("%s...\n",fMove ? "Moving" : "Adding");

	// Add files & directories
	char szFileName[_MAX_FNAME+1];
	int iFileCount = 0;
	DirectoryIterator i;

	// Process segmented path & search wildcards
	char cSeparator = (SCharCount(';', szFolders) ? ';' : '|');
	for (int cseg=0; SCopySegment(szFolders, cseg, szFileName, cSeparator); cseg++)
	{
		i.Reset(szFileName);
		while (*i)
		{
			// File count
			iFileCount++;
			// Process output & callback
			if (StdOutput) printf("%s\n",GetFilename(*i));
			if (fnProcessCallback)
				fnProcessCallback(GetFilename(*i),0); // cbytes/tbytes
			// AddEntryOnDisk
			AddEntryOnDisk(*i, nullptr, fMove);
			++i;
		}
	}

	if (StdOutput) printf("%d file(s) %s.\n",iFileCount,fMove ? "moved" : "added");

	return true;
}

bool C4Group::AddEntryOnDisk(const char *szFilename,
                             const char *szAddAs,
                             bool fMove)
{

	// Do not process yourself
	if (ItemIdentical(szFilename,FileName)) return true;

	// File is a directory: copy to temp path, pack, and add packed file
	if (DirectoryExists(szFilename))
	{
		// Ignore
		if (C4Group_TestIgnore(szFilename)) return true;
		// Temp filename
		char szTempFilename[_MAX_PATH+1];
		if (C4Group_TempPath[0]) { SCopy(C4Group_TempPath,szTempFilename,_MAX_PATH); SAppend(GetFilename(szFilename),szTempFilename,_MAX_PATH); }
		else SCopy(szFilename,szTempFilename,_MAX_PATH);
		MakeTempFilename(szTempFilename);
		// Copy or move item to temp file (moved items might be killed if later process fails)
		if (fMove) { if (!MoveItem(szFilename,szTempFilename)) return Error("AddEntryOnDisk: Move failure"); }
		else { if (!CopyItem(szFilename,szTempFilename)) return Error("AddEntryOnDisk: Copy failure"); }
		// Pack temp file
		if (!C4Group_PackDirectory(szTempFilename)) return Error("AddEntryOnDisk: Pack directory failure");
		// Add temp file
		if (!szAddAs) szAddAs = GetFilename(szFilename);
		szFilename = szTempFilename;
		fMove = true;
	}

	// Determine size
	bool fIsGroup = !!C4Group_IsGroup(szFilename);
	int iSize = fIsGroup ? UncompressedFileSize(szFilename) : FileSize(szFilename);

	// Determine executable bit (linux only)
	bool fExecutable = false;
#ifdef __linux__
	fExecutable = (access(szFilename, X_OK) == 0);
#endif

	// AddEntry
	return AddEntry(C4GroupEntry::C4GRES_OnDisk,
	                fIsGroup,
	                szFilename,
	                iSize,
	                szAddAs,
	                nullptr,
	                fMove,
	                false,
	                fExecutable);

}

bool C4Group::Add(const char *szFile, const char *szAddAs)
{
	bool fMove = false;

	if (StdOutput) printf("%s %s as %s...\n",fMove ? "Moving" : "Adding",GetFilename(szFile),szAddAs);

	return AddEntryOnDisk(szFile, szAddAs, fMove);
}

bool C4Group::Move(const char *szFile, const char *szAddAs)
{
	bool fMove = true;

	if (StdOutput) printf("%s %s as %s...\n",fMove ? "Moving" : "Adding",GetFilename(szFile),szAddAs);

	return AddEntryOnDisk(szFile, szAddAs, fMove);
}

bool C4Group::Delete(const char *szFiles, bool fRecursive)
{
	int fcount = 0;
	C4GroupEntry *tentry;

	// Segmented file specs
	if (SCharCount(';', szFiles) || SCharCount('|', szFiles))
	{
		char cSeparator = (SCharCount(';', szFiles) ? ';' : '|');
		bool success = true;
		char filespec[_MAX_FNAME+1];
		for (int cseg = 0; SCopySegment(szFiles, cseg, filespec, cSeparator, _MAX_FNAME); cseg++)
			if (!Delete(filespec, fRecursive))
				success=false;
		return success; // Would be nicer to return the file count and add up all counts from recursive actions...
	}

	// Delete all matching Entries
	ResetSearch();
	while ((tentry = SearchNextEntry(szFiles)))
	{
		// StdOutput
		if (StdOutput) printf("%s\n",tentry->FileName);
		if (!DeleteEntry(tentry->FileName))
			return Error("Delete: Could not delete entry");
		fcount++;
	}

	// Recursive: process sub groups
	if (fRecursive)
	{
		C4Group hChild;
		ResetSearch();
		while ((tentry = SearchNextEntry("*")))
			if (tentry->ChildGroup)
				if (hChild.OpenAsChild(this, tentry->FileName))
				{
					hChild.SetStdOutput(StdOutput);
					hChild.Delete(szFiles, fRecursive);
					hChild.Close();
				}
	}

	// StdOutput
	if (StdOutput)
		printf("%d file(s) deleted.\n",fcount);

	return true; // Would be nicer to return the file count and add up all counts from recursive actions...
}

bool C4Group::DeleteEntry(const char *szFilename, bool fRecycle)
{
	switch (Status)
	{
	case GRPF_File:
		// Get entry
		C4GroupEntry *pEntry;
		if (!(pEntry=GetEntry(szFilename))) return false;
		// Delete moved source files
		if (pEntry->Status == C4GroupEntry::C4GRES_OnDisk)
			if (pEntry->DeleteOnDisk)
			{
				EraseItem(pEntry->DiskPath);
			}
		// (moved buffers are deleted by ~C4GroupEntry)
		// Delete status and update virtual file count
		pEntry->Status = C4GroupEntry::C4GRES_Deleted;
		Head.Entries--;
		break;
	case GRPF_Folder:
		StdFile.Close();
		char szPath[_MAX_FNAME+1];
		sprintf(szPath,"%s%c%s",FileName,DirectorySeparator,szFilename);

		if (fRecycle)
		{
			if (!EraseItemSafe(szPath)) return false;
		}
		else
		{
			if (!EraseItem(szPath)) return false;
		}
		break;
		// refresh file list
		ResetSearch(true);
	default: break; // InGrp & Deleted ignored
	}
	return true;
}

bool C4Group::Rename(const char *szFile, const char *szNewName)
{

	if (StdOutput) printf("Renaming %s to %s...\n",szFile,szNewName);

	switch (Status)
	{
	case GRPF_File:
		// Get entry
		C4GroupEntry *pEntry;
		if (!(pEntry=GetEntry(szFile))) return Error("Rename: File not found");
		// Check double name
		if (GetEntry(szNewName) && !SEqualNoCase(szNewName, szFile)) return Error("Rename: File exists already");
		// Rename
		SCopy(szNewName,pEntry->FileName,_MAX_FNAME);
		Modified=true;
		break;
	case GRPF_Folder:
		StdFile.Close();
		char path[_MAX_FNAME+1]; SCopy(FileName,path,_MAX_PATH-1);
		AppendBackslash(path); SAppend(szFile,path,_MAX_PATH);
		char path2[_MAX_FNAME+1]; SCopy(FileName,path2,_MAX_PATH-1);
		AppendBackslash(path2); SAppend(szNewName,path2,_MAX_PATH);
		if (!RenameFile(path,path2)) return Error("Rename: Failure");
		// refresh file list
		ResetSearch(true);
		break;
	default: break; // InGrp & Deleted ignored
	}

	return true;
}

bool C4Group_IsExcluded(const char *szFile, const char *szExcludeList)
{
	// No file or no exclude list
	if (!szFile || !szFile[0] || !szExcludeList || !szExcludeList[0]) return false;
	// Process segmented exclude list
	char cSeparator = (SCharCount(';', szExcludeList) ? ';' : '|');
	char szSegment[_MAX_PATH + 1];
	for (int i = 0; SCopySegment(szExcludeList, i, szSegment, cSeparator); i++)
		if (WildcardMatch(szSegment, GetFilename(szFile)))
			return true;
	// No match
	return false;
}

bool C4Group::Extract(const char *szFiles, const char *szExtractTo, const char *szExclude)
{

	// StdOutput
	if (StdOutput)
	{
		printf("Extracting");
		if (szExtractTo) printf(" to %s",szExtractTo);
		printf("...\n");
	}

	int fcount=0;
	int cbytes,tbytes;
	C4GroupEntry *tentry;

	cbytes=0; tbytes=EntrySize();

	// Process segmented list
	char cSeparator = (SCharCount(';', szFiles) ? ';' : '|');
	char szFileName[_MAX_PATH + 1];
	for (int cseg=0; SCopySegment(szFiles, cseg, szFileName, cSeparator); cseg++)
	{
		// Search all entries
		ResetSearch();
		while ((tentry = SearchNextEntry(szFileName)))
		{
			// skip?
			if (C4Group_IsExcluded(tentry->FileName, szExclude)) continue;
			// Process data & output
			if (StdOutput) printf("%s\n",tentry->FileName);
			cbytes+=tentry->Size;
			if (fnProcessCallback)
				fnProcessCallback(tentry->FileName,100*cbytes/std::max(tbytes,1));

			// Extract
			if (!ExtractEntry(tentry->FileName,szExtractTo))
				return Error("Extract: Could not extract entry");

			fcount++;
		}
	}

	if (StdOutput) printf("%d file(s) extracted.\n",fcount);

	return true;
}

bool C4Group::ExtractEntry(const char *szFilename, const char *szExtractTo)
{
	CStdFile tfile;
	CStdFile hDummy;
	char szTempFName[_MAX_FNAME+1],szTargetFName[_MAX_FNAME+1];

	// Target file name
	if (szExtractTo)
	{
		SCopy(szExtractTo,szTargetFName,_MAX_FNAME-1);
		if (DirectoryExists(szTargetFName))
		{
			AppendBackslash(szTargetFName);
			SAppend(szFilename,szTargetFName,_MAX_FNAME);
		}
	}
	else
		SCopy(szFilename,szTargetFName,_MAX_FNAME);

	// Extract
	switch (Status)
	{
	case GRPF_File: // Copy entry to target
		// Get entry
		C4GroupEntry *pEntry;
		if (!(pEntry=GetEntry(szFilename))) return Error("Extract: Entry not found");
		// Create dummy file to reserve target file name
		hDummy.Create(szTargetFName,false);
		hDummy.Write("Dummy",5);
		hDummy.Close();
		// Make temp target file name
		SCopy(szTargetFName,szTempFName,_MAX_FNAME);
		MakeTempFilename(szTempFName);
		// Create temp target file
		if (!tfile.Create(szTempFName, !!pEntry->ChildGroup, !!pEntry->Executable))
			return Error("Extract: Cannot create target file");
		// Write entry file to temp target file
		if (!AppendEntry2StdFile(pEntry,tfile))
		{
			// Failure: close and erase temp target file
			tfile.Close();
			EraseItem(szTempFName);
			// Also erase reservation target file
			EraseItem(szTargetFName);
			// Failure
			return false;
		}
		// Close target file
		tfile.Close();
		// Make temp file to original file
		if (!EraseItem(szTargetFName))
			return Error("Extract: Cannot erase temporary file");
		if (!RenameItem(szTempFName,szTargetFName))
			return Error("Extract: Cannot rename temporary file");
		break;
	case GRPF_Folder: // Copy item from folder to target
		char szPath[_MAX_FNAME+1];
		sprintf(szPath,"%s%c%s",FileName,DirectorySeparator,szFilename);
		if (!CopyItem(szPath,szTargetFName))
			return Error("ExtractEntry: Cannot copy item");
		break;
	default: break; // InGrp & Deleted ignored
	}
	return true;
}


bool C4Group::OpenAsChild(C4Group *pMother,
                          const char *szEntryName, bool fExclusive, bool fCreate)
{

	if (!pMother) return Error("OpenAsChild: No mother specified");

	if (SCharCount('*',szEntryName)) return Error("OpenAsChild: No wildcards allowed");

	// Open nested child group check: If szEntryName is a reference to
	// a nested group, open the first mother (in specified mode), then open the child
	// in exclusive mode

	if (SCharCount(DirectorySeparator,szEntryName))
	{
		char mothername[_MAX_FNAME+1];
		SCopyUntil(szEntryName,mothername,DirectorySeparator,_MAX_FNAME);

		C4Group *pMother2;
		pMother2 = new C4Group;
		pMother2->SetStdOutput(StdOutput);
		if (!pMother2->OpenAsChild(pMother, mothername, fExclusive))
		{
			delete pMother2;
			return Error("OpenAsChild: Cannot open mother");
		}
		return OpenAsChild(pMother2, szEntryName + SLen(mothername) + 1, true);
	}

	// Init
	Init();
	SCopy(szEntryName,FileName,_MAX_FNAME);
	Mother=pMother;
	ExclusiveChild=fExclusive;

	// Folder: Simply set status and return
	char path[_MAX_FNAME+1];
	SCopy( GetFullName().getData(), path, _MAX_FNAME);
	if (DirectoryExists(path))
	{
		SCopy(path,FileName, _MAX_FNAME);
		Status=GRPF_Folder;
		ResetSearch();
		return true;
	}

	// Get original entry name
	C4GroupEntry *centry;
	if ((centry = Mother->GetEntry(FileName)))
		SCopy(centry->FileName,FileName,_MAX_PATH);

	// Access entry in mother group
	size_t iSize;
	if ((!Mother->AccessEntry(FileName, &iSize, nullptr, true)))
	{
		if (!fCreate)
			{ CloseExclusiveMother(); Clear(); return Error("OpenAsChild: Entry not in mother group"); }
		else
		{
			// Create - will be added to mother in Close()
			Status=GRPF_File; Modified=true;
			return true;
		}
	}

	// Child Group?
	if (centry && !centry->ChildGroup)
		{ CloseExclusiveMother(); Clear(); return Error("OpenAsChild: Is not a child group"); }

	// Read header
	// Do not do size checks for packed subgroups of unpacked groups (there will be no entry),
	//  because that would be the PACKED size which can actually be smaller than sizeof(C4GroupHeader)!
	if (iSize < sizeof(C4GroupHeader) && centry)
		{ CloseExclusiveMother(); Clear(); return Error("OpenAsChild: Entry too small"); }
	if (!Mother->Read(&Head,sizeof(C4GroupHeader)))
		{ CloseExclusiveMother(); Clear(); return Error("OpenAsChild: Entry reading error"); }
	MemScramble((BYTE*)&Head,sizeof(C4GroupHeader));
	EntryOffset+=sizeof(C4GroupHeader);

	// Check Header
	if (!SEqual(Head.id,C4GroupFileID)
	    || (Head.Ver1!=C4GroupFileVer1) || (Head.Ver2>C4GroupFileVer2))
		{ CloseExclusiveMother(); Clear(); return Error("OpenAsChild: Invalid Header"); }

	// Read Entries
	C4GroupEntryCore corebuf;
	int file_entries=Head.Entries;
	Head.Entries=0; // Reset, will be recounted by AddEntry
	for (int cnt=0; cnt<file_entries; cnt++)
	{
		if (!Mother->Read(&corebuf,sizeof(C4GroupEntryCore)))
			{ CloseExclusiveMother(); Clear(); return Error("OpenAsChild: Entry reading error"); }
		EntryOffset+=sizeof(C4GroupEntryCore);
		if (!AddEntry(C4GroupEntry::C4GRES_InGroup, !!corebuf.ChildGroup,
		              corebuf.FileName,corebuf.Size,
		              nullptr, nullptr, false, false,
		              !!corebuf.Executable))
			{ CloseExclusiveMother(); Clear(); return Error("OpenAsChild: Insufficient memory"); }
	}

	ResetSearch();

	// File
	Status=GRPF_File;

	// save position in mother group
	if (centry) MotherOffset = centry->Offset;

	return true;
}

bool C4Group::AccessEntry(const char *szWildCard,
                          size_t *iSize, char *sFileName,
                          bool NeedsToBeAGroup)
{
#ifdef C4GROUP_DUMP_ACCESS
	LogF("Group access in %s: %s", GetFullName().getData(), szWildCard);
#endif
	StdStrBuf fname;
	if (!FindEntry(szWildCard,&fname,&iCurrFileSize))
		return false;
#ifdef _DEBUG
	szCurrAccessedEntry = fname.getMData();
#endif
	bool fResult = SetFilePtr2Entry(fname.getData(), NeedsToBeAGroup);
#ifdef _DEBUG
	sPrevAccessedEntry.Copy(szCurrAccessedEntry);
	szCurrAccessedEntry = nullptr;
#endif
	if (!fResult) return false;
	if (sFileName) SCopy(fname.getData(),sFileName);
	if (iSize) *iSize=iCurrFileSize;
	return true;
}

bool C4Group::AccessNextEntry(const char *szWildCard,
                              size_t *iSize, char *sFileName,
                              bool fStartAtFilename)
{
	char fname[_MAX_FNAME+1];
	if (!FindNextEntry(szWildCard,fname,&iCurrFileSize,fStartAtFilename)) return false;
#ifdef _DEBUG
	szCurrAccessedEntry = fname;
#endif
	bool fResult = SetFilePtr2Entry(fname);
#ifdef _DEBUG
	szCurrAccessedEntry = nullptr;
#endif
	if (!fResult) return false;
	if (sFileName) SCopy(fname,sFileName);
	if (iSize) *iSize=iCurrFileSize;
	return true;
}

bool C4Group::SetFilePtr2Entry(const char *szName, bool NeedsToBeAGroup)
{
	C4GroupEntry *centry = GetEntry(szName);
	// Read cached entries directly from memory (except child groups. that is not supported.)
	if (centry && centry->bpMemBuf && !NeedsToBeAGroup)
	{
		pInMemEntry = centry->bpMemBuf;
		iInMemEntrySize = centry->Size;
		return true;
	}
	else
	{
		pInMemEntry = nullptr;
	}

	// Not cached. Access from disk.
	switch (Status)
	{

	case GRPF_File:
		if ((!centry) || (centry->Status != C4GroupEntry::C4GRES_InGroup)) return false;
		return SetFilePtr(centry->Offset);

	case GRPF_Folder: {
		StdFile.Close();
		char path[_MAX_FNAME+1]; SCopy(FileName,path,_MAX_FNAME);
		AppendBackslash(path); SAppend(szName,path);
		bool fSuccess = StdFile.Open(path, NeedsToBeAGroup);
		return fSuccess;
	}

	default: break; // InGrp & Deleted ignored
	}
	return false;
}

bool C4Group::FindEntry(const char *szWildCard, StdStrBuf *sFileName, size_t *iSize)
{
	ResetSearch();
	return FindNextEntry(szWildCard,sFileName,iSize);
}

bool C4Group::FindNextEntry(const char *szWildCard,
                            StdStrBuf *sFileName,
                            size_t *iSize,
                            bool fStartAtFilename)
{
	C4GroupEntry *centry;
	if (!szWildCard) return false;

	// Reset search to specified position
	if (fStartAtFilename) FindEntry(sFileName->getData());

	if (!(centry=SearchNextEntry(szWildCard))) return false;
	if (sFileName) sFileName->Copy(centry->FileName);
	if (iSize) *iSize=centry->Size;
	return true;
}

bool C4Group::Add(const char *szName, void *pBuffer, int iSize, bool fChild, bool fHoldBuffer, bool fExecutable)
{
	return AddEntry(C4GroupEntry::C4GRES_InMemory,
	                fChild,
	                szName,
	                iSize,
	                szName,
	                (BYTE*) pBuffer,
	                false,
	                fHoldBuffer,
	                fExecutable);
}

bool C4Group::Add(const char *szName, StdBuf &pBuffer, bool fChild, bool fHoldBuffer, bool fExecutable)
{
	if (!AddEntry(C4GroupEntry::C4GRES_InMemory,
	              fChild,
	              szName,
	              pBuffer.getSize(),
	              szName,
	              (BYTE*) pBuffer.getMData(),
	              false,
	              fHoldBuffer,
	              fExecutable,
	              true)) return false;
	// Pointer is now owned and released by C4Group!
	if (fHoldBuffer) pBuffer.GrabPointer();
	return true;
}

bool C4Group::Add(const char *szName, StdStrBuf &pBuffer, bool fChild, bool fHoldBuffer, bool fExecutable)
{
	if (!AddEntry(C4GroupEntry::C4GRES_InMemory,
	              fChild,
	              szName,
	              pBuffer.getLength(),
	              szName,
	              (BYTE*) pBuffer.getMData(),
	              false,
	              fHoldBuffer,
	              fExecutable,
	              true)) return false;
	// Pointer is now owned and released by C4Group!
	if (fHoldBuffer) pBuffer.GrabPointer();
	return true;
}


const char* C4Group::GetName()
{
	return FileName;
}

int C4Group::EntryCount(const char *szWildCard)
{
	int fcount;
	C4GroupEntry *tentry;
	// All files if no wildcard
	if (!szWildCard) szWildCard="*";
	// Match wildcard
	ResetSearch(); fcount=0;
	while ((tentry=SearchNextEntry(szWildCard))) fcount++;
	return fcount;
}

size_t C4Group::EntrySize(const char *szWildCard)
{
	int fsize;
	C4GroupEntry *tentry;
	// All files if no wildcard
	if (!szWildCard) szWildCard="*";
	// Match wildcard
	ResetSearch(); fsize=0;
	while ((tentry=SearchNextEntry(szWildCard)))
		fsize+=tentry->Size;
	return fsize;
}

unsigned int C4Group::EntryCRC32(const char *szWildCard)
{
	if (!szWildCard) szWildCard="*";
	// iterate thorugh child
	C4GroupEntry *pEntry; unsigned int iCRC = 0;
	ResetSearch();
	while ((pEntry = SearchNextEntry(szWildCard)))
	{
		iCRC ^= CalcCRC32(pEntry);
	}
	// return
	return iCRC;
}

bool C4Group::LoadEntry(const char *szEntryName, char **lpbpBuf, size_t *ipSize, int iAppendZeros)
{
	size_t size;

	// Access entry, allocate buffer, read data
	(*lpbpBuf)=nullptr; if (ipSize) *ipSize=0;
	if (!AccessEntry(szEntryName,&size)) return Error("LoadEntry: Not found");
	*lpbpBuf = new char[size+iAppendZeros];
	if (!Read(*lpbpBuf,size))
	{
		delete [] (*lpbpBuf); *lpbpBuf = nullptr;
		return Error("LoadEntry: Reading error");
	}

	if (ipSize) *ipSize=size;

	if (iAppendZeros)
		ZeroMem( (*lpbpBuf)+size, iAppendZeros );

	return true;
}

bool C4Group::LoadEntry(const char *szEntryName, StdBuf * Buf)
{
	size_t size;
	// Access entry, allocate buffer, read data
	if (!AccessEntry(szEntryName,&size)) return Error("LoadEntry: Not found");
	// Allocate memory
	Buf->New(size);
	// Load data
	if (!Read(Buf->getMData(),size))
	{
		Buf->Clear();
		return Error("LoadEntry: Reading error");
	}
	// ok
	return true;
}

bool C4Group::LoadEntryString(const char *szEntryName, StdStrBuf *Buf)
{
	size_t size;
	// Access entry, allocate buffer, read data
	if (!AccessEntry(szEntryName,&size)) return Error("LoadEntry: Not found");
	// Allocate memory
	Buf->SetLength(size);
	// other parts crash when they get a zero length buffer, so fail here
	if (!size) return false;
	// Load data
	if (!Read(Buf->getMData(),size))
	{
		Buf->Clear();
		return Error("LoadEntry: Reading error");
	}
	// ok
	return true;
}

int SortRank(const char *szElement, const char *szSortList)
{
	int cnt;
	char csegment[_MAX_FNAME+1];

	for (cnt=0; SCopySegment(szSortList,cnt,csegment,'|',_MAX_FNAME); cnt++)
		if (WildcardMatch(csegment,szElement))
			return (SCharCount('|',szSortList)+1)-cnt;

	return 0;
}

bool C4Group::Sort(const char *szSortList)
{
	bool fBubble;
	C4GroupEntry *centry,*prev,*next,*nextnext;

	if (!szSortList || !szSortList[0]) return false;

	if (StdOutput) printf("Sorting...\n");

	do
	{
		fBubble=false;

		for (prev=nullptr,centry=FirstEntry; centry; prev=centry,centry=next)
			if ((next=centry->Next))
			{
				// primary sort by file list
				int iS1 = SortRank(centry->FileName,szSortList);
				int iS2 = SortRank(next->FileName,szSortList);
				if (iS1 > iS2) continue;
				// secondary sort by filename
				if (iS1 == iS2)
					if (stricmp(centry->FileName, next->FileName) <= 0) continue;
				// wrong order: Swap!
				nextnext=next->Next;
				if (prev) prev->Next=next;
				else FirstEntry=next;
				next->Next=centry;
				centry->Next=nextnext;
				next=nextnext;

				fBubble=true;
				Modified=true;
			}

	}
	while (fBubble);

	return true;
}

C4Group* C4Group::GetMother()
{
	return Mother;
}

bool C4Group::CloseExclusiveMother()
{
	if (Mother && ExclusiveChild)
	{
		Mother->Close();
		delete Mother;
		Mother=nullptr;
		return true;
	}
	return false;
}

bool C4Group::SortByList(const char **ppSortList, const char *szFilename)
{
	// No sort list specified
	if (!ppSortList) return false;
	// No group name specified, use own
	if (!szFilename) szFilename = FileName;
	szFilename = GetFilename(szFilename);
	// Find matching filename entry in sort list
	const char **ppListEntry;
	for (ppListEntry = ppSortList; *ppListEntry; ppListEntry+=2)
		if (WildcardMatch( *ppListEntry, szFilename ))
			break;
	// Sort by sort list entry
	if (*ppListEntry && *(ppListEntry+1))
		Sort(*(ppListEntry+1));
	// Success
	return true;
}

void C4Group::ProcessOut(const char *szMessage, int iProcess)
{
	if (fnProcessCallback) fnProcessCallback(szMessage,iProcess);
	if (C4Group_ProcessCallback) C4Group_ProcessCallback(szMessage,iProcess);
}

bool C4Group::EnsureChildFilePtr(C4Group *pChild)
{

	// group file
	if (Status == GRPF_File)
	{
		// check if FilePtr has to be moved
		if (FilePtr != pChild->MotherOffset + pChild->EntryOffset +  pChild->FilePtr)
			// move it to the position the child thinks it is
			if (!SetFilePtr(pChild->MotherOffset + pChild->EntryOffset +  pChild->FilePtr))
				return false;
		// ok
		return true;
	}

	// Open standard file is not the child file     ...or StdFile ptr does not match pChild->FilePtr
	char szChildPath[_MAX_PATH+1]; sprintf(szChildPath,"%s%c%s",FileName,DirectorySeparator,GetFilename(pChild->FileName));
	if ( !ItemIdentical( StdFile.Name, szChildPath ) )
	{
		// Reopen correct child stdfile
		if ( !SetFilePtr2Entry( GetFilename(pChild->FileName), true ) )
			return false;
		// Advance to child's old file ptr
		if ( !AdvanceFilePtr( pChild->EntryOffset + pChild->FilePtr ) )
			return false;
	}

	// Looks okay
	return true;

}

StdStrBuf C4Group::GetFullName() const
{
	char str[_MAX_PATH+1]; *str='\0';
	char sep[] = "/"; sep[0] = DirectorySeparator;
	for (const C4Group *pGroup=this; pGroup; pGroup=pGroup->Mother)
	{
		if (*str) SInsert(str, sep, 0, _MAX_PATH);
		// Avoid double slash
		if (pGroup == this || SLen(pGroup->FileName) > 1 || pGroup->FileName[0] != '/')
			SInsert(str, pGroup->FileName, 0, _MAX_PATH);
		if (pGroup->Status == GRPF_Folder) break; // Folder is assumed to have full path
	}
	StdStrBuf sResult; sResult.Copy(str);
	return sResult;
}

uint32_t C4Group::CalcCRC32(C4GroupEntry *pEntry)
{
	uint32_t CRC;
	// child group?
	if (pEntry->ChildGroup || (pEntry->Status == C4GroupEntry::C4GRES_OnDisk && (DirectoryExists(pEntry->DiskPath) || C4Group_IsGroup(pEntry->DiskPath))))
	{
		// open
		C4Group Child;
		switch (pEntry->Status)
		{
		case C4GroupEntry::C4GRES_InGroup:
			if (!Child.OpenAsChild(this, pEntry->FileName))
				return 0;
			break;
		case C4GroupEntry::C4GRES_OnDisk:
			if (!Child.Open(pEntry->DiskPath))
				return 0;
			break;
		default:
			return 0;
		}
		// get checksum
		CRC = Child.EntryCRC32();
	}
	else if (!pEntry->Size)
		CRC = 0;
	else
	{
		BYTE *pData = nullptr; bool fOwnData; CStdFile f;
		// get data
		switch (pEntry->Status)
		{
		case C4GroupEntry::C4GRES_InGroup:
			// create buffer
			pData = new BYTE [pEntry->Size]; fOwnData = true;
			// go to entry
			if (!SetFilePtr2Entry(pEntry->FileName)) { delete [] pData; return false; }
			// read
			if (!Read(pData, pEntry->Size)) { delete [] pData; return false; }
			break;
		case C4GroupEntry::C4GRES_OnDisk:
			// create buffer
			pData = new BYTE [pEntry->Size]; fOwnData = true;
			// open
			if (!f.Open(pEntry->DiskPath)) { delete [] pData; return false; }
			// read
			if (!f.Read(pData, pEntry->Size)) { delete [] pData; return false; }
			break;
		case C4GroupEntry::C4GRES_InMemory:
			// set
			pData = pEntry->bpMemBuf; fOwnData = false;
			break;
		default:
			return false;
		}
		if (!pData) return false;
		// calc crc
		CRC = crc32(0, pData, pEntry->Size);
		// discard buffer
		if (fOwnData) delete [] pData;
		// add file name
		CRC = crc32(CRC, reinterpret_cast<BYTE *>(pEntry->FileName), SLen(pEntry->FileName));
	}
	// ok
	return CRC;
}

bool C4Group::OpenChild(const char* strEntry)
{
	// hack: The seach-handle would be closed twice otherwise
	FolderSearch.Reset();
	// Create a memory copy of ourselves
	C4Group *pOurselves = new C4Group;
	*pOurselves = *this;

	// Open a child from the memory copy
	C4Group hChild;
	if (!hChild.OpenAsChild(pOurselves, strEntry, false))
	{
		// Silently delete our memory copy
		pOurselves->Default(); delete pOurselves;
		return false;
	}

	// hack: The seach-handle would be closed twice otherwise
	FolderSearch.Reset();
	hChild.FolderSearch.Reset();

	// We now become our own child
	*this = hChild;

	// Make ourselves exclusive (until we hit our memory copy parent)
	for (C4Group *pGroup = this; pGroup != pOurselves; pGroup = pGroup->Mother)
		pGroup->ExclusiveChild = true;

	// Reset the temporary child variable so it doesn't delete anything
	hChild.Default();

	// Yeehaw
	return true;
}

bool C4Group::OpenMother()
{
	// This only works if we are an exclusive child
	if (!Mother || !ExclusiveChild) return false;

	// Store a pointer to our mother
	C4Group *pMother = Mother;

	// Clear ourselves without deleting our mother
	ExclusiveChild = false;
	Clear();

	// hack: The seach-handle would be closed twice otherwise
	pMother->FolderSearch.Reset();
	FolderSearch.Reset();
	// We now become our own mother (whoa!)
	*this = *pMother;

	// Now silently delete our former mother
	pMother->Default();
	delete pMother;

	// Yeehaw
	return true;
}

int C4Group::PreCacheEntries(const char *szSearchPattern, bool cache_previous)
{
	assert(szSearchPattern);
	int result = 0;
	// pre-load entries to memory. return number of loaded entries.
	for (C4GroupEntry * p = FirstEntry; p; p = p->Next)
	{
		// is this to be cached?
		if (!WildcardListMatch(szSearchPattern, p->FileName)) continue;
		// if desired, cache all entries up to that one to allow rewind in unpacked memory
		// (only makes sense for groups)
		if (cache_previous && Status == GRPF_File)
		{
			for (C4GroupEntry * p_pre = FirstEntry; p_pre != p; p_pre = p_pre->Next)
				if (p_pre->Offset >= FilePtr)
					PreCacheEntry(p_pre);
		}
		// cache the given entry
		PreCacheEntry(p);
	}
	return result;
}

void C4Group::PreCacheEntry(C4GroupEntry * p)
{
	// skip some stuff that can not be cached or has already been cached
	if (p->ChildGroup || p->bpMemBuf || !p->Size) return;
	// now load it!
	StdBuf buf;
	if (!this->LoadEntry(p->FileName, &buf)) return;
	p->HoldBuffer = true;
	p->BufferIsStdbuf = true;
	p->Size = buf.getSize(); // update size in case group changed on disk between calls
	p->bpMemBuf = static_cast<BYTE *>(buf.GrabPointer());
}
