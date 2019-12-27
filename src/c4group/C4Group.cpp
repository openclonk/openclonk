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

void C4Group_SetProcessCallback(bool (*callback)(const char *, int))
{
	C4Group_ProcessCallback = callback;
}

void C4Group_SetSortList(const char **sort_list)
{
	C4Group_SortList = sort_list;
}

void C4Group_SetTempPath(const char *path)
{
	if (!path || !path[0]) C4Group_TempPath[0]=0;
	else { SCopy(path,C4Group_TempPath,_MAX_PATH); AppendBackslash(C4Group_TempPath); }
}

const char *C4Group_GetTempPath()
{
	return C4Group_TempPath;
}

bool C4Group_TestIgnore(const char *filename)
{
	if(!*filename) return true; //poke out empty strings
	const char* name = GetFilename(filename);
	return *name == '.' //no hidden files and the directory itself
		|| name[strlen(name) - 1] == '~' //no temp files
		|| SIsModule(C4Group_Ignore,name); //not on Blacklist
}

bool C4Group_IsGroup(const char *filename)
{
	C4Group hGroup; if (hGroup.Open(filename))  { hGroup.Close(); return true; }
	return false;
}

bool C4Group_CopyItem(const char *source, const char *szTarget1, bool fNoSort, bool fResetAttributes)
{
	// Parameter check
	if (!source || !szTarget1 || !source[0] || !szTarget1[0]) return false;
	char target[_MAX_PATH+1]; SCopy(szTarget1,target,_MAX_PATH);

	// Backslash terminator indicates target is a path only (append filename)
	if (target[SLen(target)-1]==DirectorySeparator) SAppend(GetFilename(source),target);

	// Check for identical source and target
	// Note that attributes aren't reset here
	if (ItemIdentical(source,target)) return true;

	// Source and target are simple items
	if (ItemExists(source) && CreateItem(target)) return CopyItem(source,target, fResetAttributes);

	// For items within groups, attribute resetting isn't needed, because packing/unpacking will kill all
	// attributes anyway

	// Source & target
	C4Group hSourceParent, hTargetParent;
	char szSourceParentPath[_MAX_PATH+1],szTargetParentPath[_MAX_PATH+1];
	GetParentPath(source,szSourceParentPath); GetParentPath(target,szTargetParentPath);

	// Temp filename
	char szTempFilename[_MAX_PATH+1];
	SCopy(C4Group_TempPath,szTempFilename,_MAX_PATH);
	SAppend(GetFilename(source),szTempFilename);
	MakeTempFilename(szTempFilename);

	// Extract source to temp file
	if ( !hSourceParent.Open(szSourceParentPath)
	     || !hSourceParent.Extract(GetFilename(source),szTempFilename)
	     || !hSourceParent.Close() ) return false;

	// Move temp file to target
	if ( !hTargetParent.Open(szTargetParentPath)
	     || !hTargetParent.SetNoSort(fNoSort)
	     || !hTargetParent.Move(szTempFilename, GetFilename(target))
	     || !hTargetParent.Close() ) { EraseItem(szTempFilename); return false; }

	return true;
}

bool C4Group_MoveItem(const char *source, const char *szTarget1, bool fNoSort)
{
	// Parameter check
	if (!source || !szTarget1 || !source[0] || !szTarget1[0]) return false;
	char target[_MAX_PATH+1]; SCopy(szTarget1,target,_MAX_PATH);

	// Backslash terminator indicates target is a path only (append filename)
	if (target[SLen(target)-1]==DirectorySeparator) SAppend(GetFilename(source),target);

	// Check for identical source and target
	if (ItemIdentical(source,target)) return true;

	// Source and target are simple items
	if (ItemExists(source) && CreateItem(target))
	{
		// erase test file, because it may block moving a directory
		EraseItem(target);
		return MoveItem(source,target);
	}

	// Source & target
	C4Group hSourceParent, hTargetParent;
	char szSourceParentPath[_MAX_PATH+1],szTargetParentPath[_MAX_PATH+1];
	GetParentPath(source,szSourceParentPath); GetParentPath(target,szTargetParentPath);

	// Temp filename
	char szTempFilename[_MAX_PATH+1];
	SCopy(C4Group_TempPath,szTempFilename,_MAX_PATH);
	SAppend(GetFilename(source),szTempFilename);
	MakeTempFilename(szTempFilename);

	// Extract source to temp file
	if ( !hSourceParent.Open(szSourceParentPath)
	     || !hSourceParent.Extract(GetFilename(source),szTempFilename)
	     || !hSourceParent.Close() ) return false;

	// Move temp file to target
	if ( !hTargetParent.Open(szTargetParentPath)
	     || !hTargetParent.SetNoSort(fNoSort)
	     || !hTargetParent.Move(szTempFilename, GetFilename(target))
	     || !hTargetParent.Close() ) { EraseItem(szTempFilename); return false; }

	// Delete original file
	if ( !hSourceParent.Open(szSourceParentPath)
	     || !hSourceParent.DeleteEntry(GetFilename(source))
	     || !hSourceParent.Close() ) return false;

	return true;
}

bool C4Group_DeleteItem(const char *item_name, bool do_recycle)
{
	// Parameter check
	if (!item_name || !item_name[0]) return false;

	// simple item?
	if (ItemExists(item_name))
	{
		if (do_recycle)
			return EraseItemSafe(item_name);
		else
			return EraseItem(item_name);
	}

	// delete from mother
	C4Group hParent;
	char szParentPath[_MAX_PATH+1];
	GetParentPath(item_name,szParentPath);

	// Delete original file
	if ( !hParent.Open(szParentPath)
	     || !hParent.DeleteEntry(GetFilename(item_name), do_recycle)
	     || !hParent.Close() ) return false;

	return true;
}

bool C4Group_PackDirectoryTo(const char *filename, const char *to_filename)
{
	// Check file type
	if (!DirectoryExists(filename)) return false;
	// Target mustn't exist
	if (FileExists(to_filename)) return false;
	// Ignore
	if (C4Group_TestIgnore(filename))
		return true;
	// Process message
	if (C4Group_ProcessCallback)
		C4Group_ProcessCallback(filename,0);
	// Create group file
	C4Group hGroup;
	if (!hGroup.Open(to_filename,true))
		return false;
	// Add folder contents to group
	DirectoryIterator i(filename);
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
		EraseItem(to_filename);
		return false;
	}
	// Reset iterator
	i.Reset();
	// Close group
	hGroup.SortByList(C4Group_SortList,filename);
	if (!hGroup.Close())
		return false;
	// Done
	return true;
}

bool C4Group_PackDirectory(const char *filename)
{
	// Make temporary filename
	char szTempFilename[_MAX_PATH+1];
	SCopy(filename, szTempFilename, _MAX_PATH);
	MakeTempFilename(szTempFilename);
	// Pack directory
	if (!C4Group_PackDirectoryTo(filename, szTempFilename))
		return false;
	// Rename folder
	char szTempFilename2[_MAX_PATH+1];
	SCopy(filename, szTempFilename2, _MAX_PATH);
	MakeTempFilename(szTempFilename2);
	if (!RenameFile(filename, szTempFilename2))
		return false;
	// Name group file
	if (!RenameFile(szTempFilename,filename))
		return false;
	// Last: Delete folder
	return EraseDirectory(szTempFilename2);
}

bool C4Group_UnpackDirectory(const char *filename)
{
	// Already unpacked: success
	if (DirectoryExists(filename)) return true;

	// Not a real file: unpack mother directory first
	char szParentFilename[_MAX_PATH+1];
	if (!FileExists(filename))
		if (GetParentPath(filename,szParentFilename))
			if (!C4Group_UnpackDirectory(szParentFilename))
				return false;

	// Open group
	C4Group hGroup;
	if (!hGroup.Open(filename)) return false;

	// Process message
	if (C4Group_ProcessCallback)
		C4Group_ProcessCallback(filename,0);

	// Create target directory
	char szFoldername[_MAX_PATH+1];
	SCopy(filename,szFoldername,_MAX_PATH);
	MakeTempFilename(szFoldername);
	if (!CreatePath(szFoldername)) { hGroup.Close(); return false; }

	// Extract files to folder
	if (!hGroup.Extract("*",szFoldername)) { hGroup.Close(); return false; }

	// Close group
	hGroup.Close();

	// Rename group file
	char szTempFilename[_MAX_PATH+1];
	SCopy(filename,szTempFilename,_MAX_PATH);
	MakeTempFilename(szTempFilename);
	if (!RenameFile(filename, szTempFilename)) return false;

	// Rename target directory
	if (!RenameFile(szFoldername,filename)) return false;

	// Delete renamed group file
	return EraseItem(szTempFilename);
}

