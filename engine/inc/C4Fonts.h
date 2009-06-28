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

class C4Group;
class C4GroupSet;
class C4Config;
class CStdFont;

// font definition to be read
class C4FontDef
	{
	public:
		StdCopyStrBuf Name;        // font name
		int32_t iSize;             // average font height of base font
		StdCopyStrBuf LogFont;     // very small font used for log messages
		StdCopyStrBuf SmallFont;   // pretty small font used in tiny dialogs
		StdCopyStrBuf Font;        // base font used for anything
		StdCopyStrBuf CaptionFont; // caption font used in GUI
		StdCopyStrBuf TitleFont;   // font used to draw the loader caption

		C4FontDef(): iSize(0) { }  // ctor
		void CompileFunc(StdCompiler * pComp);
	};

// holder class for loaded ttf fonts
class C4VectorFont
	{
	protected:
		StdStrBuf Name;
		StdBuf Data;
		CStdVectorFont * pFont;
		char FileName[_MAX_PATH+1]; // file name of temprarily extracted file
		bool fIsTempFile;           // if set, the file resides at the temp path and is to be deleted

	public:
		C4VectorFont *pNext; // next font

		C4VectorFont() : pFont(NULL), fIsTempFile(false), pNext(NULL) { *FileName=0; } // ctor
		~C4VectorFont(); // dtor - releases font and deletes temp file

		bool Init(C4Group &hGrp, const char *szFilename, C4Config &rCfg); // load font from group
		bool Init(const char *szFacename, int32_t iSize, uint32_t dwWeight); // load system font specified by face name
		void Init(const char *szName, CStdVectorFont *pFont); // init from a font that has been laoded already
		friend class C4FontLoader;
	};

// font loader
class C4FontLoader
	{
	protected:
		std::vector<C4FontDef> FontDefs; // array of loaded font definitions
		C4VectorFont *pVectorFonts; // vector fonts loaded and extracted to temp store

	public:
		// enum of different fonts used in the clonk engine
		enum FontType { C4FT_Log, C4FT_MainSmall, C4FT_Main, C4FT_Caption, C4FT_Title };

	public:
		C4FontLoader() : pVectorFonts(NULL) { } // ctor
		~C4FontLoader() { Clear(); } // dtor

		void Clear();                   // clear loaded fonts
		int32_t LoadDefs(C4Group &hGroup, C4Config &rCfg); // load font definitions from group file; return number of loaded font defs
		bool IsFontLoaded(const char *szFontName); // return whether given font name is found in the list
		const char *GetFontNameByIndex(int32_t iIndex); // get indexed font name; not doubling fonts of same name
		int32_t GetClosestAvailableSize(const char *szFontName, int32_t iWantedSize); // return possible font size that mathces the desired value closest
		void AddVectorFont(C4VectorFont *pAddFont); // adds a new font to the list

		bool InitFont(CStdFont &rFont, C4VectorFont * pFont, int32_t iSize, uint32_t dwWeight, bool fDoShadow);
		// init a font class of the given type
		// iSize is always the size of the normal font, which is adjusted for larger (title) and smaller (log) font types
		bool InitFont(CStdFont &rFont, const char *szFontName, FontType eType, int32_t iSize, C4GroupSet *pGfxGroups, bool fDoShadow=true);
	};


#endif // INC_C4Fonts
