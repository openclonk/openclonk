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
int iC4GroupRewindFilePtrNoWarn = 0;
#endif

#ifdef _DEBUG
//#define C4GROUP_DUMP_ACCESS
#endif

//---------------------------- Global C4Group_Functions -------------------------------------------

char C4Group_TempPath[_MAX_PATH_LEN] = "";
char C4Group_Ignore[_MAX_PATH_LEN]="cvs;CVS;Thumbs.db;.orig;.svn";
const char **C4Group_SortList = nullptr;
bool (*C4Group_ProcessCallback)(const char *, int) = nullptr;

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
	if (!path || !path[0])
	{
		C4Group_TempPath[0] = 0;
	}
	else
	{
		SCopy(path, C4Group_TempPath, _MAX_PATH);
		AppendBackslash(C4Group_TempPath);
	}
}

const char *C4Group_GetTempPath()
{
	return C4Group_TempPath;
}

bool C4Group_TestIgnore(const char *filename)
{
	if (!*filename)
	{
		return true; //poke out empty strings
	}
	const char* name = GetFilename(filename);
	return *name == '.' //no hidden files and the directory itself
		 || name[strlen(name) - 1] == '~' //no temp files
		 || SIsModule(C4Group_Ignore, name); //not on Blacklist
}

bool C4Group_IsGroup(const char *filename)
{
	C4Group group;
	if (group.Open(filename))
	{
		group.Close();
		return true;
	}
	return false;
}

bool C4Group_CopyItem(const char *source, const char *target, bool no_sorting, bool reset_attributes)
{
	// Parameter check
	if (!source || !target || !source[0] || !target[0])
	{
		return false;
	}
	char target_path[_MAX_PATH_LEN];
	SCopy(target, target_path, _MAX_PATH);

	// Backslash terminator indicates target is a path only (append filename)
	if (target_path[SLen(target_path) - 1] == DirectorySeparator)
	{
		SAppend(GetFilename(source), target_path);
	}

	// Check for identical source and target
	// Note that attributes aren't reset here
	if (ItemIdentical(source, target_path))
	{
		return true;
	}

	// Source and target are simple items
	if (ItemExists(source) && CreateItem(target_path))
	{
		return CopyItem(source, target_path, reset_attributes);
	}

	// For items within groups, attribute resetting isn't needed, because packing/unpacking will kill all
	// attributes anyway

	// Source & target
	C4Group source_parent;
	C4Group target_parent;
	char source_parent_path[_MAX_PATH_LEN];
	char target_parent_path[_MAX_PATH_LEN];
	GetParentPath(source, source_parent_path);
	GetParentPath(target_path, target_parent_path);

	// Temp filename
	char temp_filename[_MAX_PATH_LEN];
	SCopy(C4Group_TempPath, temp_filename, _MAX_PATH);
	SAppend(GetFilename(source), temp_filename);
	MakeTempFilename(temp_filename);

	// Extract source to temp file
	if ( !source_parent.Open(source_parent_path)
	  || !source_parent.Extract(GetFilename(source), temp_filename)
	  || !source_parent.Close())
	{
		return false;
	}

	// Move temp file to target
	if ( !target_parent.Open(target_parent_path)
	  || !target_parent.SetNoSort(no_sorting)
	  || !target_parent.Move(temp_filename, GetFilename(target_path))
	  || !target_parent.Close())
	{
		EraseItem(temp_filename);
		return false;
	}

	return true;
}

bool C4Group_MoveItem(const char *source, const char *target, bool no_sorting)
{
	// Parameter check
	if (!source || !target || !source[0] || !target[0])
	{
		return false;
	}
	char target_path[_MAX_PATH_LEN];
	SCopy(target, target_path, _MAX_PATH);

	// Backslash terminator indicates target is a path only (append filename)
	if (target_path[SLen(target_path)-1] == DirectorySeparator)
	{
		SAppend(GetFilename(source), target_path);
	}

	// Check for identical source and target
	if (ItemIdentical(source, target_path))
	{
		return true;
	}

	// Source and target are simple items
	if (ItemExists(source) && CreateItem(target_path))
	{
		// erase test file, because it may block moving a directory
		EraseItem(target_path);
		return MoveItem(source, target_path);
	}

	// Source & target
	C4Group source_parent;
	C4Group target_parent;
	char source_parent_path[_MAX_PATH_LEN];
	char target_parent_path[_MAX_PATH_LEN];
	GetParentPath(source, source_parent_path);
	GetParentPath(target_path, target_parent_path);

	// Temp filename
	char temp_filename[_MAX_PATH_LEN];
	SCopy(C4Group_TempPath, temp_filename, _MAX_PATH);
	SAppend(GetFilename(source),temp_filename);
	MakeTempFilename(temp_filename);

	// Extract source to temp file
	if ( !source_parent.Open(source_parent_path)
	  || !source_parent.Extract(GetFilename(source),temp_filename)
	  || !source_parent.Close())
	{
		return false;
	}

	// Move temp file to target_path
	if ( !target_parent.Open(target_parent_path)
	  || !target_parent.SetNoSort(no_sorting)
	  || !target_parent.Move(temp_filename, GetFilename(target_path))
	  || !target_parent.Close())
	{
		EraseItem(temp_filename);
		return false;
	}

	// Delete original file
	if ( !source_parent.Open(source_parent_path)
	  || !source_parent.DeleteEntry(GetFilename(source))
	   || !source_parent.Close() )
	{
		return false;
	}

	return true;
}

bool C4Group_DeleteItem(const char *item_name, bool do_recycle)
{
	// Parameter check
	if (!item_name || !item_name[0])
	{
		return false;
	}

	// simple item?
	if (ItemExists(item_name))
	{
		if (do_recycle)
		{
			return EraseItemSafe(item_name);
		}
		else
		{
			return EraseItem(item_name);
		}
	}

	// delete from mother
	C4Group parent;
	char parent_path[_MAX_PATH_LEN];
	GetParentPath(item_name, parent_path);

	// Delete original file
	if ( !parent.Open(parent_path)
	  || !parent.DeleteEntry(GetFilename(item_name), do_recycle)
	  || !parent.Close() )
	{
		return false;
	}

	return true;
}

bool C4Group_PackDirectoryTo(const char *filename, const char *to_filename)
{
	// Check file type
	if (!DirectoryExists(filename))
	{
		return false;
	}
	// Target must not exist
	if (FileExists(to_filename))
	{
		return false;
	}
	// Ignore
	if (C4Group_TestIgnore(filename))
	{
		return true;
	}
	// Process message
	if (C4Group_ProcessCallback)
	{
		C4Group_ProcessCallback(filename, 0);
	}
	// Create group file
	C4Group group;
	if (!group.Open(to_filename, true))
	{
		return false;
	}
	// Add folder contents to group
	DirectoryIterator i(filename);
	for (; *i; i++)
	{
		// Ignore
		if (C4Group_TestIgnore(*i))
		{
			continue;
		}
		// Must pack?
		if (DirectoryExists(*i))
		{
			// Find temporary filename
			char temp_filename[_MAX_PATH_LEN];
			// At C4Group temp path
			SCopy(C4Group_TempPath, temp_filename, _MAX_PATH);
			SAppend(GetFilename(*i), temp_filename, _MAX_PATH);
			// Make temporary filename
			MakeTempFilename(temp_filename);
			// Pack and move into group
			if (!C4Group_PackDirectoryTo(*i, temp_filename))
			{
				break;
			}
			if (!group.Move(temp_filename, GetFilename(*i)))
			{
				EraseFile(temp_filename);
				break;
			}
		}
		// Add normally otherwise
		else if (!group.Add(*i, nullptr))
		{
			break;
		}
	}
	// Something went wrong?
	if (*i)
	{
		// Close group and remove temporary file
		group.Close();
		EraseItem(to_filename);
		return false;
	}
	// Reset iterator
	i.Reset();
	// Close group
	group.SortByList(C4Group_SortList, filename);
	return group.Close();
}