bool C4Group_ExplodeDirectory(const char *filename)
{
	// Ignore
	if (C4Group_TestIgnore(filename)) return true;

	// Unpack this directory
	if (!C4Group_UnpackDirectory(filename)) return false;

	// Explode all children
	ForEachFile(filename,C4Group_ExplodeDirectory);

	// Success
	return true;
}

bool C4Group_ReadFile(const char *filename, char **data, size_t *size)
{
	// security
	if (!filename || !data) return false;
	// get mother path & file name
	char path[_MAX_PATH + 1];
	GetParentPath(filename, path);
	const char *pFileName = GetFilename(filename);
	// open mother group
	C4Group MotherGroup;
	if (!MotherGroup.Open(path)) return false;
	// access the file
	size_t iFileSize;
	if (!MotherGroup.AccessEntry(pFileName, &iFileSize)) return false;
	// create buffer
	*data = new char [iFileSize];
	// read it
	if (!MotherGroup.Read(*data, iFileSize)) { delete [] *data; *data = nullptr; return false; }
	// ok
	MotherGroup.Close();
	if (size) *size = iFileSize;
	return true;
}

void MemScramble(BYTE *bypBuffer, int size)
{
	int cnt; BYTE temp;
	// XOR deface
	for (cnt=0; cnt<size; cnt++)
		bypBuffer[cnt] ^= 237;
	// BYTE swap
	for (cnt=0; cnt+2<size; cnt+=3)
	{
		temp = bypBuffer[cnt];
		bypBuffer[cnt] = bypBuffer[cnt+2];
		bypBuffer[cnt+2] = temp;
	}
}

//---------------------------------- C4Group ---------------------------------------------

struct C4Group::P
{
	enum SourceType
	{
		// No source; C4Group inactive
		ST_None,
		// C4Group backed by archive file
		ST_Packed,
		// C4Group backed by raw file system
		ST_Unpacked
	};

	SourceType SourceType = ST_None;
	std::string FileName;
	// Parent status
	C4Group *Mother = nullptr;
	bool ExclusiveChild = false;
	// File & Folder
	C4GroupEntry *SearchPtr = nullptr;
	CStdFile StdFile;
	size_t iCurrFileSize = 0; // size of last accessed file
						  // File only
	int FilePtr = 0;
	int MotherOffset = 0;
	int EntryOffset = 0;
	bool Modified = false;
	C4GroupEntry *FirstEntry = nullptr;
	BYTE *pInMemEntry = nullptr; size_t iInMemEntrySize = 0; // for reading from entries prefetched into memory
#ifdef _DEBUG
	StdStrBuf sPrevAccessedEntry;
#endif
	// Folder only
	DirectoryIterator FolderSearch;
	C4GroupEntry FolderSearchEntry;
	C4GroupEntry LastFolderSearchEntry;

	bool StdOutput = false;
	bool(*fnProcessCallback)(const char *, int) = nullptr;
	std::string ErrorString;

	bool NoSort = false; // If this flag is set, all entries will be marked NoSort in AddEntry
};

C4GroupEntry::~C4GroupEntry()
{
	if (HoldBuffer)
		if (MemoryBuffer)
		{
			if (BufferIsStdbuf)
				StdBuf::DeletePointer(MemoryBuffer);
			else
				delete [] MemoryBuffer;
		}
}

void C4GroupEntry::Set(const DirectoryIterator &directories, const char * path)
{
	InplaceReconstruct(this);

	SCopy(GetFilename(*directories),FileName,_MAX_FNAME);
	SCopy(*directories, DiskPath, _MAX_PATH-1);
	Size = directories.GetFileSize();
	Status = C4GRES_OnDisk;
	Packed = false;
	ChildGroup = false;
	// Notice folder entries are not checked for ChildGroup status.
	// This would cause extreme performance loss and be good for
	// use in entry list display only.
}

C4Group::C4Group()
	: p(new P)
{}

void C4Group::Init()
{
	auto new_p = std::make_unique<P>();
	// Copy persistent variables
	new_p->fnProcessCallback = p->fnProcessCallback;
	new_p->NoSort = p->NoSort;
	new_p->StdOutput = p->StdOutput;

	InplaceReconstruct(&Head);
	p = std::move(new_p);
}

C4Group::~C4Group()
{
	Clear();
}

bool C4Group::Error(const char *szStatus)
{
	p->ErrorString = szStatus;
	return false;
}

const char *C4Group::GetError()
{
	return p->ErrorString.c_str();
}

void C4Group::SetStdOutput(bool fStatus)
{
	p->StdOutput=fStatus;
}

bool C4Group::Open(const char *group_name, bool do_create)
{
	if (!group_name) return Error("Open: Null filename");
	if (!group_name[0]) return Error("Open: Empty filename");

	char szGroupNameN[_MAX_FNAME];
	SCopy(group_name,szGroupNameN,_MAX_FNAME);
	// Convert to native path
	SReplaceChar(szGroupNameN, AltDirectorySeparator, DirectorySeparator);

	// Real reference
	if (FileExists(szGroupNameN))
	{
		// Init
		Init();
		// Open group or folder
		return OpenReal(szGroupNameN);
	}

	// If requested, try creating a new group file
	if (do_create)
	{
		CStdFile temp;
		if (temp.Create(szGroupNameN,false))
		{
			// Temporary file has been created
			temp.Close();
			// Init
			Init();
			p->SourceType=P::ST_Packed; p->Modified=true;
			p->FileName = szGroupNameN;
			return true;
		}
	}

	// While not a real reference (child group), trace back to mother group or folder.
	// Open mother and child in exclusive mode.
	char szRealGroup[_MAX_FNAME];
	SCopy(szGroupNameN,szRealGroup,_MAX_FNAME);
	do
		{ if (!TruncatePath(szRealGroup)) return Error(FormatString(R"(Open("%s"): File not found)", szGroupNameN).getData()); }
	while (!FileExists(szRealGroup));

	// Open mother and child in exclusive mode
	C4Group *mother = new C4Group;
	mother->SetStdOutput(p->StdOutput);
	if (!mother->Open(szRealGroup))
		{ Clear(); Error(mother->GetError()); delete mother; return false; }
	if (!OpenAsChild(mother,szGroupNameN+SLen(szRealGroup)+1,true))
		{ Clear(); return false; }

	// Success
	return true;

}

