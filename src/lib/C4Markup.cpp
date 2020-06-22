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

#include "C4Include.h"
#include "lib/C4Markup.h"
#include "graphics/C4BltTransform.h"

using namespace std::string_literals;

std::string C4MarkupTag::OpeningTag() const
{
	return "<"s + TagName() + ">";
}

std::string C4MarkupTag::ClosingTag() const
{
	return "</"s + TagName() + ">";
}

void C4MarkupTagItalic::Apply(C4BltTransform &rBltTrf, bool fDoClr, DWORD &dwClr)
{
	// do sheering
	rBltTrf.mat[1]-=0.3f;
}

void C4MarkupTagColor::Apply(C4BltTransform &rBltTrf, bool fDoClr, DWORD &dwClr)
{
	// set color
	if (fDoClr) dwClr = this->dwClr;
}


std::string C4MarkupTagColor::OpeningTag() const
{
	return "<c "s + FormatString("%x", dwClr).getData() + ">";
}

bool C4Markup::Read(const char **ppText, bool fSkip)
{
	char Tag[50]; C4MarkupTag *pNewTag=nullptr; int iTagLen,iParLen;
	// get tag
	if (!SCopyEnclosed(*ppText, '<', '>', Tag, 49)) return false;
	iTagLen=SLen(Tag);
	// split tag to name and pars
	char *szPars=nullptr; int iSPos;
	if ((iSPos=SCharPos(' ', Tag))>-1)
	{
		Tag[iSPos]=0;
		szPars=Tag+iSPos+1;
	}
	// closing tag?
	if (Tag[0]=='/')
	{
		// no parameters
		if (szPars) return false;
		if (!fSkip)
		{
			// is this the tag to be closed?
			if (!pLast) return false;
			if (!SEqual(pLast->TagName(), Tag+1)) return false;
			// close it
			delete Pop();
		}
	}
	// italic
	else if (SEqual(Tag, "i"))
	{
		// no parameters
		if (szPars) return false;
		// create italic tag
		if (!fSkip) pNewTag=new C4MarkupTagItalic();
	}
	// colored
	else if (SEqual(Tag, "c"))
	{
		// no parameters?
		if (!szPars) return false;
		if ((iParLen=SLen(szPars))>8) return false;
		if (!fSkip)
		{
			// get color value by parameter
			DWORD dwClr=0;
			for (int i=0; i<iParLen; ++i)
			{
				BYTE b;
				if (szPars[i]>='0' && szPars[i]<='9') b=szPars[i]-'0';
				else if (szPars[i]>='a' && szPars[i]<='f') b=szPars[i]-'a'+10;
				else return false;
				dwClr|=(b<<((iParLen-i-1)*4));
			}
			// adjust alpha if not given
			if (iParLen<=6) dwClr|=0xff000000;
			// create color tag
			pNewTag=new C4MarkupTagColor(dwClr);
		}
	}
	// unknown tag
	else return false;
	// add created tag
	if (pNewTag) Push(pNewTag);
	// advance past tag
	*ppText+=iTagLen+2;
	// success
	return true;
}



bool C4Markup::SkipTags(const char **ppText)
{
	// read tags as long as found
	while (**ppText=='<') if (!Read(ppText, true)) break;
	// return whether end is reached
	return !**ppText;
}


std::string C4Markup::ClosingTags() const
{
	std::string result;
	for (auto tag = pLast; tag; tag = tag->pPrev)
		result += tag->ClosingTag();
	return result;
}

std::string C4Markup::OpeningTags() const
{
	std::string result;
	for (auto tag = pTags; tag; tag = tag->pNext)
		result += tag->OpeningTag();
	return result;
}

bool C4Markup::StripMarkup(char *szText)
{
	// skip any tags and inline-images
	C4Markup mkup(false);
	const char *szRead = szText, *szPos2;
	do
	{
		mkup.SkipTags(&szRead);
		if (szRead[0] == '{' && szRead[1] == '{' && szRead[2] != '{') // skip at {{{, because {{{id}} should be parsed as { {{id}} }.
		{
			if ((szPos2 = SSearch(szRead+2, "}}")))
				// valid {{blub}}-tag
				szRead = szPos2;
			else
				// invalid {{-tag
				szRead += 2;
		}
		else if (szRead[0] == '}' && szRead[1] == '}')
			// invalid }}-tag
			szRead += 2;
	}
	while ((*szText++ = *szRead++));
	return szText != szRead;
}

bool C4Markup::StripMarkup(StdStrBuf *sText)
{
	// strip any markup codes from given text buffer
	char *buf = sText->GrabPointer();
	if (!buf) return false;
	bool fSuccess = StripMarkup(buf);
	sText->Take(buf);
	return fSuccess;
}
