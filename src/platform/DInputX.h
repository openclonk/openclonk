/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
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

/* A mouse handling wrapper to DirectInput */

extern int MouseX,MouseY,MouseB0,MouseB1,MouseB2;
extern long MouseStatus;

void SetMouseRange(int x1, int y1, int x2, int y2);
void CenterMouse();
bool InitDirectInput(HINSTANCE g_hinst, HWND hwnd, int resx, int resy);
void DeInitDirectInput();
void DirectInputSyncAcquire(bool fActive);
void DirectInputCritical();



