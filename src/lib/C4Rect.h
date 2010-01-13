/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2007  Matthes Bender
 * Copyright (c) 2001, 2003-2006  Sven Eberhardt
 * Copyright (c) 2005-2006  Peter Wortmann
 * Copyright (c) 2006, 2009  Günther Brammer
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

/* Basic classes for rectangles and vertex outlines */

#ifndef INC_C4Rect
#define INC_C4Rect

#define C4D_VertexCpyPos (C4D_MaxVertex/2)

#include <vector>

class StdCompiler;

struct FLOAT_RECT { float left,right,top,bottom; };
class C4TargetFacet;

class C4Rect
  {
  public:
    int32_t x,y,Wdt,Hgt;
	public:
		void Set(int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt);
		void Default();
		bool Overlap(C4Rect &rTarget);
		void Intersect(const C4Rect &r2);
		void Add(const C4Rect &r2);
		bool operator ==(const C4Rect &r2) { return !((x-r2.x) | (y-r2.y) | (Wdt-r2.Wdt) | (Hgt-r2.Hgt)); }
		bool operator !=(const C4Rect &r2) { return 0 != ((x-r2.x) | (y-r2.y) | (Wdt-r2.Wdt) | (Hgt-r2.Hgt)); }

		bool Contains(int32_t iX, int32_t iY)
			{ return iX>=x && iX<x+Wdt && iY>=y && iY<y+Hgt; }
		bool Contains(int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt)
			{ return iX>=x && iX+iWdt<x+Wdt && iY>=y && iY+iHgt<y+Hgt; }
		bool Contains(const C4Rect &rect)
			{ return Contains(rect.x, rect.y, rect.Wdt, rect.Hgt); }
		bool IntersectsLine(int32_t iX, int32_t iY, int32_t iX2, int32_t iY2);

		void Normalize()
			{ if(Wdt < 0) { x+=Wdt+1; Wdt=-Wdt; } if(Hgt < 0) { y+=Hgt+1; Hgt=-Hgt; } }

    void Enlarge(int32_t iBy)
      { x -= iBy; y -= iBy; Wdt += 2*iBy; Hgt += 2*iBy; }

		int32_t GetMiddleX() { return x+Wdt/2; }
		int32_t GetMiddleY() { return y+Hgt/2; }
		int32_t GetBottom() { return y+Hgt; }

		C4Rect(int32_t tx, int32_t ty, int32_t twdt, int32_t thgt) // ctor
			{ x=tx; y=ty; Wdt=twdt; Hgt=thgt; }
		C4Rect() { } // default ctor; doesn't initialize
		C4Rect(const FLOAT_RECT &rcfOuter) // set to surround floating point rectangle
			{ x=static_cast<int32_t>(rcfOuter.left); y=static_cast<int32_t>(rcfOuter.top);
			  Wdt=static_cast<int32_t>(ceilf(rcfOuter.right)-floorf(rcfOuter.left));
			  Hgt=static_cast<int32_t>(ceilf(rcfOuter.bottom)-floorf(rcfOuter.top)); }

		void CompileFunc(StdCompiler *pComp);
  };

class C4TargetRect: public C4Rect
  {
  public:
    int32_t tx,ty;
  public:
		C4TargetRect(int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt, int32_t iTX, int32_t iTY)
			: C4Rect(iX, iY, iWdt, iHgt), tx(iTX), ty(iTY) { }
		C4TargetRect() { } // default ctor; doesn't initialize
	  void Set(int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt, int32_t iTX, int32_t iTY);
	  void Default();
		bool ClipBy(C4TargetRect &rClip); // clip this rectangle by the given one (adding target positions) - return false if they don't overlap

		void Set(const C4TargetFacet &rSrc); // copy contents from facet

		void CompileFunc(StdCompiler *pComp);
  };

const C4Rect Rect0(0,0,0,0);
const C4TargetRect TargetRect0(0,0,0,0,0,0);

// a bunch of rectangles
// rects NOT including pos+size-point
class C4RectList : public std::vector<C4Rect>
	{
	public:
		void AddRect(const C4Rect &rNewRect)
			{ push_back(rNewRect); }
		void RemoveIndexedRect(int32_t idx)
			{ if (idx<GetCount()-1) Get(idx)=Get(GetCount()-1); pop_back(); }
		void Clear() { clear(); }
		int32_t GetCount() const { return size(); }
		C4Rect &Get(int32_t idx) { return (*this)[idx]; } // access w/o range check

		void ClipByRect(const C4Rect &rClip); // split up rectangles
	};


#endif // INC_C4Rect
