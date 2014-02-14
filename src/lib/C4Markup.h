/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2011-2013, The OpenClonk Team and contributors
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
// markup tags for fonts

#ifndef INC_STDMARKUP
#define INC_STDMARKUP

// one markup tag
class C4MarkupTag
{
public:
	C4MarkupTag *pPrev, *pNext;

	C4MarkupTag(): pPrev(0), pNext(0) { }; // ctor
	virtual ~C4MarkupTag() { };    // dtor

	virtual void Apply(C4BltTransform &rBltTrf, bool fDoClr, DWORD &dwClr)=0; // assign markup
	virtual const char *TagName()=0;  // get character string for this tag
};

// markup tag for italic text
class C4MarkupTagItalic : public C4MarkupTag
{
public:
	C4MarkupTagItalic() : C4MarkupTag() { } // ctor

	virtual void Apply(C4BltTransform &rBltTrf, bool fDoClr, DWORD &dwClr); // assign markup
	virtual const char *TagName() { return "i"; }
};

// markup tag for colored text
class C4MarkupTagColor : public C4MarkupTag
{
private:
	DWORD dwClr;    // color
public:
	C4MarkupTagColor(DWORD dwClr) : C4MarkupTag(), dwClr(dwClr) { } // ctor

	virtual void Apply(C4BltTransform &rBltTrf, bool fDoClr, DWORD &dwClr); // assign markup
	virtual const char *TagName() { return "c"; }
};

// markup rendering functionality for text
class C4Markup
{
private:
	C4MarkupTag *pTags, *pLast;    // tag list; single linked
	bool fDoClr;                  // set if color changes should be made (not in text shadow!)

	void Push(C4MarkupTag *pTag)
	{ if ((pTag->pPrev=pLast)) pLast->pNext=pTag; else pTags=pTag; pLast=pTag; }
	C4MarkupTag *Pop()
	{ C4MarkupTag *pL=pLast; if (!pL) return 0; if ((pLast=pL->pPrev)) pLast->pNext=0; else pTags=0; return pL; }
public:
	C4Markup(bool fDoClr) { pTags=pLast=0; this->fDoClr=fDoClr; };   // ctor
	~C4Markup() // dtor
	{ C4MarkupTag *pTag=pTags,*pNext; while (pTag) { pNext=pTag->pNext; delete pTag; pTag=pNext; } }

	bool Read(const char **ppText, bool fSkip=false);   // get markup from text
	bool SkipTags(const char **ppText); // extract markup from text; return whether end is reached
	void Apply(C4BltTransform &rBltTrf, DWORD &dwClr)  // assign markup to vertices
	{ for (C4MarkupTag *pTag=pTags; pTag; pTag=pTag->pNext) pTag->Apply(rBltTrf, fDoClr, dwClr); }
	bool Clean() { return !pTags; } // empty?

	static bool StripMarkup(char *szText); // strip any markup codes from given text buffer
	static bool StripMarkup(class StdStrBuf *sText); // strip any markup codes from given text buffer
};
#endif // INC_STDMARKUP
