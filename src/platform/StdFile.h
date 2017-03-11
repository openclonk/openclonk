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

#ifndef STDFILE_INCLUDED
#define STDFILE_INCLUDED

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
void DefaultExtension(StdStrBuf *sFilename, const char *szExtension);
void EnforceExtension(char *szFileName, const char *szExtension);
void EnforceExtension(StdStrBuf *sFilename, const char *szExtension);
void RemoveExtension(char *szFileName);
void RemoveExtension(StdStrBuf *psFileName);
void AppendBackslash(char *szFileName);
void TruncateBackslash(char *szFilename);
void MakeTempFilename(char *szFileName);
void MakeTempFilename(StdStrBuf *sFileName);
bool WildcardListMatch(const char *szWildcardList, const char *szString); // match string in list like *.png|*.bmp
bool IsWildcardString(const char *szString); // does szString contain wildcard characters?
bool WildcardMatch(const char *szWildcard, const char *szString);
bool TruncatePath(char *szPath);
// szBuffer has to be of at least _MAX_PATH length.
bool GetParentPath(const char *szFilename, char *szBuffer);
bool GetParentPath(const char *szFilename, StdStrBuf *outBuf);
const char *GetRelativePathS(const char *strPath, const char *strRelativeTo);
bool IsGlobalPath(const char *szPath);

bool DirectoryExists(const char *szFileName);
bool FileExists(const char *szFileName);
size_t FileSize(const char *fname);
size_t FileSize(int fdes);
int FileTime(const char *fname);
bool EraseFile(const char *szFileName);
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
bool CopyItem(const char *szSource, const char *szTarget, bool fResetAttributes=false);
bool CreateItem(const char *szItemname);
bool MoveItem(const char *szSource, const char *szTarget);

int ForEachFile(const char *szDirName, bool (*fnCallback)(const char *));

struct DirectoryIteratorP;
class DirectoryIterator
{
	// Shallow copyable, ordered directory iterator
public:
	DirectoryIterator(const char * dirname);
	DirectoryIterator();
	DirectoryIterator & operator = (const DirectoryIterator &);
	DirectoryIterator(const DirectoryIterator &);
	~DirectoryIterator();

	const char * operator * () const;
	const char *GetName() const { return **this; }
	size_t GetFileSize() const;

	DirectoryIterator& operator ++ ();
	DirectoryIterator operator ++ (int);
	void Clear(); // put iterator into empty state and clear any cached directory listing
	void Reset(const char * dirname, bool force_reread=false); // reset iterator to front of file list. re-read directory if it changed or force_reread is set.
	void Reset(); // reset iterator to front of file list without re-reading directory
private:
	void Read(const char *dirname);
	friend struct DirectoryIteratorP;
	typedef std::vector<std::pair<std::string, size_t>> FileList;
	DirectoryIteratorP *p;
	FileList::iterator iter;
};

#endif // STDFILE_INCLUDED
