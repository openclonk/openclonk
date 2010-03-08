/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2007  Matthes Bender
 * Copyright (c) 2002, 2004-2005, 2007  Sven Eberhardt
 * Copyright (c) 2005, 2007, 2009  Peter Wortmann
 * Copyright (c) 2005-2009  Günther Brammer
 * Copyright (c) 2009  Nicolas Hake
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

/* All the ifdefs in one place (Hah, I wish) */

#ifndef INC_PLATFORMABSTRACTION
#define INC_PLATFORMABSTRACTION

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H



#ifdef _MSC_VER
#pragma warning(disable : 4786) // long symbol names
#pragma warning(disable: 4706)
#pragma warning(disable: 4239)
#endif



#ifdef _MSC_VER
#define DEPRECATED(f) __declspec(deprecated) f
#elif defined(__GNUC__)
#define DEPRECATED(f) f __attribute__((deprecated))
#else
#define DEPRECATED(f) f
#endif



// debug memory management
#ifndef NODEBUGMEM
#if defined(_DEBUG) && defined(_MSC_VER)
#if _MSC_VER <= 1200
#include <new>
#include <memory>
#include <crtdbg.h>
#include <malloc.h>
inline void *operator new(unsigned int s, const char *szFile, long iLine)
	{ return ::operator new(s, _NORMAL_BLOCK, szFile, iLine); }
inline void operator delete(void *p, const char *, long)
	{ ::operator delete(p); }
#define new new(__FILE__, __LINE__)
#define malloc(size) ::_malloc_dbg(size, _NORMAL_BLOCK, __FILE__, __LINE__)
#else
#include <crtdbg.h>
#endif
#endif
#endif



// C++0x nullptr
#if defined(HAVE_NULLPTR) && !defined(NULL_IS_NULLPTR_T)
#undef NULL
#define NULL nullptr
#endif



// Integer dataypes
#ifdef HAVE_STDINT_H
#include <stdint.h>
#elif defined(_MSC_VER)
#include <cstddef>
typedef signed __int8 int8_t;
typedef signed __int16 int16_t;
typedef signed __int32 int32_t;
typedef signed __int64 int64_t;
typedef unsigned __int8 uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
// Copied from newer stddef.h
#ifndef _INTPTR_T_DEFINED
#ifdef  _WIN64
typedef __int64 intptr_t;
#else
typedef __int32 intptr_t;
#endif
#define _INTPTR_T_DEFINED
#endif
#else
#error Could not find integer datatypes!
#endif



#ifdef HAVE_UNISTD_H
#include <unistd.h>
#else
typedef ptrdiff_t ssize_t;
#endif



#if defined(__GNUC__)
// Allow checks for correct printf-usage
#define GNUC_FORMAT_ATTRIBUTE __attribute__ ((format (printf, 1, 2)))
#define GNUC_FORMAT_ATTRIBUTE_O __attribute__ ((format (printf, 2, 3)))
#define ALWAYS_INLINE inline __attribute__ ((always_inline))
#else
#define GNUC_FORMAT_ATTRIBUTE
#define GNUC_FORMAT_ATTRIBUTE_O
#define ALWAYS_INLINE __forceinline
#endif



// Temporary-To-Reference-Fix
#if defined(__GNUC__) && ((__GNUC__ < 4) || (__GNUC__ == 4 && __GNUC_MINOR__ < 3))
#define ALLOW_TEMP_TO_REF(ClassName) operator ClassName & () { return *this; }
#else
#define ALLOW_TEMP_TO_REF(ClassName)
#endif

#ifdef HAVE_RVALUE_REF
#	define RREF &&
#else
#	define RREF &
	namespace std { template<typename T> inline T &move (T &t) { return t; } }
#endif



#if defined(_DEBUG) && defined(_MSC_VER)
// use inline assembler to invoke the "breakpoint exception"
#  define BREAKPOINT_HERE __debugbreak()
#elif defined(_DEBUG) && defined(__GNUC__)
#  define BREAKPOINT_HERE asm volatile("int $3")
#elif defined(_DEBUG) && defined(HAVE_SIGNAL_H)
#  include <signal.h>
#  if defined(SIGTRAP)
#    define BREAKPOINT_HERE raise(SIGTRAP);
#  else
#    define BREAKPOINT_HERE
#  endif
#else
#  define BREAKPOINT_HERE
#endif



#ifdef _WIN32

#ifndef _INC_WINDOWS
#define _WIN32_WINDOWS 0x0500
#define _WIN32_WINNT  0x0501
#define WINVER 0x0500
//#define _WIN32_WINNT 0x0500
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <mmsystem.h>
#endif

#else // _WIN32

// Windows integer types
typedef uint32_t       DWORD;
typedef uint8_t        BYTE;
typedef uint16_t       WORD;

typedef struct {
    long left; long top; long right; long bottom;
} RECT;

unsigned long timeGetTime(void);

#include <strings.h>
inline int stricmp(const char *s1, const char *s2) {
	return strcasecmp(s1, s2);
}

#define GetRValue(rgb) ((unsigned char)(rgb))
#define GetGValue(rgb) ((unsigned char)(((unsigned short)(rgb)) >> 8))
#define GetBValue(rgb) ((unsigned char)((rgb)>>16))
#define RGB(r,g,b) ((DWORD)((BYTE)(r)|((BYTE)(g) << 8)|((BYTE)(b) << 16)))
#define ZeroMemory(d,l) memset((d), 0, (l))
#endif //_WIN32



#ifdef _WIN32
	#define C4_OS "win32"
#elif defined(__linux__)
	#define C4_OS "linux"
#elif defined(__APPLE__)
	#define C4_OS "mac"
#else
	#define C4_OS "unknown";
#endif



#ifdef _MSC_VER
#define vsnprintf _vsnprintf
#endif



// open a weblink in an external browser
bool OpenURL(const char *szURL);

bool EraseItemSafe(const char *szFilename);

#endif // INC_PLATFORMABSTRACTION
