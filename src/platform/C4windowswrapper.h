/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2010-2013, The OpenClonk Team and contributors
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

#ifndef INC_C4windowswrapper
#define INC_C4windowswrapper

#include <windows.h>
#undef RGB
#undef GetRValue
#undef GetGValue
#undef GetBValue
#undef TextOut
#undef GetObject
#undef CreateFont
#undef LoadBitmap
#undef DrawText

// implemented in StdBuf.cpp
StdStrBuf::wchar_t_holder GetWideChar(const char * utf8);
StdBuf GetWideCharBuf(const char * utf8);

#define ADDL2(s) L##s
#define ADDL(s) ADDL2(s)

#define GET_X_LPARAM(lp)                        ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp)                        ((int)(short)HIWORD(lp))

#endif // INC_C4windowswrapper