bool C4Group_PackDirectory(const char *filename)
{
	// Make temporary filename
	char temp_filename[_MAX_PATH_LEN];
	SCopy(filename, temp_filename, _MAX_PATH);
	MakeTempFilename(temp_filename);

	// Pack directory
	if (!C4Group_PackDirectoryTo(filename, temp_filename))
	{
		return false;
	}

	// Rename folder
	char temp_filename2[_MAX_PATH_LEN];
	SCopy(filename, temp_filename2, _MAX_PATH);
	MakeTempFilename(temp_filename2);
	if (!RenameFile(filename, temp_filename2))
	{
		return false;
	}
	// Name group file
	if (!RenameFile(temp_filename, filename))
	{
		return false;
	}
	// Last: Delete folder
	return EraseDirectory(temp_filename2);
}

bool C4Group_UnpackDirectory(const char *filename)
{
	// Already unpacked: success
	if (DirectoryExists(filename))
	{
		return true;
	}

	// Not a real file: unpack mother directory first
	char parent_filename[_MAX_PATH_LEN];
	if (!FileExists(filename)
	 && GetParentPath(filename, parent_filename)
	 && !C4Group_UnpackDirectory(parent_filename))
	{
		return false;
	}

	// Open group
	C4Group group;
	if (!group.Open(filename))
	{
		return false;
	}

	// Process message
	if (C4Group_ProcessCallback)
	{
		C4Group_ProcessCallback(filename, 0);
	}

	// Create target directory
	char target_directory[_MAX_PATH_LEN];
	SCopy(filename, target_directory, _MAX_PATH);
	MakeTempFilename(target_directory);
	if (!CreatePath(target_directory))
	{
		group.Close();
		return false;
	}

	// Extract files to folder
	if (!group.Extract("*",target_directory))
	{
		group.Close();
		return false;
	}

	// Close group
	group.Close();

	// Rename group file
	char temp_filename[_MAX_PATH_LEN];
	SCopy(filename, temp_filename, _MAX_PATH);
	MakeTempFilename(temp_filename);
	if (!RenameFile(filename, temp_filename))
	{
		return false;
	}

	// Rename target directory
	if (!RenameFile(target_directory, filename))
	{
		return false;
	}

	// Delete renamed group file
	return EraseItem(temp_filename);
}

bool C4Group_ExplodeDirectory(const char *filename)
{
	// Ignore
	if (C4Group_TestIgnore(filename))
	{
		return true;
	}

	// Unpack this directory
	if (!C4Group_UnpackDirectory(filename))
	{
		return false;
	}

	// Explode all children
	ForEachFile(filename, C4Group_ExplodeDirectory);

	// Success
	return true;
}

bool C4Group_ReadFile(const char *filename, char **data, size_t *size)
{
	// security
	if (!filename || !data)
	{
		return false;
	}

	// get mother path & file name
	char parent_path[_MAX_PATH_LEN];
	GetParentPath(filename, parent_path);
	const char *entry_name = GetFilename(filename);

	// open parent group
	C4Group parent_group;
	if (!parent_group.Open(parent_path))
	{
		return false;
	}
	// access the file
	size_t filesize;
	if (!parent_group.AccessEntry(entry_name, &filesize))
	{
		return false;
	}
	// create buffer
	*data = new char [filesize];
	// read it
	if (!parent_group.Read(*data, filesize))
	{
		delete [] *data;
		*data = nullptr;
		return false;
	}
	// ok
	parent_group.Close();
	if (size)
	{
		*size = filesize;
	}
	return true;
}

