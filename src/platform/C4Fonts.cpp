/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2004-2007  Sven Eberhardt
 * Copyright (c) 2005-2007  Günther Brammer
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

C4VectorFont::~C4VectorFont()
{
	CStdFont::DestroyFont(pFont);
	// temp file loaded?
	if (*FileName)
	{
		// release font
#if defined(_WIN32) && !defined(HAVE_FREETYPE)
		//RemoveFontResourceEx(FileName, FR_PRIVATE, NULL); requires win2k
		RemoveFontResource(FileName);
#endif
		if (fIsTempFile) EraseFile(FileName);
	}
}

bool C4VectorFont::Init(C4Group &hGrp, const char *szFilename, C4Config &rCfg)
{
	// name by file
	Name.Copy(GetFilenameOnly(szFilename));
#if defined(_WIN32) && !defined(HAVE_FREETYPE)
	// check whether group is directory or packed
	if (!hGrp.IsPacked())
	{
		// it's open: use the file directly
		SCopy(hGrp.GetFullName().getData(), FileName, _MAX_PATH);
		AppendBackslash(FileName);
		SAppend(szFilename, FileName);
		if (!FileExists(FileName)) { *FileName=0; return false; }
		fIsTempFile = false;
	}
	else
	{
		// it's packed: extract to temp path
		SCopy(rCfg.AtTempPath(szFilename), FileName, _MAX_PATH);
		// make sure the filename is not in use, in case multiple instances of the engine are run
		if (FileExists(FileName))
		{
			RemoveExtension(FileName);
			StdStrBuf sNewFilename;
			for (int i=0; i<1000; ++i)
			{
				sNewFilename.Format("%s%x", FileName, (int)rand());
				if (*GetExtension(szFilename))
				{
					sNewFilename.AppendChar('.');
					sNewFilename.Append(GetExtension(szFilename));
				}
				if (!FileExists(sNewFilename.getData())) break;
			}
			SCopy(sNewFilename.getData(), FileName, _MAX_PATH);
		}
		if (!hGrp.ExtractEntry(szFilename, FileName)) { *FileName=0; return false; }
		fIsTempFile = true;
	}
	// add the font resource
	//if (!AddFontResourceEx(FileName, FR_PRIVATE, NULL)) requires win2k
	if (!AddFontResource(FileName))
	{
		if (fIsTempFile) EraseFile(FileName);
		*FileName='\0';
		return false;
	}
#else
	if (!hGrp.LoadEntry(szFilename, Data)) return false;
#endif
	// success
	return true;
}

bool C4VectorFont::Init(const char *szFacename, int32_t iSize, uint32_t dwWeight)
{
	// name by face
	Name.Copy(szFacename);
#if defined(_WIN32) && defined(HAVE_FREETYPE)
	// Win32 using freetype: Load TrueType-data from WinGDI into Data-buffer to be used by FreeType
	bool fSuccess = false;
	HDC hDC = ::CreateCompatibleDC(NULL);
	if (hDC)
	{
		HFONT hFont = ::CreateFont(iSize, 0, 0, 0, dwWeight, false,
		                           false, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
		                           CLIP_DEFAULT_PRECIS, 5,
		                           VARIABLE_PITCH, szFacename);
		if (hFont)
		{
			SelectObject( hDC, hFont );
			uint32_t dwTTFSize = ::GetFontData(hDC, 0, 0, NULL, 0);
			if (dwTTFSize && dwTTFSize != GDI_ERROR)
			{
				Data.SetSize(dwTTFSize);
				uint32_t dwRealTTFSize = ::GetFontData(hDC, 0, 0, Data.getMData(), dwTTFSize);
				if (dwRealTTFSize == dwTTFSize)
				{
					fSuccess = true;
				}
				else
					Data.Clear();
			}
			DeleteObject(hFont);
		}
		DeleteDC(hDC);
	}
	if (!fSuccess) return false;
#else
	// otherwise, just assume that font can be created by face name
#endif
	// success
	return true;
}

void C4VectorFont::Init(const char *szFaceName, CStdVectorFont *pFont)
{
	// just set data
	Name.Copy(szFaceName);
	this->pFont = pFont;
}

void C4FontLoader::Clear()
{
	// delete loaded vector fonts
	C4VectorFont *pVecFont, *pNextVecFont = pVectorFonts;
	while ((pVecFont = pNextVecFont))
	{
		pNextVecFont = pVecFont->pNext;
		delete pVecFont;
	}
	pVectorFonts=NULL;
}

void C4FontLoader::AddVectorFont(C4VectorFont *pNewFont)
{
	// add as last to chain
	C4VectorFont **ppFont = &pVectorFonts;
	while (*ppFont) ppFont = &((*ppFont)->pNext);
	*ppFont = pNewFont;
}

