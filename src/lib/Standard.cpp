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

/* All kinds of valuable helpers */

#include "C4Include.h"
#include "lib/Standard.h"

//------------------------------------- Basics ----------------------------------------

int32_t Distance(int32_t iX1, int32_t iY1, int32_t iX2, int32_t iY2)
{
	int64_t dx = int64_t(iX1)-iX2, dy = int64_t(iY1)-iY2;
	int64_t d2 = dx*dx+dy*dy;
	if (d2 < 0) return -1;
	int32_t dist = int32_t(sqrt(double(d2)));
	if (int64_t(dist)*dist < d2) ++dist;
	if (int64_t(dist)*dist > d2) --dist;
	return dist;
}

// Angle between points (iX1, iY1) and (iX2, iY2) with range [0, 360), angle = 0 means vertically upward and increasing angles in clockwise direction.
int32_t Angle(int32_t iX1, int32_t iY1, int32_t iX2, int32_t iY2, int32_t iPrec)
{
	int32_t iAngle;
	int32_t dx = iX2 - iX1, dy = iY2 - iY1;
	if (!dx)
	{
		if (dy > 0) return 180 * iPrec;
		else return 0;
	}
	if (!dy)
	{
		if (dx > 0) return 90 * iPrec;
		else return 270 * iPrec;
	}

	iAngle = static_cast<int32_t>(180.0 * iPrec * atan2(static_cast<double>(Abs(dy)), static_cast<double>(Abs(dx))) / M_PI);

	if (iX2 > iX1)
	{
		if (iY2 < iY1) iAngle = (90 * iPrec) - iAngle;
		else iAngle = (90 * iPrec) + iAngle;
	}
	else
	{
		if (iY2 < iY1) iAngle = (270 * iPrec) + iAngle;
		else iAngle = (270 * iPrec) - iAngle;
	}
	
	return iAngle;
}

/* Fast integer exponentiation */
int Pow(int base, int exponent)
{
	if (exponent < 0) return 0;

	int result = 1;

	if (exponent & 1) result = base;
	exponent >>= 1;

	while (exponent)
	{
		base *= base;
		if (exponent & 1) result *= base;
		exponent >>= 1;
	}

	return result;
}

//--------------------------------- Characters ------------------------------------------

bool IsIdentifier(char cChar)
{
	if (Inside(cChar,'A','Z')) return true;
	if (Inside(cChar,'a','z')) return true;
	if (Inside(cChar,'0','9')) return true;
	if (cChar=='_') return true;
	if (cChar=='~') return true;
	if (cChar=='+') return true;
	if (cChar=='-') return true;
	return false;
}

static bool IsNumber(char c, int base)
{
	return (c >= '0' && c <= '9' && c < ('0' + base)) ||
	       (c >= 'a' && c <= 'z' && c < ('a' + base - 10)) ||
	       (c >= 'A' && c <= 'Z' && c < ('A' + base - 10));
}

static int ToNumber(char c)
{
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'z') return 10 + c - 'a';
	if (c >= 'A' && c <= 'Z') return 10 + c - 'A';
	assert(false);
	return 0;
}

//------------------------------- Strings ------------------------------------------------

int32_t StrToI32(const char *str, int base, const char **scan_end)
{
	const char *s = str;
	int sign = 1;
	int32_t result = 0;
	if (*s == '-')
	{
		sign = -1;
		s++;
	}
	else if (*s == '+')
	{
		s++;
	}
	if (!*s)
	{
		// Abort if there are no digits to parse
		if (scan_end) *scan_end = str;
		return 0;
	}
	while (IsNumber(*s,base))
	{
		int value = ToNumber(*s++);
		assert (value < base && value >= 0);
		result *= base;
		result += value;
	}
	if (scan_end != nullptr) *scan_end = s;
	result *= sign;
	return result;
}

void SCopy(const char *szSource, char *sTarget, size_t iMaxL)
{
	if (szSource == sTarget) return;
	if (!sTarget) return; *sTarget=0; if (!szSource) return;
	while (*szSource && (iMaxL>0))
		{ *sTarget=*szSource; iMaxL--; szSource++; sTarget++; }
	*sTarget=0;
}

void SCopy(const char *szSource, char *sTarget)
{
	if (szSource == sTarget) return;
		if (!sTarget) return; *sTarget=0; if (!szSource) return;
		strcpy(sTarget,szSource);
}

