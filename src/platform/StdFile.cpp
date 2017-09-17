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

/* Lots of file helpers */

#include "C4Include.h"
#include "platform/StdFile.h"

#ifdef HAVE_IO_H
#include <io.h>
#endif
#ifdef HAVE_DIRECT_H
#include <direct.h>
#endif
#ifdef _WIN32
#include "platform/C4windowswrapper.h"
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <zlib.h>

/* Path & Filename */
#ifdef _WIN32
static const char *DirectorySeparators = R"(/\)";
#else
static const char *DirectorySeparators = "/";
#endif

// Return pointer to position after last backslash.

char *GetFilename(char *szPath)
{
	if (!szPath) return nullptr;
	char *pPos,*pFilename=szPath;
	for (pPos=szPath; *pPos; pPos++) if (*pPos==DirectorySeparator || *pPos=='/') pFilename = pPos+1;
	return pFilename;
}
const char *GetFilename(const char *szPath)
{
	if (!szPath) return nullptr;
	const char *pPos,*pFilename=szPath;
	for (pPos=szPath; *pPos; pPos++) if (*pPos==DirectorySeparator || *pPos=='/') pFilename = pPos+1;
	return pFilename;
}

const char* GetFilenameOnly(const char *strFilename)
{
	// Get filename to static buffer
	static char strBuffer[_MAX_PATH + 1];
	SCopy(GetFilename(strFilename), strBuffer);
	// Truncate extension
	RemoveExtension(strBuffer);
	// Return buffer
	return strBuffer;
}

const char *GetC4Filename(const char *szPath)
{
	// returns path to file starting at first .c4*-directory.
	if (!szPath) return nullptr;
	const char *pPos,*pFilename=szPath;
	for (pPos=szPath; *pPos; pPos++)
	{
		if (*pPos==DirectorySeparator || *pPos=='/')
		{
			if (pPos >= szPath+4 && SEqual2NoCase(pPos-4, ".oc")) return pFilename;
			pFilename = pPos+1;
		}
	}
	return pFilename;
}

int GetTrailingNumber(const char *strString)
{
	// Default
	int iNumber = 0;
	// Start from end
	const char *cpPos = strString + SLen(strString);
	// Walk back while number
	while ((cpPos > strString) && Inside(*(cpPos - 1), '0', '9')) cpPos--;
	// Scan number
	sscanf(cpPos, "%d", &iNumber);
	// Return result
	return iNumber;
}

// Like GetFilename, but searches for a slash instead of a backslash
// (unix-style paths)

char *GetFilenameWeb(char *szPath)
{
	if (!szPath) return nullptr;
	char *pPos, *pFilename=szPath;
	for (pPos=szPath; *pPos; pPos++) if (*pPos == '/') pFilename = pPos+1;
	return pFilename;
}
const char *GetFilenameWeb(const char *szPath)
{
	if (!szPath) return nullptr;
	const char *pPos, *pFilename=szPath;
	for (pPos=szPath; *pPos; pPos++) if (*pPos == '/') pFilename = pPos+1;
	return pFilename;
}

// Return pointer to last file extension.

char *GetExtension(char *szFilename)
{
	int pos, end;
	for (end=0; szFilename[end]; end++) {}
	pos = end;
	while ((pos > 0) && (szFilename[pos-1] != '.') && (szFilename[pos-1] != DirectorySeparator)) --pos;
	if ((pos > 0) && szFilename[pos-1] == '.') return szFilename + pos;
	return szFilename + end;
}
const char *GetExtension(const char *szFilename)
{
	int pos, end;
	for (end=0; szFilename[end]; end++) {}
	pos = end;
	while ((pos>0) && (szFilename[pos-1] != '.') && (szFilename[pos-1] != DirectorySeparator)) pos--;
	if ((pos > 0) && szFilename[pos-1] == '.') return szFilename+pos;
	return szFilename+end;
}


