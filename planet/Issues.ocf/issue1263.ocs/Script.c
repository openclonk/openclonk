/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2015, The OpenClonk Team and contributors
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

/* Issue #1263: Drawing Line = 1 objects without vertices results in array OOB */

func Initialize()
{
	// Drawing this object should not crash
	var line = CreateObjectAbove(LineDummy, 100, 100, NO_OWNER);
}