void SCopyUntil(const char *szSource, char *sTarget, char cUntil, int iMaxL, int iIndex)
{
	if (szSource == sTarget) return;
	if (!sTarget) return; *sTarget=0; if (!szSource) return;
	while ( *szSource && ((*szSource!=cUntil) || (iIndex>0)) && (iMaxL!=0) )
		{ *sTarget=*szSource; if (*szSource==cUntil) iIndex--; szSource++; sTarget++; iMaxL--; }
	*sTarget=0;
}

void SCopyUntil(const char *szSource, char *sTarget, const char * sUntil, size_t iMaxL)
{
	size_t n = std::min(strcspn(szSource, sUntil), iMaxL - 1);
	strncpy(sTarget, szSource, n);
	sTarget[n] = 0;
}

bool SEqualUntil(const char *szStr1, const char *szStr2, char cWild)
{
	if (!szStr1 || !szStr2) return false;
	while (*szStr1 || *szStr2)
	{
		if ((*szStr1==cWild) || (*szStr2==cWild)) return true;
		if (*szStr1!=*szStr2) return false;
		szStr1++; szStr2++;
	}
	return true;
}

// Beginning of string 1 needs to match string 2.

bool SEqual2(const char *szStr1, const char *szStr2)
{
	if (!szStr1 || !szStr2) return false;
	while (*szStr1 && *szStr2)
		if (*szStr1++ != *szStr2++) return false;
	if (*szStr2) return false; // Str1 is shorter
	return true;
}

bool SEqualNoCase(const char *szStr1, const char *szStr2, int iLen)
{
	if (!szStr1 || !szStr2) return false;
	if (iLen==0) return true;
	while (*szStr1 && *szStr2)
	{
		if ( CharCapital(*szStr1++) != CharCapital(*szStr2++)) return false;
		if (iLen>0) { iLen--; if (iLen==0) return true; }
	}
	if (*szStr1 || *szStr2) return false;
	return true;
}

bool SEqual2NoCase(const char *szStr1, const char *szStr2, int iLen)
{
	if (!szStr1 || !szStr2) return false;
	if (iLen==0) return true;
	while (*szStr1 && *szStr2)
	{
		if ( CharCapital(*szStr1++) != CharCapital(*szStr2++)) return false;
		if (iLen>0) { iLen--; if (iLen==0) return true; }
	}
	if (*szStr2) return false; // Str1 is shorter
	return true;
}

int SCharPos(char cTarget, const char *szInStr, int iIndex)
{
	const char *cpos;
	int ccpos;
	if (!szInStr) return -1;
	for (cpos=szInStr,ccpos=0; *cpos; cpos++,ccpos++)
		if (*cpos==cTarget)
		{
			if (iIndex==0) return ccpos;
			else iIndex--;
		}
	return -1;
}

int SCharLastPos(char cTarget, const char *szInStr)
{
	const char *cpos;
	int ccpos,lcpos;
	if (!szInStr) return -1;
	for (cpos=szInStr,ccpos=0,lcpos=-1; *cpos; cpos++,ccpos++)
		if (*cpos==cTarget) lcpos=ccpos;
	return lcpos;
}

void SAppend(const char *szSource, char *szTarget, int iMaxL)
{
	if (iMaxL == -1)
		SCopy(szSource, szTarget + SLen(szTarget));
	else
		SCopy(szSource, szTarget + SLen(szTarget), iMaxL - SLen(szTarget));
}

void SAppendChar(char cChar, char *szStr)
{
	if (!szStr) return;
	char *cPos;
	for (cPos=szStr; *cPos; cPos++) {}
	*cPos=cChar; *(cPos+1)=0;
}

bool SCopySegment(const char *szString, int iSegment, char *sTarget,
                  char cSeparator, int iMaxL, bool fSkipWhitespace)
{
	// Advance to indexed segment
	while (iSegment>0)
	{
		if (SCharPos(cSeparator,szString) == -1)
			{ sTarget[0]=0; return false; }
		szString += SCharPos(cSeparator,szString)+1;
		iSegment--;
	}
	// Advance whitespace
	if (fSkipWhitespace)
		szString = SAdvanceSpace(szString);
	// Copy segment contents
	SCopyUntil(szString,sTarget,cSeparator,iMaxL);
	return true;
}

