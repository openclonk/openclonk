/*
 * Copyright (c) 2007, 2010  Günther Brammer
 * Copyright (c) 2010  Benjamin Herr
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
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
