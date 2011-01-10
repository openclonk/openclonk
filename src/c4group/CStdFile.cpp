/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2004, 2008  Peter Wortmann
 * Copyright (c) 2005, 2007, 2009, 2011  Günther Brammer
 * Copyright (c) 2005-2006  Sven Eberhardt
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

/* A handy wrapper class to gzio files */

#include "C4Include.h"
#ifdef _WIN32
#	include "platform/C4windowswrapper.h"
#endif
#include <StdFile.h>
#include <CStdFile.h>
#include <SHA1.h>

#include <zlib.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>

CStdFile::CStdFile()
{
	Status=false;
	hFile=NULL;
	hgzFile=NULL;
	pMemory=NULL;
	ClearBuffer();
	ModeWrite=false;
	Name[0]=0;
}

CStdFile::~CStdFile()
{
	Close();
}

bool CStdFile::Create(const char *szFilename, bool fCompressed, bool fExecutable, bool fMemory)
{
	SCopy(szFilename,Name,_MAX_PATH);
	// Set modes
	ModeWrite=true;
	// Open in memory?
	if (fMemory)
	{
		if (!(pMemory = new StdBuf())) return false;
		MemoryPtr = 0;
	}
	// Open standard file
	else if (fCompressed)
	{
#ifdef _WIN32
		int mode = _S_IREAD|_S_IWRITE;
		int flags = _O_BINARY|_O_CREAT|_O_WRONLY|_O_TRUNC;
		int fd = _wopen(GetWideChar(Name), flags, mode);
#else
		mode_t mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH;
		int flags = O_CREAT|O_WRONLY|O_TRUNC;
		int fd = open(Name, flags, mode);
#endif
		if (!(hgzFile = gzdopen(fd,"wb1"))) return false;
	}
	else
	{
#ifdef _WIN32
		int mode = _S_IREAD|_S_IWRITE;
		int flags = _O_BINARY|_O_CREAT|_O_WRONLY|_O_TRUNC;
		int fd = _wopen(GetWideChar(Name), flags, mode);
#else
		mode_t mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH;
		if (fExecutable)
			mode |= S_IXUSR|S_IXGRP|S_IXOTH;
		int flags = O_CREAT|O_WRONLY|O_TRUNC;
		int fd = open(Name, flags, mode);
#endif
		if (fd == -1) return false;
		if (!(hFile = fdopen(fd,"wb"))) return false;
	}
	// Reset buffer
	ClearBuffer();
	// Set status
	Status=true;
	return true;
}

bool CStdFile::Open(const char *szFilename, bool fCompressed)
{
	SCopy(szFilename,Name,_MAX_PATH);
	// Set modes
	ModeWrite=false;
	// Open standard file
	if (fCompressed)
	{
#ifdef _WIN32
		int mode = _S_IREAD|_S_IWRITE;
		int flags = _O_BINARY|_O_RDONLY;
		int fd = _wopen(GetWideChar(Name), flags, mode);
#else
		mode_t mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH;
		int flags = O_RDONLY;
		int fd = open(Name, flags, mode);
#endif
		if (!(hgzFile = gzdopen(fd,"rb"))) return false;
	}
	else
	{ 
#ifdef _WIN32
		if (!(hFile=_wfopen(GetWideChar(Name),L"rb"))) return false;
#else
		if (!(hFile=fopen(Name,"rb"))) return false;
#endif
	}
	// Reset buffer
	ClearBuffer();
	// Set status
	Status=true;
	return true;
}

bool CStdFile::Append(const char *szFilename)
{
	SCopy(szFilename,Name,_MAX_PATH);
	// Set modes
	ModeWrite=true;
	// Open standard file
#ifdef _WIN32
	if (!(hFile=_wfopen(GetWideChar(Name),L"ab"))) return false;
#else
	if (!(hFile=fopen(Name,"ab"))) return false;
#endif
	// Reset buffer
	ClearBuffer();
	// Set status
	Status=true;
	return true;
}

bool CStdFile::Close(StdBuf **ppMemory)
{
	bool rval=true;
	Status=false;
	Name[0]=0;
	// Save buffer if in write mode
	if (ModeWrite && BufferLoad) if (!SaveBuffer()) rval=false;
	// Close file(s)
	if (hgzFile) if (gzclose(hgzFile)!=Z_OK) rval=false;
	if (hFile) if (fclose(hFile)!=0) rval=false;
	if (pMemory)
	{
		if (ppMemory)
			{ *ppMemory = pMemory; pMemory = NULL; }
		else
			delete pMemory;
	}
	MemoryPtr=0;
	hgzFile=NULL; hFile=NULL;
	return !!rval;
}

bool CStdFile::Default()
{
	Status=false;
	Name[0]=0;
	hgzFile=NULL;
	hFile=NULL;
	pMemory=NULL;
	MemoryPtr=0;
	BufferLoad=BufferPtr=0;
	return true;
}

bool CStdFile::Read(void *pBuffer, size_t iSize, size_t *ipFSize)
{
	int transfer;
	if (!pBuffer) return false;
	if (ModeWrite) return false;
	BYTE *bypBuffer= (BYTE*) pBuffer;
	if (ipFSize) *ipFSize = 0;
	while (iSize>0)
	{
		// Valid data in the buffer: Transfer as much as possible
		if (BufferLoad>BufferPtr)
		{
			transfer=Min<size_t>(BufferLoad-BufferPtr,iSize);
			memmove(bypBuffer, Buffer+BufferPtr, transfer);
			BufferPtr+=transfer;
			bypBuffer+=transfer;
			iSize-=transfer;
			if (ipFSize) *ipFSize += transfer;
		}
		// Buffer empty: Load
		else if (LoadBuffer()<=0) return false;
	}
	return true;
}

