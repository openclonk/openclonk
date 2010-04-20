/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2004, 2007  Günther Brammer
 * Copyright (c) 2008  Peter Wortmann
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

/* A handy wrapper class to gzio files */

#ifndef INC_CSTDFILE
#define INC_CSTDFILE

#include <stdio.h>
#include <StdFile.h>
#include <StdBuf.h>

const int CStdFileBufSize = 4096;

typedef void* gzFile;

class CStdStream
{
public:
	virtual bool Read(void *pBuffer, size_t iSize) = 0;
	virtual bool Advance(int iOffset) = 0;
	// Get size. compatible with c4group!
	virtual size_t AccessedEntrySize() = 0;
	virtual ~CStdStream() {}
};

class CStdFile: public CStdStream
{
public:
	CStdFile();
	~CStdFile();
	bool Status;
	char Name[_MAX_PATH+1];
protected:
	FILE *hFile;
	gzFile hgzFile;
	StdBuf *pMemory;
	int MemoryPtr;
	BYTE Buffer[CStdFileBufSize];
	int BufferLoad,BufferPtr;
	bool ModeWrite;
public:
	bool Create(const char *szFileName, bool fCompressed=false, bool fExecutable=false, bool fMemory=false);
	bool Open(const char *szFileName, bool fCompressed=false);
	bool Append(const char *szFilename); // append (uncompressed only)
	bool Close(StdBuf **ppMemory = NULL);
	bool Default();
	bool Read(void *pBuffer, size_t iSize) { return Read(pBuffer, iSize, 0); }
	bool Read(void *pBuffer, size_t iSize, size_t *ipFSize);
	bool Write(const void *pBuffer, int iSize);
	bool WriteString(const char *szStr);
	bool Rewind();
	bool Advance(int iOffset);
	// Single line commands
	bool Load(const char *szFileName, BYTE **lpbpBuf,
	          int *ipSize=NULL, int iAppendZeros=0,
	          bool fCompressed = false);
	bool Save(const char *szFileName, const BYTE *bpBuf,
	          int iSize,
	          bool fCompressed = false);
	// flush contents to disk
	inline bool Flush() { if (ModeWrite && BufferLoad) return SaveBuffer(); else return true; }
	size_t AccessedEntrySize();
protected:
	void ClearBuffer();
	int LoadBuffer();
	bool SaveBuffer();
};

int UncompressedFileSize(const char *szFileName);

#endif // INC_CSTDFILE
