/*
 * Copyright (c) 2007, Günther Brammer
 * Copyright (c) 2010-2013, The OpenClonk Team and contributors
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

#include <C4Include.h>
#include <CStdFile.h>
#include <stdio.h>

bool EraseItemSafe(const char *szFilename)
{
	return false;
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		fprintf(stderr, "%s infile [outfile]", argv[0]);
		return 1;
	}
	CStdFile MyFile;
	MyFile.Open(argv[1], true);
	char Data [4096];
	size_t iSize;
	if (argc < 3)
	{
		do
			{
			MyFile.Read(Data,sizeof(Data),&iSize);
			fwrite(Data, 1, iSize, stdout);
			}
		while(iSize);
	}
	else
	{
		CStdFile OutFile;
		OutFile.Create(argv[2], false);
		do
			{
			MyFile.Read(Data,sizeof(Data),&iSize);
			OutFile.Write(Data, iSize);
			}
		while(iSize);
		OutFile.Close();
	}
	MyFile.Close();
}
