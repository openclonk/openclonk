/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2004-2005  Sven Eberhardt
 * Copyright (c) 2006  Günther Brammer
 * Copyright (c) 2004-2009, RedWolf Design GmbH, http://www.clonk.de
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
// engine font loading

#ifndef INC_C4Fonts
#define INC_C4Fonts

#include <vector>

// font loader
class C4FontLoader
{
protected:
	CStdVectorFont * pLastUsedFont; // cache
	StdCopyStrBuf LastUsedName;
	int32_t LastUsedGrpID;
	
public:
	// enum of different fonts used in the clonk engine
	enum FontType { C4FT_Log, C4FT_MainSmall, C4FT_Main, C4FT_Caption, C4FT_Title };

public:
	C4FontLoader(): pLastUsedFont(NULL), LastUsedGrpID(0) { } // ctor
	~C4FontLoader() { Clear(); } // dtor

	void Clear();                   // clear loaded fonts
	// init a font class of the given type
	// iSize is always the size of the normal font, which is adjusted for larger (title) and smaller (log) font types
	bool InitFont(CStdFont &rFont, const char *szFontName, FontType eType, int32_t iSize, C4GroupSet *pGfxGroups, bool fDoShadow=true);
};

extern C4FontLoader FontLoader;

#endif // INC_C4Fonts
