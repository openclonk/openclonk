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

#ifndef INC_STANDARD
#define INC_STANDARD

#include <type_traits>

// The Clear/Default functions that exist on most OpenClonk classes are A
// BAD IDEA because the caller has no guarantee that every member has been
// properly cleared/initialized. The proper way to handle this, which is
// using ctors/dtors, is hard to integrate into the current design. To help
// with this, InplaceReconstruct will destroy and default-construct an object
// in the same memory. The nothrow test attempts to ensure that the object
// will not end up in a destroyed state when the ctor fails (by checking that
// the ctor cannot fail beforehand).
template<class T>
typename std::enable_if<std::is_nothrow_default_constructible<T>::value>::type
inline InplaceReconstruct(T *obj)
{
	obj->~T();
	new (obj) T();
}

// Small helpers
template <class T> inline T Abs(T val) { return val > 0 ? val : -val; }
template <class T, class U, class V> inline bool Inside(T ival, U lbound, V rbound) { return ival >= lbound && ival <= rbound; }
template <class T> inline T Clamp(T bval, T lbound, T rbound) { return bval < lbound ? lbound : bval > rbound ? rbound : bval; }
template <class T> inline int Sign(T val) { return val < 0 ? -1 : val > 0 ? 1 : 0; }

inline int DWordAligned(int val)
{
	if (val%4) { val>>=2; val<<=2; val+=4; }
	return val;
}

int32_t Distance(int32_t iX1, int32_t iY1, int32_t iX2, int32_t iY2);
int32_t Angle(int32_t iX1, int32_t iY1, int32_t iX2, int32_t iY2, int32_t iPrec = 1);
int Pow(int base, int exponent);
int32_t StrToI32(const char *s, int base, const char **scan_end);

template <class T>
typename std::enable_if<std::is_pod<T>::value>::type
inline ZeroMem(T *lpMem, size_t dwSize)
{
	std::memset(lpMem,'\0',dwSize);
}

inline void MemCopy(const void *lpMem1, void *lpMem2, size_t dwSize)
{
	std::memmove(lpMem2,lpMem1,dwSize);
}

inline char CharCapital(char cChar) { return std::toupper(cChar); }
bool IsIdentifier(char cChar);
inline bool IsWhiteSpace(char cChar) { return !!std::isspace((unsigned char)cChar); }

inline size_t SLen(const char *sptr) { return sptr?std::strlen(sptr):0; }
inline size_t SLenUntil(const char *szStr, char cUntil)
{
	if (!szStr) return 0;
	const char *end = std::strchr(szStr, cUntil);
	return end ? end-szStr : std::strlen(szStr);
}

// get a character at the current string pos and advance pos by that character
uint32_t GetNextUTF8Character(const char **pszString); // GetNextCharacter helper
inline uint32_t GetNextCharacter(const char **pszString)
{
	unsigned char c=**pszString;
	if (c<128) { ++*pszString; return c; }
	else return GetNextUTF8Character(pszString);
}
// Get string length in characters (not bytes)
int GetCharacterCount(const char * s);

inline bool SEqual(const char *szStr1, const char *szStr2) { return szStr1&&szStr2?!std::strcmp(szStr1,szStr2):false; }
bool SEqual2(const char *szStr1, const char *szStr2);
bool SEqualUntil(const char *szStr1, const char *szStr2, char cWild);
bool SEqualNoCase(const char *szStr1, const char *szStr2, int iLen=-1);
bool SEqual2NoCase(const char *szStr1, const char *szStr2, int iLen = -1);