void MemScramble(BYTE *buffer, int size)
{
	// XOR deface
	for (int cnt = 0; cnt < size; cnt++)
	{
		buffer[cnt] ^= 237;
	}
	// BYTE swap
	for (int cnt = 0; cnt + 2 < size; cnt += 3)
	{
		BYTE temp = buffer[cnt];
		buffer[cnt] = buffer[cnt + 2];
		buffer[cnt + 2] = temp;
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
	BYTE *pInMemEntry = nullptr;
	size_t iInMemEntrySize = 0; // for reading from entries prefetched into memory
#ifdef _DEBUG
	StdStrBuf sPrevAccessedEntry;
#endif
	// Folder only
	DirectoryIterator FolderSearch;
	C4GroupEntry FolderSearchEntry;
	C4GroupEntry LastFolderSearchEntry;

	bool LogToStdOutput = false;
	bool(*ProcessCallback)(const char *, int) = nullptr;
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

	SCopy(GetFilename(*directories),FileName, _MAX_FNAME);
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
	new_p->ProcessCallback = p->ProcessCallback;
	new_p->NoSort = p->NoSort;
	new_p->LogToStdOutput = p->LogToStdOutput;

	InplaceReconstruct(&Head);
	p = std::move(new_p);
}

C4Group::~C4Group()
{
	Clear();
}

bool C4Group::Error(const char *status_message)
{
	p->ErrorString = status_message;
	return false;
}

const char *C4Group::GetError()
{
	return p->ErrorString.c_str();
}

void C4Group::SetStdOutput(bool log_status)
{
	p->LogToStdOutput = log_status;
}

bool C4Group::Open(const char *group_name, bool do_create)
{
	if (!group_name)
	{
		return Error("Open: Null filename");
	}
	if (!group_name[0])
	{
		return Error("Open: Empty filename");
	}

	char group_name_native[_MAX_FNAME];
	SCopy(group_name, group_name_native, _MAX_FNAME);
	// Convert to native path
	SReplaceChar(group_name_native, AltDirectorySeparator, DirectorySeparator);

	// Real reference
	if (FileExists(group_name_native))
	{
		Init();
		return OpenReal(group_name_native);
	}

	// If requested, try creating a new group file
	if (do_create)
	{
		CStdFile temp;
		if (temp.Create(group_name_native, false))
		{
			// Temporary file has been created
			temp.Close();
			Init();
			p->SourceType = P::ST_Packed; p->Modified = true;
			p->FileName = group_name_native;
			return true;
		}
	}

	// While not a real reference (child group), trace back to mother group or folder.
	// Open mother and child in exclusive mode.
	char group_name_real[_MAX_FNAME];
	SCopy(group_name_native, group_name_real, _MAX_FNAME);
	do
	{
		if (!TruncatePath(group_name_real))
		{
			return Error(FormatString(R"(Open("%s"): File not found)", group_name_native).getData());
		}
	}
	while (!FileExists(group_name_real));

	// Open mother and child in exclusive mode
	C4Group *mother = new C4Group;
	mother->SetStdOutput(p->LogToStdOutput);
	if (!mother->Open(group_name_real))
	{
		Clear();
		Error(mother->GetError());
		delete mother;
		return false;
	}
	if (!OpenAsChild(mother, group_name_native+SLen(group_name_real) + 1, true))
	{
		Clear();
		return false;
	}

	// Success
	return true;
}

bool C4Group::OpenReal(const char *filename)
{
	// Get original filename
	if (!filename)
	{
		return false;
	}
	p->FileName = filename;

	// Folder
	if (DirectoryExists(GetName()))
	{
		// Ignore
		if (C4Group_TestIgnore(filename))
		{
			return Error(FormatString("OpenReal: filename '%s' ignored", filename).getData());
		}
		// OpenReal: Simply set status and return
		p->SourceType = P::ST_Unpacked;
		ResetSearch();
		// Success
		return true;
	}

	// File: Try reading header and entries
	if (OpenRealGrpFile())
	{
		p->SourceType = P::ST_Packed;
		ResetSearch();
		return true;
	}
	else
	{
		return false;
	}

	return Error("OpenReal: Not a valid group");
}

bool C4Group::OpenRealGrpFile()
{

	// Open StdFile
	if (!p->StdFile.Open(GetName(), true))
	{
		return Error("OpenRealGrpFile: Cannot open standard file");
	}

	// Read header
	if (!p->StdFile.Read((BYTE*)&Head, sizeof(C4GroupHeader)))
	{
		return Error("OpenRealGrpFile: Error reading header");
	}
	MemScramble((BYTE*)&Head, sizeof(C4GroupHeader));
	p->EntryOffset += sizeof(C4GroupHeader);

	// Check Header
	if (!SEqual(Head.Id, C4GroupFileID)
	|| (Head.Ver1 != C4GroupFileVer1)
	|| (Head.Ver2 > C4GroupFileVer2))
	{
		return Error("OpenRealGrpFile: Invalid header");
	}

	// Read Entries
	int file_entries = Head.Entries;
	Head.Entries = 0; // Reset, will be recounted by AddEntry
	C4GroupEntryCore corebuf;
	for (int cnt = 0; cnt<file_entries; cnt++)
	{
		if (!p->StdFile.Read((BYTE*)&corebuf, sizeof(C4GroupEntryCore)))
		{
			return Error("OpenRealGrpFile: Error reading entries");
		}
		// New C4Groups have filenames in UTF-8
		StdStrBuf entryname(corebuf.FileName);
		entryname.EnsureUnicode();
		// Prevent overwriting of user stuff by malicuous groups
		C4InVal::ValidateFilename(const_cast<char *>(entryname.getData()),entryname.getLength());
		p->EntryOffset+=sizeof(C4GroupEntryCore);
		if (!AddEntry(C4GroupEntry::C4GRES_InGroup,
				      !!corebuf.ChildGroup,
		              corebuf.FileName,
					  corebuf.Size,
		              entryname.getData(),
		              nullptr,
					  false,
					  false,
		              !!corebuf.Executable))
		{
			return Error("OpenRealGrpFile: Cannot add entry");
		}
	}

	return true;
}

bool C4Group::AddEntry(C4GroupEntry::EntryStatus status,
                       bool add_as_child,
                       const char *filename,
                       long size,
                       const char *entry_name,
                       BYTE *buffer,
                       bool delete_on_disk,
                       bool hold_buffer,
                       bool is_executable,
                       bool buffer_is_stdbuf)
{

	// Folder: add file to folder immediately
	if (p->SourceType == P::ST_Unpacked)
	{
		// Close open StdFile
		p->StdFile.Close();

		// Get path to target folder file
		char target_folder_name[_MAX_FNAME];
		SCopy(GetName(),target_folder_name, _MAX_FNAME);
		AppendBackslash(target_folder_name);
		if (entry_name)
		{
			SAppend(entry_name, target_folder_name);
		}
		else
		{
			SAppend(GetFilename(filename), target_folder_name);
		}

		switch (status)
		{

		case C4GroupEntry::C4GRES_OnDisk: // Copy/move file to folder
			if (!CopyItem(filename, target_folder_name))
			{
				return false;
			}
			// Reset directory iterator to reflect new file
			ResetSearch(true);
			if (delete_on_disk && !EraseItem(filename))
			{
				return false;
			}
			return true;

		case C4GroupEntry::C4GRES_InMemory: { // Save buffer to file in folder
			CStdFile file;
			bool okay = false;
			if (file.Create(target_folder_name, !!add_as_child))
			{
				okay = !!file.Write(buffer, size);
			}
			file.Close();
			ResetSearch(true);

			if (hold_buffer)
			{
				if (buffer_is_stdbuf) StdBuf::DeletePointer(buffer);
				else delete [] buffer;
			}

			return okay;
		}

		default: break; // InGrp & Deleted ignored
		}

		return Error("Add to folder: Invalid request");
	}


	// Group file: add to virtual entry list

	C4GroupEntry *next;
	C4GroupEntry *last;
	C4GroupEntry *current;

	// Delete existing entries of same name
	current = GetEntry(GetFilename(entry_name ? entry_name : filename));
	if (current)
	{
		current->Status = C4GroupEntry::C4GRES_Deleted;
		Head.Entries--;
	}

	// Allocate memory for new entry
	next = new C4GroupEntry;

	// Find end of list
	for (last = p->FirstEntry; last && last->Next; last = last->Next) {}

	// Init entry core data
	if (entry_name) SCopy(entry_name, next->FileName, _MAX_FNAME);
	else SCopy(GetFilename(filename),next->FileName, _MAX_FNAME);

	next->Size = size;
	next->ChildGroup = add_as_child;
	next->Offset = 0;
	next->Executable = is_executable;
	next->DeleteOnDisk = delete_on_disk;
	next->HoldBuffer = hold_buffer;
	next->BufferIsStdbuf = buffer_is_stdbuf;
	if (last)
	{
		next->Offset = last->Offset + last->Size;
	}

	// Init list entry data
	SCopy(filename, next->DiskPath, _MAX_FNAME);
	next->Status = status;
	next->MemoryBuffer = buffer;
	next->Next = nullptr;
	next->NoSort = p->NoSort;

	// Append entry to list
	if (last) last->Next = next;
	else p->FirstEntry = next;

	// Increase virtual file count of group
	Head.Entries++;

	return true;
}

C4GroupEntry* C4Group::GetEntry(const char *entry_name)
{
	if (p->SourceType == P::ST_Unpacked)
	{
		return nullptr;
	}
	for (C4GroupEntry *entry = p->FirstEntry; entry; entry = entry->Next)
	{
		if (entry->Status != C4GroupEntry::C4GRES_Deleted
		&&  WildcardMatch(entry_name, entry->FileName))
		{
			return entry;
		}
	}
	return nullptr;
}

bool C4Group::Close()
{
	bool rewrite = false;

	if (p->SourceType == P::ST_None)
	{
		return false;
	}

	// Folder: just close
	if (p->SourceType == P::ST_Unpacked)
	{
		CloseExclusiveMother();
		Clear();
		return true;
	}

	// Rewrite check
	for (C4GroupEntry *entry = p->FirstEntry; entry; entry = entry->Next)
	{
		if (entry->Status != C4GroupEntry::C4GRES_InGroup)
		{
			rewrite = true;
		}
	}
	if (p->Modified)
	{
		rewrite = true;
	}

	// No rewrite: just close
	if (!rewrite)
	{
		CloseExclusiveMother();
		Clear();
		return true;
	}

	if (p->LogToStdOutput)
	{
		printf("Writing group file...\n");
	}

	// Set new version
	Head.Ver1 = C4GroupFileVer1;
	Head.Ver2 = C4GroupFileVer2;

	// Automatic sort
	SortByList(C4Group_SortList);

	// Save group contents to disk
	bool success = Save(false);

	// Close files
	CloseExclusiveMother();
	Clear();

	return !!success;
}

bool C4Group::Save(bool reopen)
{
	char temp_filename[_MAX_FNAME+1];
	char group_filename[_MAX_FNAME+1];

	// Create temporary core list with new actual offsets to be saved
	int32_t contents_size = 0;
	C4GroupEntryCore *save_core = new C4GroupEntryCore[Head.Entries];
	int core_index = 0;
	for (C4GroupEntry *entry = p->FirstEntry; entry; entry = entry->Next)
	{
		if (entry->Status != C4GroupEntry::C4GRES_Deleted)
		{
			save_core[core_index]=(C4GroupEntryCore)*entry;
			// Make actual offset
			save_core[core_index].Offset = contents_size;
			contents_size += entry->Size;
			core_index++;
		}
	}

	// Hold contents in memory?
	bool hold_in_memory = !reopen && p->Mother && contents_size < C4GroupSwapThreshold;
	if (!hold_in_memory)
	{
		// Create target temp file (in temp directory!)
		SCopy(GetName(), group_filename, _MAX_FNAME);
		if (C4Group_TempPath[0])
		{
			SCopy(C4Group_TempPath, temp_filename, _MAX_FNAME);
			SAppend(GetFilename(GetName()),temp_filename, _MAX_FNAME);
		}
		else
		{
			SCopy(GetName(), temp_filename, _MAX_FNAME);
		}
		MakeTempFilename(temp_filename);
		// (Temp file must not have the same name as the group.)
		if (SEqual(temp_filename, group_filename))
		{
			SAppend(".tmp", temp_filename); // Add a second temp extension
			MakeTempFilename(temp_filename);
		}
	}

	// Create the new (temp) group file
	CStdFile temp_file;
	if (!temp_file.Create(temp_filename, true, false, hold_in_memory))
	{
		delete [] save_core;
		return Error("Close: ...");
	}

	// Save header and core list
	C4GroupHeader header_buffer = Head;
	MemScramble((BYTE*)&header_buffer, sizeof(C4GroupHeader));
	if (!temp_file.Write((BYTE*)&header_buffer, sizeof(C4GroupHeader))
	 || !temp_file.Write((BYTE*)save_core, Head.Entries*sizeof(C4GroupEntryCore)))
	{
		temp_file.Close();
		delete [] save_core;
		return Error("Close: ...");
	}
	delete [] save_core;

	// Save Entries to temp file
	int total_size = 0;
	for (C4GroupEntry *entry = p->FirstEntry; entry; entry = entry->Next)
	{
		total_size += entry->Size;
	}
	int size_done = 0;
	for (C4GroupEntry *entry = p->FirstEntry; entry; entry = entry->Next)
	{
		if (AppendEntry2StdFile(entry, temp_file))
		{
			size_done += entry->Size;
			if (total_size && p->ProcessCallback)
			{
				p->ProcessCallback(entry->FileName, 100 * size_done / total_size);
			}
		}
		else
		{
			temp_file.Close();
			return false;
		}
	}

	// Write
	StdBuf *buffer;
	temp_file.Close(hold_in_memory ? &buffer : nullptr);

	// Child: move temp file to mother
	if (p->Mother)
	{
		if (hold_in_memory)
		{
			if (!p->Mother->Add(GetFilename(GetName()), *buffer, true, true))
			{
				delete buffer;
				CloseExclusiveMother();
				Clear();
				return Error("Close: Cannot move rewritten child data to mother");
			}
			delete buffer;
		}
		else
		{
			if (!p->Mother->Move(temp_filename, GetFilename(GetName())))
			{
				CloseExclusiveMother();
				Clear();
				return Error("Close: Cannot move rewritten child temp file to mother");
			}
		}
		Clear();
		return true;
	}

	// Clear (close file)
	Clear();

	// Delete old group file, rename new file
	if (!EraseFile(group_filename))
	{
		return Error("Close: Cannot erase temp file");
	}
	if (!RenameFile(temp_filename, group_filename))
	{
		return Error("Close: Cannot rename group file");
	}

	// Should reopen the file?
	if (reopen)
	{
		OpenReal(group_filename);
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
	CStdFile source;
	BYTE buffer;

	switch (entry->Status)
	{

	case C4GroupEntry::C4GRES_InGroup: // Copy from group to std file
		if (!SetFilePtr(entry->Offset))
			return Error("AE2S: Cannot set file pointer");
		for (long current_size = entry->Size; current_size > 0; current_size--)
		{
			if (!Read(&buffer, 1))
			{
				return Error("AE2S: Cannot read entry from group file");
			}
			if (!target.Write(&buffer, 1))
			{
				return Error("AE2S: Cannot write to target file");
			}
		}
		break;

	case C4GroupEntry::C4GRES_OnDisk: // Copy/move from disk item to std file
	{
		char file_source[_MAX_FNAME_LEN];
		SCopy(entry->DiskPath, file_source, _MAX_FNAME);

		// Disk item is a directory
		if (DirectoryExists(entry->DiskPath))
		{
			return Error("AE2S: Cannot add directory to group file");
		}

		// Resort group if neccessary
		// (The group might be renamed by adding, forcing a resort)
		bool has_temp_file = false;
		if (entry->ChildGroup
		&& !entry->NoSort
		&& !SEqual(GetFilename(file_source), entry->FileName))
		{
			// copy group
			MakeTempFilename(file_source);
			if (!CopyItem(entry->DiskPath, file_source))
			{
				return Error("AE2S: Cannot copy item");
			}
			// open group and resort
			C4Group SortGrp;
			if (!SortGrp.Open(file_source))
			{
				return Error("AE2S: Cannot open group");
			}
			if (!SortGrp.SortByList(C4Group_SortList, entry->FileName))
			{
				return Error("AE2S: Cannot resort group");
			}
			has_temp_file = true;
			// close group (won't be saved if the sort didn't change)
			SortGrp.Close();
		}

		// Append disk source to target file
		if (!source.Open(file_source, !!entry->ChildGroup))
		{
			return Error("AE2S: Cannot open on-disk file");
		}
		for (long current_size = entry->Size; current_size > 0; current_size--)
		{
			if (!source.Read(&buffer, 1))
			{
				source.Close();
				return Error("AE2S: Cannot read on-disk file");
			}
			if (!target.Write(&buffer, 1))
			{
				source.Close();
				return Error("AE2S: Cannot write to target file");
			}
		}
		source.Close();

		// Erase temp file
		if (has_temp_file)
		{
			EraseItem(file_source);
		}
		// Erase disk source if requested
		if (entry->DeleteOnDisk)
		{
			EraseItem(entry->DiskPath);
		}

		break;
	}

	case C4GroupEntry::C4GRES_InMemory: // Copy from mem to std file
		if (!entry->MemoryBuffer)
		{
			return Error("AE2S: no buffer");
		}
		if (!target.Write(entry->MemoryBuffer, entry->Size))
		{
			return Error("AE2S: writing error");
		}
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
		p->SearchPtr = nullptr;
		p->FolderSearch.Reset(GetName(), reload_contents);
		if (*p->FolderSearch)
		{
			p->FolderSearchEntry.Set(p->FolderSearch, GetName());
			p->SearchPtr = &p->FolderSearchEntry;
		}
		break;
	case P::ST_Packed:
		p->SearchPtr = p->FirstEntry;
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
	{
		entry_name = "*";
	}
	// Search by group type
	C4GroupEntry *pEntry;
	switch (p->SourceType)
	{
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case P::ST_Packed:
		for (pEntry = p->SearchPtr; pEntry; pEntry = pEntry->Next)
			if (pEntry->Status != C4GroupEntry::C4GRES_Deleted)
				if (WildcardMatch(entry_name, pEntry->FileName))
				{
					p->SearchPtr = pEntry->Next;
					return pEntry;
				}
		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case P::ST_Unpacked:
		for (pEntry = p->SearchPtr; pEntry; pEntry = GetNextFolderEntry())
			if (WildcardMatch(entry_name, pEntry->FileName))
				if (!C4Group_TestIgnore(pEntry->FileName))
				{
					p->LastFolderSearchEntry=(*pEntry);
					pEntry=&p->LastFolderSearchEntry;
					p->SearchPtr = GetNextFolderEntry();
					return pEntry;
				}
		break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	default: break; // InGrp & Deleted ignored
	}
	// No entry found: reset search pointer
	p->SearchPtr = nullptr;
	return nullptr;
}

bool C4Group::SetFilePtr(int offset)
{

	if (p->SourceType == P::ST_Unpacked)
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
		if (!Read(&buf, 1)) return false;
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
			if (!p->Mother->Read(buffer, size))
				{ RewindFilePtr(); return Error("Read:"); }
		}
		// Regular group: read from standard file
		else
		{
			if (!p->StdFile.Read(buffer, size))
				{ RewindFilePtr(); return Error("Read:"); }
		}
		p->FilePtr+=size;
		break;
	case P::ST_Unpacked:
		if (!p->StdFile.Read(buffer, size)) return Error("Read: Error reading from folder contents");
		break;
	default: break; // InGrp & Deleted ignored
	}

	return true;
}

bool C4Group::AdvanceFilePtr(int offset)
{
	// Child group file: pass command to mother
	if ((p->SourceType == P::ST_Packed) && p->Mother)
	{

		// Ensure mother file ptr for it may have been moved by foreign access to mother
		if (!p->Mother->EnsureChildFilePtr(this))
			return false;

		if (!p->Mother->AdvanceFilePtr(offset))
			return false;

	}
	// Regular group
	else if (p->SourceType == P::ST_Packed)
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
		szCurrAccessedEntry = nullptr;
	}
#endif

	// Child group file: pass command to mother
	if ((p->SourceType == P::ST_Packed) && p->Mother)
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

	p->FilePtr = 0;

	return true;
}

