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

// These are filled in automatically by CMake. When in doubt, edit 
// Version.txt instead!

#define C4CFG_Company         "OpenClonk Project"
#define C4COPYRIGHT_YEAR      "2010"
#define C4COPYRIGHT_COMPANY   "OpenClonk Project"

#define C4ENGINENAME          "OpenClonk Reborn"
#define C4ENGINENICK          "reborn"

#define C4ENGINECAPTION       "OpenClonk Reborn"
#define C4EDITORCAPTION       "Clonk Editor"
#define C4ENGINEID            "org.openclonk.reborn"

#define C4XVER1               4
#define C4XVER2               10
#define C4XVER3               0
#define C4XVER4               0
#define C4XVERBUILD           2
#define C4VERSIONBUILDNAME    "Cerulean"
#define C4VERSIONEXTRA        "Beta"
#define C4REVISION            "25a791c06fff+"

#define C4ENGINEINFO          "OpenClonk Reborn Beta"
#define C4ENGINEINFOLONG      "OpenClonk Reborn Beta (Cerulean)"

// Build Options
#ifdef _DEBUG
#define C4BUILDDEBUG " dbg"
#else
#define C4BUILDDEBUG
#endif
#define C4BUILDOPT C4BUILDDEBUG
#define C4VERSION             "4.10.0.0 [002] Beta mac" C4BUILDOPT

#endif
