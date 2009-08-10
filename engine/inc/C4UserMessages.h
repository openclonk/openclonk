/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2001  Sven Eberhardt
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

/* Custom windows messages */

#ifndef INC_C4UserMessaged
#define INC_C4UserMessages


#define WM_USER_UPDATEITEM			(WM_USER+1)
#define WM_USER_SETSTATUS				(WM_USER+2)
#define WM_USER_GAMEPAD					(WM_USER+3)
#define WM_USER_RELOAD					(WM_USER+4)
#define WM_USER_INIT						(WM_USER+5)
#define WM_USER_ADDHOST					(WM_USER+6)
#define WM_USER_RESETNET				(WM_USER+7)
#define WM_USER_RELOADFILE			(WM_USER+8)
#define WM_USER_RESETFONT				(WM_USER+9)
#define WM_USER_DROPDEF					(WM_USER+10)
#define WM_USER_LOG							(WM_USER+11)

#define WM_USER_RELOADITEM			(WM_USER+19)
#define WM_USER_REFRESHITEM			(WM_USER+20)
#define WM_USER_INSERTITEM			(WM_USER+21)
#define WM_USER_REMOVEITEM			(WM_USER+22)
#define WM_USER_SHOWQUICKSTART	(WM_USER+23)
#define WM_USER_DLSTART					(WM_USER+24)
#define WM_USER_DLEND						(WM_USER+25)
#define WM_USER_PAINTCAPTION	  (WM_USER+26)
#define WM_USER_REMOVEREFERENCE	(WM_USER+27)
#define WM_USER_ADDREFERENCE	  (WM_USER+28)

#endif