bool SCopySegmentEx(const char *szString, int iSegment, char *sTarget,
                    char cSep1, char cSep2, int iMaxL, bool fSkipWhitespace)
{
	// Advance to indexed segment
	while (iSegment>0)
	{
		// use the separator that's closer
		int iPos1 = SCharPos(cSep1,szString), iPos2 = SCharPos(cSep2,szString);
		if (iPos1 == -1)
			if (iPos2 == -1)
				{ sTarget[0]=0; return false; }
			else
				iPos1=iPos2;
		else if (iPos2 != -1 && iPos2 < iPos1)
			iPos1 = iPos2;
		szString += iPos1+1;
		iSegment--;
	}
	// Advance whitespace
	if (fSkipWhitespace)
		szString = SAdvanceSpace(szString);
	// Copy segment contents; use separator that's closer
	int iPos1 = SCharPos(cSep1,szString), iPos2 = SCharPos(cSep2,szString);
	if (iPos2 != -1 && (iPos2 < iPos1 || iPos1 == -1)) cSep1 = cSep2;
	SCopyUntil(szString,sTarget,cSep1,iMaxL);
	return true;
}

unsigned int SCharCount(char cTarget, const char *szInStr, const char *cpUntil)
{
	unsigned int iResult=0;
	// Scan string
	while (*szInStr)
	{
		// End position reached (end character is not included)
		if (szInStr==cpUntil) return iResult;
		// Character found
		if (*szInStr==cTarget) iResult++;
		// Advance
		szInStr++;
	}
	// Done
	return iResult;
}

unsigned int SCharCountEx(const char *szString, const char *szCharList)
{
	unsigned int iResult = 0;
	while ( *szCharList )
	{
		iResult += SCharCount( *szCharList, szString );
		szCharList++;
	}
	return iResult;
}

void SReplaceChar(char *str, char fc, char tc)
{
	while (str && *str)
		{ if (*str==fc) *str=tc; str++; }
}

void SCapitalize(char *str)
{
	while (str && *str)
	{
		*str=CharCapital(*str);
		str++;
	}
}

const char *SSearch(const char *szString, const char *szIndex)
{
	const char *cscr;
	size_t indexlen,match=0;
	if (!szString || !szIndex) return nullptr;
	indexlen=SLen(szIndex);
	for (cscr=szString; cscr && *cscr; cscr++)
	{
		if (*cscr==szIndex[match]) match++;
		else match=0;
		if (match>=indexlen) return cscr+1;
	}
	return nullptr;
}

const char *SSearchNoCase(const char *szString, const char *szIndex)
{
	const char *cscr;
	size_t indexlen,match=0;
	if (!szString || !szIndex) return nullptr;
	indexlen=SLen(szIndex);
	for (cscr=szString; cscr && *cscr; cscr++)
	{
		if (CharCapital(*cscr)==CharCapital(szIndex[match])) match++;
		else match=0;
		if (match>=indexlen) return cscr+1;
	}
	return nullptr;
}

void SWordWrap(char *szText, char cSpace, char cSepa, int iMaxLine)
{
	if (!szText) return;
	// Scan string
	char *cPos,*cpLastSpace=nullptr;
	int iLineRun=0;
	for (cPos=szText; *cPos; cPos++)
	{
		// Store last space
		if (*cPos==cSpace) cpLastSpace=cPos;
		// Separator encountered: reset line run
		if (*cPos==cSepa) iLineRun=0;
		// Need a break, insert at last space
		if (iLineRun>=iMaxLine)
			if (cpLastSpace)
				{ *cpLastSpace=cSepa; iLineRun=cPos - cpLastSpace; }
		// Line run
		iLineRun++;
	}
}

const char *SAdvanceSpace(const char *szSPos)
{
	if (!szSPos) return nullptr;
	while (IsWhiteSpace(*szSPos)) szSPos++;
	return szSPos;
}

const char *SRewindSpace(const char *szSPos, const char *pBegin)
{
	if (!szSPos || !pBegin) return nullptr;
	while (IsWhiteSpace(*szSPos))
	{
		szSPos--;
		if (szSPos<pBegin) return nullptr;
	}
	return szSPos;
}

const char *SAdvancePast(const char *szSPos, char cPast)
{
	if (!szSPos) return nullptr;
	while (*szSPos)
	{
		if (*szSPos==cPast) { szSPos++; break; }
		szSPos++;
	}
	return szSPos;
}