bool C4Group::Merge(const char *folders)
{
	bool move = true;

	if (p->LogToStdOutput) printf("%s...\n",move ? "Moving" : "Adding");

	// Add files & directories
	char szFileName[_MAX_FNAME_LEN];
	int iFileCount = 0;
	DirectoryIterator i;

	// Process segmented path & search wildcards
	char cSeparator = (SCharCount(';', folders) ? ';' : '|');
	for (int cseg = 0; SCopySegment(folders, cseg, szFileName, cSeparator); cseg++)
	{
		i.Reset(szFileName);
		while (*i)
		{
			// File count
			iFileCount++;
			// Process output & callback
			if (p->LogToStdOutput) printf("%s\n",GetFilename(*i));
			if (p->ProcessCallback)
				p->ProcessCallback(GetFilename(*i),0); // cbytes/tbytes
			// AddEntryOnDisk
			AddEntryOnDisk(*i, nullptr, move);
			++i;
		}
	}

	if (p->LogToStdOutput) printf("%d file(s) %s.\n",iFileCount, move ? "moved" : "added");

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
		char temp_filename[_MAX_PATH_LEN];
		if (C4Group_TempPath[0]) { SCopy(C4Group_TempPath, temp_filename, _MAX_PATH); SAppend(GetFilename(filename),temp_filename, _MAX_PATH); }
		else SCopy(filename, temp_filename, _MAX_PATH);
		MakeTempFilename(temp_filename);
		// Copy or move item to temp file (moved items might be killed if later process fails)
		if (move) { if (!MoveItem(filename, temp_filename)) return Error("AddEntryOnDisk: Move failure"); }
		else { if (!CopyItem(filename, temp_filename)) return Error("AddEntryOnDisk: Copy failure"); }
		// Pack temp file
		if (!C4Group_PackDirectory(temp_filename)) return Error("AddEntryOnDisk: Pack directory failure");
		// Add temp file
		if (!entry_name) entry_name = GetFilename(filename);
		filename = temp_filename;
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

	if (p->LogToStdOutput)
	{
		printf("%s %s as %s...\n", move ? "Moving" : "Adding", GetFilename(filename), entry_name);
	}

	return AddEntryOnDisk(filename, entry_name, move);
}