int CStdFile::LoadBuffer()
{
	if (hFile) BufferLoad = fread(Buffer,1,CStdFileBufSize,hFile);
	if (hgzFile) BufferLoad = gzread(hgzFile, Buffer,CStdFileBufSize);
	BufferPtr=0;
	return BufferLoad;
}

bool CStdFile::SaveBuffer()
{
	int saved = 0;
	if (hFile) saved=fwrite(Buffer,1,BufferLoad,hFile);
	if (hgzFile) saved=gzwrite(hgzFile,Buffer,BufferLoad);
	if (pMemory) { pMemory->Append(Buffer, BufferLoad); saved = BufferLoad; }
	if (saved!=BufferLoad) return false;
	BufferLoad=0;
	return true;
}

void CStdFile::ClearBuffer()
{
	BufferLoad=BufferPtr=0;
}

bool CStdFile::Write(const void *pBuffer, int iSize)
{
	int transfer;
	if (!pBuffer) return false;
	if (!ModeWrite) return false;
	BYTE *bypBuffer= (BYTE*) pBuffer;
	while (iSize>0)
	{
		// Space in buffer: Transfer as much as possible
		if (BufferLoad<CStdFileBufSize)
		{
			transfer=Min(CStdFileBufSize-BufferLoad,iSize);
			memcpy(Buffer+BufferLoad,bypBuffer,transfer);
			BufferLoad+=transfer;
			bypBuffer+=transfer;
			iSize-=transfer;
		}
		// Buffer full: Save
		else if (!SaveBuffer()) return false;
	}
	return true;
}

bool CStdFile::WriteString(const char *szStr)
{
	BYTE nl[2]={0x0D,0x0A};
	if (!szStr) return false;
	int size=SLen(szStr);
	if (!Write((void*)szStr,size)) return false;
	if (!Write(nl,2)) return false;
	return true;
}

bool CStdFile::Rewind()
{
	if (ModeWrite) return false;
	ClearBuffer();
	if (hFile) rewind(hFile);
	if (hgzFile) gzrewind(hgzFile);
	return true;
}

bool CStdFile::Advance(int iOffset)
{
	if (ModeWrite) return false;
	while (iOffset > 0)
	{
		// Valid data in the buffer: Transfer as much as possible
		if (BufferLoad > BufferPtr)
		{
			int transfer = Min(BufferLoad-BufferPtr,iOffset);
			BufferPtr += transfer;
			iOffset -= transfer;
		}
		// Buffer empty: Load or skip
		else
		{
			if (hFile) return !fseek(hFile, iOffset, SEEK_CUR); // uncompressed: Just skip
			if (LoadBuffer()<=0) return false; // compressed: Read...
		}
	}
	return true;
}

int UncompressedFileSize(const char *szFilename)
{
	int rd,rval=0;
	BYTE buf[1024];
#ifdef _WIN32
	int mode = _S_IREAD|_S_IWRITE;
	int flags = _O_BINARY|_O_RDONLY;
	int fd = _wopen(GetWideChar(szFilename), flags, mode);
#else
	mode_t mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH;
	int flags = O_RDONLY;
	int fd = open(szFilename, flags, mode);
#endif
	gzFile hFile;
	if (!(hFile = gzdopen(fd,"rb"))) return 0;
	do
	{
		rd = gzread(hFile,&buf,sizeof(buf));
		if (rd < 0) break;
		rval += rd;
	}
	while (rd == sizeof buf);
	gzclose(hFile);
	return rval;
}

size_t CStdFile::AccessedEntrySize()
{
	if (hFile)
		return FileSize(fileno(hFile));
	assert(!hgzFile);
	return 0;
}

bool GetFileCRC(const char *szFilename, uint32_t *pCRC32)
{
	if (!pCRC32) return false;
	// open file
	CStdFile File;
	if (!File.Open(szFilename))
		return false;
	// calculcate CRC
	uint32_t iCRC32 = 0;
	for (;;)
	{
		// read a chunk of data
		BYTE szData[CStdFileBufSize]; size_t iSize = 0;
		if (!File.Read(szData, CStdFileBufSize, &iSize))
			if (!iSize)
				break;
		// update CRC
		iCRC32 = crc32(iCRC32, szData, iSize);
	}
	// close file
	File.Close();
	// okay
	*pCRC32 = iCRC32;
	return true;
}

bool GetFileSHA1(const char *szFilename, BYTE *pSHA1)
{
	if (!pSHA1) return false;
	// open file
	CStdFile File;
	if (!File.Open(szFilename))
		return false;
	// calculcate CRC
	sha1 ctx;
	for (;;)
	{
		// read a chunk of data
		BYTE szData[CStdFileBufSize]; size_t iSize = 0;
		if (!File.Read(szData, CStdFileBufSize, &iSize))
			if (!iSize)
				break;
		// update CRC
		ctx.process_bytes(szData, iSize);
	}
	// close file
	File.Close();
	// finish calculation
	ctx.get_digest((sha1::digest_type) pSHA1);
	return true;
}
