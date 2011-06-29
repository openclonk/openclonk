/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2003-2004, 2007  Sven Eberhardt
 * Copyright (c) 2005, 2007  Günther Brammer
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
// text drawing facility for CStdDDraw

#ifndef INC_STDFONT
#define INC_STDFONT

#include <StdMarkup.h>
#include <StdFacet.h>
#include <StdBuf.h>
#include <stdio.h>
#include <map>

// Font rendering flags
#define STDFONT_CENTERED    0x0001
#define STDFONT_TWOSIDED    0x0002
#define STDFONT_FILTERED    0x0004
#define STDFONT_RIGHTALGN   0x0008
#define STDFONT_SHADOW      0x0010
#define STDFONT_NOMARKUP    0x0020

#ifndef FW_NORMAL
#define FW_NORMAL 400
#define FW_MEDIUM 500
#define FW_SEMIBOLD 600
#define FW_BOLD 700
#endif

class CMarkup;
class CFacet;
class CStdVectorFont;

class CStdFont
{
public:
	// callback class to allow custom images
	class CustomImages
	{
	protected:
		virtual bool GetFontImage(const char *szImageTag, CFacet &rOutImgFacet) = 0;

		friend class CStdFont;
	public:
		virtual ~CustomImages() { }
	};
	static CStdVectorFont * CreateFont(StdBuf & Data);
	static CStdVectorFont * CreateFont(const char *szFaceName);
	static void DestroyFont(CStdVectorFont * pFont);
public:
	int id;                // used by the engine to keep track of where the font came from

protected:
	DWORD dwDefFontHeight; // configured font size (in points)
	char szFontName[80+1]; // used font name (or surface file name)

	CSurface **psfcFontData; // font recource surfaces - additional surfaces created as needed
	int iNumFontSfcs;       // number of created font surfaces
	int iSfcSizes;          // size for font surfaces
	int iFontZoom;          // zoom of font in texture

	CSurface *sfcCurrent;  // current surface font data can be written to at runtime
	int32_t iCurrentSfcX, iCurrentSfcY; // current character rendering position

	int iHSpace; // horizontal space to be added betwen two characters
	int iGfxLineHgt; // height of chaacters; may be larger than line height
	DWORD dwWeight; // font weight (usually FW_NORMAL or FW_BOLD)
	bool fDoShadow; // if the font is shadowed

	CFacet fctAsciiTexCoords[256-' '];     // texture coordinates of ASCII letters
	std::map<uint32_t, CFacet> fctUnicodeMap; // texture coordinates of Unicode letters

	CustomImages *pCustomImages; // callback class for custom images

#if defined _WIN32 && !(defined HAVE_FREETYPE)
	HDC hDC;
	HBITMAP hbmBitmap;
	DWORD *pBitmapBits; int iBitmapSize;
	HFONT hFont;
#elif (defined HAVE_FREETYPE)
	CStdVectorFont *pVectorFont; // class assumed to be held externally!
#endif

	bool AddSurface();
	bool CheckRenderedCharSpace(uint32_t iCharWdt, uint32_t iCharHgt);
	bool AddRenderedChar(uint32_t dwChar, CFacet *pfctTarget);

	CFacet &GetCharacterFacet(uint32_t c)
	{
		if (c<128) return fctAsciiTexCoords[c-' ']; else return GetUnicodeCharacterFacet(c);
	}
	CFacet &GetUnicodeCharacterFacet(uint32_t c);

public:
	int iLineHgt;        // height of one line of font (in pixels)
	// draw ine line of text
	void DrawText(SURFACE sfcDest, float iX, float iY, DWORD dwColor, const char *szText, DWORD dwFlags, CMarkup &Markup, float fZoom);

	// get text size
	bool GetTextExtent(const char *szText, int32_t &rsx, int32_t &rsy, bool fCheckMarkup = true);
	// get height of a line
	inline int GetLineHeight() { return iLineHgt; }
	// Sometimes, only the width of a text is needed
	int32_t GetTextWidth(const char *szText, bool fCheckMarkup = true) { int32_t x, y; GetTextExtent(szText, x, y, fCheckMarkup); return x; }
	// insert line breaks into a message and return overall height - uses and regards '|' as line breaks
	int BreakMessage(const char *szMsg, int iWdt, char *szOut, int iMaxOutLen, bool fCheckMarkup, float fZoom=1.0f);
	int BreakMessage(const char *szMsg, int iWdt, class StdStrBuf *pOut, bool fCheckMarkup, float fZoom=1.0f);
	// get message break and pos after message break - does not regard any manual line breaks!
	int GetMessageBreak(const char *szMsg, const char **ppNewPos, int iBreakWidth, float fZoom=1.0f);

	// ctor
	CStdFont();
	~CStdFont() { Clear(); }

	// function throws std::runtime_error in case of failure
	// font initialization from vector font
	void Init(CStdVectorFont & VectorFont, const char *font_face_name, DWORD dwHeight, DWORD dwFontWeight=FW_NORMAL, bool fDoShadow=true);

	void Clear(); // clear font

	// query whether font is initialized
	bool IsInitialized() { return !!*szFontName; }

	// query whether font is already initialized with certain data
	bool IsSameAsID(const char *szCFontName, int iCID, int iCIndent)
	{ return SEqual(szCFontName, szFontName) && iCID==id && iCIndent==-iHSpace; }
	bool IsSameAs(const char *szCFontName, DWORD iCHeight, DWORD dwCWeight)
	{ return SEqual(szCFontName, szFontName) && !id && iCHeight==dwDefFontHeight && dwCWeight==dwWeight; }

	// set custom image request handler
	void SetCustomImages(CustomImages *pHandler)
	{ pCustomImages = pHandler; }
};

#endif // INC_STDFONT