bool C4Group::Move(const char *filename, const char *entry_name)
{
	bool move = true;

	if (p->LogToStdOutput)
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
		char filespec[_MAX_FNAME_LEN];
		for (int cseg = 0; SCopySegment(files, cseg, filespec, cSeparator, _MAX_FNAME); cseg++)
			if (!Delete(filespec, recursive))
				success = false;
		return success; // Would be nicer to return the file count and add up all counts from recursive actions...
	}

	// Delete all matching Entries
	ResetSearch();
	while ((tentry = SearchNextEntry(files)))
	{
		// StdOutput
		if (p->LogToStdOutput) printf("%s\n",tentry->FileName);
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
					hChild.SetStdOutput(p->LogToStdOutput);
					hChild.Delete(files, recursive);
					hChild.Close();
				}
	}

	// StdOutput
	if (p->LogToStdOutput)
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
		if (!(pEntry = GetEntry(filename))) return false;
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
		char path[_MAX_FNAME_LEN];
		sprintf(path,"%s%c%s", GetName(),DirectorySeparator, filename);

		if (do_recycle)
		{
			if (!EraseItemSafe(path)) return false;
		}
		else
		{
			if (!EraseItem(path)) return false;
		}
		// refresh file list
		ResetSearch(true);
		break;
	default: break; // InGrp & Deleted ignored
	}
	return true;
}

bool C4Group::Rename(const char *filename, const char *new_name)
{

	if (p->LogToStdOutput)
	{
		printf("Renaming %s to %s...\n",filename, new_name);
	}

	switch (p->SourceType)
	{
	case P::ST_Packed:
		// Get entry
		C4GroupEntry *pEntry;
		if (!(pEntry = GetEntry(filename)))
		{
			return Error("Rename: File not found");
		}
		// Check double name
		if (GetEntry(new_name) && !SEqualNoCase(new_name, filename))
		{
			return Error("Rename: File exists already");
		}
		// Rename
		SCopy(new_name, pEntry->FileName, _MAX_FNAME);
		p->Modified = true;
		break;
	case P::ST_Unpacked:
		p->StdFile.Close();

		char path[_MAX_FNAME_LEN];
		SCopy(GetName(),path, _MAX_PATH-1);
		AppendBackslash(path);
		SAppend(filename, path, _MAX_PATH);

		char path2[_MAX_FNAME_LEN];
		SCopy(GetName(),path2, _MAX_PATH-1);
		AppendBackslash(path2);
		SAppend(new_name, path2, _MAX_PATH);

		if (!RenameFile(path, path2))
		{
			return Error("Rename: Failure");
		}
		// refresh file list
		ResetSearch(true);
		break;
	default: break; // InGrp & Deleted ignored
	}

	return true;
}

bool C4Group_IsExcluded(const char *filename, const char *exclude_list)
{
	// No file or no exclude list
	if (!filename || !filename[0] || !exclude_list || !exclude_list[0])
	{
		return false;
	}
	// Process segmented exclude list
	char separator = (SCharCount(';', exclude_list) ? ';' : '|');
	char segment[_MAX_PATH_LEN];
	for (int i = 0; SCopySegment(exclude_list, i, segment, separator); i++)
	{
		if (WildcardMatch(segment, GetFilename(filename)))
		{
			return true;
		}
	}
	// No match
	return false;
}

