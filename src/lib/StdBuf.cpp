/*
 * OpenClonk, http://www.openclonk.org
 *
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
#include "C4Include.h"
#include "lib/StdBuf.h"
#include "lib/StdCompiler.h"
#include "lib/StdAdaptors.h"
#include "platform/StdFile.h"

#include <stdarg.h>
#include <stdio.h>
#ifdef _WIN32
#include <io.h>
#include "platform/C4windowswrapper.h"
#else
#define O_BINARY 0
#define O_SEQUENTIAL 0
#include <unistd.h>
#include <stdlib.h>
#endif
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>

// *** StdBuf

bool StdBuf::LoadFromFile(const char *szFile)
{
	// Open file
#ifdef _WIN32
	int fh = _wopen(::GetWideChar(szFile), O_BINARY | O_RDONLY | O_SEQUENTIAL, S_IREAD | S_IWRITE);
#else
	int fh = open(szFile, O_BINARY | O_CLOEXEC | O_RDONLY | O_SEQUENTIAL, S_IREAD | S_IWRITE);
#endif
	if (fh < 0) return false;
	// Create buf
	New(FileSize(fh));
	// Read
	if (read(fh, getMData(), getSize()) != (signed int) getSize())
	{
		close(fh);
		return false;
	}
	close(fh);
	// Ok
	return true;
}
bool StdBuf::SaveToFile(const char *szFile) const
{
	// Open file
#ifdef _WIN32
	int fh = _wopen(::GetWideChar(szFile), O_BINARY | O_CREAT | O_WRONLY | O_SEQUENTIAL | O_TRUNC, S_IREAD | S_IWRITE);
#else
	int fh = open(szFile, O_BINARY | O_CLOEXEC | O_CREAT | O_WRONLY | O_SEQUENTIAL | O_TRUNC, S_IREAD | S_IWRITE);
#endif
	if (fh < 0) return false;
	// Write data
	if (write(fh, getData(), getSize()) != (signed int) getSize())
	{
		close(fh);
		return false;
	}
	close(fh);
	// Ok
	return true;
}

bool StdStrBuf::LoadFromFile(const char *szFile)
{
	// Open file
#ifdef _WIN32
	int fh = _wopen(::GetWideChar(szFile), O_BINARY | O_RDONLY | O_SEQUENTIAL, S_IREAD | S_IWRITE);
#else
	int fh = open(szFile, O_BINARY | O_CLOEXEC | O_RDONLY | O_SEQUENTIAL, S_IREAD | S_IWRITE);
#endif
	if (fh < 0) return false;
	// Create buf
	SetLength(FileSize(fh));
	// Read
	if (read(fh, getMData(), getLength()) != (ssize_t) getLength())
	{
		close(fh);
		return false;
	}
	close(fh);
	// Ok
	return true;
}
bool StdStrBuf::SaveToFile(const char *szFile) const
{
	// Open file
#ifdef _WIN32
	int fh = _wopen(::GetWideChar(szFile), O_BINARY | O_CREAT | O_WRONLY | O_SEQUENTIAL | O_TRUNC, S_IREAD | S_IWRITE);
#else
	int fh = open(szFile, O_BINARY | O_CLOEXEC | O_CREAT | O_WRONLY | O_SEQUENTIAL | O_TRUNC, S_IREAD | S_IWRITE);
#endif
	if (fh < 0) return false;
	// Write data
	if (write(fh, getData(), getLength()) != (ssize_t) getLength())
	{
		close(fh);
		return false;
	}
	close(fh);
	// Ok
	return true;
}

void StdBuf::CompileFunc(StdCompiler *pComp, int iType)
{
	// Size (guess it is a small value most of the time - if it's big, an extra byte won't hurt anyway)
	uint32_t tmp = iSize; pComp->Value(mkIntPackAdapt(tmp)); iSize = tmp;
	pComp->Separator(StdCompiler::SEP_PART2);
	// Read/write data
	if (pComp->isDeserializer())
	{
		New(iSize);
		pComp->Raw(getMData(), iSize, StdCompiler::RawCompileType(iType));
	}
	else
	{
		pComp->Raw(const_cast<void *>(getData()), iSize, StdCompiler::RawCompileType(iType));
	}
}

// *** StdStringBuf

#ifdef _WIN32
StdStrBuf::StdStrBuf(const wchar_t * utf16)
{
	int len = WideCharToMultiByte(CP_UTF8, 0, utf16, -1, nullptr, 0, 0, 0);
	SetSize(len);
	WideCharToMultiByte(CP_UTF8, 0, utf16, -1, getMData(), getSize(), 0, 0);
}
StdStrBuf::wchar_t_holder StdStrBuf::GetWideChar() const
{
	if (!getSize()) return StdStrBuf::wchar_t_holder(nullptr);

	int len = MultiByteToWideChar(CP_UTF8, 0, getData(), getSize(), nullptr, 0);
	wchar_t * p = new wchar_t[len];
	MultiByteToWideChar(CP_UTF8, 0, getData(), getSize(), p, len);
	return StdStrBuf::wchar_t_holder(p);
}
StdBuf StdStrBuf::GetWideCharBuf()
{
	int len = MultiByteToWideChar(CP_UTF8, 0, getData(), getSize(), nullptr, 0);
	StdBuf r; r.SetSize(len * sizeof(wchar_t));
	MultiByteToWideChar(CP_UTF8, 0, getData(), getSize(), getMBufPtr<wchar_t>(r), len);
	return r;
}
StdStrBuf::wchar_t_holder GetWideChar(const char * utf8, bool double_null_terminate)
{
	int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
	if (double_null_terminate) ++len;
	wchar_t * p = new wchar_t[len];
	MultiByteToWideChar(CP_UTF8, 0, utf8, -1, p, len);
	if (double_null_terminate) p[len - 1] = wchar_t(0);
	return StdStrBuf::wchar_t_holder(p);
}
StdBuf GetWideCharBuf(const char * utf8)
{
	int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
	StdBuf r; r.SetSize(len * sizeof(wchar_t));
	MultiByteToWideChar(CP_UTF8, 0, utf8, -1, getMBufPtr<wchar_t>(r), len);
	return r;
}
#endif

void StdStrBuf::Format(const char *szFmt, ...)
{
	// Create argument list
	va_list args; va_start(args, szFmt);
	// Format
	FormatV(szFmt, args);
}

void StdStrBuf::FormatV(const char *szFmt, va_list args)
{
	// Clear previous contents
	Clear();
	// Format
	AppendFormatV(szFmt, args);
}

void StdStrBuf::AppendFormat(const char *szFmt, ...)
{
	// Create argument list
	va_list args; va_start(args, szFmt);
	// Format
	AppendFormatV(szFmt, args);
}

void StdStrBuf::AppendFormatV(const char *szFmt, va_list args)
{
#ifdef HAVE_VASPRINTF
	// Format
	char *pStr; int iBytes = vasprintf(&pStr, szFmt, args);
	if (iBytes < 0 || !pStr) return;
	// Append
	if (isNull())
		Take(pStr, iBytes);
	else
	{
		Append(pStr, iBytes);
		free(pStr);
	}
#elif defined(HAVE_VSCPRINTF)
	// Save append start
	int iStart = getLength();
	// Calculate size, allocate
	int iLength = vscprintf(szFmt, args);
	Grow(iLength);
	// Format
	char *pPos = getMElem<char>(*this, iSize - iLength - 1);
	vsprintf(getMPtr(iStart), szFmt, args);
#else
	// Save append start
	int iStart = getLength(), iBytes;
	do
	{
		// Grow
		Grow(512);
		// Try output
		va_list args_copy;
		#ifdef va_copy
			va_copy(args_copy, args);
		#else
			args_copy = args;
		#endif
		iBytes = vsnprintf(getMPtr(iStart), getLength() - iStart, szFmt, args_copy);
		#ifdef va_copy
			va_end(args_copy);
		#endif
	}
	while (iBytes < 0 || (unsigned int)(iBytes) >= getLength() - iStart);
	// Calculate real length, if vsnprintf didn't return anything of value
	iBytes = strlen(getMPtr(iStart));
	// Shrink to fit
	SetSize(iStart + iBytes + 1);
#endif
}

void StdStrBuf::AppendBackslash()
{
	if(getLength() && *getPtr(getLength() - 1) == DirectorySeparator) return;
	AppendChar(DirectorySeparator);
}

void StdStrBuf::CompileFunc(StdCompiler *pComp, int iRawType)
{
	if (pComp->isDeserializer())
	{
		char *pnData;
		pComp->String(&pnData, StdCompiler::RawCompileType(iRawType));
		Take(pnData);
	}
	else
	{
		char *pData = const_cast<char *>(getData());
		if (!pData) pData = const_cast<char *>("");
		pComp->String(&pData, StdCompiler::RawCompileType(iRawType));
	}
}

StdStrBuf FormatString(const char *szFmt, ...)
{
	va_list args; va_start(args, szFmt);
	return FormatStringV(szFmt, args);
}

StdStrBuf FormatStringV(const char *szFmt, va_list args)
{
	StdStrBuf Buf;
	Buf.FormatV(szFmt, args);
	return Buf;
}

// replace all occurences of one string with another. Return number of replacements.
int StdStrBuf::Replace(const char *szOld, const char *szNew, size_t iStartSearch)
{
	if (!getPtr(0) || !szOld) return 0;
	if (!szNew) szNew = "";
	int cnt=0;
	size_t iOldLen = strlen(szOld), iNewLen = strlen(szNew);
	if (iOldLen != iNewLen)
	{
		// count number of occurences to calculate new string length
		size_t iResultLen = getLength();
		const char *szPos = getPtr(iStartSearch);
		while ((szPos = SSearch(szPos, szOld)))
		{
			iResultLen += iNewLen - iOldLen;
			++cnt;
		}
		if (!cnt) return 0;
		// now construct new string by replacement
		StdStrBuf sResult;
		sResult.New(iResultLen+1);
		const char *szRPos = getPtr(0), *szRNextPos;
		char *szWrite = sResult.getMPtr(0);
		if (iStartSearch)
		{
			memcpy(szWrite, szRPos, iStartSearch * sizeof(char));
			szRPos += iStartSearch;
			szWrite += iStartSearch;
		}
		while ((szRNextPos = SSearch(szRPos, szOld)))
		{
			memcpy(szWrite, szRPos, (szRNextPos - szRPos - iOldLen) * sizeof(char));
			szWrite += (szRNextPos - szRPos - iOldLen);
			memcpy(szWrite, szNew, iNewLen * sizeof(char));
			szWrite += iNewLen;
			szRPos = szRNextPos;
		}
		strcpy(szWrite, szRPos);
		Take(std::move(sResult));
	}
	else
	{
		// replace directly in this string
		char *szRPos = getMPtr(iStartSearch);
		while ((szRPos = const_cast<char *>(SSearch(szRPos, szOld))))
		{
			memcpy(szRPos - iOldLen, szNew, iOldLen * sizeof(char));
			++cnt;
		}
	}
	return cnt;
}

int StdStrBuf::ReplaceChar(char cOld, char cNew)
{
	if (isNull()) return 0;
	char *szPos = getMPtr(0);
	if (!cOld) return 0;
	if (!cNew) cNew = '_';
	int cnt=0;
	while ((szPos = strchr(szPos, cOld)))
	{
		*szPos++ = cNew;
		++cnt;
	}
	return cnt;
}

void StdStrBuf::ReplaceEnd(size_t iPos, const char *szNewEnd)
{
	size_t iLen = getLength();
	assert(iPos <= iLen); if (iPos > iLen) return;
	size_t iEndLen = strlen(szNewEnd);
	if (iLen - iPos != iEndLen) SetLength(iPos + iEndLen);
	memcpy(getMPtr(iPos), szNewEnd, iEndLen * sizeof(char));
}

bool StdStrBuf::ValidateChars(const char *szInitialChars, const char *szMidChars)
{
	// only given chars may be in string
	for (size_t i=0; i<getLength(); ++i)
		if (!strchr(i ? szMidChars : szInitialChars, getData()[i]))
			return false;
	return true;
}

bool StdStrBuf::GetSection(size_t idx, StdStrBuf *psOutSection, char cSeparator) const
{
	assert(psOutSection);
	psOutSection->Clear();
	const char *szStr = getData(), *szSepPos;
	if (!szStr) return false; // invaid argument
	while ((szSepPos = strchr(szStr, cSeparator)) && idx) { szStr = szSepPos+1; --idx; }
	if (idx) return false; // indexed section not found
	// fill output buffer with section, if not empty
	if (!szSepPos) szSepPos = getData() + getLength();
	if (szSepPos != szStr) psOutSection->Copy(szStr, szSepPos - szStr);
	// return true even if section is empty, because the section obviously exists
	// (to enable loops like while (buf.GetSection(i++, &sect)) if (sect) ...)
	return true;
}

void StdStrBuf::ToLowerCase()
{
	if (!isNull())
		for (char *szPos = getMPtr(0); *szPos; ++szPos)
			*szPos = tolower(*szPos);
}

void StdStrBuf::AppendCharacter(uint32_t unicodechar)
{
	if (unicodechar < 0x80)
		AppendChar(unicodechar);
	else if (unicodechar < 0x800)
	{
		Grow(2);
		*getMPtr(getLength() - 2) = (0xC0 | (unicodechar >> 6));
		*getMPtr(getLength() - 1) = (0x80 | (unicodechar & 0x3F));
	}
	else if (unicodechar < 0x10000)
	{
		Grow(3);
		*getMPtr(getLength() - 3) = (0xE0 | (unicodechar >> 12));
		*getMPtr(getLength() - 2) = (0x80 | ((unicodechar >> 6) & 0x3F));
		*getMPtr(getLength() - 1) = (0x80 | (unicodechar & 0x3F));
	}
	else if (unicodechar < 0x110000)
	{
		Grow(4);
		*getMPtr(getLength() - 4) = (0xF0 | (unicodechar >> 18));
		*getMPtr(getLength() - 3) = (0x80 | ((unicodechar >> 12) & 0x3F));
		*getMPtr(getLength() - 2) = (0x80 | ((unicodechar >> 6) & 0x3F));
		*getMPtr(getLength() - 1) = (0x80 | (unicodechar & 0x3F));
	}
	else /* not an unicode code point, ignore */ {}
}

