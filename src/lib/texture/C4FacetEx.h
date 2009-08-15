/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2001, 2004, 2008-2009  Sven Eberhardt
 * Copyright (c) 2008  Günther Brammer
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

/* A facet that can hold its own surface and also target coordinates */

#ifndef INC_C4FacetEx
#define INC_C4FacetEx

#include <C4Facet.h>
#include <C4Surface.h>

const int C4FCT_Full   = -1,
					C4FCT_Height = -2,
					C4FCT_Width	 = -3;

class C4TargetRect;
class CSurface;
class C4Rect;

class C4TargetFacet: public C4Facet
	{
	public:
		C4TargetFacet() { Default(); }
		~C4TargetFacet() { }
	public:
		float TargetX,TargetY;
	public:
		void Default() { TargetX=TargetY=0; C4Facet::Default(); }
		void Clear() { Surface=NULL; }

		void Set(const C4Facet &cpy) { TargetX=TargetY=0; C4Facet::Set(cpy); }
		void Set(const C4TargetFacet &cpy) { *this = cpy; }
		void Set(class CSurface *nsfc, int nx, int ny, int nwdt, int nhgt, float ntx=0, float nty=0);
		void Set(class CSurface *nsfc, const C4Rect & r, float ntx=0, float nty=0);

		void DrawBolt(int iX1, int iY1, int iX2, int iY2, BYTE bCol, BYTE bCol2);
		void DrawLine(int iX1, int iY1, int iX2, int iY2, BYTE bCol1, BYTE bCol2);
		void DrawLineDw(int iX1, int iY1, int iX2, int iY2, uint32_t col1, uint32_t col2);
	public:
		C4TargetFacet &operator = (const C4Facet& rhs)
			{
			Set(rhs.Surface,rhs.X,rhs.Y,rhs.Wdt,rhs.Hgt);
			return *this;
			}
		void SetRect(C4TargetRect &rSrc);
	};

// facet that can hold its own surface
class C4FacetSurface : public C4Facet
	{
	private:
		C4Surface Face;

	private:
		C4FacetSurface(const C4FacetSurface &rCpy); // NO copy!
		C4FacetSurface &operator = (const C4FacetSurface &rCpy); // NO copy assignment!
	public:
		C4FacetSurface() { Default(); }
		~C4FacetSurface() { Clear(); }

		void Default() { Face.Default(); C4Facet::Default(); }
		void Clear() { Face.Clear(); }

		void Set(const C4Facet &cpy) { Clear(); C4Facet::Set(cpy); }
		void Set(const C4TargetFacet &cpy) { Clear(); C4Facet::Set(cpy); }
		void Set(SURFACE nsfc, int nx, int ny, int nwdt, int nhgt, int ntx=0, int nty=0)
			{ C4Facet::Set(nsfc, nx,ny,nwdt,nhgt); }

		void Grayscale(int32_t iOffset = 0);
		bool Create(int iWdt, int iHgt, int iWdt2=C4FCT_Full, int iHgt2=C4FCT_Full);
		C4Surface &GetFace() { return Face; } // get internal face
		bool CreateClrByOwner(CSurface *pBySurface);
		bool EnsureSize(int iMinWdt, int iMinHgt);
		bool EnsureOwnSurface();
		bool Load(C4Group &hGroup, const char *szName, int iWdt=C4FCT_Full, int iHgt=C4FCT_Full, bool fOwnPal=false, bool fNoErrIfNotFound=false);
		bool Load(BYTE *bpBitmap, int iWdt=C4FCT_Full, int iHgt=C4FCT_Full);
		bool Save(C4Group &hGroup, const char *szName);
		void GrabFrom(C4FacetSurface &rSource)
			{
			Clear(); Default();
			Face.MoveFrom(&rSource.Face);
			Set(rSource.Surface == &rSource.Face ? &Face : rSource.Surface, rSource.X, rSource.Y, rSource.Wdt, rSource.Hgt);
			rSource.Default();
			}
		bool CopyFromSfcMaxSize(C4Surface &srcSfc, int32_t iMaxSize, uint32_t dwColor=0u);
	};

// facet with source group ID; used to avoid doubled loading from same group
class C4FacetID : public C4FacetSurface
	{
	public:
		int32_t idSourceGroup;

		C4FacetID() : C4FacetSurface(), idSourceGroup(0) { } // ctor

		void Default() { C4FacetSurface::Default(); idSourceGroup = 0; } // default to std values
		void Clear() { C4FacetSurface::Clear(); idSourceGroup = 0; } // clear all data in class
	};

#endif