bool C4Group::Extract(const char *files, const char *destination, const char *exclude)
{
	// StdOutput
	if (p->LogToStdOutput)
	{
		printf("Extracting");
		if (destination)
		{
			printf(" to %s",destination);
		}
		printf("...\n");
	}

	int filecount = 0;
	int current_bytes = 0;
	int total_bytes = EntrySize();
	C4GroupEntry *entry;


	// Process segmented list
	char separator = (SCharCount(';', files) ? ';' : '|');
	char filename[_MAX_PATH_LEN];
	for (int segment_index = 0; SCopySegment(files, segment_index, filename, separator); segment_index++)
	{
		// Search all entries
		ResetSearch();
		while ((entry = SearchNextEntry(filename)))
		{
			// skip?
			if (C4Group_IsExcluded(entry->FileName, exclude))
			{
				continue;
			}
			// Process data & output
			if (p->LogToStdOutput)
			{
				printf("%s\n", entry->FileName);
			}
			current_bytes += entry->Size;
			if (p->ProcessCallback)
			{
				p->ProcessCallback(entry->FileName, 100 * current_bytes / std::max(total_bytes, 1));
			}

			// Extract
			if (!ExtractEntry(entry->FileName, destination))
			{
				return Error("Extract: Could not extract entry");
			}

			filecount++;
		}
	}

	if (p->LogToStdOutput)
	{
		printf("%d file(s) extracted.\n", filecount);
	}

	return true;
}

bool C4Group::ExtractEntry(const char *filename, const char *destination)
{
	CStdFile temp_file;
	CStdFile dummy;
	char temp_file_name[_MAX_FNAME_LEN];
	char target_file_name[_MAX_FNAME_LEN];

	// Target file name
	if (destination)
	{
		SCopy(destination, target_file_name, _MAX_FNAME-1);
		if (DirectoryExists(target_file_name))
		{
			AppendBackslash(target_file_name);
			SAppend(filename, target_file_name, _MAX_FNAME);
		}
	}
	else
	{
		SCopy(filename, target_file_name, _MAX_FNAME);
	}

	// Extract
	switch (p->SourceType)
	{
	case P::ST_Packed: // Copy entry to target
		// Get entry
		C4GroupEntry *entry;
		if (!(entry = GetEntry(filename)))
		{
			return Error("Extract: Entry not found");
		}

		// Create dummy file to reserve target file name
		dummy.Create(target_file_name, false);
		dummy.Write("Dummy",5);
		dummy.Close();

		// Make temp target file name
		SCopy(target_file_name, temp_file_name, _MAX_FNAME);
		MakeTempFilename(temp_file_name);
		// Create temp target file
		if (!temp_file.Create(temp_file_name, !!entry->ChildGroup, !!entry->Executable))
		{
			return Error("Extract: Cannot create target file");
		}
		// Write entry file to temp target file
		if (!AppendEntry2StdFile(entry, temp_file))
		{
			// Failure: close and erase temp target file
			temp_file.Close();
			EraseItem(temp_file_name);
			// Also erase reservation target file
			EraseItem(target_file_name);
			// Failure
			return false;
		}
		// Close target file
		temp_file.Close();
		// Make temp file to original file
		if (!EraseItem(target_file_name))
		{
			return Error("Extract: Cannot erase temporary file");
		}
		if (!RenameItem(temp_file_name, target_file_name))
		{
			return Error("Extract: Cannot rename temporary file");
		}
		break;
	case P::ST_Unpacked: // Copy item from folder to target
		char path[_MAX_FNAME_LEN];
		sprintf(path,"%s%c%s", GetName(),DirectorySeparator, filename);
		if (!CopyItem(path, target_file_name))
		{
			return Error("ExtractEntry: Cannot copy item");
		}
		break;
	}
	return true;
}


bool C4Group::OpenAsChild(C4Group *mother, const char *entry_name, bool is_exclusive, bool do_create)
{

	if (!mother) return Error("OpenAsChild: No mother specified");

	if (SCharCount('*',entry_name))
	{
		return Error("OpenAsChild: No wildcards allowed");
	}

	// Open nested child group check: If entry_name is a reference to
	// a nested group, open the first mother (in specified mode), then open the child
	// in exclusive mode

	if (SCharCount(DirectorySeparator, entry_name))
	{
		char mothername[_MAX_FNAME_LEN];
		SCopyUntil(entry_name, mothername, DirectorySeparator, _MAX_FNAME);

		C4Group *mother2;
		mother2 = new C4Group;
		mother2->SetStdOutput(p->LogToStdOutput);
		if (!mother2->OpenAsChild(mother, mothername, is_exclusive))
		{
			delete mother2;
			return Error("OpenAsChild: Cannot open mother");
		}
		return OpenAsChild(mother2, entry_name + SLen(mothername) + 1, true);
	}

	// Init
	Init();
	p->FileName = entry_name;
	p->Mother = mother;
	p->ExclusiveChild = is_exclusive;

	// Folder: Simply set status and return
	char path[_MAX_FNAME_LEN];
	SCopy( GetFullName().getData(), path, _MAX_FNAME);
	if (DirectoryExists(path))
	{
		p->FileName = path;
		p->SourceType = P::ST_Unpacked;
		ResetSearch();
		return true;
	}

	// Get original entry name
	C4GroupEntry *centry;
	if ((centry = p->Mother->GetEntry(GetName())))
	{
		p->FileName = centry->FileName;
	}

	// Access entry in mother group
	size_t size;
	if ((!p->Mother->AccessEntry(GetName(), &size, nullptr, true)))
	{
		if (!do_create)
		{
			CloseExclusiveMother();
			Clear();
			return Error("OpenAsChild: Entry not in mother group");
		}
		else
		{
			// Create - will be added to mother in Close()
			p->SourceType = P::ST_Packed;
			p->Modified = true;
			return true;
		}
	}

	// Child Group?
	if (centry && !centry->ChildGroup)
	{
		CloseExclusiveMother();
		Clear();
		return Error("OpenAsChild: Is not a child group");
	}

	// Read header
	// Do not do size checks for packed subgroups of unpacked groups (there will be no entry),
	//  because that would be the PACKED size which can actually be smaller than sizeof(C4GroupHeader)!
	if (size < sizeof(C4GroupHeader) && centry)
	{
		CloseExclusiveMother();
		Clear();
		return Error("OpenAsChild: Entry too small");
	}
	if (!p->Mother->Read(&Head, sizeof(C4GroupHeader)))
	{
		CloseExclusiveMother();
		Clear();
		return Error("OpenAsChild: Entry reading error");
	}
	MemScramble((BYTE*)&Head, sizeof(C4GroupHeader));
	p->EntryOffset += sizeof(C4GroupHeader);

	// Check Header
	if (!SEqual(Head.Id, C4GroupFileID)
	|| (Head.Ver1 != C4GroupFileVer1)
	|| (Head.Ver2 > C4GroupFileVer2))
	{
		CloseExclusiveMother();
		Clear();
		return Error("OpenAsChild: Invalid Header");
	}

	// Read Entries
	C4GroupEntryCore buffer;
	int file_entries = Head.Entries;
	Head.Entries = 0; // Reset, will be recounted by AddEntry
	for (int cnt = 0; cnt < file_entries; cnt++)
	{
		if (!p->Mother->Read(&buffer, sizeof(C4GroupEntryCore)))
		{
			CloseExclusiveMother();
			Clear();
			return Error("OpenAsChild: Entry reading error");
		}
		p->EntryOffset += sizeof(C4GroupEntryCore);
		if (!AddEntry(C4GroupEntry::C4GRES_InGroup,
				      !!buffer.ChildGroup,
		              buffer.FileName,
					  buffer.Size,
		              nullptr,
					  nullptr,
					  false,
					  false,
		              !!buffer.Executable))
		{
			CloseExclusiveMother();
			Clear();
			return Error("OpenAsChild: Insufficient memory");
		}
	}

	ResetSearch();

	// File
	p->SourceType = P::ST_Packed;

	// save position in mother group
	if (centry)
	{
		p->MotherOffset = centry->Offset;
	}

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
	{
		return false;
	}
#ifdef _DEBUG
	szCurrAccessedEntry = fname.getMData();
#endif
	bool okay = SetFilePtr2Entry(fname.getData(), needs_to_be_a_group);
#ifdef _DEBUG
	p->sPrevAccessedEntry.Copy(szCurrAccessedEntry);
	szCurrAccessedEntry = nullptr;
#endif
	if (!okay)
	{
		return false;
	}
	if (filename)
	{
		SCopy(fname.getData(),filename);
	}
	if (size)
	{
		*size = p->iCurrFileSize;
	}
	return true;
}

