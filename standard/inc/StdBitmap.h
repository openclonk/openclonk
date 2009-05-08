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

/* Bitmap handling routines */

#pragma pack( push, def_pack , 1)

#include "Bitmap256.h"

class CStdBitmapHead: public BITMAPFILEHEADER
	{
	public:
		CStdBitmapHead(void);
	public:
		void Clear(void);
		void Set(int iBitOffset);
		BOOL Valid(void);
	};

class CStdBitmapInfo: public BITMAPINFOHEADER
	{
	public:
		CStdBitmapInfo(void);
	public:
		void Clear(void);
		void Set(int iWdt, int iHgt, int iBitsPerPixel);
		int Pitch(void);
	};

class CStdBitmap
	{
	public:
		CStdBitmap();
		~CStdBitmap();
	public:
		CStdBitmapHead Head;
		CStdBitmapInfo Info;
		RGBQUAD Colors[256];
		BYTE *Bits;
	public:
		BOOL Ensure(int iWdt, int iHgt, int iBitsPerPixel);
		int getPitch();
		int getHeight();
		int getWidth();
		void Clear(void);
		BOOL Create(int iWdt, int iHgt, int iBitsPerPixel);
		BOOL Save(const char *szFileName);
		BOOL Load(const char *szFileName);
		BOOL Enlarge(int iWdt, int iHgt);
	};

#pragma pack( pop, def_pack )
