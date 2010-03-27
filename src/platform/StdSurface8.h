/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2007  Günther Brammer
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
// a class holding a 8 bpp memory surface

#ifndef INC_StdSurface8
#define INC_StdSurface8

#include <StdColors.h>

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
		CStdPalette *pPal;						// pal for this surface (usually points to the main pal)
		bool HasOwnPal();							// return whether the surface palette is owned
		void HLine(int iX, int iX2, int iY, int iCol);
		void Polygon(int iNum, int *ipVtx, int iCol, uint8_t *conversion_table=NULL);
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
		BYTE GetPix(int iX, int iY)	// get pixel
			{
			if (iX<0 || iY<0 || iX>=Wdt || iY>=Hgt) return 0;
			return Bits ? Bits[iY*Pitch+iX] : 0;
			}
		inline BYTE _GetPix(int x, int y) // get pixel (bounds not checked)
			{
			return Bits[y*Pitch+x];
			}
		bool Create(int iWdt, int iHgt, bool fOwnPal=false);
		void MoveFrom(CSurface *psfcFrom); // grab data from other surface - invalidates other surface
		void Clear();
		void Clip(int iX, int iY, int iX2, int iY2);
		void NoClip();
		bool Read(class CStdStream &hGroup, bool fOwnPal);
		bool Save(const char *szFilename, BYTE *bpPalette = NULL);
		void Wipe(); // empty to transparent
		void GetSurfaceSize(int &irX, int &irY); // get surface size
		void EnforceC0Transparency() { pPal->EnforceC0Transparency(); }
		void AllowColor(BYTE iRngLo, BYTE iRngHi, bool fAllowZero=false);
		void SetBuffer(BYTE *pbyToBuf, int Wdt, int Hgt, int Pitch);
		void ReleaseBuffer();
	protected:
		void MapBytes(BYTE *bpMap);
		bool ReadBytes(BYTE **lpbpData, void *bpTarget, int iSize);
	};

#endif