void SCopyIdentifier(const char *szSource, char *sTarget, int iMaxL)
{
	if (!szSource || !sTarget) return;
	while (IsIdentifier(*szSource))
	{
		if (iMaxL==1) { *sTarget++ = *szSource++; break; }
		iMaxL--;
		*sTarget++ = *szSource++;
	}
	*sTarget=0;
}

int SClearFrontBack(char *szString, char cClear)
{
	int cleared=0;
	char *cpos;
	if (!szString) return 0;
	for (cpos=szString; *cpos && (*cpos==cClear); cpos++,cleared++) {}
	// strcpy is undefined when used on overlapping strings...
	if (cpos!=szString) memmove(szString, cpos, SLen(cpos) + 1);
	for (cpos=szString+SLen(szString)-1; (cpos>szString) && (*cpos==cClear); cpos--,cleared++)
		*cpos=0x00;
	return cleared;
}

void SNewSegment(char *szStr, const char *szSepa)
{
	if (szStr[0]) SAppend(szSepa,szStr);
}

int SGetLine(const char *szText, const char *cpPosition)
{
	if (!szText || !cpPosition) return 0;
	int iLines = 1;
	while (*szText && (szText<cpPosition))
	{
		if (*szText == 0x0A) iLines++;
		szText++;
	}
	return iLines;
}

int SLineGetCharacters(const char *szText, const char *cpPosition)
{
	if (!szText || !cpPosition) return 0;
	int iChars = 1;
	while (*szText && (szText<cpPosition))
	{
		if (*szText == 0x0A)
			iChars = 1;
		else if (*szText == '\t')
			// assume a tab stop every 8 characters
			iChars = ((iChars - 1 + 8) & ~7) + 1;
		else
			iChars++;
		szText++;
	}
	return iChars;
}

void SInsert(char *szString, const char *szInsert, int iPosition, int iMaxLen)
{
	// Safety
	if (!szString || !szInsert || !szInsert[0]) return;
	size_t insertlen = strlen(szInsert);
	if (iMaxLen >= 0 && strlen(szString) + insertlen > (size_t) iMaxLen) return;
	// Move up string remainder
	memmove (szString + iPosition + insertlen, szString + iPosition, SLen(szString+  iPosition) + 1);
	// Copy insertion
	MemCopy( szInsert, szString+iPosition, SLen(szInsert) );
}

void SDelete(char *szString, int iLen, int iPosition)
{
	// Safety
	if (!szString) return;
	// Move down string remainder
	MemCopy( szString+iPosition+iLen, szString+iPosition, SLen(szString+iPosition+iLen)+1 );
}

bool SCopyEnclosed(const char *szSource, char cOpen, char cClose, char *sTarget, int iSize)
{
	int iPos,iLen;
	if (!szSource || !sTarget) return false;
	if ((iPos = SCharPos(cOpen,szSource)) < 0) return false;
	if ((iLen = SCharPos(cClose,szSource+iPos+1)) < 0) return false;
	SCopy(szSource+iPos+1,sTarget,std::min(iLen,iSize));
	return true;
}

bool SGetModule(const char *szList, int iIndex, char *sTarget, int iSize)
{
	if (!szList || !sTarget) return false;
	if (!SCopySegment(szList,iIndex,sTarget,';',iSize)) return false;
	SClearFrontBack(sTarget);
	return true;
}

bool SIsModule(const char *szList, const char *szString, int *ipIndex, bool fCaseSensitive)
{
	char szModule[1024+1];
	// Compare all modules
	for (int iMod=0; SGetModule(szList,iMod,szModule,1024); iMod++)
		if (fCaseSensitive ? SEqual(szString,szModule) : SEqualNoCase(szString,szModule))
		{
			// Provide index if desired
			if (ipIndex) *ipIndex = iMod;
			// Found
			return true;
		}
	// Not found
	return false;
}

bool SAddModule(char *szList, const char *szModule, bool fCaseSensitive)
{
	// Safety / no empties
	if (!szList || !szModule || !szModule[0]) return false;
	// Already a module?
	if (SIsModule(szList,szModule,nullptr,fCaseSensitive)) return false;
	// New segment, add string
	SNewSegment(szList);
	SAppend(szModule,szList);
	// Success
	return true;
}