bool C4Group::OpenReal(const char *filename)
{
	// Get original filename
	if (!filename) return false;
	p->FileName = filename;

	// Folder
	if (DirectoryExists(GetName()))
	{
		// Ignore
		if (C4Group_TestIgnore(filename))
			return Error(FormatString("OpenReal: filename '%s' ignored", filename).getData());
		// OpenReal: Simply set status and return
		p->SourceType=P::ST_Unpacked;
		ResetSearch();
		// Success
		return true;
	}

	// File: Try reading header and entries
	if (OpenRealGrpFile())
	{
		p->SourceType=P::ST_Packed;
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
	if (!p->StdFile.Open(GetName(),true)) return Error("OpenRealGrpFile: Cannot open standard file");

	// Read header
	if (!p->StdFile.Read((BYTE*)&Head,sizeof(C4GroupHeader))) return Error("OpenRealGrpFile: Error reading header");
	MemScramble((BYTE*)&Head,sizeof(C4GroupHeader));
	p->EntryOffset+=sizeof(C4GroupHeader);

	// Check Header
	if (!SEqual(Head.Id,C4GroupFileID)
	    || (Head.Ver1!=C4GroupFileVer1) || (Head.Ver2>C4GroupFileVer2))
		return Error("OpenRealGrpFile: Invalid header");

	// Read Entries
	file_entries=Head.Entries;
	Head.Entries=0; // Reset, will be recounted by AddEntry
	for (cnt=0; cnt<file_entries; cnt++)
	{
		if (!p->StdFile.Read((BYTE*)&corebuf,sizeof(C4GroupEntryCore))) return Error("OpenRealGrpFile: Error reading entries");
		// New C4Groups have filenames in UTF-8
		StdStrBuf entryname(corebuf.FileName);
		entryname.EnsureUnicode();
		// Prevent overwriting of user stuff by malicuous groups
		C4InVal::ValidateFilename(const_cast<char *>(entryname.getData()),entryname.getLength());
		p->EntryOffset+=sizeof(C4GroupEntryCore);
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
                       const char *filename,
                       long size,
                       const char *entry_name,
                       BYTE *membuf,
                       bool delete_on_disk,
                       bool hold_buffer,
                       bool is_executable,
                       bool buffer_is_stdbuf)
{

	// Folder: add file to folder immediately
	if (p->SourceType==P::ST_Unpacked)
	{

		// Close open StdFile
		p->StdFile.Close();

		// Get path to target folder file
		char tfname[_MAX_FNAME];
		SCopy(GetName(),tfname,_MAX_FNAME);
		AppendBackslash(tfname);
		if (entry_name) SAppend(entry_name,tfname);
		else SAppend(GetFilename(filename),tfname);

		switch (status)
		{

		case C4GroupEntry::C4GRES_OnDisk: // Copy/move file to folder
			if (!CopyItem(filename, tfname))
				return false;
			// Reset directory iterator to reflect new file
			ResetSearch(true);
			if (delete_on_disk && !EraseItem(filename))
				return false;
			return true;

		case C4GroupEntry::C4GRES_InMemory: { // Save buffer to file in folder
			CStdFile hFile;
			bool fOkay = false;
			if (hFile.Create(tfname, !!childgroup))
				fOkay = !!hFile.Write(membuf,size);
			hFile.Close();
			ResetSearch(true);

			if (hold_buffer) { if (buffer_is_stdbuf) StdBuf::DeletePointer(membuf); else delete [] membuf; }

			return fOkay;
		}

		default: break; // InGrp & Deleted ignored
		}

		return Error("Add to folder: Invalid request");
	}


	// Group file: add to virtual entry list

	C4GroupEntry *nentry,*lentry,*centry;

	// Delete existing entries of same name
	centry=GetEntry(GetFilename(entry_name ? entry_name : filename));
	if (centry) { centry->Status = C4GroupEntry::C4GRES_Deleted; Head.Entries--; }

	// Allocate memory for new entry
	nentry = new C4GroupEntry;

	// Find end of list
	for (lentry=p->FirstEntry; lentry && lentry->Next; lentry=lentry->Next) {}

	// Init entry core data
	if (entry_name) SCopy(entry_name,nentry->FileName,_MAX_FNAME);
	else SCopy(GetFilename(filename),nentry->FileName,_MAX_FNAME);
	nentry->Size=size;
	nentry->ChildGroup=childgroup;
	nentry->Offset=0;
	nentry->Executable=is_executable;
	nentry->DeleteOnDisk=delete_on_disk;
	nentry->HoldBuffer=hold_buffer;
	nentry->BufferIsStdbuf=buffer_is_stdbuf;
	if (lentry) nentry->Offset=lentry->Offset+lentry->Size;

	// Init list entry data
	SCopy(filename,nentry->DiskPath,_MAX_FNAME);
	nentry->Status=status;
	nentry->MemoryBuffer=membuf;
	nentry->Next=nullptr;
	nentry->NoSort = p->NoSort;

	// Append entry to list
	if (lentry) lentry->Next=nentry;
	else p->FirstEntry=nentry;

	// Increase virtual file count of group
	Head.Entries++;

	return true;
}

C4GroupEntry* C4Group::GetEntry(const char *entry_name)
{
	if (p->SourceType==P::ST_Unpacked) return nullptr;
	C4GroupEntry *centry;
	for (centry=p->FirstEntry; centry; centry=centry->Next)
		if (centry->Status != C4GroupEntry::C4GRES_Deleted)
			if (WildcardMatch(entry_name,centry->FileName))
				return centry;
	return nullptr;
}

bool C4Group::Close()
{
	C4GroupEntry *centry;
	bool fRewrite=false;

	if (p->SourceType==P::ST_None) return false;

	// Folder: just close
	if (p->SourceType==P::ST_Unpacked)
		{ CloseExclusiveMother(); Clear(); return true; }

	// Rewrite check
	for (centry=p->FirstEntry; centry; centry=centry->Next)
		if (centry->Status != C4GroupEntry::C4GRES_InGroup)
			fRewrite=true;
	if (p->Modified) fRewrite=true;

	// No rewrite: just close
	if (!fRewrite)
		{ CloseExclusiveMother(); Clear(); return true; }

	if (p->StdOutput) printf("Writing group file...\n");

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

bool C4Group::Save(bool reopen)
{

	int cscore;
	C4GroupEntryCore *save_core;
	C4GroupEntry *centry;
	char szTempFileName[_MAX_FNAME+1],szGrpFileName[_MAX_FNAME+1];

	// Create temporary core list with new actual offsets to be saved
	int32_t iContentsSize = 0;
	save_core = new C4GroupEntryCore [Head.Entries];
	cscore=0;
	for (centry=p->FirstEntry; centry; centry=centry->Next)
		if (centry->Status != C4GroupEntry::C4GRES_Deleted)
		{
			save_core[cscore]=(C4GroupEntryCore)*centry;
			// Make actual offset
			save_core[cscore].Offset = iContentsSize;
			iContentsSize += centry->Size;
			cscore++;
		}

	// Hold contents in memory?
	bool fToMemory = !reopen && p->Mother && iContentsSize < C4GroupSwapThreshold;
	if (!fToMemory)
	{
		// Create target temp file (in temp directory!)
		SCopy(GetName(),szGrpFileName,_MAX_FNAME);
		if (C4Group_TempPath[0]) { SCopy(C4Group_TempPath,szTempFileName,_MAX_FNAME); SAppend(GetFilename(GetName()),szTempFileName,_MAX_FNAME); }
		else SCopy(GetName(),szTempFileName,_MAX_FNAME);
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
	for (centry=p->FirstEntry; centry; centry=centry->Next) iTotalSize+=centry->Size;
	for (centry=p->FirstEntry; centry; centry=centry->Next)
		if (AppendEntry2StdFile(centry,tfile))
			{ iSizeDone+=centry->Size; if (iTotalSize && p->fnProcessCallback) p->fnProcessCallback(centry->FileName,100*iSizeDone/iTotalSize); }
		else
		{
			tfile.Close(); return false;
		}

	// Write
	StdBuf *pBuf;
	tfile.Close(fToMemory ? &pBuf : nullptr);

	// Child: move temp file to mother
	if (p->Mother)
	{
		if (fToMemory)
		{
			if (!p->Mother->Add(GetFilename(GetName()), *pBuf, true, true))
				{ delete pBuf; CloseExclusiveMother(); Clear(); return Error("Close: Cannot move rewritten child data to mother"); }
			delete pBuf;
		}
		else
		{
			if (!p->Mother->Move(szTempFileName,GetFilename(GetName())))
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
	if (reopen)
	{
		OpenReal(szGrpFileName);
	}

	return true;
}

void C4Group::Clear()
{
	if (p)
	{
		// Delete entries
		C4GroupEntry *next;
		while (p->FirstEntry)
		{
			next = p->FirstEntry->Next;
			delete p->FirstEntry;
			p->FirstEntry = next;
		}
		// Close std file
		p->StdFile.Close();
		// Delete mother
		if (p->Mother && p->ExclusiveChild)
		{
			delete p->Mother;
			p->Mother = nullptr;
		}
	}
	// Reset
	Init();
}

bool C4Group::AppendEntry2StdFile(C4GroupEntry *entry, CStdFile &target)
{
	CStdFile hSource;
	long csize;
	BYTE fbuf;

	switch (target->Status)
	{

	case C4GroupEntry::C4GRES_InGroup: // Copy from group to std file
		if (!SetFilePtr(target->Offset))
			return Error("AE2S: Cannot set file pointer");
		for (csize=target->Size; csize>0; csize--)
		{
			if (!Read(&fbuf,1))
				return Error("AE2S: Cannot read entry from group file");
			if (!target.Write(&fbuf,1))
				return Error("AE2S: Cannot write to target file");
		}
		break;

	case C4GroupEntry::C4GRES_OnDisk: // Copy/move from disk item to std file
	{
		char szFileSource[_MAX_FNAME+1];
		SCopy(entry->DiskPath,szFileSource,_MAX_FNAME);

		// Disk item is a directory
		if (DirectoryExists(entry->DiskPath))
			return Error("AE2S: Cannot add directory to group file");

		// Resort group if neccessary
		// (The group might be renamed by adding, forcing a resort)
		bool fTempFile = false;
		if (entry->ChildGroup)
			if (!entry->NoSort)
				if (!SEqual(GetFilename(szFileSource), entry->FileName))
				{
					// copy group
					MakeTempFilename(szFileSource);
					if (!CopyItem(entry->DiskPath, szFileSource))
						return Error("AE2S: Cannot copy item");
					// open group and resort
					C4Group SortGrp;
					if (!SortGrp.Open(szFileSource))
						return Error("AE2S: Cannot open group");
					if (!SortGrp.SortByList(C4Group_SortList, entry->FileName))
						return Error("AE2S: Cannot resort group");
					fTempFile = true;
					// close group (won't be saved if the sort didn't change)
					SortGrp.Close();
				}

		// Append disk source to target file
		if (!hSource.Open(szFileSource, !!entry->ChildGroup))
			return Error("AE2S: Cannot open on-disk file");
		for (csize=entry->Size; csize>0; csize--)
		{
			if (!hSource.Read(&fbuf,1))
				{ hSource.Close(); return Error("AE2S: Cannot read on-disk file"); }
			if (!target.Write(&fbuf,1))
				{ hSource.Close(); return Error("AE2S: Cannot write to target file"); }
		}
		hSource.Close();

		// Erase temp file
		if (fTempFile)
			EraseItem(szFileSource);
		// Erase disk source if requested
		if (entry->DeleteOnDisk)
			EraseItem(entry->DiskPath);

		break;
	}

	case C4GroupEntry::C4GRES_InMemory: // Copy from mem to std file
		if (!entry->MemoryBuffer) return Error("AE2S: no buffer");
		if (!target.Write(entry->MemoryBuffer,entry->Size)) return Error("AE2S: writing error");
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
	switch (p->SourceType)
	{
	case P::ST_Unpacked:
		p->SearchPtr=nullptr;
		p->FolderSearch.Reset(GetName(), reload_contents);
		if (*p->FolderSearch)
		{
			p->FolderSearchEntry.Set(p->FolderSearch, GetName());
			p->SearchPtr=&p->FolderSearchEntry;
		}
		break;
	case P::ST_Packed:
		p->SearchPtr=p->FirstEntry;
		break;
	default: break; // InGrp & Deleted ignored
	}
}

C4GroupEntry* C4Group::GetNextFolderEntry()
{
	if (*++p->FolderSearch)
	{
		p->FolderSearchEntry.Set(p->FolderSearch, GetName());
		return &p->FolderSearchEntry;
	}
	else
	{
		return nullptr;
	}
}

C4GroupEntry* C4Group::SearchNextEntry(const char *entry_name)
{
	// Wildcard "*.*" is expected to find all files: substitute correct wildcard "*"
	if (SEqual(entry_name, "*.*"))
		entry_name = "*";
	// Search by group type
	C4GroupEntry *pEntry;
	switch (p->SourceType)
	{
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case P::ST_Packed:
		for (pEntry=p->SearchPtr; pEntry; pEntry=pEntry->Next)
			if (pEntry->Status != C4GroupEntry::C4GRES_Deleted)
				if (WildcardMatch(entry_name,pEntry->FileName))
				{
					p->SearchPtr=pEntry->Next;
					return pEntry;
				}
		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case P::ST_Unpacked:
		for (pEntry=p->SearchPtr; pEntry; pEntry=GetNextFolderEntry())
			if (WildcardMatch(entry_name,pEntry->FileName))
				if (!C4Group_TestIgnore(pEntry->FileName))
				{
					p->LastFolderSearchEntry=(*pEntry);
					pEntry=&p->LastFolderSearchEntry;
					p->SearchPtr=GetNextFolderEntry();
					return pEntry;
				}
		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	default: break; // InGrp & Deleted ignored
	}
	// No entry found: reset search pointer
	p->SearchPtr=nullptr;
	return nullptr;
}

bool C4Group::SetFilePtr(int offset)
{

	if (p->SourceType==P::ST_Unpacked)
		return Error("SetFilePtr not implemented for Folders");

	// ensure mother is at correct pos
	if (p->Mother) p->Mother->EnsureChildFilePtr(this);

	// Rewind if necessary
	if (p->FilePtr>offset)
		if (!RewindFilePtr()) return false;

	// Advance to target pointer
	if (p->FilePtr<offset)
		if (!AdvanceFilePtr(offset- p->FilePtr)) return false;

	return true;
}

bool C4Group::Advance(int offset)
{
	assert(offset >= 0);
	// cached advance
	if (p->pInMemEntry)
	{
		if (p->iInMemEntrySize < size_t(offset)) return false;
		p->iInMemEntrySize -= offset;
		p->pInMemEntry += offset;
		return true;
	}
	// uncached advance
	if (p->SourceType == P::ST_Unpacked) return !!p->StdFile.Advance(offset);
	// FIXME: reading the file one byte at a time sounds just slow.
	BYTE buf;
	for (; offset>0; offset--)
		if (!Read(&buf,1)) return false;
	return true;
}

bool C4Group::Read(void *buffer, size_t size)
{
	// Access cached entry from memory?
	if (p->pInMemEntry)
	{
		if (p->iInMemEntrySize < size) return Error("ReadCached:");
		memcpy(buffer, p->pInMemEntry, size);
		p->iInMemEntrySize -= size;
		p->pInMemEntry += size;
		return true;
	}
	// Not cached. Read from file.
	switch (p->SourceType)
	{
	case P::ST_Packed:
		// Child group: read from mother group
		if (p->Mother)
		{
			if (!p->Mother->Read(buffer,size))
				{ RewindFilePtr(); return Error("Read:"); }
		}
		// Regular group: read from standard file
		else
		{
			if (!p->StdFile.Read(buffer,size))
				{ RewindFilePtr(); return Error("Read:"); }
		}
		p->FilePtr+=size;
		break;
	case P::ST_Unpacked:
		if (!p->StdFile.Read(buffer,size)) return Error("Read: Error reading from folder contents");
		break;
	default: break; // InGrp & Deleted ignored
	}

	return true;
}

bool C4Group::AdvanceFilePtr(int offset)
{
	// Child group file: pass command to mother
	if ((p->SourceType==P::ST_Packed) && p->Mother)
	{

		// Ensure mother file ptr for it may have been moved by foreign access to mother
		if (!p->Mother->EnsureChildFilePtr(this))
			return false;

		if (!p->Mother->AdvanceFilePtr(offset))
			return false;

	}
	// Regular group
	else if (p->SourceType==P::ST_Packed)
	{
		if (!p->StdFile.Advance(offset))
			return false;
	}
	// Open folder
	else
	{
		if (!p->StdFile.Advance(offset))
			return false;
	}

	// Advanced
	p->FilePtr+=offset;

	return true;
}

bool C4Group::RewindFilePtr()
{

#ifdef _DEBUG
	if (szCurrAccessedEntry && !iC4GroupRewindFilePtrNoWarn)
	{
		LogF("C4Group::RewindFilePtr() for %s (%s) after %s", szCurrAccessedEntry ? szCurrAccessedEntry : "???", GetName(), p->sPrevAccessedEntry.getLength() ? p->sPrevAccessedEntry.getData() : "???");
		szCurrAccessedEntry=nullptr;
	}
#endif

	// Child group file: pass command to mother
	if ((p->SourceType==P::ST_Packed) && p->Mother)
	{
		if (!p->Mother->SetFilePtr2Entry(GetName(),true)) // Set to group file start
			return false;
		if (!p->Mother->AdvanceFilePtr(p->EntryOffset)) // Advance data offset
			return false;
	}
	// Regular group or open folder: rewind standard file
	else
	{
		if (!p->StdFile.Rewind()) // Set to group file start
			return false;
		if (!p->StdFile.Advance(p->EntryOffset)) // Advance data offset
			return false;
	}

	p->FilePtr=0;

	return true;
}

bool C4Group::Merge(const char *folders)
{
	bool move = true;

	if (p->StdOutput) printf("%s...\n",move ? "Moving" : "Adding");

	// Add files & directories
	char szFileName[_MAX_FNAME+1];
	int iFileCount = 0;
	DirectoryIterator i;

	// Process segmented path & search wildcards
	char cSeparator = (SCharCount(';', folders) ? ';' : '|');
	for (int cseg=0; SCopySegment(folders, cseg, szFileName, cSeparator); cseg++)
	{
		i.Reset(szFileName);
		while (*i)
		{
			// File count
			iFileCount++;
			// Process output & callback
			if (p->StdOutput) printf("%s\n",GetFilename(*i));
			if (p->fnProcessCallback)
				p->fnProcessCallback(GetFilename(*i),0); // cbytes/tbytes
			// AddEntryOnDisk
			AddEntryOnDisk(*i, nullptr, move);
			++i;
		}
	}

	if (p->StdOutput) printf("%d file(s) %s.\n",iFileCount,move ? "moved" : "added");

	return true;
}

bool C4Group::AddEntryOnDisk(const char *filename,
                             const char *entry_name,
                             bool move)
{

	// Do not process yourself
	if (ItemIdentical(filename, GetName())) return true;

	// File is a directory: copy to temp path, pack, and add packed file
	if (DirectoryExists(filename))
	{
		// Ignore
		if (C4Group_TestIgnore(filename)) return true;
		// Temp filename
		char szTempFilename[_MAX_PATH+1];
		if (C4Group_TempPath[0]) { SCopy(C4Group_TempPath,szTempFilename,_MAX_PATH); SAppend(GetFilename(filename),szTempFilename,_MAX_PATH); }
		else SCopy(filename,szTempFilename,_MAX_PATH);
		MakeTempFilename(szTempFilename);
		// Copy or move item to temp file (moved items might be killed if later process fails)
		if (move) { if (!MoveItem(filename,szTempFilename)) return Error("AddEntryOnDisk: Move failure"); }
		else { if (!CopyItem(filename,szTempFilename)) return Error("AddEntryOnDisk: Copy failure"); }
		// Pack temp file
		if (!C4Group_PackDirectory(szTempFilename)) return Error("AddEntryOnDisk: Pack directory failure");
		// Add temp file
		if (!entry_name) entry_name = GetFilename(filename);
		filename = szTempFilename;
		move = true;
	}

	// Determine size
	bool fIsGroup = !!C4Group_IsGroup(filename);
	int size = fIsGroup ? UncompressedFileSize(filename) : FileSize(filename);

	// Determine executable bit (linux only)
	bool is_executable = false;
#ifdef __linux__
	is_executable = (access(filename, X_OK) == 0);
#endif

	// AddEntry
	return AddEntry(C4GroupEntry::C4GRES_OnDisk,
	                fIsGroup,
	                filename,
	                size,
					entry_name,
	                nullptr,
	                move,
	                false,
	                is_executable);

}

bool C4Group::Add(const char *filename, const char *entry_name)
{
	bool move = false;

	if (p->StdOutput)
	{
		printf("%s %s as %s...\n", move ? "Moving" : "Adding", GetFilename(filename), entry_name);
	}

	return AddEntryOnDisk(filename, entry_name, move);
}

bool C4Group::Move(const char *filename, const char *entry_name)
{
	bool move = true;

	if (p->StdOutput)
	{
		printf("%s %s as %s...\n", move ? "Moving" : "Adding", GetFilename(filename), entry_name);
	}

	return AddEntryOnDisk(filename, entry_name, move);
}

bool C4Group::Delete(const char *files, bool recursive)
{
	int fcount = 0;
	C4GroupEntry *tentry;

	// Segmented file specs
	if (SCharCount(';', files) || SCharCount('|', files))
	{
		char cSeparator = (SCharCount(';', files) ? ';' : '|');
		bool success = true;
		char filespec[_MAX_FNAME+1];
		for (int cseg = 0; SCopySegment(files, cseg, filespec, cSeparator, _MAX_FNAME); cseg++)
			if (!Delete(filespec, recursive))
				success=false;
		return success; // Would be nicer to return the file count and add up all counts from recursive actions...
	}

	// Delete all matching Entries
	ResetSearch();
	while ((tentry = SearchNextEntry(files)))
	{
		// StdOutput
		if (p->StdOutput) printf("%s\n",tentry->FileName);
		if (!DeleteEntry(tentry->FileName))
			return Error("Delete: Could not delete entry");
		fcount++;
	}

	// Recursive: process sub groups
	if (recursive)
	{
		C4Group hChild;
		ResetSearch();
		while ((tentry = SearchNextEntry("*")))
			if (tentry->ChildGroup)
				if (hChild.OpenAsChild(this, tentry->FileName))
				{
					hChild.SetStdOutput(p->StdOutput);
					hChild.Delete(files, recursive);
					hChild.Close();
				}
	}

	// StdOutput
	if (p->StdOutput)
		printf("%d file(s) deleted.\n",fcount);

	return true; // Would be nicer to return the file count and add up all counts from recursive actions...
}

bool C4Group::DeleteEntry(const char *filename, bool do_recycle)
{
	switch (p->SourceType)
	{
	case P::ST_Packed:
		// Get entry
		C4GroupEntry *pEntry;
		if (!(pEntry=GetEntry(filename))) return false;
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
	case P::ST_Unpacked:
		p->StdFile.Close();
		char path[_MAX_FNAME+1];
		sprintf(path,"%s%c%s", GetName(),DirectorySeparator,filename);

		if (do_recycle)
		{
			if (!EraseItemSafe(path)) return false;
		}
		else
		{
			if (!EraseItem(path)) return false;
		}
		break;
		// refresh file list
		ResetSearch(true);
	default: break; // InGrp & Deleted ignored
	}
	return true;
}

bool C4Group::Rename(const char *filename, const char *new_name)
{

	if (p->StdOutput) printf("Renaming %s to %s...\n",filename,new_name);

	switch (p->SourceType)
	{
	case P::ST_Packed:
		// Get entry
		C4GroupEntry *pEntry;
		if (!(pEntry=GetEntry(filename))) return Error("Rename: File not found");
		// Check double name
		if (GetEntry(new_name) && !SEqualNoCase(new_name, filename)) return Error("Rename: File exists already");
		// Rename
		SCopy(new_name,pEntry->FileName,_MAX_FNAME);
		p->Modified=true;
		break;
	case P::ST_Unpacked:
		p->StdFile.Close();
		char path[_MAX_FNAME+1]; SCopy(GetName(),path,_MAX_PATH-1);
		AppendBackslash(path); SAppend(filename,path,_MAX_PATH);
		char path2[_MAX_FNAME+1]; SCopy(GetName(),path2,_MAX_PATH-1);
		AppendBackslash(path2); SAppend(new_name,path2,_MAX_PATH);
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

bool C4Group::Extract(const char *files, const char *destination, const char *exclude)
{
	// StdOutput
	if (p->StdOutput)
	{
		printf("Extracting");
		if (destination) printf(" to %s",destination);
		printf("...\n");
	}

	int fcount=0;
	int cbytes,tbytes;
	C4GroupEntry *tentry;

	cbytes=0; tbytes=EntrySize();

	// Process segmented list
	char cSeparator = (SCharCount(';', files) ? ';' : '|');
	char szFileName[_MAX_PATH + 1];
	for (int cseg=0; SCopySegment(files, cseg, szFileName, cSeparator); cseg++)
	{
		// Search all entries
		ResetSearch();
		while ((tentry = SearchNextEntry(szFileName)))
		{
			// skip?
			if (C4Group_IsExcluded(tentry->FileName, exclude)) continue;
			// Process data & output
			if (p->StdOutput) printf("%s\n",tentry->FileName);
			cbytes+=tentry->Size;
			if (p->fnProcessCallback)
				p->fnProcessCallback(tentry->FileName,100*cbytes/std::max(tbytes,1));

			// Extract
			if (!ExtractEntry(tentry->FileName,destination))
				return Error("Extract: Could not extract entry");

			fcount++;
		}
	}

	if (p->StdOutput) printf("%d file(s) extracted.\n",fcount);

	return true;
}

bool C4Group::ExtractEntry(const char *filename, const char *destination)
{
	CStdFile tfile;
	CStdFile hDummy;
	char szTempFName[_MAX_FNAME+1],szTargetFName[_MAX_FNAME+1];

	// Target file name
	if (destination)
	{
		SCopy(destination,szTargetFName,_MAX_FNAME-1);
		if (DirectoryExists(szTargetFName))
		{
			AppendBackslash(szTargetFName);
			SAppend(filename,szTargetFName,_MAX_FNAME);
		}
	}
	else
		SCopy(filename,szTargetFName,_MAX_FNAME);

	// Extract
	switch (p->SourceType)
	{
	case P::ST_Packed: // Copy entry to target
		// Get entry
		C4GroupEntry *pEntry;
		if (!(pEntry=GetEntry(filename))) return Error("Extract: Entry not found");
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
	case P::ST_Unpacked: // Copy item from folder to target
		char path[_MAX_FNAME+1];
		sprintf(path,"%s%c%s", GetName(),DirectorySeparator,filename);
		if (!CopyItem(path,szTargetFName))
			return Error("ExtractEntry: Cannot copy item");
		break;
	}
	return true;
}


bool C4Group::OpenAsChild(C4Group *mother,
                          const char *entry_name, bool is_exclusive, bool do_create)
{

	if (!mother) return Error("OpenAsChild: No mother specified");

	if (SCharCount('*',entry_name)) return Error("OpenAsChild: No wildcards allowed");

	// Open nested child group check: If entry_name is a reference to
	// a nested group, open the first mother (in specified mode), then open the child
	// in exclusive mode

	if (SCharCount(DirectorySeparator,entry_name))
	{
		char mothername[_MAX_FNAME+1];
		SCopyUntil(entry_name,mothername,DirectorySeparator,_MAX_FNAME);

		C4Group *pMother2;
		pMother2 = new C4Group;
		pMother2->SetStdOutput(p->StdOutput);
		if (!pMother2->OpenAsChild(mother, mothername, is_exclusive))
		{
			delete pMother2;
			return Error("OpenAsChild: Cannot open mother");
		}
		return OpenAsChild(pMother2, entry_name + SLen(mothername) + 1, true);
	}

	// Init
	Init();
	p->FileName = entry_name;
	p->Mother=mother;
	p->ExclusiveChild=is_exclusive;

	// Folder: Simply set status and return
	char path[_MAX_FNAME+1];
	SCopy( GetFullName().getData(), path, _MAX_FNAME);
	if (DirectoryExists(path))
	{
		p->FileName = path;
		p->SourceType=P::ST_Unpacked;
		ResetSearch();
		return true;
	}

	// Get original entry name
	C4GroupEntry *centry;
	if ((centry = p->Mother->GetEntry(GetName())))
		p->FileName = centry->FileName;

	// Access entry in mother group
	size_t size;
	if ((!p->Mother->AccessEntry(GetName(), &size, nullptr, true)))
	{
		if (!do_create)
			{ CloseExclusiveMother(); Clear(); return Error("OpenAsChild: Entry not in mother group"); }
		else
		{
			// Create - will be added to mother in Close()
			p->SourceType=P::ST_Packed; p->Modified=true;
			return true;
		}
	}

	// Child Group?
	if (centry && !centry->ChildGroup)
		{ CloseExclusiveMother(); Clear(); return Error("OpenAsChild: Is not a child group"); }

	// Read header
	// Do not do size checks for packed subgroups of unpacked groups (there will be no entry),
	//  because that would be the PACKED size which can actually be smaller than sizeof(C4GroupHeader)!
	if (size < sizeof(C4GroupHeader) && centry)
		{ CloseExclusiveMother(); Clear(); return Error("OpenAsChild: Entry too small"); }
	if (!p->Mother->Read(&Head,sizeof(C4GroupHeader)))
		{ CloseExclusiveMother(); Clear(); return Error("OpenAsChild: Entry reading error"); }
	MemScramble((BYTE*)&Head,sizeof(C4GroupHeader));
	p->EntryOffset+=sizeof(C4GroupHeader);

	// Check Header
	if (!SEqual(Head.Id,C4GroupFileID)
	    || (Head.Ver1!=C4GroupFileVer1) || (Head.Ver2>C4GroupFileVer2))
		{ CloseExclusiveMother(); Clear(); return Error("OpenAsChild: Invalid Header"); }

	// Read Entries
	C4GroupEntryCore corebuf;
	int file_entries=Head.Entries;
	Head.Entries=0; // Reset, will be recounted by AddEntry
	for (int cnt=0; cnt<file_entries; cnt++)
	{
		if (!p->Mother->Read(&corebuf,sizeof(C4GroupEntryCore)))
			{ CloseExclusiveMother(); Clear(); return Error("OpenAsChild: Entry reading error"); }
		p->EntryOffset+=sizeof(C4GroupEntryCore);
		if (!AddEntry(C4GroupEntry::C4GRES_InGroup, !!corebuf.ChildGroup,
		              corebuf.FileName,corebuf.Size,
		              nullptr, nullptr, false, false,
		              !!corebuf.Executable))
			{ CloseExclusiveMother(); Clear(); return Error("OpenAsChild: Insufficient memory"); }
	}

	ResetSearch();

	// File
	p->SourceType=P::ST_Packed;

	// save position in mother group
	if (centry) p->MotherOffset = centry->Offset;

	return true;
}

bool C4Group::AccessEntry(const char *wildcard,
                          size_t *size,
						  char *filename,
                          bool needs_to_be_a_group)
{
#ifdef C4GROUP_DUMP_ACCESS
	LogF("Group access in %s: %s", GetFullName().getData(), wildcard);
#endif
	StdStrBuf fname;
	if (!FindEntry(wildcard,&fname,&p->iCurrFileSize))
		return false;
#ifdef _DEBUG
	szCurrAccessedEntry = fname.getMData();
#endif
	bool fResult = SetFilePtr2Entry(fname.getData(), needs_to_be_a_group);
#ifdef _DEBUG
	p->sPrevAccessedEntry.Copy(szCurrAccessedEntry);
	szCurrAccessedEntry = nullptr;
#endif
	if (!fResult) return false;
	if (filename) SCopy(fname.getData(),filename);
	if (size) *size=p->iCurrFileSize;
	return true;
}

bool C4Group::AccessNextEntry(const char *wildcard,
                              size_t *size,
							  char *filename,
                              bool start_at_filename)
{
	char fname[_MAX_FNAME+1];
	if (!FindNextEntry(wildcard,fname,&p->iCurrFileSize,start_at_filename)) return false;
#ifdef _DEBUG
	szCurrAccessedEntry = fname;
#endif
	bool fResult = SetFilePtr2Entry(fname);
#ifdef _DEBUG
	szCurrAccessedEntry = nullptr;
#endif
	if (!fResult) return false;
	if (filename) SCopy(fname,filename);
	if (size) *size=p->iCurrFileSize;
	return true;
}

bool C4Group::SetFilePtr2Entry(const char *entry_name, bool needs_to_be_a_group)
{
	C4GroupEntry *centry = GetEntry(entry_name);
	// Read cached entries directly from memory (except child groups. that is not supported.)
	if (centry && centry->MemoryBuffer && !needs_to_be_a_group)
	{
		p->pInMemEntry = centry->MemoryBuffer;
		p->iInMemEntrySize = centry->Size;
		return true;
	}
	else
	{
		p->pInMemEntry = nullptr;
	}

	// Not cached. Access from disk.
	switch (p->SourceType)
	{

	case P::ST_Packed:
		if ((!centry) || (centry->Status != C4GroupEntry::C4GRES_InGroup)) return false;
		return SetFilePtr(centry->Offset);

	case P::ST_Unpacked: {
		p->StdFile.Close();
		char path[_MAX_FNAME+1]; SCopy(GetName(),path,_MAX_FNAME);
		AppendBackslash(path); SAppend(entry_name,path);
		bool fSuccess = p->StdFile.Open(path, needs_to_be_a_group);
		return fSuccess;
	}

	default: break; // InGrp & Deleted ignored
	}
	return false;
}

bool C4Group::FindEntry(const char *wildcard, StdStrBuf *filename, size_t *size)
{
	ResetSearch();
	return FindNextEntry(wildcard,filename,size);
}

bool C4Group::FindNextEntry(const char *wildcard,
                            StdStrBuf *filename,
                            size_t *size,
                            bool start_at_filename)
{
	C4GroupEntry *centry;
	if (!wildcard) return false;

	// Reset search to specified position
	if (start_at_filename) FindEntry(filename->getData());

	if (!(centry=SearchNextEntry(wildcard))) return false;
	if (filename) filename->Copy(centry->FileName);
	if (size) *size=centry->Size;
	return true;
}

bool C4Group::Add(const char *entry_name, void *buffer, int size, bool add_as_child, bool hold_buffer, bool is_executable)
{
	return AddEntry(C4GroupEntry::C4GRES_InMemory,
	                add_as_child,
	                entry_name,
	                size,
	                entry_name,
	                (BYTE*) buffer,
	                false,
	                hold_buffer,
	                is_executable);
}

bool C4Group::Add(const char *entry_name, StdBuf &buffer, bool add_as_child, bool hold_buffer, bool is_executable)
{
	if (!AddEntry(C4GroupEntry::C4GRES_InMemory,
	              add_as_child,
	              entry_name,
	              buffer.getSize(),
	              entry_name,
	              (BYTE*) buffer.getMData(),
	              false,
	              hold_buffer,
	              is_executable,
	              true)) return false;
	// Pointer is now owned and released by C4Group!
	if (hold_buffer) buffer.GrabPointer();
	return true;
}

bool C4Group::Add(const char *entry_name, StdStrBuf &buffer, bool add_as_child, bool hold_buffer, bool is_executable)
{
	if (!AddEntry(C4GroupEntry::C4GRES_InMemory,
	              add_as_child,
	              entry_name,
	              buffer.getLength(),
	              entry_name,
	              (BYTE*) buffer.getMData(),
	              false,
	              hold_buffer,
	              is_executable,
	              true)) return false;
	// Pointer is now owned and released by C4Group!
	if (hold_buffer) buffer.GrabPointer();
	return true;
}


const char* C4Group::GetName() const
{
	return p->FileName.c_str();
}

int C4Group::EntryCount(const char *wildcard)
{
	int fcount;
	C4GroupEntry *tentry;
	// All files if no wildcard
	if (!wildcard) wildcard="*";
	// Match wildcard
	ResetSearch(); fcount=0;
	while ((tentry=SearchNextEntry(wildcard))) fcount++;
	return fcount;
}

size_t C4Group::EntrySize(const char *wildcard)
{
	int fsize;
	C4GroupEntry *tentry;
	// All files if no wildcard
	if (!wildcard) wildcard="*";
	// Match wildcard
	ResetSearch(); fsize=0;
	while ((tentry=SearchNextEntry(wildcard)))
		fsize+=tentry->Size;
	return fsize;
}

size_t C4Group::AccessedEntrySize() const { return p->iCurrFileSize; }

unsigned int C4Group::EntryCRC32(const char *wildcard)
{
	if (!wildcard) wildcard="*";
	// iterate thorugh child
	C4GroupEntry *pEntry; unsigned int iCRC = 0;
	ResetSearch();
	while ((pEntry = SearchNextEntry(wildcard)))
	{
		iCRC ^= CalcCRC32(pEntry);
	}
	// return
	return iCRC;
}

bool C4Group::IsOpen() const { return p->SourceType != P::ST_None; }

bool C4Group::LoadEntry(const char *entry_name, char **buffer, size_t *size_info, int zeros_to_append)
{
	size_t size;

	// Access entry, allocate buffer, read data
	(*buffer)=nullptr; if (size_info) *size_info=0;
	if (!AccessEntry(entry_name,&size)) return Error("LoadEntry: Not found");
	*buffer = new char[size+zeros_to_append];
	if (!Read(*buffer,size))
	{
		delete [] (*buffer); *buffer = nullptr;
		return Error("LoadEntry: Reading error");
	}

	if (size_info) *size_info=size;

	if (zeros_to_append)
		ZeroMem( (*buffer)+size, zeros_to_append );

	return true;
}

bool C4Group::LoadEntry(const char *entry_name, StdBuf * buffer)
{
	size_t size;
	// Access entry, allocate buffer, read data
	if (!AccessEntry(entry_name,&size)) return Error("LoadEntry: Not found");
	// Allocate memory
	buffer->New(size);
	// Load data
	if (!Read(buffer->getMData(),size))
	{
		buffer->Clear();
		return Error("LoadEntry: Reading error");
	}
	// ok
	return true;
}

bool C4Group::LoadEntryString(const char *entry_name, StdStrBuf *buffer)
{
	size_t size;
	// Access entry, allocate buffer, read data
	if (!AccessEntry(entry_name,&size)) return Error("LoadEntry: Not found");
	// Allocate memory
	buffer->SetLength(size);
	// other parts crash when they get a zero length buffer, so fail here
	if (!size) return false;
	// Load data
	if (!Read(buffer->getMData(),size))
	{
		buffer->Clear();
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

bool C4Group::Sort(const char *list)
{
	bool fBubble;
	C4GroupEntry *centry,*prev,*next,*nextnext;

	if (!list || !list[0]) return false;

	if (p->StdOutput) printf("Sorting...\n");

	do
	{
		fBubble=false;

		for (prev=nullptr,centry=p->FirstEntry; centry; prev=centry,centry=next)
			if ((next=centry->Next))
			{
				// primary sort by file list
				int iS1 = SortRank(centry->FileName,list);
				int iS2 = SortRank(next->FileName,list);
				if (iS1 > iS2) continue;
				// secondary sort by filename
				if (iS1 == iS2)
					if (stricmp(centry->FileName, next->FileName) <= 0) continue;
				// wrong order: Swap!
				nextnext=next->Next;
				if (prev) prev->Next=next;
				else p->FirstEntry=next;
				next->Next=centry;
				centry->Next=nextnext;
				next=nextnext;

				fBubble=true;
				p->Modified=true;
			}

	}
	while (fBubble);

	return true;
}

C4Group* C4Group::GetMother()
{
	return p->Mother;
}

bool C4Group::IsPacked() const { return p->SourceType == P::ST_Packed; }

bool C4Group::HasPackedMother() const { if (!p->Mother) return false; return p->Mother->IsPacked(); }

bool C4Group::SetNoSort(bool no_sorting) { p->NoSort = no_sorting; return true; }

bool C4Group::CloseExclusiveMother()
{
	if (p->Mother && p->ExclusiveChild)
	{
		p->Mother->Close();
		delete p->Mother;
		p->Mother=nullptr;
		return true;
	}
	return false;
}

bool C4Group::SortByList(const char **list, const char *filename)
{
	// No sort list specified
	if (!list) return false;
	// No group name specified, use own
	if (!filename) filename = GetName();
	filename = GetFilename(filename);
	// Find matching filename entry in sort list
	const char **ppListEntry;
	for (ppListEntry = list; *ppListEntry; ppListEntry+=2)
		if (WildcardMatch( *ppListEntry, filename ))
			break;
	// Sort by sort list entry
	if (*ppListEntry && *(ppListEntry+1))
		Sort(*(ppListEntry+1));
	// Success
	return true;
}

bool C4Group::EnsureChildFilePtr(C4Group *pChild)
{

	// group file
	if (p->SourceType == P::ST_Packed)
	{
		// check if FilePtr has to be moved
		if (p->FilePtr != pChild->p->MotherOffset + pChild->p->EntryOffset +  pChild->p->FilePtr)
			// move it to the position the child thinks it is
			if (!SetFilePtr(pChild->p->MotherOffset + pChild->p->EntryOffset +  pChild->p->FilePtr))
				return false;
		// ok
		return true;
	}

	// Open standard file is not the child file     ...or StdFile ptr does not match pChild->FilePtr
	char szChildPath[_MAX_PATH+1]; sprintf(szChildPath,"%s%c%s", GetName(),DirectorySeparator,GetFilename(pChild->GetName()));
	if ( !ItemIdentical(p->StdFile.Name, szChildPath))
	{
		// Reopen correct child stdfile
		if ( !SetFilePtr2Entry( GetFilename(pChild->GetName()), true ) )
			return false;
		// Advance to child's old file ptr
		if ( !AdvanceFilePtr( pChild->p->EntryOffset + pChild->p->FilePtr ) )
			return false;
	}

	// Looks okay
	return true;

}

StdStrBuf C4Group::GetFullName() const
{
	char str[_MAX_PATH+1]; *str='\0';
	char sep[] = "/"; sep[0] = DirectorySeparator;
	for (const C4Group *pGroup=this; pGroup; pGroup=pGroup->p->Mother)
	{
		if (*str) SInsert(str, sep, 0, _MAX_PATH);
		// Avoid double slash
		if (pGroup == this || pGroup->p->FileName.length() > 1 || pGroup->p->FileName[0] != '/')
			SInsert(str, pGroup->GetName(), 0, _MAX_PATH);
		if (pGroup->p->SourceType == P::ST_Unpacked) break; // Folder is assumed to have full path
	}
	StdStrBuf sResult; sResult.Copy(str);
	return sResult;
}

uint32_t C4Group::CalcCRC32(C4GroupEntry *entry)
{
	uint32_t CRC;
	// child group?
	if (entry->ChildGroup || (entry->Status == C4GroupEntry::C4GRES_OnDisk && (DirectoryExists(entry->DiskPath) || C4Group_IsGroup(entry->DiskPath))))
	{
		// open
		C4Group Child;
		switch (entry->Status)
		{
		case C4GroupEntry::C4GRES_InGroup:
			if (!Child.OpenAsChild(this, entry->FileName))
				return 0;
			break;
		case C4GroupEntry::C4GRES_OnDisk:
			if (!Child.Open(entry->DiskPath))
				return 0;
			break;
		default:
			return 0;
		}
		// get checksum
		CRC = Child.EntryCRC32();
	}
	else if (!entry->Size)
		CRC = 0;
	else
	{
		BYTE *pData = nullptr; bool fOwnData; CStdFile f;
		// get data
		switch (entry->Status)
		{
		case C4GroupEntry::C4GRES_InGroup:
			// create buffer
			pData = new BYTE [entry->Size]; fOwnData = true;
			// go to entry
			if (!SetFilePtr2Entry(entry->FileName)) { delete [] pData; return false; }
			// read
			if (!Read(pData, entry->Size)) { delete [] pData; return false; }
			break;
		case C4GroupEntry::C4GRES_OnDisk:
			// create buffer
			pData = new BYTE [entry->Size]; fOwnData = true;
			// open
			if (!f.Open(entry->DiskPath)) { delete [] pData; return false; }
			// read
			if (!f.Read(pData, entry->Size)) { delete [] pData; return false; }
			break;
		case C4GroupEntry::C4GRES_InMemory:
			// set
			pData = entry->MemoryBuffer; fOwnData = false;
			break;
		default:
			return false;
		}
		if (!pData) return false;
		// calc crc
		CRC = crc32(0, pData, entry->Size);
		// discard buffer
		if (fOwnData) delete [] pData;
		// add file name
		CRC = crc32(CRC, reinterpret_cast<BYTE *>(entry->FileName), SLen(entry->FileName));
	}
	// ok
	return CRC;
}

bool C4Group::OpenChild(const char* entry_name)
{
	// hack: The seach-handle would be closed twice otherwise
	p->FolderSearch.Reset();
	// Create a memory copy of ourselves
	C4Group *pOurselves = new C4Group;
	*pOurselves->p = *p;

	// Open a child from the memory copy
	C4Group hChild;
	if (!hChild.OpenAsChild(pOurselves, entry_name, false))
	{
		// Silently delete our memory copy
		pOurselves->p.reset();
		delete pOurselves;
		return false;
	}

	// hack: The seach-handle would be closed twice otherwise
	p->FolderSearch.Reset();
	hChild.p->FolderSearch.Reset();

	// We now become our own child
	*p = *hChild.p;

	// Make ourselves exclusive (until we hit our memory copy mother)
	for (C4Group *pGroup = this; pGroup != pOurselves; pGroup = pGroup->p->Mother)
		pGroup->p->ExclusiveChild = true;

	// Reset the temporary child variable so it doesn't delete anything
	hChild.p.reset();

	// Yeehaw
	return true;
}

bool C4Group::OpenMother()
{
	// This only works if we are an exclusive child
	if (!p->Mother || !p->ExclusiveChild) return false;

	// Store a pointer to our mother
	C4Group *mother = p->Mother;

	// Clear ourselves without deleting our mother
	p->ExclusiveChild = false;
	Clear();

	// hack: The seach-handle would be closed twice otherwise
	mother->p->FolderSearch.Reset();
	p->FolderSearch.Reset();
	// We now become our own mother (whoa!)
	*this = std::move(*mother);

	// Now silently delete our former mother
	delete mother;

	// Yeehaw
	return true;
}

int C4Group::PreCacheEntries(const char *search_pattern, bool cache_previous)
{
	assert(search_pattern);
	int result = 0;
	// pre-load entries to memory. return number of loaded entries.
	for (C4GroupEntry * e = p->FirstEntry; e; e = e->Next)
	{
		// is this to be cached?
		if (!WildcardListMatch(search_pattern, e->FileName)) continue;
		// if desired, cache all entries up to that one to allow rewind in unpacked memory
		// (only makes sense for groups)
		if (cache_previous && p->SourceType == P::ST_Packed)
		{
			for (C4GroupEntry * e_pre = p->FirstEntry; e_pre != e; e_pre = e_pre->Next)
				if (e_pre->Offset >= p->FilePtr)
					PreCacheEntry(e_pre);
		}
		// cache the given entry
		PreCacheEntry(e);
	}
	return result;
}

const C4GroupHeader &C4Group::GetHeader() const { return Head; }

const C4GroupEntry *C4Group::GetFirstEntry() const { return p->FirstEntry; }

void C4Group::PreCacheEntry(C4GroupEntry * entry)
{
	// skip some stuff that can not be cached or has already been cached
	if (entry->ChildGroup || entry->MemoryBuffer || !entry->Size) return;
	// now load it!
	StdBuf buf;
	if (!this->LoadEntry(entry->FileName, &buf)) return;
	entry->HoldBuffer = true;
	entry->BufferIsStdbuf = true;
	entry->Size = buf.getSize(); // update size in case group changed on disk between calls
	entry->MemoryBuffer = static_cast<BYTE *>(buf.GrabPointer());
}
