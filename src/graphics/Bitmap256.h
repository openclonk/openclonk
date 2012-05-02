/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2002  Sven Eberhardt
 * Copyright (c) 2005, 2008  Günther Brammer
 * Copyright (c) 2010  Benjamin Herr
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

/* A structure for handling 256-color bitmap files */

#ifndef BITMAP256_H_INC
#define BITMAP256_H_INC

#ifdef _WIN32
#include <C4windowswrapper.h>
#else
#pragma pack(push,2)
typedef struct tagBITMAPFILEHEADER
{
	WORD  bfType;
	DWORD bfSize;
	WORD  bfReserved1;
	WORD  bfReserved2;
	DWORD bfOffBits;
} BITMAPFILEHEADER,*LPBITMAPFILEHEADER,*PBITMAPFILEHEADER;
#pragma pack(pop)
typedef struct tagBITMAPINFOHEADER
{
	DWORD biSize;
	int32_t biWidth;
	int32_t biHeight;
	WORD  biPlanes;
	WORD  biBitCount;
	DWORD biCompression;
	DWORD biSizeImage;
	int32_t biXPelsPerMeter;
	int32_t biYPelsPerMeter;
	DWORD biClrUsed;
	DWORD biClrImportant;
} BITMAPINFOHEADER,*LPBITMAPINFOHEADER,*PBITMAPINFOHEADER;
typedef struct tagRGBQUAD
{
	BYTE  rgbBlue;
	BYTE  rgbGreen;
	BYTE  rgbRed;
	BYTE  rgbReserved;
} RGBQUAD,*LPRGBQUAD;
#endif

#pragma pack( push, def_pack , 1)

class C4BMPInfo
{
public:
	C4BMPInfo();
	void Default();
public:
	BITMAPFILEHEADER Head;
	BITMAPINFOHEADER Info;
	void Set(int iWdt, int iHgt, int iBitDepth);

	int FileBitsOffset();
};

class C4BMP256Info : public C4BMPInfo
{
public:
	C4BMP256Info();
	RGBQUAD Colors[256];
public:
	void Default();
	void Set(int iWdt, int iHgt, BYTE *bypPalette);
	bool Valid();

	int FileBitsOffset();
};

#pragma pack( pop, def_pack )

#endif // BITMAP256_H_INC
