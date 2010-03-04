/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2002  Sven Eberhardt
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

#include "C4Include.h"
#include <Bitmap256.h>

CBitmapInfo::CBitmapInfo()
	{
	Default();
	}

void CBitmapInfo::Default()
	{
	ZeroMem(this,sizeof(CBitmapInfo));
	}

int CBitmapInfo::FileBitsOffset()
	{
	return Head.bfOffBits-sizeof(CBitmapInfo);
	}

void CBitmapInfo::Set(int iWdt, int iHgt, int iBitDepth)
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


CBitmap256Info::CBitmap256Info()
	{
	Default();
	}

bool CBitmap256Info::Valid()
	{
  if (Head.bfType != *((WORD*)"BM") ) return false;
  if ((Info.biBitCount!=8) || (Info.biCompression!=0)) return false;
	return true;
	}

int CBitmap256Info::FileBitsOffset()
	{
	return Head.bfOffBits-sizeof(CBitmap256Info);
	}

void CBitmap256Info::Set(int iWdt, int iHgt, BYTE *bypPalette)
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
    Colors[cnt].rgbRed	 = bypPalette[cnt*3+0];
    Colors[cnt].rgbGreen = bypPalette[cnt*3+1];
    Colors[cnt].rgbBlue  = bypPalette[cnt*3+2];
    }
	}

void CBitmap256Info::Default()
	{
	ZeroMem(this,sizeof(CBitmap256Info));
	}

