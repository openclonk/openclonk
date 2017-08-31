/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2003-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2010-2016, The OpenClonk Team and contributors
 *
 * Distributed under the terms of the ISC license; see accompanying file
 * "COPYING" for details.
 *
 * "Clonk" is a registered trademark of Matthes Bender, used with permission.
 * See accompanying file "TRADEMARK" for details.
 *
 * To redistribute this file separately, substitute the full license texts
 * for the above references.
 */
// text drawing facility for C4Draw

#ifndef INC_STDFONT
#define INC_STDFONT

#include "C4ForbidLibraryCompilation.h"
#include "lib/C4Markup.h"
#include "graphics/C4Facet.h"
#include "graphics/C4Surface.h"
#include "graphics/C4FontLoaderCustomImages.h"

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

class C4Markup;
class CStdVectorFont;

// font loader
class C4FontLoader
{
public:
	// enum of different fonts used in the clonk engine
	enum FontType { C4FT_Log, C4FT_MainSmall, C4FT_Main, C4FT_Caption, C4FT_Title };

	C4FontLoader() = default; // ctor
	~C4FontLoader() { Clear(); } // dtor

	void Clear();                   // clear loaded fonts
	// init a font class of the given type
	// iSize is always the size of the normal font, which is adjusted for larger (title) and smaller (log) font types
	bool InitFont(CStdFont * Font, const char *szFontName, FontType eType, int32_t iSize, C4GroupSet *pGfxGroups, bool fDoShadow=true);

protected:
#ifndef USE_CONSOLE
	CStdVectorFont * pLastUsedFont{nullptr}; // cache
	StdCopyStrBuf LastUsedName;
	int32_t LastUsedGrpID{0};

	CStdVectorFont * CreateFont(StdBuf & Data);
	CStdVectorFont * CreateFont(const char *szFaceName);
	void DestroyFont(CStdVectorFont * pFont);
	friend class CStdFont;
#endif
};

extern C4FontLoader FontLoader;

class CStdFont
{
public:
	typedef CStdFontCustomImages CustomImages;
	int id;                // used by the engine to keep track of where the font came from

protected:
#ifndef USE_CONSOLE
	DWORD dwDefFontHeight; // configured font size (in points)
	char szFontName[80+1]; // used font name (or surface file name)

	std::vector<std::unique_ptr<C4Surface>> psfcFontData; // font resource surfaces - additional surfaces created as needed
	int iSfcSizes;          // size for font surfaces
	int iFontZoom;          // zoom of font in texture

	C4Surface *sfcCurrent;  // current surface font data can be written to at runtime
	int32_t iCurrentSfcX, iCurrentSfcY; // current character rendering position

	int iHSpace; // horizontal space to be added betwen two characters
	int iGfxLineHgt; // height of chaacters; may be larger than line height
	DWORD dwWeight; // font weight (usually FW_NORMAL or FW_BOLD)
	bool fDoShadow; // if the font is shadowed

	C4Facet fctAsciiTexCoords[256-' '];     // texture coordinates of ASCII letters
	std::map<uint32_t, C4Facet> fctUnicodeMap; // texture coordinates of Unicode letters

	CustomImages *pCustomImages; // callback class for custom images

	CStdVectorFont *pVectorFont; // class assumed to be held externally!

	bool AddSurface();
	bool CheckRenderedCharSpace(uint32_t iCharWdt, uint32_t iCharHgt);
	bool AddRenderedChar(uint32_t dwChar, C4Facet *pfctTarget);

	C4Facet &GetCharacterFacet(uint32_t c)
	{
		if (c<128) return fctAsciiTexCoords[c-' ']; else return GetUnicodeCharacterFacet(c);
	}
	C4Facet &GetUnicodeCharacterFacet(uint32_t c);

	int iLineHgt;        // height of one line of font (in pixels)
#endif

public:
	// draw ine line of text
	void DrawText(C4Surface * sfcDest, float iX, float iY, DWORD dwColor, const char *szText, DWORD dwFlags, C4Markup &Markup, float fZoom);

	// get text size
	bool GetTextExtent(const char *szText, int32_t &rsx, int32_t &rsy, bool fCheckMarkup = true);
	// get height of a line
	inline int GetLineHeight() const
	{
#ifdef USE_CONSOLE
		return 1;
#else
		return iLineHgt;
#endif
	}
	// get height of the font in pixels (without line spacing)
	inline int GetFontHeight() const
	{
		// Currently, we do not use spacing between lines - if someone implements that, this needs to be adjusted.
		return GetLineHeight();
	}
	// Sometimes, only the width of a text is needed
	int32_t GetTextWidth(const char *szText, bool fCheckMarkup = true) { int32_t x, y; GetTextExtent(szText, x, y, fCheckMarkup); return x; }
	// insert line breaks into a message and return overall height - uses and regards '|' as line breaks
	std::tuple<std::string, int> BreakMessage(const char *szMsg, int iWdt, bool fCheckMarkup, float fZoom=1.0f);
	int BreakMessage(const char *szMsg, int iWdt, char *szOut, int iMaxOutLen, bool fCheckMarkup, float fZoom=1.0f);
	int BreakMessage(const char *szMsg, int iWdt, StdStrBuf *pOut, bool fCheckMarkup, float fZoom=1.0f);
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
	bool IsInitialized() const {
#ifdef USE_CONSOLE
		return true;
#else
		return !!*szFontName;
#endif
	}

	// query whether font is already initialized with certain data
	bool IsSameAsID(const char *szCFontName, int iCID, int iCIndent) const
	{
#ifdef USE_CONSOLE
		return true;
#else
		return SEqual(szCFontName, szFontName) && iCID==id && iCIndent==-iHSpace;
#endif
	}
	bool IsSameAs(const char *szCFontName, DWORD iCHeight, DWORD dwCWeight) const
	{
#ifdef USE_CONSOLE
		return true;
#else
		return SEqual(szCFontName, szFontName) && !id && iCHeight==dwDefFontHeight && dwCWeight==dwWeight;
#endif
	}

	// set custom image request handler
	void SetCustomImages(CustomImages *pHandler)
	{
#ifndef USE_CONSOLE
		pCustomImages = pHandler;
#endif
	}

	bool GetFontImageSize(const char* szTag, int& width, int& height) const;
};

#endif // INC_STDFONT