bool SAddModules(char *szList, const char *szModules, bool fCaseSensitive)
{
	// Safety / no empties
	if (!szList || !szModules || !szModules[0]) return false;
	// Add modules
	char szModule[1024+1]; // limited
	for (int cnt=0; SGetModule(szModules,cnt,szModule,1024); cnt++)
		SAddModule(szList,szModule,fCaseSensitive);
	// Success
	return true;
}

bool SRemoveModule(char *szList, const char *szModule, bool fCaseSensitive)
{
	int iMod,iPos,iLen;
	// Not a module
	if (!SIsModule(szList,szModule,&iMod,fCaseSensitive)) return false;
	// Get module start
	iPos = 0;
	if (iMod > 0) iPos = SCharPos(';',szList,iMod-1)+1;
	// Get module length
	iLen = SCharPos(';',szList+iPos);
	if (iLen<0) iLen=SLen(szList); else iLen+=1;
	// Delete module
	SDelete(szList,iLen,iPos);
	// Success
	return true;
}

bool SRemoveModules(char *szList, const char *szModules, bool fCaseSensitive)
{
	// Safety / no empties
	if (!szList || !szModules || !szModules[0]) return false;
	// Remove modules
	char szModule[1024+1]; // limited
	for (int cnt=0; SGetModule(szModules,cnt,szModule,1024); cnt++)
		SRemoveModule(szList,szModule,fCaseSensitive);
	// Success
	return true;
}

int SModuleCount(const char *szList)
{
	if (!szList) return 0;
	int iCount = 0;
	bool fNewModule = true;
	while (*szList)
	{
		switch (*szList)
		{
		case ' ': break;
		case ';': fNewModule=true; break;
		default: if (fNewModule) iCount++; fNewModule=false; break;
		}
		szList++;
	}
	return iCount;
}

bool SWildcardMatchEx(const char *szString, const char *szWildcard)
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
		else if (*pWild == '?' || *pWild == *pPos)
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

// UTF-8 conformance checking
namespace
{
	static const int utf8_continuation_byte_table[256] =
	{
		// How many continuation bytes must follow a byte with this value?
		// Negative values mean that this byte can never start a valid
		// UTF-8 sequence.
		// Note that while the encoding scheme allows more than three
		// trailing bytes in principle, it is not actually allowed for UTF-8.
		// Values 0xC0 and 0xC1 can never occur in UTF-8 because they
		// would mark the beginning of an overlong encoding of characters
		// below 0x80.
		// Values 0xF5 to 0xFD are invalid because they can only be used
		// to encode characters beyond the Unicode range.
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 0b00000000..0b00001111, 0x00..0x0F
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 0b00010000..0b00011111, 0x10..0x1F
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 0b00100000..0b00101111, 0x20..0x2F
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 0b00110000..0b00111111, 0x30..0x3F
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 0b01000000..0b01001111, 0x40..0x4F
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 0b01010000..0b01011111, 0x50..0x5F
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 0b01100000..0b01101111, 0x60..0x6F
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 0b01110000..0b01111111, 0x70..0x7F
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 0b10000000..0b10001111, 0x80..0x8F
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 0b10010000..0b10011111, 0x90..0x9F
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 0b10100000..0b10101111, 0xA0..0xAF
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 0b10110000..0b10111111, 0xB0..0xBF
		-1, -1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 0b11000000..0b11001111, 0xC0..0xCF
		 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  // 0b11010000..0b11011111, 0xD0..0xDF
		 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  // 0b11100000..0b11101111, 0xE0..0xEF
		 3,  3,  3,  3,  3, -3, -3, -3, -4, -4, -4, -4, -5, -5, -1, -1   // 0b11110000..0b11111111, 0xF0..0xFF
	};
	static const uint32_t utf8_min_char_value[4] =
	{
		// Which is the lowest character value that may be encoded
		// using this many continuation bytes?
		0, 0x80, 0x800, 0x10000
	};
}

