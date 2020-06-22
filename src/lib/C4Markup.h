/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2011-2016, The OpenClonk Team and contributors
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
	C4MarkupTag *pPrev{nullptr}, *pNext{nullptr};

	C4MarkupTag() = default; // ctor
	virtual ~C4MarkupTag() = default;    // dtor

	virtual void Apply(C4BltTransform &rBltTrf, bool fDoClr, DWORD &dwClr)=0; // assign markup
	virtual const char *TagName() const = 0;  // get character string for this tag
	virtual std::string OpeningTag() const;   // get opening tag, i.e. "<{TagName()}>"
	virtual std::string ClosingTag() const;   // get closing tag, i.e. "</{TagName()}>"
};

// markup tag for italic text
class C4MarkupTagItalic : public C4MarkupTag
{
public:
	C4MarkupTagItalic() : C4MarkupTag() { } // ctor

	void Apply(C4BltTransform &rBltTrf, bool fDoClr, DWORD &dwClr) override; // assign markup
	const char *TagName() const override { return "i"; }
};

// markup tag for colored text
class C4MarkupTagColor : public C4MarkupTag
{
private:
	DWORD dwClr;    // color
public:
	C4MarkupTagColor(DWORD dwClr) : C4MarkupTag(), dwClr(dwClr) { } // ctor

	void Apply(C4BltTransform &rBltTrf, bool fDoClr, DWORD &dwClr) override; // assign markup
	const char *TagName() const override { return "c"; }
	std::string OpeningTag() const override;
};

// markup rendering functionality for text
class C4Markup
{
private:
	C4MarkupTag *pTags, *pLast;  // tag list; double linked
	bool fDoClr;                 // set if color changes should be made (not in text shadow!)

	void Push(C4MarkupTag *pTag)
	{ if ((pTag->pPrev=pLast)) pLast->pNext=pTag; else pTags=pTag; pLast=pTag; }
	C4MarkupTag *Pop()
	{ C4MarkupTag *pL=pLast; if (!pL) return nullptr; if ((pLast=pL->pPrev)) pLast->pNext=nullptr; else pTags=nullptr; return pL; }
public:
	C4Markup(bool fDoClr) { pTags=pLast=nullptr; this->fDoClr=fDoClr; };   // ctor
	~C4Markup() // dtor
	{ C4MarkupTag *pTag=pTags,*pNext; while (pTag) { pNext=pTag->pNext; delete pTag; pTag=pNext; } }

	bool Read(const char **ppText, bool fSkip=false);   // get markup from text
	bool SkipTags(const char **ppText); // extract markup from text; return whether end is reached
	void Apply(C4BltTransform &rBltTrf, DWORD &dwClr)  // assign markup to vertices
	{ for (C4MarkupTag *pTag=pTags; pTag; pTag=pTag->pNext) pTag->Apply(rBltTrf, fDoClr, dwClr); }
	bool Clean() { return !pTags; } // empty?

	// The following two functions are for splitting a markup string.
	std::string ClosingTags() const; // get all closing tags at the current location
	std::string OpeningTags() const; // get all opening tags at the current location

	static bool StripMarkup(char *szText); // strip any markup codes from given text buffer
	static bool StripMarkup(StdStrBuf *sText); // strip any markup codes from given text buffer
};
#endif // INC_STDMARKUP
