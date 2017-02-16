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

#include "C4Include.h"
#ifdef _WIN32
#	include "platform/C4windowswrapper.h"
#endif
#include "platform/StdFile.h"
#include "c4group/CStdFile.h"
#include "lib/SHA1.h"

#include <zlib.h>
#include "zlib/gzio.h"
#include <stdio.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>

CStdFile::CStdFile()
{
	thread_check.Set();
	Status=false;
	hFile=nullptr;
	hgzFile=nullptr;
	pMemory=nullptr;
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
	thread_check.Set();
	// Set modes
	ModeWrite=true;
	// Open in memory?
	if (fMemory)
	{
		if (!(pMemory = new StdBuf())) return false;
		MemoryPtr = 0;

		StdStrBuf buf;
		buf.Format("<%p>", pMemory);
		SCopy(buf.getData(), Name, _MAX_PATH);
	}
	// Open standard file
	else
	{
		SCopy(szFilename,Name,_MAX_PATH);
		int flags = _O_BINARY|O_CLOEXEC|O_CREAT|O_WRONLY|O_TRUNC;
#ifdef _WIN32
		int mode = _S_IREAD|_S_IWRITE;
		int fd = _wopen(GetWideChar(Name), flags, mode);
#else
		mode_t mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH;
		if (fExecutable)
			mode |= S_IXUSR|S_IXGRP|S_IXOTH;
		int fd = open(Name, flags, mode);
#endif
		if (fd == -1) return false;
		if (fCompressed)
		{
			if (!(hgzFile = c4_gzdopen(fd,"wb1"))) return false;
		}
		else
		{
			if (!(hFile = fdopen(fd,"wb"))) return false;
		}
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
	thread_check.Set();
	// Set modes
	ModeWrite=false;
	int flags = _O_BINARY|O_CLOEXEC|O_RDONLY;
#ifdef _WIN32
	int mode = _S_IREAD|_S_IWRITE;
	int fd = _wopen(GetWideChar(Name), flags, mode);
#else
	mode_t mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH;
	int fd = open(Name, flags, mode);
#endif
	if(fd == -1) return false;
	if (fCompressed)
	{
		if (!(hgzFile = c4_gzdopen(fd,"rb"))) { close(fd); return false; }
		/* Reject uncompressed files */
		if(c4_gzdirect(hgzFile))
		{
			c4_gzclose(hgzFile);
			hgzFile = nullptr;
			return false;
		}
	}
	else
	{ 
		if (!(hFile = fdopen(fd,"rb"))) return false;
	}
	// Reset buffer
	ClearBuffer();
	// Set status
	Status=true;
	return true;
}

bool CStdFile::Append(const char *szFilename, bool text)
{
	SCopy(szFilename,Name,_MAX_PATH);
	thread_check.Set();
	// Set modes
	ModeWrite=true;
	// Open standard file
#ifdef _WIN32
	if (!(hFile = _wfopen(GetWideChar(Name), text ? L"at" : L"ab"))) return false;
#else
	if (!(hFile=fopen(Name,text ? "at" : "ab"))) return false;
#endif
	// Reset buffer
	ClearBuffer();
	// Set status
	Status=true;
	return true;
}

bool CStdFile::Close(StdBuf **ppMemory)
{
	thread_check.Check();
	bool rval=true;
	Status=false;
	Name[0]=0;
	// Save buffer if in write mode
	if (ModeWrite && BufferLoad) if (!SaveBuffer()) rval=false;
	// Close file(s)
	if (hgzFile) if (c4_gzclose(hgzFile)!=Z_OK) rval=false;
	if (hFile) if (fclose(hFile)!=0) rval=false;
	if (pMemory)
	{
		if (ppMemory)
			{ *ppMemory = pMemory; pMemory = nullptr; }
		else
			delete pMemory;
	}
	MemoryPtr=0;
	hgzFile=nullptr; hFile=nullptr;
	return !!rval;
}

bool CStdFile::Default()
{
	Status=false;
	Name[0]=0;
	hgzFile=nullptr;
	hFile=nullptr;
	pMemory=nullptr;
	MemoryPtr=0;
	BufferLoad=BufferPtr=0;
	thread_check.Set();
	return true;
}

bool CStdFile::Read(void *pBuffer, size_t iSize, size_t *ipFSize)
{
	thread_check.Check();
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
			transfer=std::min<size_t>(BufferLoad-BufferPtr,iSize);
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
	thread_check.Check();
	if (hFile) BufferLoad = fread(Buffer,1,CStdFileBufSize,hFile);
	if (hgzFile) BufferLoad = c4_gzread(hgzFile, Buffer,CStdFileBufSize);
	BufferPtr=0;
	return BufferLoad;
}

bool CStdFile::SaveBuffer()
{
	thread_check.Check();
	int saved = 0;
	if (hFile) saved=fwrite(Buffer,1,BufferLoad,hFile);
	if (hgzFile) saved=c4_gzwrite(hgzFile,Buffer,BufferLoad);
	if (pMemory) { pMemory->Append(Buffer, BufferLoad); saved = BufferLoad; }
	if (saved!=BufferLoad) return false;
	BufferLoad=0;
	return true;
}

void CStdFile::ClearBuffer()
{
	thread_check.Check();
	BufferLoad=BufferPtr=0;
}

bool CStdFile::Write(const void *pBuffer, int iSize)
{
	thread_check.Check();
	int transfer;
	if (!pBuffer) return false;
	if (!ModeWrite) return false;
	const BYTE *bypBuffer= (const BYTE*) pBuffer;
	while (iSize>0)
	{
		// Space in buffer: Transfer as much as possible
		if (BufferLoad<CStdFileBufSize)
		{
			transfer=std::min(CStdFileBufSize-BufferLoad,iSize);
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
	thread_check.Check();
	BYTE nl[2]={0x0D,0x0A};
	if (!szStr) return false;
	int size=SLen(szStr);
	if (!Write((const void*)szStr,size)) return false;
	if (!Write(nl,2)) return false;
	return true;
}

bool CStdFile::Rewind()
{
	thread_check.Check();
	if (ModeWrite) return false;
	ClearBuffer();
	if (hFile) rewind(hFile);
	if (hgzFile) c4_gzrewind(hgzFile);
	return true;
}

bool CStdFile::Advance(int iOffset)
{
	thread_check.Check();
	if (ModeWrite) return false;
	while (iOffset > 0)
	{
		// Valid data in the buffer: Transfer as much as possible
		if (BufferLoad > BufferPtr)
		{
			int transfer = std::min(BufferLoad-BufferPtr,iOffset);
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

int CStdFile::Seek(long int offset, int whence)
{
	// seek in file by offset and stdio-style SEEK_* constants. Only implemented for uncompressed files.
	assert(!hgzFile);
	return fseek(hFile, offset, whence);
}

long int CStdFile::Tell()
{
	// get current file pos. Only implemented for uncompressed files.
	assert(!hgzFile);
	return ftell(hFile);
}

int UncompressedFileSize(const char *szFilename)
{
	int rd,rval=0;
	BYTE buf[1024];
	int flags = _O_BINARY|O_CLOEXEC|O_RDONLY;
#ifdef _WIN32
	int mode = _S_IREAD|_S_IWRITE;
	int fd = _wopen(GetWideChar(szFilename), flags, mode);
#else
	mode_t mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH;
	int fd = open(szFilename, flags, mode);
#endif
	gzFile hFile;
	if (!(hFile = c4_gzdopen(fd,"rb"))) return 0;
	do
	{
		rd = c4_gzread(hFile,&buf,sizeof(buf));
		if (rd < 0) break;
		rval += rd;
	}
	while (rd == sizeof buf);
	c4_gzclose(hFile);
	return rval;
}

size_t CStdFile::AccessedEntrySize() const
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
	ctx.get_digest((sha1::digest_type) *pSHA1);
	return true;
}