bool IsValidUtf8(const char *text, int length)
{
	// Intentionally using a C-style cast to always get a uint8_t* from char*;
	// reinterpret_cast would fail here on platforms that have unsigned char,
	// while static_cast would fail on platforms with a signed char type
	const uint8_t *input = (const uint8_t*)(text);

	for (const uint8_t *cursor = input; length < 0 ? *cursor != 0 : cursor - input < length; ++cursor)
	{
		int continuation_bytes = utf8_continuation_byte_table[*cursor];
		if (continuation_bytes < 0)
			return false;
		else if (continuation_bytes == 0)
		{
			// Standard 7-bit ASCII value (i.e., 1 byte codepoint)
			continue;
		}
		else if (length >= 0 && cursor - input + continuation_bytes >= length)
		{
			// Too few remaining bytes
			return false;
		}
		
		// Compute character value, so we can detect overlong sequences
		assert((*cursor & 0xC0) == 0xC0);
		uint32_t value = *cursor;
		// strip length bits off the start byte
		value &= (0xFF >> (continuation_bytes + 1));
		for (int byte = 0; byte < continuation_bytes; ++byte)
		{
			// check that this is actually a continuation byte
			if ((cursor[byte + 1] & 0xC0) != 0x80)
				return false;
			// merge continuation byte into value
			value <<= 6;
			value |= cursor[byte + 1] & 0x3F;
		}
		// make sure this is not overlong
		if (value < utf8_min_char_value[continuation_bytes])
			return false;
		// and also not beyond 0x10FFFF
		if (value > 0x10FFFF)
			return false;
		// and also not a wrongly encoded UTF-16 surrogate half
		if (value >= 0xD800 && value <= 0xDFFF)
			return false;
		cursor += continuation_bytes;
	}
	// Looks fine
	return true;
}

// UTF-8 iteration
uint32_t GetNextUTF8Character(const char **pszString)
{
	// assume the current character is UTF8 already (i.e., highest bit set)
	const uint32_t REPLACEMENT_CHARACTER = 0xFFFDu;
	const char *szString = *pszString;
	unsigned char c = *szString++;
	uint32_t dwResult = REPLACEMENT_CHARACTER;
	assert(c>127);
	if (c>191 && c<224)
	{
		unsigned char c2 = *szString++;
		if ((c2 & 192) != 128) { *pszString = szString; return REPLACEMENT_CHARACTER; }
		dwResult = (int(c&31)<<6) | (c2&63); // two char code
	}
	else if (c >= 224 && c <= 239)
	{
		unsigned char c2 = *szString++;
		if ((c2 & 192) != 128) { *pszString = szString; return REPLACEMENT_CHARACTER; }
		unsigned char c3 = *szString++;
		if ((c3 & 192) != 128) { *pszString = szString; return REPLACEMENT_CHARACTER; }
		dwResult = (int(c&15)<<12) | (int(c2&63)<<6) | int(c3&63); // three char code
	}
	else if (c >= 240 && c <= 247)
	{
		unsigned char c2 = *szString++;
		if ((c2 & 192) != 128) { *pszString = szString; return REPLACEMENT_CHARACTER; }
		unsigned char c3 = *szString++;
		if ((c3 & 192) != 128) { *pszString = szString; return REPLACEMENT_CHARACTER; }
		unsigned char c4 = *szString++;
		if ((c4 & 192) != 128) { *pszString = szString; return REPLACEMENT_CHARACTER; }
		dwResult = (int(c&7)<<18) | (int(c2&63)<<12) | (int(c3&63)<<6) | int(c4&63); // four char code
	}
	*pszString = szString;
	return dwResult;
}

int GetCharacterCount(const char * s)
{
	int l = 0;
	while (*s)
	{
		unsigned char c = *s;
		if (c < 128 || c > 247)
		{
			++l;
			s += 1;
		}
		else if (c > 191 && c < 224)
		{
			++l;
			s += 2;
		}
		else if (c >= 224 && c <= 239)
		{
			++l;
			s += 3;
		}
		else if (c >= 240 && c <= 247)
		{
			++l;
			s += 4;
		}
		else assert(false);
	}
	return l;
}

std::string vstrprintf(const char *format, va_list args)
{
	va_list argcopy;
	va_copy(argcopy, args);
	int size = vsnprintf(nullptr, 0, format, argcopy);
	if (size < 0)
		throw std::invalid_argument("invalid argument to strprintf");
	va_end(argcopy);
	std::string s;
	s.resize(size + 1);
	size = vsnprintf(&s[0], s.size(), format, args);
	assert(size >= 0);
	s.resize(size);
	return s;
}

std::string strprintf(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	std::string s = vstrprintf(format, args);
	va_end(args);
	return s;
}
