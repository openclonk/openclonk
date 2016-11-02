/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2013-2016, The OpenClonk Team and contributors
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
// a class holding a 8 bpp memory surface

#ifndef INC_StdSurface8
#define INC_StdSurface8

class CSurface8
{
public:
	CSurface8();
	~CSurface8();
	CSurface8(int iWdt, int iHgt); // create new surface and init it
public:
	int Wdt,Hgt,Pitch; // size of surface
	int ClipX,ClipY,ClipX2,ClipY2;
	BYTE *Bits;
	CStdPalette *pPal;            // pal for this surface (usually points to the main pal)
	bool HasOwnPal();             // return whether the surface palette is owned
	void HLine(int iX, int iX2, int iY, int iCol);
	void Box(int iX, int iY, int iX2, int iY2, int iCol);
	void Circle(int x, int y, int r, BYTE col);
	void ClearBox8Only(int iX, int iY, int iWdt, int iHgt); // clear box in 8bpp-surface only
	void SetPix(int iX, int iY, BYTE byCol)
	{
		// clip
		if ((iX<ClipX) || (iX>ClipX2) || (iY<ClipY) || (iY>ClipY2)) return;
		// set pix in local copy...
		if (Bits) Bits[iY*Pitch+iX]=byCol;
	}
	void _SetPix(int iX, int iY, BYTE byCol)
	{
		// set pix in local copy without bounds or surface checks
		Bits[iY*Pitch+iX]=byCol;
	}
	BYTE GetPix(int iX, int iY) const // get pixel
	{
		if (iX<0 || iY<0 || iX>=Wdt || iY>=Hgt) return 0;
		return Bits ? Bits[iY*Pitch+iX] : 0;
	}
	inline BYTE _GetPix(int x, int y) const // get pixel (bounds not checked)
	{
		return Bits[y*Pitch+x];
	}
	bool Create(int iWdt, int iHgt);
	void MoveFrom(C4Surface *psfcFrom); // grab data from other surface - invalidates other surface
	void Clear();
	void Clip(int iX, int iY, int iX2, int iY2);
	void NoClip();
	bool Read(class CStdStream &hGroup);
	bool Save(const char *szFilename, CStdPalette * = nullptr);
	void GetSurfaceSize(int &irX, int &irY) const; // get surface size
	void AllowColor(BYTE iRngLo, BYTE iRngHi, bool fAllowZero=false);
	void SetBuffer(BYTE *pbyToBuf, int Wdt, int Hgt, int Pitch);
	void ReleaseBuffer();
protected:
	void MapBytes(BYTE *bpMap);
	bool ReadBytes(BYTE **lpbpData, void *bpTarget, int iSize);
};

#endif
