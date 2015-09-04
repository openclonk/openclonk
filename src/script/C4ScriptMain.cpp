/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2012-2015, The OpenClonk Team and contributors
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

#include <stdio.h>
#include <string.h>

int usage(const char *argv0)
{
	fprintf(stderr, "Usage:\n%s -e <script>\n%s <file>\n", argv0, argv0);
	return 1;
}

int main(int argc, const char * argv[])
{
	if (argc < 2)
		return usage(argv[0]);

	if (strcmp(argv[1], "-e") == 0)
	{
		if (argc != 3)
			return usage(argv[0]);
		return c4s_runstring(argv[2]);
	}
	else
	{
		if (argc != 2)
			return usage(argv[0]);
		return c4s_runfile(argv[1]);
	}
}