// Returns true if charset was converted.
bool StdStrBuf::EnsureUnicode()
{
	// assume that it's windows-1252 and convert to utf-8
	if (!IsValidUtf8(getData(), getLength()))
	{
		size_t j = 0;
		StdStrBuf buf;
		buf.Grow(getLength());
		// totally unfounded statistic: most texts have less than 20 umlauts.
		enum { GROWSIZE = 20 };
		for (size_t i = 0; i < getSize(); ++i)
		{
			unsigned char c = *getPtr(i);
			// ASCII
			if (c < 0x80)
			{
				if (j >= buf.getLength())
					buf.Grow(GROWSIZE);
				*buf.getMPtr(j++) = c;
				continue;
			}
			// Is c one of the control characters only in ISO/IEC_8859-1 or part of the common subset with windows-1252?
			if (c == 0x81 || c == 0x8D || c == 0x8F || c == 0x90 || c == 0x9D || c >= 0xA0)
			{
				if (j + 1 >= buf.getLength())
					buf.Grow(GROWSIZE);
				*buf.getMPtr(j++) = (0xC0 | (c >> 6));
				*buf.getMPtr(j++) = (0x80 | (c & 0x3F));
				continue;
			}
			// Extra windows-1252-characters
			buf.SetLength(j);
			static const char * extra_chars [] =
			{
				//"€",   0, "‚", "ƒ", "„", "…", "†", "‡", "ˆ", "‰", "Š", "‹", "Œ",   0, "Ž",   0,
				//  0, "‘", "’", "“", "”", "•", "–", "—", "˜", "™", "š", "›", "œ",   0, "ž", "Ÿ" };
				"\xe2\x82\xac", 0, "\xe2\x80\x9a", "\xc6\x92", "\xe2\x80\x9e", "\xe2\x80\xa6", "\xe2\x80\xa0", "\xe2\x80\xa1", "\xcb\x86", "\xe2\x80\xb0", "\xc5\xa0", "\xe2\x80\xb9", "\xc5\x92", 0, "\xc5\xbd", 0,
				0, "\xe2\x80\x98", "\xe2\x80\x99", "\xe2\x80\x9c", "\xe2\x80\x9d", "\xe2\x80\xa2", "\xe2\x80\x93", "\xe2\x80\x94", "\xcb\x9c", "\xe2\x84\xa2", "\xc5\xa1", "\xe2\x80\xba", "\xc5\x93",   0, "\xc5\xbe", "\xc5\xb8"
			};
			buf.Append(extra_chars[c - 0x80]);
			j += strlen(extra_chars[c - 0x80]);
		}
		buf.SetLength(j);
		Take(std::move(buf));
		return true;
	}
	return false;
}