void SCopy(const char *szSource, char *sTarget);
void SCopy(const char *szSource, char *sTarget, size_t iMaxL);
void SCopyUntil(const char *szSource, char *sTarget, char cUntil, int iMaxL=-1, int iIndex=0);
void SCopyUntil(const char *szSource, char *sTarget, const char * sUntil, size_t iMaxL);
void SCopyIdentifier(const char *szSource, char *sTarget, int iMaxL=0);
bool SCopySegment(const char *fstr, int segn, char *tstr, char sepa=';', int iMaxL=-1, bool fSkipWhitespace=false);
bool SCopySegmentEx(const char *fstr, int segn, char *tstr, char sepa1, char sepa2, int iMaxL=-1, bool fSkipWhitespace=false);
bool SCopyEnclosed(const char *szSource, char cOpen, char cClose, char *sTarget, int iSize);

void SAppend(const char *szSource, char *szTarget, int iMaxL=-1);
void SAppendChar(char cChar, char *szStr);

void SInsert(char *szString, const char *szInsert, int iPosition=0, int iMaxLen=-1);
void SDelete(char *szString, int iLen, int iPosition=0);

int  SCharPos(char cTarget, const char *szInStr, int iIndex=0);
int  SCharLastPos(char cTarget, const char *szInStr);
unsigned int  SCharCount(char cTarget, const char *szInStr, const char *cpUntil=nullptr);
unsigned int  SCharCountEx(const char *szString, const char *szCharList);

void SReplaceChar(char *str, char fc, char tc);

const char *SSearch(const char *szString, const char *szIndex);
const char *SSearchNoCase(const char *szString, const char *szIndex);

const char *SAdvanceSpace(const char *szSPos);
const char *SAdvancePast(const char *szSPos, char cPast);

bool SGetModule(const char *szList, int iIndex, char *sTarget, int iSize=-1);
bool SIsModule(const char *szList, const char *szString, int *ipIndex=nullptr, bool fCaseSensitive=false);
bool SAddModule(char *szList, const char *szModule, bool fCaseSensitive=false);
bool SAddModules(char *szList, const char *szModules, bool fCaseSensitive=false);
bool SRemoveModule(char *szList, const char *szModule, bool fCaseSensitive=false);
bool SRemoveModules(char *szList, const char *szModules, bool fCaseSensitive=false);
int SModuleCount(const char *szList);

void SNewSegment(char *szStr, const char *szSepa=";");
void SCapitalize(char *szString);
void SWordWrap(char *szText, char cSpace, char cSepa, int iMaxLine);
int  SClearFrontBack(char *szString, char cClear=' ');

int SGetLine(const char *szText, const char *cpPosition);
int SLineGetCharacters(const char *szText, const char *cpPosition);

// case sensitive wildcard match with some extra functionality
// can match strings like  "*Cl?nk*vour" to "Clonk Endeavour"
bool SWildcardMatchEx(const char *szString, const char *szWildcard);

#define LineFeed "\x00D\x00A"

// sprintf wrapper

#ifdef _WIN32
#define vsnprintf _vsprintf_p
#endif

// old, insecure sprintf
inline int osprintf(char *str, const char *fmt, ...) GNUC_FORMAT_ATTRIBUTE_O;
inline int osprintf(char *str, const char *fmt, ...)
{
	va_list args; va_start(args, fmt);
	return vsprintf(str, fmt, args);
}

// secure sprintf
#define sprintf ssprintf
template <size_t N> inline int ssprintf(char(&str)[N], const char *fmt, ...) GNUC_FORMAT_ATTRIBUTE_O;
template <size_t N>
inline int ssprintf(char(&str)[N], const char *fmt, ...)
{
	va_list args; va_start(args, fmt);
	int m = vsnprintf(str, N, fmt, args);
	// Quick exit if vsnprintf failed
	if (m < 0) return m;
	if (static_cast<size_t>(m) >= N) { m = N-1; str[m] = 0; }
	return m;
}

// Checks a string for conformance with UTF-8
bool IsValidUtf8(const char *string, int length = -1);


std::string vstrprintf(const char *format, va_list args);
std::string strprintf(const char *format, ...) GNUC_FORMAT_ATTRIBUTE;

#endif // INC_STANDARD
