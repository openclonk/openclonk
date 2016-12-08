/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2016, The OpenClonk Team and contributors
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
// a wrapper class to DirectDraw surfaces

#include "C4Include.h"
#include "graphics/CSurface8.h"

#include "graphics/Bitmap256.h"
#include "c4group/CStdFile.h"
#include "lib/StdColors.h"

CSurface8::CSurface8()
{
	Wdt=Hgt=Pitch=0;
	ClipX=ClipY=ClipX2=ClipY2=0;
	Bits=nullptr;
	pPal=nullptr;
}

CSurface8::CSurface8(int iWdt, int iHgt)
{
	Wdt=Hgt=Pitch=0;
	ClipX=ClipY=ClipX2=ClipY2=0;
	Bits=nullptr;
	pPal=nullptr;
	Create(iWdt, iHgt);
}

CSurface8::~CSurface8()
{
	Clear();
}

void CSurface8::Clear()
{
	// clear bitmap-copy
	delete [] Bits; Bits=nullptr;
	// clear pal
	delete pPal;
	pPal=nullptr;
}

void CSurface8::Box(int iX, int iY, int iX2, int iY2, int iCol)
{
	for (int cy=iY; cy<=iY2; cy++) HLine(iX,iX2,cy,iCol);
}

void CSurface8::NoClip()
{
	ClipX=0; ClipY=0; ClipX2=Wdt-1; ClipY2=Hgt-1;
}

void CSurface8::Clip(int iX, int iY, int iX2, int iY2)
{
	ClipX=Clamp(iX,0,Wdt-1); ClipY=Clamp(iY,0,Hgt-1);
	ClipX2=Clamp(iX2,0,Wdt-1); ClipY2=Clamp(iY2,0,Hgt-1);
}

void CSurface8::HLine(int iX, int iX2, int iY, int iCol)
{
	for (int cx=iX; cx<=iX2; cx++) SetPix(cx,iY,iCol);
}

bool CSurface8::Create(int iWdt, int iHgt)
{
	Clear();
	// check size
	if (!iWdt || !iHgt) return false;
	Wdt=iWdt; Hgt=iHgt;

	// create pal
	pPal = new CStdPalette;
	if (!pPal) return false;
	memset(pPal->Colors, 0, sizeof(pPal->Colors));

	Bits=new BYTE[Wdt*Hgt];
	if (!Bits) return false;
	memset(Bits, 0, Wdt*Hgt);
	Pitch=Wdt;
	// update clipping
	NoClip();
	return true;
}

bool CSurface8::Read(CStdStream &hGroup)
{
	int cnt,lcnt;
	C4BMP256Info BitmapInfo;
	// read bmpinfo-header
	if (!hGroup.Read(&BitmapInfo,sizeof(C4BMPInfo))) return false;
	// is it 8bpp?
	if (BitmapInfo.Info.biBitCount == 8)
	{
		if (!hGroup.Read(((BYTE *) &BitmapInfo)+sizeof(C4BMPInfo),sizeof(BitmapInfo)-sizeof(C4BMPInfo))) return false;
		if (!hGroup.Advance(BitmapInfo.FileBitsOffset())) return false;
	}
	else
	{
		// read 24bpp
		if (BitmapInfo.Info.biBitCount != 24) return false;
		if (!hGroup.Advance(((C4BMPInfo) BitmapInfo).FileBitsOffset())) return false;
	}

	// Create and lock surface
	if (!Create(BitmapInfo.Info.biWidth,BitmapInfo.Info.biHeight)) return false;

	if (BitmapInfo.Info.biBitCount == 8)
	{
		// Copy palette
		for (cnt=0; cnt<256; cnt++)
		{
			pPal->Colors[cnt] = C4RGB(BitmapInfo.Colors[cnt].rgbRed,
			                          BitmapInfo.Colors[cnt].rgbGreen,
			                          BitmapInfo.Colors[cnt].rgbBlue);
		}
	}

	// create line buffer
	int iBufSize=DWordAligned(BitmapInfo.Info.biWidth*BitmapInfo.Info.biBitCount/8);
	BYTE *pBuf = new BYTE[iBufSize];
	// Read lines
	for (lcnt=Hgt-1; lcnt>=0; lcnt--)
	{
		if (!hGroup.Read(pBuf, iBufSize))
			{ Clear(); delete [] pBuf; return false; }
		BYTE *pPix=pBuf;
		for (int x=0; x<BitmapInfo.Info.biWidth; ++x)
			switch (BitmapInfo.Info.biBitCount)
			{
			case 8:
				SetPix(x, lcnt, *pPix++);
				break;
			case 24:
				return false;
				break;
			}
	}
	// free buffer again
	delete [] pBuf;

	return true;
}