bool C4Group::AccessNextEntry(const char *wildcard,
                              size_t *size,
							  char *filename,
                              bool start_at_filename)
{
	char entry_name[_MAX_FNAME_LEN];
	if (!FindNextEntry(wildcard, entry_name, &p->iCurrFileSize, start_at_filename))
	{
		return false;
	}
#ifdef _DEBUG
	szCurrAccessedEntry = filename;
#endif
	bool okay = SetFilePtr2Entry(entry_name);
#ifdef _DEBUG
	szCurrAccessedEntry = nullptr;
#endif
	if (!okay)
	{
		return false;
	}
	if (filename)
	{
		SCopy(entry_name, filename);
	}
	if (size)
	{
		*size = p->iCurrFileSize;
	}
	return true;
}

bool C4Group::SetFilePtr2Entry(const char *entry_name, bool needs_to_be_a_group)
{
	C4GroupEntry *entry = GetEntry(entry_name);
	// Read cached entries directly from memory (except child groups. that is not supported.)
	if (entry && entry->MemoryBuffer && !needs_to_be_a_group)
	{
		p->pInMemEntry = entry->MemoryBuffer;
		p->iInMemEntrySize = entry->Size;
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
		if ((!entry) || (entry->Status != C4GroupEntry::C4GRES_InGroup))
		{
			return false;
		}
		return SetFilePtr(entry->Offset);

	case P::ST_Unpacked:
		p->StdFile.Close();
		char path[_MAX_FNAME_LEN];
		SCopy(GetName(),path, _MAX_FNAME);
		AppendBackslash(path);
		SAppend(entry_name, path);
		return p->StdFile.Open(path, needs_to_be_a_group);

	default: break; // InGrp & Deleted ignored
	}
	return false;
}

bool C4Group::FindEntry(const char *wildcard, StdStrBuf *filename, size_t *size)
{
	ResetSearch();
	return FindNextEntry(wildcard, filename, size);
}

bool C4Group::FindNextEntry(const char *wildcard,
                            StdStrBuf *filename,
                            size_t *size,
                            bool start_at_filename)
{
	if (!wildcard)
	{
		return false;
	}

	// Reset search to specified position
	if (start_at_filename)
	{
		FindEntry(filename->getData());
	}

	C4GroupEntry *entry;
	if (!(entry = SearchNextEntry(wildcard)))
	{
		return false;
	}
	if (filename)
	{
		filename->Copy(entry->FileName);
	}
	if (size)
	{
		*size = entry->Size;
	}
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
	              true))
	{
		return false;
	}
	// Pointer is now owned and released by C4Group!
	if (hold_buffer)
	{
		buffer.GrabPointer();
	}
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
	              true))
	{
		return false;
	}
	// Pointer is now owned and released by C4Group!
	if (hold_buffer)
	{
		buffer.GrabPointer();
	}
	return true;
}


const char* C4Group::GetName() const
{
	return p->FileName.c_str();
}

int C4Group::EntryCount(const char *wildcard)
{
	// All files if no wildcard
	if (!wildcard)
	{
		wildcard = "*";
	}
	// Match wildcard
	ResetSearch();

	int filecount = 0;
	C4GroupEntry *entry;
	while ((entry = SearchNextEntry(wildcard)))
	{
		filecount++;
	}
	return filecount;
}

size_t C4Group::EntrySize(const char *wildcard)
{
	// All files if no wildcard
	if (!wildcard)
	{
		wildcard = "*";
	}
	// Match wildcard
	ResetSearch();

	int filesize = 0;
	C4GroupEntry *entry;
	while ((entry = SearchNextEntry(wildcard)))
	{
		filesize += entry->Size;
	}
	return filesize;
}

size_t C4Group::AccessedEntrySize() const { return p->iCurrFileSize; }

unsigned int C4Group::EntryCRC32(const char *wildcard)
{
	if (!wildcard)
	{
		wildcard = "*";
	}
	ResetSearch();

	// iterate thorugh child
	unsigned int CRC = 0;
	C4GroupEntry *entry;
	while ((entry = SearchNextEntry(wildcard)))
	{
		CRC ^= CalcCRC32(entry);
	}
	// return
	return CRC;
}

bool C4Group::IsOpen() const { return p->SourceType != P::ST_None; }

bool C4Group::LoadEntry(const char *entry_name, char **buffer, size_t *size_info, int zeros_to_append)
{
	size_t size;

	// Access entry, allocate buffer, read data
	(*buffer) = nullptr;
	if (size_info)
	{
		*size_info = 0;
	}
	if (!AccessEntry(entry_name, &size))
	{
		return Error("LoadEntry: Not found");
	}
	*buffer = new char[size + zeros_to_append];
	if (!Read(*buffer, size))
	{
		delete [] (*buffer);
		*buffer = nullptr;
		return Error("LoadEntry: Reading error");
	}

	if (size_info)
	{
		*size_info = size;
	}

	if (zeros_to_append)
	{
		ZeroMem( (*buffer)+size, zeros_to_append );
	}

	return true;
}

bool C4Group::LoadEntry(const char *entry_name, StdBuf * buffer)
{
	size_t size;
	// Access entry, allocate buffer, read data
	if (!AccessEntry(entry_name, &size))
	{
		return Error("LoadEntry: Not found");
	}
	// Allocate memory
	buffer->New(size);
	// Load data
	if (!Read(buffer->getMData(), size))
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
	if (!AccessEntry(entry_name, &size))
	{
		return Error("LoadEntry: Not found");
	}
	// Allocate memory
	buffer->SetLength(size);
	// other parts crash when they get a zero length buffer, so fail here
	if (!size)
	{
		return false;
	}
	// Load data
	if (!Read(buffer->getMData(), size))
	{
		buffer->Clear();
		return Error("LoadEntry: Reading error");
	}
	// ok
	return true;
}

int SortRank(const char *element, const char *sort_list)
{
	char segment[_MAX_FNAME_LEN];

	for (int cnt = 0; SCopySegment(sort_list, cnt, segment,'|',_MAX_FNAME); cnt++)
	{
		if (WildcardMatch(segment, element))
		{
			return (SCharCount('|',sort_list) + 1) - cnt;
		}
	}

	return 0;
}

bool C4Group::Sort(const char *list)
{
	bool bubble;
	C4GroupEntry *current;
	C4GroupEntry *prev;
	C4GroupEntry *next;
	C4GroupEntry *nextnext;

	if (!list || !list[0])
	{
		return false;
	}

	if (p->LogToStdOutput)
	{
		printf("Sorting...\n");
	}

	do
	{
		bubble = false;

		for (prev = nullptr, current = p->FirstEntry; current; prev = current, current = next)
		{
			if ((next = current->Next))
			{
				// primary sort by file list
				int rank_a = SortRank(current->FileName, list);
				int rank_b = SortRank(next->FileName, list);
				if (rank_a > rank_b)
				{
					continue;
				}
				// secondary sort by filename
				if (rank_a == rank_b)
				{
					if (stricmp(current->FileName, next->FileName) <= 0)
					{
						continue;
					}
				}
				// wrong order: Swap!
				nextnext = next->Next;
				if (prev)
				{
					prev->Next = next;
				}
				else
				{
					p->FirstEntry = next;
				}
				next->Next = current;
				current->Next = nextnext;
				next = nextnext;

				bubble = true;
				p->Modified = true;
			}
		}

	}
	while (bubble);

	return true;
}