void RealPath(const char *szFilename, char *pFullFilename)
{
#ifdef _WIN32
	wchar_t *wpath = _wfullpath(nullptr, GetWideChar(szFilename), 0);
	StdStrBuf path(wpath);
	// I'm pretty sure pFullFilename will always have at least a size of _MAX_PATH, but ughh
	// This should return a StdStrBuf
	SCopy(path.getData(), pFullFilename, _MAX_PATH);
	free(wpath);
#else
	char *pSuffix = nullptr;
	char szCopy[_MAX_PATH + 1];
	for (;;)
	{
		// Try to convert to full filename. Note this might fail if the given file doesn't exist
		if (realpath(szFilename, pFullFilename))
			break;
		// ... which is undesired behaviour here. Try to reduce the filename until it works.
		if (!pSuffix)
		{
			SCopy(szFilename, szCopy, _MAX_PATH);
			szFilename = szCopy;
			pSuffix = szCopy + SLen(szCopy);
		}
		else
			*pSuffix = '/';
		while (pSuffix > szCopy)
			if (*--pSuffix == '/')
				break;
		if (pSuffix <= szCopy)
		{
			// Give up: Just copy whatever we got
			SCopy(szFilename, pFullFilename, _MAX_PATH);
			return;
		}
		*pSuffix = 0;
	}
	// Append suffix
	if (pSuffix)
	{
		*pSuffix = '/';
		SAppend(pSuffix, pFullFilename, _MAX_PATH);
	}
#endif
}

// Copy (extended) parent path (without backslash) to target buffer.

bool GetParentPath(const char *szFilename, char *szBuffer)
{
	// Prepare filename
	SCopy(szFilename,szBuffer,_MAX_PATH);
	// Extend relative single filenames
#ifdef _WIN32
	if (!SCharCount(DirectorySeparator,szFilename)) _fullpath(szBuffer,szFilename,_MAX_PATH);
#else
	if (!SCharCount(DirectorySeparator,szFilename)) RealPath(szFilename,szBuffer);
#endif
	// Truncate path
	return TruncatePath(szBuffer);
}

bool GetParentPath(const char *szFilename, StdStrBuf *outBuf)
{
	char buf[_MAX_PATH+1]; *buf='\0';
	if (!GetParentPath(szFilename, buf)) return false;
	outBuf->Copy(buf);
	return true;
}

const char *GetRelativePathS(const char *strPath, const char *strRelativeTo)
{
	// Specified path is relative to base path
#ifdef _WIN32
	if (SEqual2NoCase(strPath, strRelativeTo))
#else
	if (SEqual2(strPath, strRelativeTo))
#endif
	{
		// return relative section
		return strPath + SLen(strRelativeTo) + ((strPath[SLen(strRelativeTo)] == DirectorySeparator) ? +1 : 0);
	}
	// Not relative: return full path
	return strPath;
}

bool IsGlobalPath(const char *szPath)
{
#ifdef _WIN32
	// C:\...
	if (*szPath && szPath[1] == ':') return true;
#endif
	// /usr/bin, \Temp\, ...
	if (*szPath == DirectorySeparator) return true;
	return false;
}

// Truncate string before last backslash.

bool TruncatePath(char *szPath)
{
	if (!szPath) return false;
	int iBSPos;
	iBSPos=SCharLastPos(DirectorySeparator,szPath);
#ifndef _WIN32
	int iBSPos2;
	iBSPos2=SCharLastPos('\\',szPath);
	if (iBSPos2 > iBSPos) fprintf(stderr, "Warning: TruncatePath with a \\ (%s)\n", szPath);
#endif
	if (iBSPos<0) return false;
	szPath[iBSPos]=0;
	return true;
}

// Append terminating backslash if not present.

void AppendBackslash(char *szFilename)
{
	int i=SLen(szFilename);
	if (i>0) if (szFilename[i-1]==DirectorySeparator) return;
	SAppendChar(DirectorySeparator,szFilename);
}

// Remove terminating backslash if present.

void TruncateBackslash(char *szFilename)
{
	int i=SLen(szFilename);
	if (i>0) if (szFilename[i-1]==DirectorySeparator) szFilename[i-1]=0;
}

// Append extension if no extension.

void DefaultExtension(char *szFilename, const char *szExtension)
{
	if (!(*GetExtension(szFilename)))
		{ SAppend(".",szFilename); SAppend(szExtension,szFilename); }
}

void DefaultExtension(StdStrBuf *sFilename, const char *szExtension)
{
	assert(sFilename);
	if (!(*GetExtension(sFilename->getData())))
		{ sFilename->AppendChar('.'); sFilename->Append(szExtension); }
}

