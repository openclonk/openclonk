/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2012-2016, The OpenClonk Team and contributors
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
#include <getopt.h>

int usage(const char *argv0)
{
	fprintf(stderr, "Usage:\n%s -e <script>\n%s <file>\n", argv0, argv0);
	return 1;
}

int main(int argc, char *argv[])
{
	if (argc < 2)
		return usage(argv[0]);

	bool check = false;
	char *runstring = nullptr;

	while (1)
	{
		static option long_options[] =
		{
			{"check", no_argument, 0, 'c'},
			{"execute", required_argument, 0, 'e'},
			{0, 0, 0, 0}
		};

		int option_index;
		int c = getopt_long(argc, argv, "ce:", long_options, &option_index);
		if (c == -1) break;
		switch (c)
		{
		case 'c':
			check = true;
			break;
		case 'e':
			runstring = optarg;
			break;
		default:
			return usage(argv[0]);
		}
	}

	if (runstring)
	{
		if (argc - optind != 0)
			return usage(argv[0]);
		if (check)
			return c4s_checkstring(runstring);
		else
			return c4s_runstring(runstring);
	}
	else
	{
		if (argc - optind != 1)
			return usage(argv[0]);
		if (check)
			return c4s_checkfile(argv[optind]);
		else
			return c4s_runfile(argv[optind]);
	}
}