C4Group* C4Group::GetMother()
{
	return p->Mother;
}

bool C4Group::IsPacked() const { return p->SourceType == P::ST_Packed; }

bool C4Group::HasPackedMother() const
{
	if (!p->Mother)
	{
		return false;
	}
	return p->Mother->IsPacked();
}

bool C4Group::SetNoSort(bool no_sorting) { p->NoSort = no_sorting; return true; }

bool C4Group::CloseExclusiveMother()
{
	if (p->Mother && p->ExclusiveChild)
	{
		p->Mother->Close();
		delete p->Mother;
		p->Mother = nullptr;
		return true;
	}
	return false;
}

bool C4Group::SortByList(const char **list, const char *filename)
{
	// No sort list specified
	if (!list)
	{
		return false;
	}
	// No group name specified, use own
	if (!filename)
	{
		filename = GetName();
	}
	filename = GetFilename(filename);
	// Find matching filename entry in sort list
	const char **list_entry;
	for (list_entry = list; *list_entry; list_entry += 2)
	{
		if (WildcardMatch( *list_entry, filename ))
		{
			break;
		}
	}
	// Sort by sort list entry
	if (*list_entry && *(list_entry + 1))
	{
		Sort(*(list_entry + 1));
	}
	// Success
	return true;
}

bool C4Group::EnsureChildFilePtr(C4Group *child)
{

	// group file
	if (p->SourceType == P::ST_Packed)
	{
		// check if FilePtr has to be moved
		if (p->FilePtr != child->p->MotherOffset + child->p->EntryOffset +  child->p->FilePtr)
		{
			// move it to the position the child thinks it is
			if (!SetFilePtr(child->p->MotherOffset + child->p->EntryOffset +  child->p->FilePtr))
			{
				return false;
			}
		}
		// ok
		return true;
	}

	// Open standard file is not the child file     ...or StdFile ptr does not match child->FilePtr
	char child_path[_MAX_PATH_LEN];
	sprintf(child_path, "%s%c%s", GetName(), DirectorySeparator, GetFilename(child->GetName()));
	if (!ItemIdentical(p->StdFile.Name, child_path))
	{
		// Reopen correct child stdfile
		if (!SetFilePtr2Entry(GetFilename(child->GetName()), true))
		{
			return false;
		}
		// Advance to child's old file ptr
		if (!AdvanceFilePtr( child->p->EntryOffset + child->p->FilePtr))
		{
			return false;
		}
	}

	// Looks okay
	return true;

}

StdStrBuf C4Group::GetFullName() const
{
	char name[_MAX_PATH_LEN];
	*name='\0';
	char sep[] = "/";
	sep[0] = DirectorySeparator;
	for (const C4Group *pGroup = this; pGroup; pGroup = pGroup->p->Mother)
	{
		if (*name)
		{
			SInsert(name, sep, 0, _MAX_PATH);
		}
		// Avoid double slash
		if (pGroup == this || pGroup->p->FileName.length() > 1 || pGroup->p->FileName[0] != '/')
		{
			SInsert(name, pGroup->GetName(), 0, _MAX_PATH);
		}
		if (pGroup->p->SourceType == P::ST_Unpacked)
		{
			break; // Folder is assumed to have full path
		}
	}
	StdStrBuf result;
	result.Copy(name);
	return result;
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
			{
				return 0;
			}
			break;
		case C4GroupEntry::C4GRES_OnDisk:
			if (!Child.Open(entry->DiskPath))
			{
				return 0;
			}
			break;
		default:
			return 0;
		}
		// get checksum
		CRC = Child.EntryCRC32();
	}
	else if (!entry->Size)
	{
		CRC = 0;
	}
	else
	{
		BYTE *data = nullptr;
		bool own_data;
		CStdFile file;
		// get data
		switch (entry->Status)
		{
		case C4GroupEntry::C4GRES_InGroup:
			// create buffer
			data = new BYTE [entry->Size];
			own_data = true;
			// go to entry
			if (!SetFilePtr2Entry(entry->FileName))
			{
				delete [] data;
				return false;
			}
			// read
			if (!Read(data, entry->Size))
			{
				delete [] data;
				return false;
			}
			break;
		case C4GroupEntry::C4GRES_OnDisk:
			// create buffer
			data = new BYTE [entry->Size];
			own_data = true;
			// open
			if (!file.Open(entry->DiskPath))
			{
				delete [] data;
				return false;
			}
			// read
			if (!file.Read(data, entry->Size))
			{
				delete [] data;
				return false;
			}
			break;
		case C4GroupEntry::C4GRES_InMemory:
			// set
			data = entry->MemoryBuffer;
			own_data = false;
			break;
		default:
			return false;
		}
		if (!data)
		{
			return false;
		}
		// calc crc
		CRC = crc32(0, data, entry->Size);
		// discard buffer
		if (own_data)
		{
			delete [] data;
		}
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
	C4Group *ourselves = new C4Group;
	*ourselves->p = *p;

	// Open a child from the memory copy
	C4Group child;
	if (!child.OpenAsChild(ourselves, entry_name, false))
	{
		// Silently delete our memory copy
		ourselves->p.reset();
		delete ourselves;
		return false;
	}

	// hack: The seach-handle would be closed twice otherwise
	p->FolderSearch.Reset();
	child.p->FolderSearch.Reset();

	// We now become our own child
	*p = *child.p;

	// Make ourselves exclusive (until we hit our memory copy mother)
	for (C4Group *group = this; group != ourselves; group = group->p->Mother)
	{
		group->p->ExclusiveChild = true;
	}

	// Reset the temporary child variable so it doesn't delete anything
	child.p.reset();

	// Yeehaw
	return true;
}

bool C4Group::OpenMother()
{
	// This only works if we are an exclusive child
	if (!p->Mother || !p->ExclusiveChild)
	{
		return false;
	}

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
	for (C4GroupEntry * entry = p->FirstEntry; entry; entry = entry->Next)
	{
		// is this to be cached?
		if (!WildcardListMatch(search_pattern, entry->FileName))
		{
			continue;
		}
		// if desired, cache all entries up to that one to allow rewind in unpacked memory
		// (only makes sense for groups)
		if (cache_previous && p->SourceType == P::ST_Packed)
		{
			for (C4GroupEntry * e_pre = p->FirstEntry; e_pre != entry; e_pre = e_pre->Next)
			{
				if (e_pre->Offset >= p->FilePtr)
				{
					PreCacheEntry(e_pre);
				}
			}
		}
		// cache the given entry
		PreCacheEntry(entry);
	}
	return result;
}

const C4GroupHeader &C4Group::GetHeader() const { return Head; }

const C4GroupEntry *C4Group::GetFirstEntry() const { return p->FirstEntry; }

void C4Group::PreCacheEntry(C4GroupEntry * entry)
{
	// skip some stuff that can not be cached or has already been cached
	if (entry->ChildGroup || entry->MemoryBuffer || !entry->Size)
	{
		return;
	}
	// now load it!
	StdBuf buffer;
	if (!this->LoadEntry(entry->FileName, &buffer))
	{
		return;
	}
	entry->HoldBuffer = true;
	entry->BufferIsStdbuf = true;
	entry->Size = buffer.getSize(); // update size in case group changed on disk between calls
	entry->MemoryBuffer = static_cast<BYTE *>(buffer.GrabPointer());
}