// Append or overwrite extension.

void EnforceExtension(char *szFilename, const char *szExtension)
{
	char *ext = GetExtension(szFilename);
	if (ext[0]) { SCopy(szExtension,ext); }
	else { SAppend(".",szFilename); SAppend(szExtension,szFilename); }
}

void EnforceExtension(StdStrBuf *sFilename, const char *szExtension)
{
	assert(sFilename);
	const char *ext = GetExtension(sFilename->getData());
	if (ext[0]) { sFilename->ReplaceEnd(ext - sFilename->getData(), szExtension); }
	else { sFilename->AppendChar('.'); sFilename->Append(szExtension); }
}

// remove extension

void RemoveExtension(char *szFilename)
{
	char *ext = GetExtension(szFilename);
	if (ext[0]) ext[-1]=0;
}

void RemoveExtension(StdStrBuf *psFileName)
{
	if (psFileName && *psFileName)
	{
		RemoveExtension(psFileName->getMData());
		psFileName->SetLength(strlen(psFileName->getData()));
	}
}

// Enforce indexed extension until item does not exist.

void MakeTempFilename(char *szFilename)
{
	DefaultExtension(szFilename,"tmp");
	char *fn_ext=GetExtension(szFilename);
	int cnum=-1;
	do
	{
		cnum++;
		osprintf(fn_ext,"%03d",cnum);
	}
	while (FileExists(szFilename) && (cnum<999));
}

void MakeTempFilename(StdStrBuf *sFilename)
{
	assert(sFilename);
	if (!sFilename->getLength()) sFilename->Copy("temp.tmp");
	EnforceExtension(sFilename, "tmp");
	char *fn_ext=GetExtension(sFilename->getMData());
	int cnum=-1;
	do
	{
		cnum++;
		osprintf(fn_ext,"%03d",cnum);
	}
	while (FileExists(sFilename->getData()) && (cnum<999));
}

bool WildcardListMatch(const char *szWildcardList, const char *szString)
{
	// safety
	if (!szString || !szWildcardList) return false;
	// match any item in list
	StdStrBuf sWildcard, sWildcardList(szWildcardList);
	int32_t i=0;
	while (sWildcardList.GetSection(i++, &sWildcard, '|'))
	{
		if (WildcardMatch(sWildcard.getData(), szString)) return true;
	}
	// none matched
	return false;
}

bool IsWildcardString(const char *szString)
{
	// safety
	if (!szString) return false;
	// known wildcard characters: *?
	return (SCharCount('?', szString)>0) || (SCharCount('*', szString)>0);
}

bool WildcardMatch(const char *szWildcard, const char *szString)
{
	// safety
	if (!szString || !szWildcard) return false;
	// match char-wise
	const char *pWild = szWildcard, *pPos = szString;
	const char *pLWild = nullptr, *pLPos = nullptr; // backtracking
	while (*pWild || pLWild)
		// string wildcard?
		if (*pWild == '*')
			{ pLWild = ++pWild; pLPos = pPos; }
	// nothing left to match?
		else if (!*pPos)
			break;
	// equal or one-character-wildcard? proceed
		else if (*pWild == '?' || tolower(*pWild) == tolower(*pPos))
			{ pWild++; pPos++; }
	// backtrack possible?
		else if (pLPos)
			{ pWild = pLWild; pPos = ++pLPos; }
	// match failed
		else
			return false;
	// match complete if both strings are fully matched
	return !*pWild && !*pPos;
}

