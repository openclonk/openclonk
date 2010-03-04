/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2002-2003  Sven Eberhardt
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
// markup tags for fonts

#ifndef INC_STDMARKUP
#define INC_STDMARKUP

struct FONT2DVERTEX;

class CBltTransform;

// one markup tag
class CMarkupTag
	{
	public:
		CMarkupTag *pPrev, *pNext;

		CMarkupTag(): pPrev(0), pNext(0) { };	// ctor
		virtual ~CMarkupTag() { };		// dtor

		virtual void Apply(CBltTransform &rBltTrf, bool fDoClr, DWORD &dwClr)=0; // assign markup
		virtual const char *TagName()=0;	// get character string for this tag
	};

// markup tag for italic text
class CMarkupTagItalic : public CMarkupTag
	{
	public:
		CMarkupTagItalic() : CMarkupTag() { } // ctor

		virtual void Apply(CBltTransform &rBltTrf, bool fDoClr, DWORD &dwClr); // assign markup
		virtual const char *TagName() { return "i"; }
	};

// markup tag for colored text
class CMarkupTagColor : public CMarkupTag
	{
	private:
		DWORD dwClr;		// color
	public:
		CMarkupTagColor(DWORD dwClr) : CMarkupTag(), dwClr(dwClr) { } // ctor

		virtual void Apply(CBltTransform &rBltTrf, bool fDoClr, DWORD &dwClr); // assign markup
		virtual const char *TagName() { return "c"; }
	};

// markup rendering functionality for text
class CMarkup
	{
	private:
		CMarkupTag *pTags, *pLast;		// tag list; single linked
		bool fDoClr;									// set if color changes should be made (not in text shadow!)

		void Push(CMarkupTag *pTag)
			{ if ((pTag->pPrev=pLast)) pLast->pNext=pTag; else pTags=pTag; pLast=pTag; }
		CMarkupTag *Pop()
			{ CMarkupTag *pL=pLast; if (!pL) return 0; if ((pLast=pL->pPrev)) pLast->pNext=0; else pTags=0; return pL; }
	public:
		CMarkup(bool fDoClr) { pTags=pLast=0; this->fDoClr=fDoClr; };		// ctor
		~CMarkup() // dtor
			{ CMarkupTag *pTag=pTags,*pNext; while (pTag) { pNext=pTag->pNext; delete pTag; pTag=pNext; } }

		bool Read(const char **ppText, bool fSkip=false);		// get markup from text
		bool SkipTags(const char **ppText);	// extract markup from text; return whether end is reached
		void Apply(CBltTransform &rBltTrf, DWORD &dwClr)	// assign markup to vertices
			{ for (CMarkupTag *pTag=pTags; pTag; pTag=pTag->pNext) pTag->Apply(rBltTrf, fDoClr, dwClr); }
		bool Clean() { return !pTags; } // empty?

		static bool StripMarkup(char *szText); // strip any markup codes from given text buffer
		static bool StripMarkup(class StdStrBuf *sText); // strip any markup codes from given text buffer
	};
#endif // INC_STDMARKUP