bool CSurface8::Save(const char *szFilename, CStdPalette *bpPalette)
{
	C4BMP256Info BitmapInfo;
	BitmapInfo.Set(Wdt,Hgt, bpPalette ? bpPalette : pPal);

	// Create file & write info
	CStdFile hFile;

	if ( !hFile.Create(szFilename)
	     || !hFile.Write(&BitmapInfo,sizeof(BitmapInfo)) )
		{ return false; }

	// Write lines
	char bpEmpty[4]; ZeroMem(bpEmpty, 4);
	const int iEmpty = DWordAligned(Wdt)-Wdt;
	for (int cnt=Hgt-1; cnt>=0; cnt--)
	{
		if (!hFile.Write(Bits+(Pitch*cnt),Wdt))
			{ return false; }
		if (iEmpty)
			if (!hFile.Write(bpEmpty,iEmpty))
				{ return false; }
	}

	// Close file
	hFile.Close();

	// Success
	return true;
}

void CSurface8::MapBytes(BYTE *bpMap)
{
	if (!bpMap) return;
	for (int cnt=0; cnt<Wdt*Hgt; cnt++) SetPix(cnt%Wdt, cnt/Wdt, bpMap[GetPix(cnt%Wdt, cnt/Wdt)]);
}

void CSurface8::GetSurfaceSize(int &irX, int &irY) const
{
	// simply assign stored values
	irX=Wdt;
	irY=Hgt;
}

void CSurface8::ClearBox8Only(int iX, int iY, int iWdt, int iHgt)
{
	// clear rect; assume clip already
	for (int y=iY; y<iY+iHgt; ++y)
		for (int x=iX; x<iX+iWdt; ++x)
			Bits[y*Pitch+x] = 0;
	// done
}


void CSurface8::Circle(int x, int y, int r, BYTE col)
{
	for (int ycnt=-r; ycnt<r; ycnt++)
	{
		int lwdt = (int) sqrt(float(r*r-ycnt*ycnt));
		for (int xcnt = 2 * lwdt - 1; xcnt >= 0; xcnt--)
			SetPix(x - lwdt + xcnt, y + ycnt, col);
	}
}

void CSurface8::AllowColor(BYTE iRngLo, BYTE iRngHi, bool fAllowZero)
{
	// change colors
	int xcnt,ycnt;
	if (iRngHi<iRngLo) return;
	for (ycnt=0; ycnt<Hgt; ycnt++)
	{
		for (xcnt=0; xcnt<Wdt; xcnt++)
		{
			BYTE px=GetPix(xcnt,ycnt);
			if (px || !fAllowZero)
				if ((px<iRngLo) || (px>iRngHi))
					SetPix(xcnt, ycnt, iRngLo + px % (iRngHi-iRngLo+1));
		}
	}
}

void CSurface8::SetBuffer(BYTE *pbyToBuf, int Wdt, int Hgt, int Pitch)
{
	// release old
	Clear();
	// set new
	this->Wdt=Wdt;
	this->Hgt=Hgt;
	this->Pitch=Pitch;
	this->Bits = pbyToBuf;
	NoClip();
}

void CSurface8::ReleaseBuffer()
{
	this->Bits = nullptr;
	Clear();
}