bool StdStrBuf::TrimSpaces()
{
	// get left trim
	int32_t iSpaceLeftCount = 0, iLength = getLength();
	if (!iLength) return false;
	const char *szStr = getData();
	while (iSpaceLeftCount < iLength)
		if (isspace((unsigned char)(unsigned char) szStr[iSpaceLeftCount]))
			++iSpaceLeftCount;
		else
			break;
	// only spaces? Clear!
	if (iSpaceLeftCount == iLength)
	{
		Clear();
		return true;
	}
	// get right trim
	int32_t iSpaceRightCount = 0;
	while (isspace((unsigned char)szStr[iLength - 1 - iSpaceRightCount])) ++iSpaceRightCount;
	// anything to trim?
	if (!iSpaceLeftCount && !iSpaceRightCount) return false;
	// only right trim? Can do this by shortening
	if (!iSpaceLeftCount)
	{
		SetLength(iLength - iSpaceRightCount);
		return true;
	}
	// left trim involved - move text and shorten
	memmove(getMPtr(0), szStr+iSpaceLeftCount, iLength - iSpaceLeftCount - iSpaceRightCount);
	SetLength(iLength - iSpaceLeftCount - iSpaceRightCount);
	return true;
}

#ifdef _WIN32
std::string WStrToString(wchar_t *ws)
{
	int len = WideCharToMultiByte(CP_UTF8, 0, ws, -1, nullptr, 0, 0, 0);
	assert(len >= 0);
	if (len <= 0) return std::string{};

	std::string s(static_cast<size_t>(len), '\0');
	s.resize(WideCharToMultiByte(CP_UTF8, 0, ws, -1, &s[0], s.size(), 0, 0) - 1);
	return s;
}
#endif
