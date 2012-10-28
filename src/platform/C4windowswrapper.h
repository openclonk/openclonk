/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2010  Armin Burgmeier
 * Copyright (c) 2011  Günther Brammer
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
