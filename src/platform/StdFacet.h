/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2004  Sven Eberhardt
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

/* A very primitive piece of surface */

#ifndef STD_FACET
#define STD_FACET

#include <StdSurface2.h>

class CFacet
	{
	public:
		CFacet() : Surface(NULL), X(0), Y(0), Wdt(0), Hgt(0) { }
		~CFacet() { }
  public:
    SURFACE Surface;
    int X,Y,Wdt,Hgt;
  public:
	  void Draw(SURFACE sfcSurface, int iX, int iY, int iPhaseX=0, int iPhaseY=0);
		void Default() { Surface=NULL; X=Y=Wdt=Hgt=0; }
		void Clear() { Surface=NULL; X=Y=Wdt=Hgt=0; }
    void Set(SURFACE nsfc, int nx, int ny, int nwdt, int nhgt)
			{ Surface=nsfc; X=nx; Y=ny; Wdt=nwdt; Hgt=nhgt; }
	};
#endif // STD_FACET