int32_t C4FontLoader::LoadDefs(C4Group &hGroup, C4Config &rCfg)
{
	// load vector fonts
	char fn[_MAX_PATH+1], fnDef[32]; int i = 0;
	int r = 0;
	while (SCopySegment(C4CFN_FontFiles, i++, fnDef, '|', 31))
	{
		hGroup.ResetSearch();
		while (hGroup.FindNextEntry(fnDef, fn))
		{
			C4VectorFont *pVecFon = new C4VectorFont();
			if (pVecFon->Init(hGroup, fn, rCfg))
			{
				pVecFon->pNext = pVectorFonts;
				pVectorFonts = pVecFon;
				++r;
			}
			else delete pVecFon;
		}
	}
	LogF("font list: %d", r);	
	return r;
}

bool C4FontLoader::InitFont(CStdFont &rFont, C4VectorFont * pFont, int32_t iSize, uint32_t dwWeight, bool fDoShadow)
{
	if (!pFont->pFont)
	{
#ifdef HAVE_FREETYPE
		// Creation from binary font data
		if (!pFont->Data.isNull()) pFont->pFont = CStdFont::CreateFont(pFont->Data);
		// creation from filename
		if (!pFont->pFont) pFont->pFont = CStdFont::CreateFont(pFont->FileName);
#endif
		// WinGDI: Try creation from font face name only
		if (!pFont->pFont) pFont->pFont = CStdFont::CreateFont(pFont->Name.getData());
		if (!pFont->pFont) return false; // this font can't be used
	}
	rFont.Init(*(pFont->pFont), pFont->GetName(), iSize, dwWeight, fDoShadow); // throws exception on error
	return true;
}

bool C4FontLoader::InitFont(CStdFont &rFont, const char *szFontName, FontType eType, int32_t iSize, C4GroupSet *pGfxGroups, bool fDoShadow)
{
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
#if defined(_WIN32) && !defined(HAVE_FREETYPE)
	switch (eType)
	{
	case C4FT_Log:     iDefFontSize = 8; break;
	case C4FT_MainSmall:iDefFontSize = iSize+1; break;
	case C4FT_Main:    iDefFontSize = iSize+4; break;
	case C4FT_Caption: iDefFontSize = iSize+6; dwDefWeight = FW_BOLD; break;
	case C4FT_Title:   iDefFontSize = iSize*3; break;
	default: assert(false); LogFatal(LoadResStr("IDS_ERR_INITFONTS")); return false; // invalid call
	}
#else
	switch (eType)
	{
	case C4FT_Log:     iDefFontSize = iSize*12/14; break;
	case C4FT_MainSmall:iDefFontSize = iSize*13/14; break;
	case C4FT_Main:    iDefFontSize = iSize; break;
	case C4FT_Caption: iDefFontSize = iSize*16/14; /*dwDefWeight = FW_MEDIUM;*/ break;
	case C4FT_Title:   iDefFontSize = iSize*22/14; /*dwDefWeight = FW_MEDIUM;*/ break;
	default: assert(false); LogFatal(LoadResStr("IDS_ERR_INITFONTS")); return false; // invalid call
	}
#endif
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
	if (!rFont.IsSameAs(FontFaceName, iDefFontSize, dwDefWeight))
	{
		// it's not; so (re-)load it now!
		if (rFont.IsInitialized())
		{
			// reloading
			rFont.Clear();
			LogF(LoadResStr("IDS_PRC_UPDATEFONT"), FontFaceName, iDefFontSize, dwDefWeight);
		}
		// init with given font name
		try
		{
			// check if one of the internally listed fonts should be used
			C4VectorFont * pFont = pVectorFonts;
			while (pFont)
			{
				if (SEqual(pFont->Name.getData(), FontFaceName))
				{
					if (InitFont(rFont, pFont, iDefFontSize, dwDefWeight, fDoShadow)) break;
				}
				pFont = pFont->pNext;
			}
			// no internal font matching? Then create one using the given face/filename (using a system font)
			if (!pFont)
			{
				pFont = new C4VectorFont();
				if (pFont->Init(FontFaceName, iDefFontSize, dwDefWeight))
				{
					AddVectorFont(pFont);
					if (!InitFont(rFont, pFont, iDefFontSize, dwDefWeight, fDoShadow))
						throw std::runtime_error(FormatString("Error initializing font %s", FontFaceName).getData());
				}
				else
				{
					delete pFont;
					// no match for font face found
					throw std::runtime_error(FormatString("Font face %s undefined", FontFaceName).getData());
				}
			}
		}
		catch (std::runtime_error & e)
		{
			LogFatal(e.what());
			LogFatal(LoadResStr("IDS_ERR_INITFONTS"));
			return false;
		}
		rFont.id = 0;
	}
	// done, success
	return true;
}

C4FontLoader FontLoader;
