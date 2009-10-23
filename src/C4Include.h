/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2007  Matthes Bender
 * Copyright (c) 2005  Tobias Zwick
 * Copyright (c) 2005, 2008  Sven Eberhardt
 * Copyright (c) 2005-2006  Günther Brammer
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

/* Main header to include all others */

#ifndef INC_C4Include
#define INC_C4Include

#ifdef _MSC_VER
#pragma warning(disable: 4786)
#pragma warning(disable: 4706)
#pragma warning(disable: 4239)
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif //HAVE_CONFIG_H

#ifdef _WIN32
	#define C4_OS "win32"
#elif defined(__linux__)
	#define C4_OS "linux"
#elif defined(__APPLE__)
	#define C4_OS "mac"
#else
	#define C4_OS "unknown";
#endif

#ifdef DEBUGREC
#define DEBUGREC_SCRIPT
#define DEBUGREC_START_FRAME 0
#define DEBUGREC_PXS
#define DEBUGREC_OBJCOM
#define DEBUGREC_MATSCAN
//#define DEBUGREC_RECRUITMENT
#define DEBUGREC_MENU
#define DEBUGREC_OCF
#endif

// solidmask debugging
//#define SOLIDMASK_DEBUG

// fmod
#if defined USE_FMOD && !defined HAVE_SDL_MIXER
#define C4SOUND_USE_FMOD
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <mmsystem.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <assert.h>

#endif // INC_C4Include
