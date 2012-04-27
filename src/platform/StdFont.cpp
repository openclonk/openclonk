/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2003-2007  Sven Eberhardt
 * Copyright (c) 2005-2007, 2009-2010  Günther Brammer
 * Copyright (c) 2008  Matthes Bender
 * Copyright (c) 2009  Armin Burgmeier
 * Copyright (c) 2009-2010  Nicolas Hake
 * Copyright (c) 2010  Benjamin Herr
 * Copyright (c) 2003-2009, RedWolf Design GmbH, http://www.clonk.de
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

// text drawing facility for C4Draw

#include "C4Include.h"
#include <StdFont.h>

#include "Standard.h"
#include <StdBuf.h>
#include <C4Draw.h>
#include <C4Surface.h>
#include <C4Markup.h>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#include <C4windowswrapper.h>
#endif

#ifdef HAVE_FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H
#endif // HAVE_FREETYPE

/* Initialization */

#ifdef HAVE_FREETYPE
class CStdVectorFont
{
	FT_Library library;
	FT_Face face;
	StdBuf Data;
public:
	CStdVectorFont(const char * FontFaceName): RefCnt(1)
	{
#if defined(_WIN32) && defined(HAVE_FREETYPE)
	// Win32 using freetype: Load TrueType-data from WinGDI into Data-buffer to be used by FreeType
	bool fSuccess = false;
	HDC hDC = ::CreateCompatibleDC(NULL);
	if (hDC)
	{
		HFONT hFont = ::CreateFontA(0, 0, 0, 0, FW_DONTCARE, false,
		                           false, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
		                           CLIP_DEFAULT_PRECIS, 5,
		                           VARIABLE_PITCH, FontFaceName);
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
	if (!fSuccess)
		throw std::runtime_error("Some Win32 error");
	// Initialize Freetype
	if (FT_Init_FreeType(&library))
		throw std::runtime_error("Cannot init Freetype");
	// Load the font
	FT_Error e;
	if ((e=FT_New_Memory_Face(library, static_cast<const FT_Byte *>(Data.getData()), Data.getSize(), 0, &face)))
		throw std::runtime_error(std::string("Cannot load font: ") + FormatString("%d",e).getData());
#else
		//FIXME: get path name from OS
		// Initialize Freetype
		if (FT_Init_FreeType(&library))
			throw std::runtime_error("Cannot init Freetype");
		// Load the font
		FT_Error e;
		if ((e=FT_New_Face(library, FontFaceName, 0, &face)))
			throw std::runtime_error(std::string("Cannot load ") + FontFaceName + ": " + FormatString("%d",e).getData());
#endif
	}
	CStdVectorFont(StdBuf & Data): RefCnt(1), Data(Data)
	{
		// Initialize Freetype
		if (FT_Init_FreeType(&library))
			throw std::runtime_error("Cannot init Freetype");
		// Load the font
		FT_Error e;
		if ((e=FT_New_Memory_Face(library, static_cast<const FT_Byte *>(Data.getData()), Data.getSize(), 0, &face)))
			throw std::runtime_error(std::string("Cannot load font: ") + FormatString("%d",e).getData());
	}
	~CStdVectorFont()
	{
		FT_Done_Face(face);
		FT_Done_FreeType(library);
	}
	operator FT_Face () { return face; }
	FT_Face operator -> () { return face; }
	int RefCnt;
};

CStdVectorFont * CStdFont::CreateFont(const char *szFaceName)
{
	return new CStdVectorFont(szFaceName);
}
CStdVectorFont * CStdFont::CreateFont(StdBuf & Data)
{
	return new CStdVectorFont(Data);
}
void CStdFont::DestroyFont(CStdVectorFont * pFont)
{
	if (!pFont) return;
	--(pFont->RefCnt);
	if (!pFont->RefCnt)
		delete pFont;
}
#else
CStdVectorFont * CStdFont::CreateFont(StdBuf & Data)
{
	return 0;
}
CStdVectorFont * CStdFont::CreateFont(const char *szFaceName)
{
	return 0;
}
void CStdFont::DestroyFont(CStdVectorFont * pFont)
{
}
#endif

CStdFont::CStdFont()
{
	// set default values
	psfcFontData = NULL;
	sfcCurrent = NULL;
	iNumFontSfcs = 0;
	iSfcSizes = 64;
	dwDefFontHeight=iLineHgt=10;
	iFontZoom=1; // default: no internal font zooming - likely no antialiasing either...
	iHSpace=-1;
	iGfxLineHgt=iLineHgt+1;
	dwWeight=FW_NORMAL;
	fDoShadow=false;
	// font not yet initialized
	*szFontName=0;
	id=0;
	pCustomImages=NULL;
#if defined HAVE_FREETYPE
	pVectorFont = NULL;
#endif
}

bool CStdFont::AddSurface()
{
	// add new surface as render target; copy old ones
	C4Surface **pNewSfcs = new C4Surface *[iNumFontSfcs+1];
	if (iNumFontSfcs) memcpy(pNewSfcs, psfcFontData, iNumFontSfcs * sizeof (C4Surface *));
	delete [] psfcFontData;
	psfcFontData = pNewSfcs;
	C4Surface *sfcNew = psfcFontData[iNumFontSfcs] = new C4Surface();
	++iNumFontSfcs;
	if (iSfcSizes) if (!sfcNew->Create(iSfcSizes, iSfcSizes)) return false;
	// If old surface was locked, unlock it and lock the new one in its stead
	if (sfcCurrent && sfcCurrent->IsLocked())
	{
		sfcCurrent->Unlock();
		sfcNew->Lock();
	}
	sfcCurrent = sfcNew;
	iCurrentSfcX = iCurrentSfcY = 0;
	return true;
}

bool CStdFont::CheckRenderedCharSpace(uint32_t iCharWdt, uint32_t iCharHgt)
{
	// need to do a line break?
	if (iCurrentSfcX + iCharWdt >= (uint32_t)iSfcSizes) if (iCurrentSfcX)
		{
			iCurrentSfcX = 0;
			iCurrentSfcY += iCharHgt;
			if (iCurrentSfcY + iCharHgt >= (uint32_t)iSfcSizes)
			{
				// surface is full: Next one
				if (!AddSurface()) return false;
			}
		}
	// OK draw it there
	return true;
}

bool CStdFont::AddRenderedChar(uint32_t dwChar, C4Facet *pfctTarget)
{
#if defined HAVE_FREETYPE
	if (!pVectorFont) return false;
	// Freetype character rendering
	FT_Set_Pixel_Sizes(*pVectorFont, dwDefFontHeight, dwDefFontHeight);
	int32_t iBoldness = dwWeight-400; // zero is normal; 300 is bold
	if (iBoldness)
	{
		iBoldness = (1<<16) + (iBoldness<<16)/400;
		FT_Matrix mat;
		mat.xx = iBoldness; mat.xy = mat.yx = 0; mat.yy = 1<<16;
		//.*(100 + iBoldness/3)/100
		FT_Set_Transform(*pVectorFont, &mat, NULL);
	}
	else
	{
		FT_Set_Transform(*pVectorFont, NULL, NULL);
	}
	// Render
	if (FT_Load_Char(*pVectorFont, dwChar, FT_LOAD_RENDER | FT_LOAD_NO_HINTING))
	{
		// although the character was not drawn, assume it's not in the font and won't be needed
		// so return success here
		return true;
	}
	// Make a shortcut to the glyph
	FT_GlyphSlot slot = (*pVectorFont)->glyph;
	if (slot->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY)
	{
		// although the character was drawn in a strange way, assume it's not in the font and won't be needed
		// so return success here
		return true;
	}
	// linebreak/ new surface check
	int width = Max<int>(slot->advance.x / 64, Max(slot->bitmap_left,0) + slot->bitmap.width) + fDoShadow;
	if (!CheckRenderedCharSpace(width, iGfxLineHgt)) return false;
	// offset from the top
	int at_y = iCurrentSfcY + dwDefFontHeight * (*pVectorFont)->ascender / (*pVectorFont)->units_per_EM - slot->bitmap_top;
	int at_x = iCurrentSfcX + Max(slot->bitmap_left,0);
	// Copy to the surface
	if (!sfcCurrent->Lock()) return false;
	for (int y = 0; y < slot->bitmap.rows + fDoShadow; ++y)
	{
		for (int x = 0; x < slot->bitmap.width + fDoShadow; ++x)
		{
			unsigned char bAlpha, bAlphaShadow;
			if (x < slot->bitmap.width && y < slot->bitmap.rows)
				bAlpha = (unsigned char)(slot->bitmap.buffer[slot->bitmap.width * y + x]);
			else
				bAlpha = 0;
			// Make a shadow from the upper-left pixel, and blur with the eight neighbors
			DWORD dwPixVal = 0u;
			bAlphaShadow = 0;
			if ((x || y) && fDoShadow)
			{
				int iShadow = 0;
				if (x < slot->bitmap.width && y < slot->bitmap.rows) iShadow += slot->bitmap.buffer[(x - 0) + slot->bitmap.width * (y - 0)];
				if (x > 1                  && y < slot->bitmap.rows) iShadow += slot->bitmap.buffer[(x - 2) + slot->bitmap.width * (y - 0)];
				if (x > 0                  && y < slot->bitmap.rows) iShadow += slot->bitmap.buffer[(x - 1) + slot->bitmap.width * (y - 0)];
				if (x < slot->bitmap.width && y > 1                ) iShadow += slot->bitmap.buffer[(x - 0) + slot->bitmap.width * (y - 2)];
				if (x > 1                  && y > 1                ) iShadow += slot->bitmap.buffer[(x - 2) + slot->bitmap.width * (y - 2)];
				if (x > 0                  && y > 1                ) iShadow += slot->bitmap.buffer[(x - 1) + slot->bitmap.width * (y - 2)];
				if (x < slot->bitmap.width && y > 0                ) iShadow += slot->bitmap.buffer[(x - 0) + slot->bitmap.width * (y - 1)];
				if (x > 1                  && y > 0                ) iShadow += slot->bitmap.buffer[(x - 2) + slot->bitmap.width * (y - 1)];
				if (x > 0                  && y > 0                ) iShadow += slot->bitmap.buffer[(x - 1) + slot->bitmap.width * (y - 1)]*8;
				bAlphaShadow += iShadow / 16;
				// because blitting on a black pixel reduces luminosity as compared to shadowless font,
				// assume luminosity as if blitting shadowless font on a 50% gray background
				unsigned char cBack = bAlpha;
				dwPixVal = RGBA(cBack/2, cBack/2, cBack/2, bAlphaShadow);
			}
			BltAlpha(dwPixVal, bAlpha << 24 | 0xffffff);
			sfcCurrent->SetPixDw(at_x + x, at_y + y, dwPixVal);
		}
	}
	sfcCurrent->Unlock();
	// Save the position of the glyph for the rendering code
	pfctTarget->Set(sfcCurrent, iCurrentSfcX, iCurrentSfcY, width, iGfxLineHgt);
#endif  // end of freetype rendering - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	// advance texture position
	iCurrentSfcX += pfctTarget->Wdt;
	return true;
}

C4Facet &CStdFont::GetUnicodeCharacterFacet(uint32_t c)
{
	// find/add facet in map
	C4Facet &rFacet = fctUnicodeMap[c];
	// create character on the fly if necessary and possible
	if (!rFacet.Surface) AddRenderedChar(c, &rFacet);
	// rendering might have failed, in which case rFacet remains empty. Should be OK; char won't be printed then
	return rFacet;
}

void CStdFont::Init(CStdVectorFont & VectorFont, const char *font_face_name, DWORD dwHeight, DWORD dwFontWeight, bool fDoShadow)
{
	// clear previous
	Clear();
	// set values
	iHSpace=fDoShadow ? -1 : 0;             // horizontal shadow
	dwWeight=dwFontWeight;
	this->fDoShadow = fDoShadow;
	// determine needed texture size
	if (dwHeight * iFontZoom > 40)
		iSfcSizes = 512;
	else if (dwDefFontHeight * iFontZoom > 20)
		iSfcSizes = 256;
	else
		iSfcSizes = 128;
	SCopy(font_face_name, szFontName, 80);
	dwDefFontHeight = dwHeight;
	// create surface
	if (!AddSurface())
	{
		Clear();
		throw std::runtime_error(std::string("Cannot create surface (") + szFontName + ")");
	}

#if defined HAVE_FREETYPE

	// Store vector font
	pVectorFont = &VectorFont;
	++(pVectorFont->RefCnt);
	// Get size
	// FIXME: use bbox or dynamically determined line heights here
	iLineHgt = (VectorFont->ascender - VectorFont->descender) * dwHeight / VectorFont->units_per_EM;
	iGfxLineHgt = iLineHgt + fDoShadow; // vertical shadow
#else
	throw std::runtime_error("You have a engine without Truetype support.");
#endif // HAVE_FREETYPE

	// loop through all ASCII printable characters and prepare them
	// Non-ASCII Unicode characters will be created on the fly
	// now render all characters!

	int cMax = 127;
	sfcCurrent->Lock();
	for (int c=' '; c <= cMax; ++c)
	{
		if (!AddRenderedChar(c, &(fctAsciiTexCoords[c-' '])))
		{
			sfcCurrent->Unlock();
			Clear();
			throw std::runtime_error(std::string("Cannot render characters for Font (") + szFontName + ")");
		}
	}
	sfcCurrent->Unlock();
	// adjust line height
	iLineHgt /= iFontZoom;
	// font successfully created; set name
	//SCopy(szFontName2, this->szFontName, 80);
	if (0) for (int i = 0; i < iNumFontSfcs; ++i)
		{
			StdStrBuf pngfilename = FormatString("%s%u%s_%d.png",szFontName,static_cast<unsigned int>(dwHeight),fDoShadow ? "_shadow" : "",i);
			psfcFontData[i]->SavePNG(pngfilename.getData(), true, false, false);
		}
}

void CStdFont::Clear()
{
#if defined HAVE_FREETYPE
	DestroyFont(pVectorFont);
	pVectorFont = NULL;
#endif
	// clear font sfcs
	if (psfcFontData)
	{
		while (iNumFontSfcs--) delete psfcFontData[iNumFontSfcs];
		delete [] psfcFontData;
		psfcFontData = NULL;
	}
	sfcCurrent = NULL;
	iNumFontSfcs = 0;
	for (int c=' '; c<256; ++c) fctAsciiTexCoords[c-' '].Default();
	fctUnicodeMap.clear();
	// set default values
	dwDefFontHeight=iLineHgt=10;
	iFontZoom=1; // default: no internal font zooming - likely no antialiasing either...
	iHSpace=-1;
	iGfxLineHgt=iLineHgt+1;
	dwWeight=FW_NORMAL;
	fDoShadow=false;
	// font not yet initialized
	*szFontName=0;
	id=0;
}

/* Text size measurement */
bool CStdFont::GetTextExtent(const char *szText, int32_t &rsx, int32_t &rsy, bool fCheckMarkup)
{
	// safety
	if (!szText) return false;
	assert(IsValidUtf8(szText));
	// keep track of each row's size
	int iRowWdt=0,iWdt=0,iHgt=iLineHgt;
	// ignore any markup
	C4Markup MarkupChecker(false);
	// go through all text
	while (*szText)
	{
		// ignore markup
		if (fCheckMarkup) MarkupChecker.SkipTags(&szText);
		// get current char
		uint32_t c = GetNextCharacter(&szText);
		// done? (must check here, because markup-skip may have led to text end)
		if (!c) break;
		// line break?
		if (c == '\n' || (fCheckMarkup && c == '|')) { iRowWdt=0; iHgt+=iLineHgt; continue; }
		// ignore system characters
		if (c < ' ') continue;
		// image?
		int iImgLgt;
		if (fCheckMarkup && c=='{' && szText[0]=='{' && szText[1]!='{' && (iImgLgt=SCharPos('}', szText+1))>0 && szText[iImgLgt+2]=='}')
		{
			char imgbuf[101];
			SCopy(szText+1, imgbuf, Min(iImgLgt, 100));

			int w2, h2;
			if(!GetFontImageSize(imgbuf, w2, h2))
				{ w2 = 0; h2 = 0; }

			iRowWdt += w2;
			// skip image tag
			szText+=iImgLgt+3;
		}
		else
		{
			// regular char
			// look up character width in texture coordinates table
			iRowWdt += GetCharacterFacet(c).Wdt / iFontZoom;
		}
		// apply horizontal indent for all but last char
		if (*szText) iRowWdt += iHSpace;
		// adjust max row size
		if (iRowWdt>iWdt) iWdt=iRowWdt;
	}
	// store output
	rsx=iWdt; rsy=iHgt;
	// done, success
	return true;
}

int CStdFont::BreakMessage(const char *szMsg, int iWdt, char *szOut, int iMaxOutLen, bool fCheckMarkup, float fZoom)
{
	// 2do: Implement this in terms of StdStrBuf-version
	// note iMaxOutLen does not include terminating null character
	// safety
	if (!iMaxOutLen) return 0;
	if (!szMsg)
	{
		if (szOut) *szOut=0;
		return 0;
	}

	// TODO: might szLastBreakOut, szLastEmergencyBreakPos, szLastEmergencyBreakOut, iXEmergencyBreak not be properly initialised before use?
	uint32_t c;
	const char *szPos=szMsg,   // current parse position in the text
	                  *szLastBreakPos = szMsg, // points to the char after at (whitespace) or after ('-') which text can be broken
	                                    *szLastEmergenyBreakPos = NULL, // same, but at last char in case no suitable linebreak could be found
	                                                              *szLastPos;              // last position until which buffer has been transferred to output
	char *szLastBreakOut = NULL, *szLastEmergencyBreakOut = NULL; // position of output pointer at break positions
	int iX=0,      // current text width at parse pos
	       iXBreak=0, // text width as it was at last break pos
	               iXEmergencyBreak = 0, // same, but at last char in case no suitable linebreak could be found
	                                  iHgt=iLineHgt; // total height of output text
	bool fIsFirstLineChar = true;
	// ignore any markup
	C4Markup MarkupChecker(false);
	// go through all text
	while (*(szLastPos = szPos))
	{
		// ignore markup
		if (fCheckMarkup) MarkupChecker.SkipTags(&szPos);
		// get current char
		c = GetNextCharacter(&szPos);
		// done? (must check here, because markup-skip may have led to text end)
		if (!c) break;
		// manual break?
		int iCharWdt = 0;
		if (c != '\n' && (!fCheckMarkup || c != '|'))
		{
			// image?
			int iImgLgt;
			if (fCheckMarkup && c=='{' && szPos[0]=='{' && szPos[1]!='{' && (iImgLgt=SCharPos('}', szPos+1))>0 && szPos[iImgLgt+2]=='}')
			{
				char imgbuf[101];
				SCopy(szPos+1, imgbuf, Min(iImgLgt, 100));

				int iCharHgt;
				if(!GetFontImageSize(imgbuf, iCharWdt, iCharHgt))
					iCharWdt = 0;

				// skip image tag
				szPos+=iImgLgt+3;
			}
			else
			{
				// regular char
				// look up character width in texture coordinates table
				if (c >= ' ')
					iCharWdt = int(fZoom * GetCharacterFacet(c).Wdt / iFontZoom) + iHSpace;
				else
					iCharWdt = 0; // OMFG ctrl char
			}
			// add chars to output
			while (szLastPos != szPos)
			{
				if (szOut)
					*szOut++ = *szLastPos++;
				else
					++szLastPos;
				if (szOut && !--iMaxOutLen)
				{
					// buffer end: cut and terminate
					*szOut = '\0';
					break;
				}
			}
			if (szOut && !iMaxOutLen) break;
			// add to line; always add one char at minimum
			if ((iX+=iCharWdt) <= iWdt || fIsFirstLineChar)
			{
				// check whether linebreak possibility shall be marked here
				// 2do: What about unicode-spaces?
				if (c<256) if (isspace((unsigned char)c) || c == '-')
					{
						szLastBreakPos = szPos;
						szLastBreakOut = szOut;
						// space: Break directly at space if it isn't the first char here
						// first char spaces must remain, in case the output area is just one char width
						if (c != '-' && !fIsFirstLineChar) --szLastBreakPos; // because c<256, the character length can be safely assumed to be 1 here
						iXBreak = iX;
					}
				// always mark emergency break after char that fitted the line
				szLastEmergenyBreakPos = szPos;
				iXEmergencyBreak = iX;
				szLastEmergencyBreakOut = szOut;
				// line OK; continue filling it
				fIsFirstLineChar = false;
				continue;
			}
			// line must be broken now
			// if there was no linebreak, do it at emergency pos
			if (szLastBreakPos == szMsg)
			{
				szLastBreakPos = szLastEmergenyBreakPos;
				szLastBreakOut = szLastEmergencyBreakOut;
				iXBreak = iXEmergencyBreak;
			}
			// insert linebreak at linebreak pos
			// was it a space? Then just overwrite space with a linebreak
			if (uint8_t(*szLastBreakPos)<128 && isspace((unsigned char)*szLastBreakPos))
				*(szLastBreakOut-1) = '\n';
			else
			{
				// otherwise, insert line break
				if (szOut && !--iMaxOutLen)
					// buffer is full
					break;
				if (szOut)
				{
					char *szOut2 = szOut;
					while (--szOut2 >= szLastBreakOut)
						szOut2[1] = *szOut2;
					szOut2[1] = '\n';
				}
			}
			// calc next line usage
			iX -= iXBreak;
		}
		else
		{
			// a static linebreak: Everything's well; this just resets the line width
			iX = 0;
		}
		// forced or manual line break: set new line beginning to char after line break
		szLastBreakPos = szMsg = szPos;
		// manual line break or line width overflow: add char to next line
		iHgt += iLineHgt;
		fIsFirstLineChar = true;
	}
	// transfer final data to buffer - markup and terminator
	if (szOut)
		while ((*szOut++ = *szLastPos++))
			if (!--iMaxOutLen)
			{
				// buffer end: cut and terminate
				*szOut = '\0';
				break;
			}
	// return text height
	return iHgt;
}

int CStdFont::BreakMessage(const char *szMsg, int iWdt, StdStrBuf *pOut, bool fCheckMarkup, float fZoom)
{
	// safety
	if (!szMsg || !pOut) return 0;
	pOut->Clear();
	// TODO: might szLastEmergenyBreakPos, iLastBreakOutLen or iXEmergencyBreak not be properly initialised before use?
	uint32_t c;
	const char *szPos=szMsg,   // current parse position in the text
	                  *szLastBreakPos = szMsg, // points to the char after at (whitespace) or after ('-') which text can be broken
	                                    *szLastEmergenyBreakPos = NULL, // same, but at last char in case no suitable linebreak could be found
	                                                              *szLastPos;              // last position until which buffer has been transferred to output
	int iLastBreakOutLen = 0, iLastEmergencyBreakOutLen = 0; // size of output string at break positions
	int iX=0,      // current text width at parse pos
	       iXBreak=0, // text width as it was at last break pos
	               iXEmergencyBreak = 0, // same, but at last char in case no suitable linebreak could be found
	                                  iHgt=iLineHgt; // total height of output text
	bool fIsFirstLineChar = true;
	// ignore any markup
	C4Markup MarkupChecker(false);
	// go through all text
	while (*(szLastPos = szPos))
	{
		// ignore markup
		if (fCheckMarkup) MarkupChecker.SkipTags(&szPos);
		// get current char
		c = GetNextCharacter(&szPos);
		// done? (must check here, because markup-skip may have led to text end)
		if (!c) break;
		// manual break?
		int iCharWdt = 0;
		if (c != '\n' && (!fCheckMarkup || c != '|'))
		{
			// image?
			int iImgLgt;
			if (fCheckMarkup && c=='{' && szPos[0]=='{' && szPos[1]!='{' && (iImgLgt=SCharPos('}', szPos+1))>0 && szPos[iImgLgt+2]=='}')
			{
				char imgbuf[101];
				SCopy(szPos+1, imgbuf, Min(iImgLgt, 100));

				int iCharHgt;
				if(!GetFontImageSize(imgbuf, iCharWdt, iCharHgt))
					iCharWdt = 0;

				// skip image tag
				szPos+=iImgLgt+3;
			}
			else
			{
				// regular char
				// look up character width in texture coordinates table
				if (c >= ' ')
					iCharWdt = int(fZoom * GetCharacterFacet(c).Wdt / iFontZoom) + iHSpace;
				else
					iCharWdt = 0; // OMFG ctrl char
			}
			// add chars to output
			pOut->Append(szLastPos, szPos - szLastPos);
			// add to line; always add one char at minimum
			if ((iX+=iCharWdt) <= iWdt || fIsFirstLineChar)
			{
				// check whether linebreak possibility shall be marked here
				// 2do: What about unicode-spaces?
				if (c<256) if (isspace((unsigned char)c) || c == '-')
					{
						szLastBreakPos = szPos;
						iLastBreakOutLen = pOut->getLength();
						// space: Break directly at space if it isn't the first char here
						// first char spaces must remain, in case the output area is just one char width
						if (c != '-' && !fIsFirstLineChar) --szLastBreakPos; // because c<256, the character length can be safely assumed to be 1 here
						iXBreak = iX;
					}
				// always mark emergency break after char that fitted the line
				szLastEmergenyBreakPos = szPos;
				iXEmergencyBreak = iX;
				iLastEmergencyBreakOutLen = pOut->getLength();
				// line OK; continue filling it
				fIsFirstLineChar = false;
				continue;
			}
			// line must be broken now
			// check if a linebreak is possible directly here, because it's a space
			// only check for space and not for other breakable characters (such as '-'), because the break would happen after those characters instead of at them
			if (c<128 && isspace((unsigned char)c))
			{
				szLastBreakPos = szPos-1;
				iLastBreakOutLen = pOut->getLength();
				iXBreak = iX;
			}
			// if there was no linebreak, do it at emergency pos
			else if (szLastBreakPos == szMsg)
			{
				szLastBreakPos = szLastEmergenyBreakPos;
				iLastBreakOutLen = iLastEmergencyBreakOutLen;
				iXBreak = iXEmergencyBreak;
			}
			// insert linebreak at linebreak pos
			// was it a space? Then just overwrite space with a linebreak
			if (uint8_t(*szLastBreakPos)<128 && isspace((unsigned char)*szLastBreakPos))
				*pOut->getMPtr(iLastBreakOutLen-1) = '\n';
			else
			{
				// otherwise, insert line break
				pOut->InsertChar('\n', iLastBreakOutLen);
			}
			// calc next line usage
			iX -= iXBreak;
		}
		else
		{
			// a static linebreak: Everything's well; this just resets the line width
			iX = 0;
			// add to output
			pOut->Append(szLastPos, szPos - szLastPos);
		}
		// forced or manual line break: set new line beginning to char after line break
		szLastBreakPos = szMsg = szPos;
		// manual line break or line width overflow: add char to next line
		iHgt += iLineHgt;
		fIsFirstLineChar = true;
	}
	// transfer final data to buffer (any missing markup)
	pOut->Append(szLastPos, szPos - szLastPos);
	// return text height
	return iHgt;
}

// get message break and pos after message break
// 2do: Function not ready for UTF-8, markup or inline images. Remove its usage using standardized BreakMessage
int CStdFont::GetMessageBreak(const char *szMsg, const char **ppNewPos, int iBreakWidth, float fZoom)
{
	// safety
	if (!szMsg || !*szMsg) { *ppNewPos = szMsg; return 0; }
	const char *szPos = szMsg; unsigned char c;
	int iWdt = 0; int iPos = 0;
	// check all message until it's too wide
	while ((c = *szPos++))
	{
		++iPos;
		// get char width
		int iCharWdt = int(fZoom * fctAsciiTexCoords[c-' '].Wdt / iFontZoom) + iHSpace;
		// add to overall line width
		iWdt += iCharWdt;
		// next char only if the line didn't overflow
		if (iWdt > iBreakWidth) break;
	}
	// did it all fit?
	if (!c)
	{
		// all OK then; use all the buffer
		*ppNewPos = szPos-1;
		return iPos;
	}
	// line must be broken - trace back until first break char
	// szPos2 will be first char of next line
	const char *szPos2 = szPos-1; int i=0;
	while ((!i++ || *szPos2 != '-') && *szPos2 != ' ')
		if (szPos2 == szMsg)
		{
			// do not go past beginning of line
			// then better print out an unfitting break
			szPos2 = szPos-1;
			break;
		}
		else
			--szPos2;
	// but do print at least one char
	if (szPos2 <= szMsg) szPos2 = szMsg+1;
	// assign next line start pos - skip spaces
	*ppNewPos = szPos2;
	if (*szPos2 == ' ') ++*ppNewPos;
	// return output string length
	return szPos2 - szMsg;
}




/* Text drawing */


void CStdFont::DrawText(C4Surface * sfcDest, float iX, float iY, DWORD dwColor, const char *szText, DWORD dwFlags, C4Markup &Markup, float fZoom)
{
	assert(IsValidUtf8(szText));
	C4DrawTransform bt, *pbt=NULL;
	// set blit color
	DWORD dwOldModClr;
	bool fWasModulated = pDraw->GetBlitModulation(dwOldModClr);
	if (fWasModulated) ModulateClr(dwColor, dwOldModClr);
	// get alpha fade percentage
	DWORD dwAlphaMod = Min<uint32_t>(((dwColor>>0x18)*0xff)/0xaf, 255)<<0x18 | 0xffffff;

	/*  char TEXT[8192];
	  sprintf(TEXT, "%s(%x-%x-%x)", szText, dwAlphaMod>>0x18, dwColor>>0x15, (((int)(dwColor>>0x15)-0x50)*0xff)/0xaf); szText=TEXT;*/
	// adjust text starting position (horizontal only)
	if (dwFlags & STDFONT_CENTERED)
	{
		// centered
		int32_t sx,sy;
		GetTextExtent(szText, sx,sy, !(dwFlags & STDFONT_NOMARKUP));
		sx = int(fZoom*sx); sy = int(fZoom*sy);
		iX-=sx/2;
	}
	else if (dwFlags & STDFONT_RIGHTALGN)
	{
		// right-aligned
		int32_t sx,sy;
		GetTextExtent(szText, sx,sy, !(dwFlags & STDFONT_NOMARKUP));
		sx = int(fZoom*sx); sy = int(fZoom*sy);
		iX-=sx;
	}
	// apply texture zoom
	fZoom /= iFontZoom;
	// set start markup transformation
	if (!Markup.Clean()) pbt=&bt;
	// output text
	uint32_t c;
	C4Facet fctFromBlt; // source facet
	while ((c = GetNextCharacter(&szText)))
	{
		// ignore system characters
		if (c < ' ') continue;
		// apply markup
		if (c=='<' && (~dwFlags & STDFONT_NOMARKUP))
		{
			// get tag
			if (Markup.Read(&--szText))
			{
				// mark transform to be done
				// (done only if tag was found, so most normal blits don't init a trasnformation matrix)
				pbt=&bt;
				// skip the tag
				continue;
			}
			// invalid tag: render it as text
			++szText;
		}
		int w2, h2; // dst width/height
		// custom image?
		int iImgLgt;
		char imgbuf[101] = "";
		if (c=='{' && szText[0]=='{' && szText[1]!='{' && (iImgLgt=SCharPos('}', szText+1))>0 && szText[iImgLgt+2]=='}' && !(dwFlags & STDFONT_NOMARKUP))
		{
			SCopy(szText+1, imgbuf, Min(iImgLgt, 100));
			szText+=iImgLgt+3;
			if(!GetFontImageSize(imgbuf, w2, h2))
				continue;
			//normal: not modulated, unless done by transform or alpha fadeout
			if ((dwColor>>0x18) >= 0xaf)
				pDraw->DeactivateBlitModulation();
			else
				pDraw->ActivateBlitModulation((dwColor&0xff000000) | 0xffffff);
		}
		else
		{
			// regular char
			// get texture coordinates
			fctFromBlt = GetCharacterFacet(c);
			if(!fctFromBlt.Surface) continue;
			w2=int(fctFromBlt.Wdt*fZoom); h2=int(fctFromBlt.Hgt*fZoom);
			pDraw->ActivateBlitModulation(dwColor);
		}
		// do color/markup
		if (pbt)
		{
			// reset data to be transformed by markup
			DWORD dwBlitClr = dwColor;
			bt.Set(1,0,0,0,1,0,0,0,1);
			// apply markup
			Markup.Apply(bt, dwBlitClr);
			if (dwBlitClr != dwColor) ModulateClrA(dwBlitClr, dwAlphaMod);
			pDraw->ActivateBlitModulation(dwBlitClr);
			// move transformation center to center of letter
			float fOffX=(float) w2/2 + iX;
			float fOffY=(float) h2/2 + iY;
			bt.mat[2] += fOffX - fOffX*bt.mat[0] - fOffY*bt.mat[1];
			bt.mat[5] += fOffY - fOffX*bt.mat[3] - fOffY*bt.mat[4];
		}
		if(imgbuf[0])
		{
			C4Facet fct;
			fct.Set(sfcDest, iX, iY + (iGfxLineHgt - h2)/2.0f, w2, h2);
			pCustomImages->DrawFontImage(imgbuf, fct, pbt);
		}
		else
		{
			// blit character
			pDraw->Blit(fctFromBlt.Surface, float(fctFromBlt.X), float(fctFromBlt.Y), float(fctFromBlt.Wdt),float(fctFromBlt.Hgt),
				           sfcDest, iX, iY, float(w2), float(h2),
				           true, pbt);
		}
		// advance pos and skip character indent
		iX+=w2+iHSpace;
	}
	// reset blit modulation
	if (fWasModulated)
		pDraw->ActivateBlitModulation(dwOldModClr);
	else
		pDraw->DeactivateBlitModulation();
}

bool CStdFont::GetFontImageSize(const char* szTag, int& width, int& height) const
{
	const float aspect = pCustomImages ? pCustomImages->GetFontImageAspect(szTag) : -1.0f;

	// aspect < 0 means there is no such image
	if (aspect < 0.0f) return false;

	// image found: adjust aspect by font height and calc appropriate width
	height = iGfxLineHgt;
	width = static_cast<int>(height * aspect + 0.5f);

	// make images not ridiciously wide
	if(width > height)
	{
		float scale = static_cast<float>(height)/static_cast<float>(width);

		width = height;//static_cast<int32_t>(width*scale + 0.5f);
		height = static_cast<int32_t>(height*scale + 0.5f);
	}

	return true;
}
