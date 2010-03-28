/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2002  Peter Wortmann
 * Copyright (c) 2005, 2009  Günther Brammer
 * Copyright (c) 2009  Matthes Bender
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
#ifndef C4VERSION_H

#define C4VERSION_H

#define C4ENGINENAME          "OpenClonk"
#define C4ENGINENICK          "openclonk"

#define C4ENGINECAPTION       C4ENGINENAME
#define C4EDITORCAPTION       "Clonk Editor"

#define C4CFG_Company "Redwolf Design"
#define C4COPYRIGHT_YEAR   "2010" // might make this dynamic some time...
#define C4COPYRIGHT_COMPANY "OpenClonk Project"

#define C4VERSIONBUILDNAME "Cerulean"

#define C4XVER1 4
#define C4XVER2 10
#define C4XVER3 0
#define C4XVER4 0
#define C4XVERBUILD 2
#define C4VERSIONEXTRA "Beta"

// Prepend space to version extra
#ifdef C4VERSIONEXTRA
#define C4VERSIONEX " " C4VERSIONEXTRA
#else
#define C4VERSIONEX
#endif

// Build Options
#ifdef _DEBUG
#define C4BUILDDEBUG " dbg"
#else
#define C4BUILDDEBUG
#endif

#define C4BUILDOPT C4BUILDDEBUG

#define C4ENGINEINFO          C4ENGINENAME C4VERSIONEX
#ifdef C4VERSIONBUILDNAME
#define C4ENGINEINFOLONG      C4ENGINENAME C4VERSIONEX " (" C4VERSIONBUILDNAME ")"
#else
#define C4ENGINEINFOLONG      C4ENGINEINFO
#endif

#define C4XVERTOC4XVERS(s) C4XVERTOC4XVERS2(s)
#define C4XVERTOC4XVERS2(s) #s
#if C4XVERBUILD <= 9
#define C4VERSION            C4XVERTOC4XVERS(C4XVER1) "." C4XVERTOC4XVERS(C4XVER2)  "." C4XVERTOC4XVERS(C4XVER3) "." C4XVERTOC4XVERS(C4XVER4) " [00" C4XVERTOC4XVERS(C4XVERBUILD) "]" C4VERSIONEX C4BUILDOPT
#elif C4XVERBUILD <= 99
#define C4VERSION            C4XVERTOC4XVERS(C4XVER1) "." C4XVERTOC4XVERS(C4XVER2)  "." C4XVERTOC4XVERS(C4XVER3) "." C4XVERTOC4XVERS(C4XVER4) " [0" C4XVERTOC4XVERS(C4XVERBUILD) "]" C4VERSIONEX C4BUILDOPT
#else
#define C4VERSION            C4XVERTOC4XVERS(C4XVER1) "." C4XVERTOC4XVERS(C4XVER2)  "." C4XVERTOC4XVERS(C4XVER3) "." C4XVERTOC4XVERS(C4XVER4) " [" C4XVERTOC4XVERS(C4XVERBUILD) "]" C4VERSIONEX C4BUILDOPT
#endif

/* entries for engine.rc (VC++ will overwrite them)

#include "..\inc\C4Version.h"
[...]
 FILEVERSION C4XVER1,C4XVER2,C4XVER3,C4XVER4
 PRODUCTVERSION C4XVER1,C4XVER2,C4XVER3,C4XVER4
[...]
            VALUE "FileDescription", C4ENGINECAPTION "\0"
            VALUE "FileVersion", C4VERSION "\0"
            VALUE "SpecialBuild", C4BUILDOPT "\0"
            VALUE "ProductVersion", C4VERSION "\0"
*/

#endif
