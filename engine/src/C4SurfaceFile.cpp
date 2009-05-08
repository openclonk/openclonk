/*
 * OpenClonk, http://www.openclonk.org
 *
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

/* Another C4Group bitmap-to-surface loader and saver */

#include <C4Include.h>
#include <C4SurfaceFile.h>

#ifndef BIG_C4INCLUDE
#include <C4Surface.h>
#include <C4Group.h>
#endif

C4Surface *GroupReadSurface(CStdStream &hGroup, BYTE *bpPalette)
	{
	// create surface
	C4Surface *pSfc=new C4Surface();
	if (!pSfc->Read(hGroup, !!bpPalette))
		{ delete pSfc; return NULL; }
	return pSfc;
	}

CSurface8 *GroupReadSurface8(CStdStream &hGroup)
	{
	// create surface
	CSurface8 *pSfc=new CSurface8();
	if (!pSfc->Read(hGroup, false))
		{ delete pSfc; return NULL; }
	return pSfc;
	}

C4Surface *GroupReadSurfaceOwnPal(CStdStream &hGroup)
	{
	// create surface
	C4Surface *pSfc=new C4Surface();
	if (!pSfc->Read(hGroup, true))
		{ delete pSfc; return NULL; }
	return pSfc;
	}

CSurface8 *GroupReadSurfaceOwnPal8(CStdStream &hGroup)
	{
	// create surface
	CSurface8 *pSfc=new CSurface8();
	if (!pSfc->Read(hGroup, true))
		{ delete pSfc; return NULL; }
	return pSfc;
	}

C4Surface *GroupReadSurfacePNG(CStdStream &hGroup)
	{
	// create surface
	C4Surface *pSfc=new C4Surface();
	pSfc->ReadPNG(hGroup);
	return pSfc;
	}

/*BOOL SaveSurface(const char *szFilename,
								 SURFACE sfcSurface,
								 BYTE *bpPalette)
  {
  BITMAPINFOHEADER bmpinfo;
  RGBQUAD rgbquad;
  BITMAPFILEHEADER bmphead;

  int cnt,lcnt,ladd,pitch;
  int imgwdt,imghgt;
  BYTE fbuf[4];
  BYTE *timgbuf;
  CStdFile hFile;

  // Lock the sfcSurface
  if (!(timgbuf=lpDDraw->LockSurface(sfcSurface,pitch,&imgwdt,&imghgt)))
    return FALSE;

  // Image line data in file is extended to be multiple of 4
  ladd=0; if (imgwdt%4!=0) ladd=4-imgwdt%4;

  // Set bitmap info
  ZeroMem((BYTE*)&bmpinfo,sizeof(BITMAPINFOHEADER));
  bmpinfo.biSize=sizeof(BITMAPINFOHEADER);
  bmpinfo.biWidth=imgwdt;
  bmpinfo.biHeight=imghgt;
  bmpinfo.biPlanes=1;
  bmpinfo.biBitCount=8;
  bmpinfo.biCompression=0;
  bmpinfo.biSizeImage=imgwdt*imghgt;
  bmpinfo.biClrUsed=bmpinfo.biClrImportant=256;

  // Set header
  ZeroMem((BYTE*)&bmphead,sizeof(BITMAPFILEHEADER));
  bmphead.bfType=*((const WORD*)"BM");
  bmphead.bfSize=sizeof(BITMAPFILEHEADER)
                +sizeof(BITMAPINFOHEADER)
                +256*sizeof(RGBQUAD)
                +(imgwdt+ladd)*imghgt;
  bmphead.bfOffBits=sizeof(BITMAPFILEHEADER)
                   +sizeof(BITMAPINFOHEADER)
                   +256*sizeof(RGBQUAD);


  if (!hFile.Create(szFilename,FALSE))
    { lpDDraw->UnLockSurface(sfcSurface); return FALSE; }


  hFile.Write(&bmphead,sizeof(bmphead));
  hFile.Write(&bmpinfo,sizeof(bmpinfo));

  for (cnt=0; cnt<256; cnt++)
    {
    rgbquad.rgbRed=  bpPalette[cnt*3+0];
    rgbquad.rgbGreen=bpPalette[cnt*3+1];
    rgbquad.rgbBlue= bpPalette[cnt*3+2];
    hFile.Write(&rgbquad,sizeof(rgbquad));
    }

  for (lcnt=imghgt-1; lcnt>=0; lcnt--)
    {
    hFile.Write(timgbuf+(pitch*lcnt),imgwdt);
    if (ladd>0) hFile.Write(fbuf,ladd);
    }

  lpDDraw->UnLockSurface(sfcSurface);
  hFile.Close();

  return TRUE;
  }*/
