/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2001  Sven Eberhardt
 * Copyright (c) 2007  Günther Brammer
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

/* Another C4Group bitmap-to-surface loader and saver */

#ifndef INC_C4SurfaceFile
#define INC_C4SurfaceFile

C4Surface *GroupReadSurface(CStdStream &hGroup, BYTE *bpPalette=NULL);
CSurface8 *GroupReadSurface8(CStdStream &hGroup);
C4Surface *GroupReadSurfaceOwnPal(CStdStream &hGroup);
CSurface8 *GroupReadSurfaceOwnPal8(CStdStream &hGroup);

/*bool SaveSurface(const char *szFilename, SURFACE sfcSurface, BYTE *bpPalette);*/

#endif
