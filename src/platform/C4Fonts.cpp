/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2004-2007  Sven Eberhardt
 * Copyright (c) 2005-2007, 2010  Günther Brammer
 * Copyright (c) 2010  Benjamin Herr
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
// engine font loading

#include <C4Include.h>
#include <C4Fonts.h>

#include <C4Language.h>
#include <C4Config.h>
#include <C4Components.h>
#include <C4Log.h>
#include <C4Surface.h>
#include <StdFont.h>

#include <StdFont.h>
#include <stdexcept>

void C4FontLoader::Clear()
{
	// delete vector font cache
	CStdFont::DestroyFont(pLastUsedFont);
	pLastUsedFont = NULL;
}

bool C4FontLoader::InitFont(CStdFont &rFont, const char *szFontName, FontType eType, int32_t iSize, C4GroupSet *pGfxGroups, bool fDoShadow)
{
#ifdef USE_CONSOLE
	return true;
#endif
	// safety
	assert(szFontName);
	if (!szFontName || !*szFontName)
	{
		LogFatal(FormatString("%s (\"%s\")", LoadResStr("IDS_ERR_INITFONTS"), szFontName ? szFontName : "(null)").getData());
		return false;
	}
	// if def has not been found, use the def as font name
	// determine font def string
	const char *szFontString = szFontName;
	// font not assigned?
	assert(*szFontString);
	if (!*szFontString)
	{
		// invalid call or spec
		LogFatal(LoadResStr("IDS_ERR_INITFONTS")); return false;
	}
	// get font name
	char FontFaceName[C4MaxName+1], FontParam[C4MaxName+1];
	SCopyUntil(szFontString, FontFaceName, ',', C4MaxName);
	// is it an image file?
	int32_t iDefFontSize; DWORD dwDefWeight=FW_NORMAL;
	switch (eType)
	{
	case C4FT_Log:     iDefFontSize = iSize*12/14; break;
	case C4FT_MainSmall:iDefFontSize = iSize*13/14; break;
	case C4FT_Main:    iDefFontSize = iSize; break;
	case C4FT_Caption: iDefFontSize = iSize*16/14; /*dwDefWeight = FW_MEDIUM;*/ break;
	case C4FT_Title:   iDefFontSize = iSize*22/14; /*dwDefWeight = FW_MEDIUM;*/ break;
	default: assert(false); LogFatal(LoadResStr("IDS_ERR_INITFONTS")); return false; // invalid call
	}
	// regular font name: let WinGDI or Freetype draw a font with the given parameters
	// font size given?
	if (SCopySegment(szFontString, 1, FontParam, ',', C4MaxName))
		sscanf(FontParam, "%i", &iDefFontSize);
	// font weight given?
	if (SCopySegment(szFontString, 2, FontParam, ',', C4MaxName))
	{
		int iDefWeight;
		sscanf(FontParam, "%i", &iDefWeight);
		dwDefWeight = iDefWeight;
	}
	// check if it's already loaded from that group with that parameters
	if (rFont.IsSameAs(FontFaceName, iDefFontSize, dwDefWeight))
		return true;
	// it's not; so (re-)load it now!
	if (rFont.IsInitialized())
	{
		// reloading
		rFont.Clear();
		LogF(LoadResStr("IDS_PRC_UPDATEFONT"), FontFaceName, iDefFontSize, dwDefWeight);
	}
	// check if one of the internally listed fonts should be used
	const char * const extensions[] = { "ttf", "otf", "ttc", "fon", "fnt", "fot", NULL };
	char FileName[_MAX_PATH+1];
	int32_t ID;
	C4Group * pGrp = pGfxGroups->FindSuitableFile(FontFaceName, extensions, FileName, &ID);
	if (pGrp)
	{
		if (LastUsedGrpID != ID || LastUsedName != FontFaceName)
		{
			CStdFont::DestroyFont(pLastUsedFont);
			pLastUsedFont = NULL;
		}
		if (!pLastUsedFont)
		{
			StdBuf Data;
			if (pGrp->LoadEntry(FileName, &Data))
			{
				try
				{
					pLastUsedFont = CStdFont::CreateFont(Data);
					LastUsedGrpID = ID;
					LastUsedName = FontFaceName;
				}
				catch (std::runtime_error & e)
				{
					LogFatal(e.what());
					pGrp = NULL;
				}
			}
		}
	}
	// no internal font match? Then create one using the given face/filename (using a system font)
	if (!pGrp)
	{
		if (LastUsedGrpID != -1 || LastUsedName != FontFaceName)
		{
			CStdFont::DestroyFont(pLastUsedFont);
			pLastUsedFont = NULL;
		}
		if (!pLastUsedFont)
		{
			try
			{
				pLastUsedFont = CStdFont::CreateFont(FontFaceName);
				if (!pLastUsedFont)
					// no match for font face found
					throw std::runtime_error(FormatString("Font face %s undefined", FontFaceName).getData());
				LastUsedGrpID = -1;
				LastUsedName = FontFaceName;
			}
			catch (std::runtime_error & e)
			{
				LogFatal(e.what());
			}
		}
	}
	if (!pLastUsedFont)
	{
		LogFatal(LoadResStr("IDS_ERR_INITFONTS"));
		return false;
	}
	try
	{
		rFont.Init(*pLastUsedFont, FontFaceName, iDefFontSize, dwDefWeight, fDoShadow); // throws exception on error
		return true;
	}
	catch (std::runtime_error & e)
	{
		LogFatal(e.what());
		LogFatal(LoadResStr("IDS_ERR_INITFONTS"));
		return false;
	}
}

C4FontLoader FontLoader;
