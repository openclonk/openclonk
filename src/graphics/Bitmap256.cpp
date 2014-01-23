/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2011-2013, The OpenClonk Team and contributors
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

/* A structure for handling 256-color bitmap files */

#include "C4Include.h"
#include <Bitmap256.h>
#include <StdColors.h>

C4BMPInfo::C4BMPInfo()
{
	Default();
}

void C4BMPInfo::Default()
{
	ZeroMem(this,sizeof(C4BMPInfo));
}

int C4BMPInfo::FileBitsOffset()
{
	return Head.bfOffBits-sizeof(C4BMPInfo);
}

void C4BMPInfo::Set(int iWdt, int iHgt, int iBitDepth)
{
	Default();
	// Set header
	Head.bfType=*((WORD*)"BM");
	Head.bfSize=sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+DWordAligned(iWdt)*iHgt;
	Head.bfOffBits=sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);
	// Set bitmap info
	Info.biSize=sizeof(BITMAPINFOHEADER);
	Info.biWidth=iWdt;
	Info.biHeight=iHgt;
	Info.biPlanes=1;
	Info.biBitCount=iBitDepth;
	Info.biCompression=0;
	Info.biSizeImage=iWdt*iHgt;
	Info.biClrUsed=Info.biClrImportant=0;
}


C4BMP256Info::C4BMP256Info()
{
	Default();
}

bool C4BMP256Info::Valid()
{
	if (Head.bfType != *((WORD*)"BM") ) return false;
	if ((Info.biBitCount!=8) || (Info.biCompression!=0)) return false;
	return true;
}

int C4BMP256Info::FileBitsOffset()
{
	return Head.bfOffBits-sizeof(C4BMP256Info);
}

void C4BMP256Info::Set(int iWdt, int iHgt, CStdPalette *Palette)
{
	Default();
	// Set header
	Head.bfType=*((WORD*)"BM");
	Head.bfSize=sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+256*sizeof(RGBQUAD)+DWordAligned(iWdt)*iHgt;
	Head.bfOffBits=sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+256*sizeof(RGBQUAD);
	// Set bitmap info
	Info.biSize=sizeof(BITMAPINFOHEADER);
	Info.biWidth=iWdt;
	Info.biHeight=iHgt;
	Info.biPlanes=1;
	Info.biBitCount=8;
	Info.biCompression=0;
	Info.biSizeImage=iWdt*iHgt;
	Info.biClrUsed=Info.biClrImportant=256;
	// Set palette
	for (int cnt=0; cnt<256; cnt++)
	{
		Colors[cnt].rgbRed   = GetRedValue(Palette->Colors[cnt]);
		Colors[cnt].rgbGreen = GetGreenValue(Palette->Colors[cnt]);
		Colors[cnt].rgbBlue  = GetBlueValue(Palette->Colors[cnt]);
	}
}

void C4BMP256Info::Default()
{
	ZeroMem(this,sizeof(C4BMP256Info));
}

