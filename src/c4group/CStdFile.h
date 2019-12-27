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

/* A handy wrapper class to gzio files */

#ifndef INC_CSTDFILE
#define INC_CSTDFILE

#include "platform/StdSync.h" // for StdThreadCheck
#include <zlib.h> // for gzFile

const int CStdFileBufSize = 4096;

class CStdStream
{
public:
	virtual bool Read(void *pBuffer, size_t iSize) = 0;
	virtual bool Advance(int iOffset) = 0;
	// Get size. compatible with c4group!
	virtual size_t AccessedEntrySize() const = 0;
	virtual ~CStdStream() = default;
};

class CStdFile: public CStdStream
{
public:
	CStdFile();
	~CStdFile() override;
	bool Status;
	char Name[_MAX_PATH_LEN];
protected:
	FILE *hFile;
	gzFile hgzFile;
	StdBuf *pMemory;
	int MemoryPtr;
	BYTE Buffer[CStdFileBufSize];
	int BufferLoad,BufferPtr;
	bool ModeWrite;
	StdThreadCheck thread_check; // thread check helper to make sure only the thread that opened the file is using it
public:
	bool Create(const char *szFileName, bool fCompressed=false, bool fExecutable=false, bool fMemory=false);
	bool Open(const char *szFileName, bool fCompressed=false);
	bool Append(const char *szFilename, bool text=false); // append (uncompressed only)
	bool Close(StdBuf **ppMemory = nullptr);
	bool Default();
	bool Read(void *pBuffer, size_t iSize) override { return Read(pBuffer, iSize, nullptr); }
	bool Read(void *pBuffer, size_t iSize, size_t *ipFSize);
	bool Write(const void *pBuffer, int iSize);
	bool WriteString(const char *szStr);
	bool Rewind();
	bool Advance(int iOffset) override;
	int Seek(long int offset, int whence); // seek in file by offset and stdio-style SEEK_* constants. Only implemented for uncompressed files.
	long int Tell(); // get current file pos. Only implemented for uncompressed files.
	bool IsOpen() const { return hFile || hgzFile; }
	// flush contents to disk
	inline bool Flush() { if (ModeWrite && BufferLoad) return SaveBuffer(); else return true; }
	size_t AccessedEntrySize() const override;
protected:
	void ClearBuffer();
	int LoadBuffer();
	bool SaveBuffer();
};

int UncompressedFileSize(const char *szFileName);
bool GetFileCRC(const char *szFilename, uint32_t *pCRC32);
bool GetFileSHA1(const char *szFilename, BYTE *pSHA1);

#endif // INC_CSTDFILE
