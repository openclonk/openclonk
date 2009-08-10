/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2002  Peter Wortmann
 * Copyright (c) 2004-2005  Günther Brammer
 * Copyright (c) 2007  Sven Eberhardt
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

/* Lots of file helpers */

#ifndef STDFILE_INCLUDED
#define STDFILE_INCLUDED

#include <Standard.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#ifdef _WIN32
#include <io.h>
#define F_OK 0
#else
#include <dirent.h>
#include <limits.h>
#define _O_BINARY 0
#define _MAX_PATH PATH_MAX
#define _MAX_FNAME NAME_MAX

bool CopyFile(const char *szSource, const char *szTarget, bool FailIfExists);
#endif

#ifdef _WIN32
#define DirectorySeparator '\\'
#define AltDirectorySeparator '/'
#else
#define DirectorySeparator '/'
#define AltDirectorySeparator '\\'
#define DIRECTORYSEPARATORS "/"
#endif
#define Wildcard '*'

/** Create a directory and all of its parents.
 * \p[in] path Directory to create
 * \returns true on success, false otherwise.
 */
bool CreatePath(const std::string &path);

const char *GetWorkingDirectory();
bool SetWorkingDirectory(const char *szPath);
char *GetFilename(char *path);
char *GetFilenameWeb(char *path);
const char* GetFilenameOnly(const char *strFilename);
const char *GetC4Filename(const char *szPath); // returns path to file starting at first .c4*-directory
int GetTrailingNumber(const char *strString);
char *GetExtension(char *fname);
const char *GetFilename(const char *path);
const char *GetFilenameWeb(const char *path);
const char *GetExtension(const char *fname);
void DefaultExtension(char *szFileName, const char *szExtension);
void DefaultExtension(class StdStrBuf *sFilename, const char *szExtension);
void EnforceExtension(char *szFileName, const char *szExtension);
void EnforceExtension(class StdStrBuf *sFilename, const char *szExtension);
void RemoveExtension(char *szFileName);
void RemoveExtension(StdStrBuf *psFileName);
void AppendBackslash(char *szFileName);
void TruncateBackslash(char *szFilename);
void MakeTempFilename(char *szFileName);
void MakeTempFilename(class StdStrBuf *sFileName);
bool WildcardListMatch(const char *szWildcardList, const char *szString); // match string in list like *.png|*.bmp
bool IsWildcardString(const char *szString); // does szString contain wildcard characters?
bool WildcardMatch(const char *szFName1, const char *szFName2);
bool TruncatePath(char *szPath);
// szBuffer has to be of at least _MAX_PATH length.
bool GetParentPath(const char *szFilename, char *szBuffer);
bool GetParentPath(const char *szFilename, StdStrBuf *outBuf);
bool GetRelativePath(const char *strPath, const char *strRelativeTo, char *strBuffer, int iBufferSize=_MAX_PATH);
const char *GetRelativePathS(const char *strPath, const char *strRelativeTo);
bool IsGlobalPath(const char *szPath);

bool DirectoryExists(const char *szFileName);
//bool FileExists(const char *szFileName, int *lpAttr=NULL);
bool FileExists(const char *szFileName);
size_t FileSize(const char *fname);
size_t FileSize(int fdes);
int FileTime(const char *fname);
bool EraseFile(const char *szFileName);
bool EraseFiles(const char *szFilePath);
bool RenameFile(const char *szFileName, const char *szNewFileName);
bool MakeOriginalFilename(char *szFilename);
void MakeFilenameFromTitle(char *szTitle);

bool CopyDirectory(const char *szSource, const char *szTarget, bool fResetAttributes=false);
bool EraseDirectory(const char *szDirName);

int ItemAttributes(const char *szItemName);
bool ItemIdentical(const char *szFilename1, const char *szFilename2);
inline bool ItemExists(const char *szItemName) { return FileExists(szItemName); }
bool RenameItem(const char *szItemName, const char *szNewItemName);
bool EraseItem(const char *szItemName);
bool EraseItems(const char *szItemPath);
bool CopyItem(const char *szSource, const char *szTarget, bool fResetAttributes=false);
bool CreateItem(const char *szItemname);
bool MoveItem(const char *szSource, const char *szTarget);

//int ForEachFile(const char *szPath, int lAttrib, bool (*fnCallback)(const char *));
int ForEachFile(const char *szDirName, bool (*fnCallback)(const char *));

class DirectoryIterator {
public:
	DirectoryIterator(const char * dirname);
	DirectoryIterator();
	~DirectoryIterator();
	// Does not actually copy anything, but does prevent misuses from crashing (I hope)
	DirectoryIterator(const DirectoryIterator &);
	const char * operator * () const;
	DirectoryIterator& operator ++ ();
	void operator ++ (int);
	void Reset(const char * dirname);
	void Reset();
protected:
	char filename[_MAX_PATH+1];
#ifdef _WIN32
	struct _finddata_t fdt; int fdthnd;
	friend class C4GroupEntry;
#else
	DIR * d;
	dirent * ent;
#endif
};

#endif // STDFILE_INCLUDED
