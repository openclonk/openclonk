/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2003-2004  Peter Wortmann
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
/* Base64 encoding / decoding */

#include "C4Include.h"
#include "StdBase64.h"

// Peter's B64Encode implementation would yield incorrect results in Release build
// so I replaced it with some other implementation which does the job...

// base 64 table
const char Base64Tbl [] =
  { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q',
    'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
    'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y',
    'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
  };

// reversed base 64 table
const unsigned char Base64RTbl[] =
{
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0x3e, 0xff, 0xff, 0xff, 0x3f,
	0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b,
	0x3c, 0x3d, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
	0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
	0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
	0x17, 0x18, 0x19, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
	0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
	0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30,
	0x31, 0x32, 0x33, 0xff, 0xff, 0xff, 0xff, 0xff
};

// base 64 padding char
const char Base64Pad = '=';

char *B64Encode(const void *pData, int iSize)
{
	// ensure b64 table has correct size
	if (sizeof(Base64Tbl) != 64) return NULL;
	// calc output size
	int ioSize = (iSize / 3) * 4;
	if (iSize % 3) ioSize += 4; // end padding
	ioSize += (ioSize / 76) * 2; // line breaks
	ioSize++; // null char
	// alloc output mem
	char *poData = new char [ioSize];
	// start encoding
	const unsigned char *pPos = (const unsigned char *)pData; char *poPos = poData;
	for (int i = 0; i < iSize / 3; i++)
	{
		unsigned char c1 = *pPos++, c2 = *pPos++, c3 = *pPos++;
		// char 1
		*(poPos++) = Base64Tbl[                    (c1 >> 2)];
		// char 2
		*(poPos++) = Base64Tbl[((c1 & 0x3) << 4) | (c2 >> 4)];
		// char 3
		*(poPos++) = Base64Tbl[((c2 & 0xf) << 2) | (c3 >> 6)];
		// char 4
		*(poPos++) = Base64Tbl[  c3 & 0x3f];
		// line break (every 76 characters)
		if (!((i + 1) % 19)) { *(poPos++) = '\r'; *(poPos++) = '\n'; }
	}
	// encode last bytes
	switch (iSize % 3)
	{
	case 0: // nothign left
		break; // no padding
	case 1: // one byte left
	{
		unsigned char c = *pPos++;
		// char 1
		*(poPos++) = Base64Tbl[                   (c >> 2)];
		// char 2
		*(poPos++) = Base64Tbl[((c & 0x3) << 4)           ];
		// char 3
		*(poPos++) = Base64Pad;
		// char 4
		*(poPos++) = Base64Pad;
	}
	break;
	case 2: // two bytes left
	{
		unsigned char c1 = *pPos++, c2 = *pPos++;
		// char 1
		*(poPos++) = Base64Tbl[                    (c1 >> 2)];
		// char 2
		*(poPos++) = Base64Tbl[((c1 & 0x3) << 4) | (c2 >> 4)];
		// char 3
		*(poPos++) = Base64Tbl[((c2 & 0xf) << 2)            ];
		// char 4
		*(poPos++) = Base64Pad;
	}
	break;
	}
	// append zero
	*(poPos++) = '\0';
	// check size
	if (poPos - poData != ioSize) { delete [] poData; return NULL; }
	// return data
	return poData;
}

const char x = '\x01';

inline unsigned char B64DecodeChar(const unsigned char c)
{
	return( c == Base64Pad ? 0    :
	        c & 128        ? 0xff :
	        Base64RTbl[c] );
}

char *B64Decode(const char *pData, int &ioSize)
{
	// first: count the characters
	const char *pPos; int iCount = 0;
	for (pPos = pData; *pPos; pPos++)
		if (B64DecodeChar(*pPos) != 0xff)
			iCount++;
	// if(iCount % 4) return NULL;
	// alloc output buffer
	char *poData = new char [ioSize = (iCount / 4) * 3];
	// begin decode
	pPos = pData; char *poPos = poData;
	for (int i = 0; i < iCount / 4; i++)
	{
		// read & decode 4 bytes
		unsigned char c[4];
		for (; *pPos;) if ((c[0] = B64DecodeChar(*(pPos++))) != 0xff) break; if (!*pPos) c[0] = 0;
		for (; *pPos;) if ((c[1] = B64DecodeChar(*(pPos++))) != 0xff) break; if (!*pPos) c[1] = 0;
		for (; *pPos;) if ((c[2] = B64DecodeChar(*(pPos++))) != 0xff) break; if (!*pPos) c[2] = 0;
		for (; *pPos;) if ((c[3] = B64DecodeChar(*(pPos++))) != 0xff) break; if (!*pPos) c[3] = 0;
		// write 3 bytes
		*(poPos++) = (c[0] << 2) | (c[1] >> 4);
		*(poPos++) = (c[1] << 4) | (c[2] >> 2);
		*(poPos++) = (c[2] << 6) |  c[3]      ;
	}
	// truncate length
	if (*(--pPos) == Base64Pad) ioSize--;
	if (*(--pPos) == Base64Pad) ioSize--;
	// ok
	return poData;
}
