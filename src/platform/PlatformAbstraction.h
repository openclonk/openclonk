/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2013, The OpenClonk Team and contributors
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

/* All the ifdefs in one place (Hah, I wish) */

#ifndef INC_PLATFORMABSTRACTION
#define INC_PLATFORMABSTRACTION

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

// We need to #define the target Windows version selector macros before we
// including any MinGW header.
#ifdef _WIN64
# define WINVER 0x0501
# define _WIN32_WINDOWS 0x0501
# define _WIN32_WINNT  0x0501
# define _WIN32_IE 0x0501
# define _AMD64_ 1
#elif defined(_WIN32)
# define WINVER 0x0500
# define _WIN32_WINDOWS 0x0500
# define _WIN32_WINNT  0x0501
# define _WIN32_IE 0x0501
# define _X86_ 1
#endif
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define UNICODE
#define _UNICODE
#ifndef NOMINMAX
# define NOMINMAX
#endif
#endif

#if defined(_WIN32) && !defined(USE_CONSOLE) && !defined(USE_SDL_MAINLOOP) && !defined(USE_X11) && !defined(USE_COCOA)
#define USE_WIN32_WINDOWS
#endif

#ifdef _MSC_VER
#define DEPRECATED __declspec(deprecated)
#elif defined(__GNUC__)
#define DEPRECATED __attribute__((deprecated))
#else
#define DEPRECATED
#endif

#ifdef _MSC_VER
#pragma warning(disable : 4786) // long symbol names
#pragma warning(disable: 4706)
#pragma warning(disable: 4239)
#pragma warning(disable: 4521) // multiple copy constructors specified
// Get non-standard <cmath> constants (M_PI etc.)
#	define _USE_MATH_DEFINES
#endif



// C++0x nullptr
#ifdef HAVE_NULLPTR
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



#ifndef HAVE_STATIC_ASSERT
#include <boost/static_assert.hpp>
#ifndef BOOST_HAS_STATIC_ASSERT
#define static_assert(x, y) BOOST_STATIC_ASSERT(x)
#endif
#endif



#if defined(__GNUC__)
// Allow checks for correct printf-usage
#define GNUC_FORMAT_ATTRIBUTE __attribute__ ((format (printf, 1, 2)))
#define GNUC_FORMAT_ATTRIBUTE_O __attribute__ ((format (printf, 2, 3)))
#define ALWAYS_INLINE inline __attribute__ ((always_inline))
#define NORETURN __attribute__ ((noreturn))
#else
#define GNUC_FORMAT_ATTRIBUTE
#define GNUC_FORMAT_ATTRIBUTE_O
#define ALWAYS_INLINE __forceinline
#define NORETURN
#endif



// Temporary-To-Reference-Fix
#if !defined(__clang__) && defined(__GNUC__) && ((__GNUC__ < 4) || (__GNUC__ == 4 && __GNUC_MINOR__ < 3))
#define ALLOW_TEMP_TO_REF(ClassName) operator ClassName & () { return *this; }
#else
#define ALLOW_TEMP_TO_REF(ClassName)
#endif

#ifdef HAVE_RVALUE_REF
# define RREF &&
#else
# define RREF &
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

typedef unsigned long DWORD;
typedef unsigned char  BYTE;
typedef unsigned short WORD;

#else

// Windows integer types
typedef uint32_t       DWORD;
typedef uint8_t        BYTE;
typedef uint16_t       WORD;

#include <strings.h>
inline int stricmp(const char *s1, const char *s2)
{
	return strcasecmp(s1, s2);
}

#endif //_WIN32



#ifdef _WIN64
#define C4_OS "win-x86_64"
#elif defined(_WIN32)
#define C4_OS "win-x86"
#elif defined(__linux__)
#if defined(__x86_64__)
#define C4_OS "linux-x86_64"
#else
#define C4_OS "linux-x86"
#endif
#elif defined(__APPLE__)
#define C4_OS "mac-x86"
#else
#define C4_OS ""
#endif

// delete item to the recycle bin
bool EraseItemSafe(const char *szFilename);

// Check whether the OS is "German"
bool IsGermanSystem();

// open a weblink in an external browser
bool OpenURL(const char* szURL);

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

#include <fcntl.h>
#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif

#ifdef _WIN32
#define DirSep "\\"
#define DirectorySeparator '\\'
#define AltDirectorySeparator '/'
#else
#define DirSep "/"
#define DirectorySeparator '/'
#define AltDirectorySeparator '\\'
#endif

#endif // INC_PLATFORMABSTRACTION