// create a valid file name from some title
void MakeFilenameFromTitle(char *szTitle)
{
	// copy all chars but those to be stripped
	char *szFilename=szTitle, *szTitle2=szTitle;
	while (*szTitle2)
	{
		bool fStrip;
		if (IsWhiteSpace(*szTitle2))
			fStrip = (szFilename==szTitle);
		else if (static_cast<unsigned int>(*szTitle2) > 127)
			fStrip = true;
		else
			fStrip = (SCharPos(*szTitle2, R"(!"'%&/=?+*#:;<>\.)") >= 0);
		if (!fStrip) *szFilename++ = *szTitle2;
		++szTitle2;
	}
	// truncate spaces from end
	while (IsWhiteSpace(*--szFilename)) if (szFilename==szTitle) { --szFilename; break; }
	// terminate
	*++szFilename=0;
	// no name? (only invalid chars)
	if (!*szTitle) SCopy("unnamed", szTitle, 50);
	// done
}

/* Files */

bool FileExists(const char *szFilename)
{
#ifdef _WIN32
	return GetFileAttributes(GetWideChar(szFilename)) != INVALID_FILE_ATTRIBUTES;
#else
	return (!access(szFilename,F_OK));
#endif
}

size_t FileSize(const char *szFilename)
{
#if defined(_WIN32) || defined(_WIN64)
	auto attributes = WIN32_FILE_ATTRIBUTE_DATA();
	if (GetFileAttributesEx(GetWideChar(szFilename), GetFileExInfoStandard, &attributes) == 0)
		return 0;
#ifdef _WIN64
	return (static_cast<size_t>(attributes.nFileSizeHigh) << (sizeof(attributes.nFileSizeLow) * 8)) | attributes.nFileSizeLow;
#else
	return attributes.nFileSizeLow;
#endif
#else
	struct stat stStats;
	if (stat(szFilename,&stStats)) return 0;
	return stStats.st_size;
#endif
}

// operates on a filedescriptor from open or fileno
size_t FileSize(int fdes)
{
#ifdef _WIN32
	return _filelength(fdes);
#else
	struct stat stStats;
	if (fstat(fdes,&stStats)) return 0;
	return stStats.st_size;
#endif
}

int FileTime(const char *szFilename)
{
#ifdef _WIN32
	auto attributes = WIN32_FILE_ATTRIBUTE_DATA();
	if (GetFileAttributesEx(GetWideChar(szFilename), GetFileExInfoStandard, &attributes) == 0)
		return 0;
	int64_t ft = (static_cast<int64_t>(attributes.ftLastWriteTime.dwHighDateTime) << (sizeof(attributes.ftLastWriteTime.dwLowDateTime) * 8)) | attributes.ftLastWriteTime.dwLowDateTime;
	ft -= 116444736000000000;
	ft /= 10000000;
	return ft;
#else
	struct stat stStats;
	if (stat(szFilename,&stStats)!=0) return 0;
	return stStats.st_mtime;
#endif
}

bool EraseFile(const char *szFilename)
{
#ifdef _WIN32
	SetFileAttributesW(GetWideChar(szFilename), FILE_ATTRIBUTE_NORMAL);
	if (DeleteFileW(GetWideChar(szFilename)) == 0)
	{
		switch (GetLastError())
		{
		case ERROR_PATH_NOT_FOUND:
		case ERROR_FILE_NOT_FOUND:
			// While deleting it didn't work, the file doesn't exist (anymore).
			// Pretend everything is fine.
			return true;
		default:
			// Some other error left us unable to delete the file.
			return false;
		}
	}
	return true;
#else
	// either unlink or remove could be used. Well, stick to ANSI C where possible.
	if (remove(szFilename))
	{
		if (errno == ENOENT)
		{
			// Hah, here the wrapper actually makes sense:
			// The engine only cares about the file not being there after this call.
			return true;
		}
		return false;
	}
	return true;
#endif
}

#ifndef _WIN32
bool CopyFile(const char *szSource, const char *szTarget, bool FailIfExists)
{
	int fds = open (szSource, O_RDONLY | O_CLOEXEC);
	if (!fds) return false;
	struct stat info; fstat(fds, &info);
	int fdt = open (szTarget, O_CLOEXEC | O_WRONLY | O_CREAT | (FailIfExists? O_EXCL : O_TRUNC), info.st_mode);
	if (!fdt)
	{
		close (fds);
		return false;
	}
	char buffer[1024]; ssize_t l;
	while ((l = read(fds, buffer, sizeof(buffer))) > 0)
		if (write(fdt, buffer, l) < l)
		{
			l = -1;
			break;
		}
	close (fds);
	close (fdt);
	// On error, return false
	return l != -1;
}

bool RenameFile(const char *szFilename, const char *szNewFilename)
{
	if (rename(szFilename,szNewFilename) < 0)
	{
		if (errno != EXDEV) return false;
		if (CopyFile(szFilename, szNewFilename, false))
		{
			return EraseFile(szFilename);
		}
		return false;
	}
	return true;
}
#else

#undef CopyFile
bool CopyFile(const char *szSource, const char *szTarget, bool FailIfExists)
{
	return !!CopyFileW(GetWideChar(szSource), GetWideChar(szTarget), FailIfExists);
}

bool RenameFile(const char *szFilename, const char *szNewFilename)
{
	return !!MoveFileExW(GetWideChar(szFilename), GetWideChar(szNewFilename), MOVEFILE_COPY_ALLOWED);
}

#endif

bool MakeOriginalFilename(char *szFilename)
{
	// safety
	if (!szFilename) return false;
#ifdef _WIN32
	// root-directory?
	if (Inside(SLen(szFilename), 2u, 3u)) if (szFilename[1]==':')
		{
			szFilename[2]='\\'; szFilename[3]=0;
			if (GetDriveTypeW(GetWideChar(szFilename)) == DRIVE_NO_ROOT_DIR) return false;
			return true;
		}
	struct _wfinddata_t fdt; intptr_t shnd;
	if ((shnd=_wfindfirst(GetWideChar(szFilename),&fdt))<0) return false;
	_findclose(shnd);
	StdStrBuf name(fdt.name);
	SCopy(GetFilename(name.getData()),GetFilename(szFilename),_MAX_FNAME);
#else
	if (SCharPos('*', szFilename) != -1)
	{
		fputs ("Warning: MakeOriginalFilename with \"", stderr);
		fputs (szFilename, stderr);
		fputs ("\"!\n", stderr);
	}
#endif
	return true;
}

/* Directories */

const char *GetWorkingDirectory()
{
#ifdef _WIN32
	static StdStrBuf buffer;
	wchar_t *widebuf = nullptr;
	DWORD widebufsz = GetCurrentDirectoryW(0, nullptr);
	widebuf = new wchar_t[widebufsz];
	if (GetCurrentDirectoryW(widebufsz, widebuf) == 0) {
		delete[] widebuf;
		return nullptr;
	}
	buffer.Take(StdStrBuf(widebuf));
	delete[] widebuf;
	return buffer.getData();
#else
	static char buf[_MAX_PATH+1];
	return getcwd(buf,_MAX_PATH);
#endif
}

bool SetWorkingDirectory(const char *path)
{
#ifdef _WIN32
	return SetCurrentDirectoryW(GetWideChar(path)) != 0;
#else
	return (chdir(path)==0);
#endif
}

bool CreatePath(const std::string &path)
{
	assert(!path.empty());
#ifdef _WIN32
	if (CreateDirectoryW(GetWideChar(path.c_str()), nullptr))
	{
		return true;
	}
	else
	{
		DWORD err = GetLastError();
		switch (err)
		{
		case ERROR_PATH_NOT_FOUND:
			break;
		case ERROR_ALREADY_EXISTS:
			return true;
		default:
			// Something major has happened: Log
		{
			wchar_t * str;
			if (FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
			                  nullptr, err, 0, (LPWSTR)&str, 0, nullptr))
			{
				LogF("CreateDirectory failed: %s", StdStrBuf(str).getData());
				LocalFree(str);
			}
			return false;
		}
		}
	}
#else
	if (!mkdir(path.c_str(), S_IREAD | S_IWRITE | S_IEXEC))
		return true;
	switch (errno)
	{
	case ENOENT:
		break;
	case EEXIST:
		// FIXME: Check whether the path is blocked by a non-directory
		return true;
	default:
		return false;
	}
#endif
	// Recursively create parent path
	std::string::size_type slash = path.find_last_of(DirectorySeparators);
	if (slash == 0 || slash == std::string::npos)
		return false;
	return CreatePath(path.substr(0, slash)) && CreatePath(path);
}

bool DirectoryExists(const char *szFilename)
{
	// Ignore trailing slash or backslash, except when we are probing the
	// root directory '/'.
	char bufFilename[_MAX_PATH + 1];
	if (szFilename && szFilename[0])
	{
		unsigned int len = SLen(szFilename);
		if (len > 1 && ((szFilename[len - 1] == '\\') || (szFilename[len - 1] == '/')))
		{
			SCopy(szFilename, bufFilename, _MAX_PATH);
			bufFilename[SLen(bufFilename) - 1] = 0;
			szFilename = bufFilename;
		}
	}
	// Check file attributes
#ifdef _WIN32
	struct _wfinddata_t fdt; intptr_t shnd;
	if ((shnd=_wfindfirst(GetWideChar(szFilename),&fdt))<0) return false;
	_findclose(shnd);
	if (fdt.attrib & _A_SUBDIR) return true;
#else
	struct stat stStats;
	if (stat(szFilename,&stStats)!=0) return 0;
	return (S_ISDIR(stStats.st_mode));
#endif
	return false;
}

bool CopyDirectory(const char *szSource, const char *szTarget, bool fResetAttributes)
{
	// Source check
	if (!szSource || !szTarget) return false;
	if (!DirectoryExists(szSource)) return false;
	// Do not process system navigation directories
	if (SEqual(GetFilename(szSource),".")
	    || SEqual(GetFilename(szSource),".."))
		return true;
	// Overwrite target
	if (!EraseItem(szTarget)) return false;
	// Create target directory
	bool status=true;
#ifdef _WIN32
	if (_wmkdir(GetWideChar(szTarget))!=0) return false;
	// Copy contents to target directory
	char contents[_MAX_PATH+1];
	SCopy(szSource,contents); AppendBackslash(contents);
	SAppend("*",contents);
	_wfinddata_t fdt; intptr_t hfdt;
	if ( (hfdt=_wfindfirst(GetWideChar(contents),&fdt)) > -1 )
	{
		do
		{
			char itemsource[_MAX_PATH+1],itemtarget[_MAX_PATH+1];
			SCopy(szSource,itemsource); AppendBackslash(itemsource); SAppend(StdStrBuf(fdt.name).getData(),itemsource);
			SCopy(szTarget,itemtarget); AppendBackslash(itemtarget); SAppend(StdStrBuf(fdt.name).getData(),itemtarget);
			if (!CopyItem(itemsource,itemtarget, fResetAttributes)) status=false;
		}
		while (_wfindnext(hfdt,&fdt)==0);
		_findclose(hfdt);
	}
#else
	if (mkdir(szTarget,0777)!=0) return false;
	DIR * d = opendir(szSource);
	dirent * ent;
	char itemsource[_MAX_PATH+1],itemtarget[_MAX_PATH+1];
	while ((ent = readdir(d)))
	{
		SCopy(szSource,itemsource); AppendBackslash(itemsource); SAppend(ent->d_name,itemsource);
		SCopy(szTarget,itemtarget); AppendBackslash(itemtarget); SAppend(ent->d_name,itemtarget);
		if (!CopyItem(itemsource,itemtarget, fResetAttributes)) status=false;
	}
	closedir(d);
#endif
	return status;
}

bool EraseDirectory(const char *szDirName)
{
	// Do not process system navigation directories
	if (SEqual(GetFilename(szDirName),".")
	    || SEqual(GetFilename(szDirName),".."))
		return true;
	char path[_MAX_PATH+1];
#ifdef _WIN32
	// Get path to directory contents
	SCopy(szDirName,path); SAppend(R"(\*.*)",path);
	// Erase subdirectories and files
	ForEachFile(path,&EraseItem);
#else
	DIR * d = opendir(szDirName);
	dirent * ent;
	while ((ent = readdir(d)))
	{
		SCopy(szDirName,path); AppendBackslash(path); SAppend(ent->d_name,path);
		if (!EraseItem(path)) return false;
	}
	closedir(d);
#endif
	// Check working directory
	if (SEqual(szDirName,GetWorkingDirectory()))
	{
		// Will work only if szDirName is full path and correct case!
		SCopy(GetWorkingDirectory(),path);
		int lbacks = SCharLastPos(DirectorySeparator,path);
		if (lbacks > -1)
		{
			path[lbacks]=0; SetWorkingDirectory(path);
		}
	}
	// Remove directory
#ifdef _WIN32
	return !!RemoveDirectoryW(GetWideChar(szDirName));
#else
	return (rmdir(szDirName)==0 || errno == ENOENT);
#endif
}

/* Items */
bool RenameItem(const char *szItemName, const char *szNewItemName)
{
	// FIXME: What if the directory would have to be copied?
	return RenameFile(szItemName,szNewItemName);
}

bool EraseItem(const char *szItemName)
{
	if (!EraseFile(szItemName)) return EraseDirectory(szItemName);
	else return true;
}

bool CreateItem(const char *szItemname)
{
	// Overwrite any old item
	EraseItem(szItemname);
	// Create dummy item
	FILE *fhnd;
#ifdef _WIN32
	if (!(fhnd=_wfopen(GetWideChar(szItemname), L"wb"))) return false;
#else
	if (!(fhnd=fopen(szItemname,"wb"))) return false;
#endif
	fclose(fhnd);
	// Success
	return true;
}

bool CopyItem(const char *szSource, const char *szTarget, bool fResetAttributes)
{
	// Check for identical source and target
	if (ItemIdentical(szSource,szTarget)) return true;
	// Copy directory
	if (DirectoryExists(szSource))
		return CopyDirectory(szSource,szTarget,fResetAttributes);
	// Copy file
	if (!CopyFile(szSource,szTarget,false)) return false;
	// Reset any attributes if desired
#ifdef _WIN32
	if (fResetAttributes) if (!SetFileAttributesW(GetWideChar(szTarget), FILE_ATTRIBUTE_NORMAL)) return false;
#else
	if (fResetAttributes) if (chmod(szTarget, S_IRWXU)) return false;
#endif
	return true;
}

bool MoveItem(const char *szSource, const char *szTarget)
{
	if (ItemIdentical(szSource,szTarget)) return true;
	return RenameFile(szSource, szTarget);
}

bool ItemIdentical(const char *szFilename1, const char *szFilename2)
{
	char szFullFile1[_MAX_PATH+1],szFullFile2[_MAX_PATH+1];
	RealPath(szFilename1, szFullFile1); RealPath(szFilename2, szFullFile2);
#ifdef _WIN32
	if (SEqualNoCase(szFullFile1,szFullFile2)) return true;
#else
	if (SEqual(szFullFile1,szFullFile2)) return true;
#endif
	return false;
}

//------------------------- Multi File Processing --------------------------------------------------------------------------------------------------------

struct DirectoryIteratorP
{
	DirectoryIteratorP() = default;
	DirectoryIterator::FileList files;
	std::string directory;
	int ref{1};
};

DirectoryIterator::DirectoryIterator()
		: p(new DirectoryIteratorP), iter(p->files.end())
{}
DirectoryIterator::DirectoryIterator(const DirectoryIterator &other)
		: p(other.p), iter(other.iter)
{
	++p->ref;
}

DirectoryIterator & DirectoryIterator::operator = (const DirectoryIterator & other)
{
	p = other.p; iter = other.iter;
	++p->ref;
	return *this;
}

DirectoryIterator::~DirectoryIterator()
{
	if (--p->ref == 0)
		delete p;
}

void DirectoryIterator::Clear()
{
	// clear cache
	if (p->ref > 1)
	{
		// Detach from shared memory
		--p->ref;
		p = new DirectoryIteratorP;
	}
	p->directory.clear();
	p->files.clear();
	iter = p->files.end();
}

void DirectoryIterator::Reset ()
{
	iter = p->files.begin();
}

void DirectoryIterator::Reset (const char * dirname, bool force_reread)
{
	if (p->directory == dirname && !force_reread)
	{
		// Skip reinitialisation and just reset the iterator
		iter = p->files.begin();
		return;
	}
	if (p->ref > 1)
	{
		// Detach from shared memory
		--p->ref;
		p = new DirectoryIteratorP;
	}
	p->files.clear();
	iter = p->files.end();
	Read(dirname);
}

DirectoryIterator::DirectoryIterator (const char * dirname)
		: p(new DirectoryIteratorP), iter(p->files.end())
{
	Read(dirname);
}

void DirectoryIterator::Read(const char *dirname)
{
	assert(dirname && *dirname);
	assert(p->files.empty());
	std::string search_path(dirname);
	if (!search_path.empty() && search_path.back() != DirectorySeparator)
		search_path.push_back(DirectorySeparator);
#ifdef WIN32
	auto file = WIN32_FIND_DATAW();
	HANDLE fh = FindFirstFileW(GetWideChar((search_path + '*').c_str()), &file);
	if (fh == INVALID_HANDLE_VALUE)
	{
		switch (GetLastError())
		{
		case ERROR_PATH_NOT_FOUND:
		case ERROR_FILE_NOT_FOUND:
			// This is okay, either the directory doesn't exist or there are no files
			return;
		default:
			// Something else broke
			Log("DirectoryIterator::Read(const char*): Unable to read file system");
			return;
		}
	}
	// Insert files into list
	do
	{
		// ...unless they're . or ..
		if (file.cFileName[0] == '.' && (file.cFileName[1] == '\0' || (file.cFileName[1] == '.' && file.cFileName[2] == '\0')))
			continue;

		size_t size = (file.nFileSizeHigh * (size_t(MAXDWORD) + 1)) + file.nFileSizeLow;

		p->files.emplace_back(StdStrBuf(file.cFileName).getData(), size);
	}
	while (FindNextFileW(fh, &file));
	FindClose(fh);
#else
	DIR *fh = opendir(dirname);
	if (fh == nullptr)
	{
		switch (errno)
		{
		case ENOENT:
		case ENOTDIR:
			// Okay, so there's no files here.
			return;
		default:
			// Something else broke
			LogF("DirectoryIterator::Read(\"%s\"): %s", dirname, strerror(errno));
			return;
		}
	}
	dirent *file;
	// Insert files into list
	while ((file = readdir(fh)) != nullptr)
	{
		// ...unless they're . or ..
		if (file->d_name[0] == '.' && (file->d_name[1] == '\0' || (file->d_name[1] == '.' && file->d_name[2] == '\0')))
			continue;
		p->files.emplace_back(file->d_name, 0);
	}
	closedir(fh);
#endif
	// Sort list
	std::sort(p->files.begin(), p->files.end());
	for (auto & file : p->files)
		file.first.insert(0, search_path); // prepend path to all file entries
	iter = p->files.begin();
	p->directory = dirname;
}

DirectoryIterator& DirectoryIterator::operator++()
{
	if (iter != p->files.end())
		++iter;
	return *this;
}

const char * DirectoryIterator::operator*() const
{
	if (iter == p->files.end())
		return nullptr;
	return iter->first.c_str();
}
DirectoryIterator DirectoryIterator::operator++(int)
{
	DirectoryIterator tmp(*this);
	++*this;
	return tmp;
}

size_t DirectoryIterator::GetFileSize() const
{
#ifdef _WIN32
	return iter->second;
#else
	return FileSize(iter->first.c_str());
#endif
}

int ForEachFile(const char *szDirName, bool (*fnCallback)(const char *))
{
	if (!szDirName || !fnCallback)
		return 0;
	char szFilename[_MAX_PATH+1];
	SCopy(szDirName,szFilename);
	bool fHasWildcard = (SCharPos('*', szFilename)>=0);
	if (!fHasWildcard) // parameter without wildcard: Append "/*.*" or "\*.*"
		AppendBackslash(szFilename);
	int iFileCount = 0;
#ifdef _WIN32
	struct _wfinddata_t fdt; intptr_t fdthnd;
	if (!fHasWildcard) // parameter without wildcard: Append "/*.*" or "\*.*"
		SAppend("*",szFilename,_MAX_PATH);
	if ((fdthnd = _wfindfirst (GetWideChar(szFilename), &fdt)) < 0)
		return 0;
	do
	{
		if (!wcscmp(fdt.name, L".") || !wcscmp(fdt.name, L"..")) continue;
		StdStrBuf name(fdt.name);
		SCopy(name.getData(),GetFilename(szFilename));
		if ((*fnCallback)(szFilename))
			iFileCount++;
	}
	while (_wfindnext(fdthnd,&fdt)==0);
	_findclose(fdthnd);
#else
	if (fHasWildcard) fprintf(stderr, "Warning: ForEachFile with * (%s)\n", szDirName);
	DIR * d = opendir(szDirName);
	if (!d) return 0;
	dirent * ent;
	while ((ent = readdir(d)))
	{
		SCopy(ent->d_name,GetFilename(szFilename));
		if ((*fnCallback)(szFilename))
			iFileCount++;
	}
	closedir(d);
#endif
	return iFileCount;
}
