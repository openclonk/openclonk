/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2012-2013, The OpenClonk Team and contributors
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

// Do not include C4Include.h - this file tests whether
// c4script.h is useable without that.
#include "../../include/c4script/c4script.h"

int main(int argc, const char * argv[])
{
	return c4s_runscript(argv[1]);
}
