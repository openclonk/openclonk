#include <CStdFile.h>
#include <stdio.h>

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

bool EraseItemSafe(const char *szFilename)
{
	return false;
}

int main(int argc, char *argv[])
{
	char *pData; int iSize;
	CStdFile MyFile;
	if (argc < 2)
	{
		fprintf(stderr, "%s infile [outfile]", argv[0]);
		return 1;
	}
	MyFile.Load(argv[1], (BYTE **)&pData, &iSize, 0, true);
	if (argc < 3)
	{
		fwrite(pData, 1, iSize, stdout);
	}
	else
	{
		MyFile.Create(argv[2], false);
		MyFile.Write(pData, iSize);
	}
	MyFile.Close();
}
