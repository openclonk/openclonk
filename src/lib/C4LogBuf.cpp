/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2013-2016, The OpenClonk Team and contributors
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
// a buffer holding a log history

#include "C4Include.h"
#include "lib/C4LogBuf.h"

#include "graphics/C4FontLoader.h"

C4LogBuffer::C4LogBuffer(int iSize, int iMaxLines, int iLBWidth, const char *szIndentChars, bool fDynamicGrow, bool fMarkup)
		: iBufSize(iSize), iFirstLinePos(0), iAfterLastLinePos(0), iLineDataPos(0),
		iNextLineDataPos(0), iMaxLineCount(iMaxLines), iLineCount(0), iLineBreakWidth(iLBWidth), fDynamicGrow(fDynamicGrow), fMarkup(fMarkup)
{
	// copy indent
	if (szIndentChars && *szIndentChars)
	{
		szIndent = new char[strlen(szIndentChars)+1];
		strcpy(szIndent, szIndentChars);
	}
	else szIndent = nullptr;
	// create buffers, if buffer size is given. Otherwise, create/grow them dynamically
	if (iBufSize) szBuf = new char[iBufSize]; else szBuf=nullptr;
	if (iMaxLineCount) pLineDataBuf = new LineData[iMaxLineCount]; else pLineDataBuf=nullptr;
	assert(fDynamicGrow || (iBufSize && iMaxLineCount));
}

C4LogBuffer::~C4LogBuffer()
{
	// free buffers
	delete [] pLineDataBuf;
	delete [] szBuf;
	// free indent
	if (szIndent) delete [] szIndent;
}

void C4LogBuffer::GrowLineCountBuffer(size_t iGrowBy)
{
	assert(fDynamicGrow);
	if (!iGrowBy) return;
	LineData *pNewBuf = new LineData[iMaxLineCount += iGrowBy];
	if (iLineCount) memcpy(pNewBuf, pLineDataBuf, sizeof(LineData) * iLineCount);
	delete [] pLineDataBuf;
	pLineDataBuf = pNewBuf;
}

void C4LogBuffer::GrowTextBuffer(size_t iGrowBy)
{
	assert(fDynamicGrow);
	if (!iGrowBy) return;
	char *pNewBuf = new char[iBufSize += iGrowBy];
	if (iAfterLastLinePos) memcpy(pNewBuf, szBuf, sizeof(char) * iAfterLastLinePos);
	delete [] szBuf;
	szBuf = pNewBuf;
}

void C4LogBuffer::DiscardFirstLine()
{
	// any line to discard? - this is guaranteed (private call)
	assert(iLineCount && szBuf && !fDynamicGrow);
	// dec line count
	--iLineCount;
	// advance first line pos until delimeter char is reached
	while (szBuf[iFirstLinePos]) ++iFirstLinePos;
	// skip delimeter
	++iFirstLinePos;
	// check if end of used buffer is reached (by size or double delimeter)
	if (iFirstLinePos == iBufSize || !szBuf[iFirstLinePos] || !iLineCount)
	{
		// end of buffer reached: wrap to front
		iFirstLinePos = 0;
	}
	// discard line data
	++iLineDataPos;
	if (!iLineCount) iLineDataPos = iNextLineDataPos = 0;
}

void C4LogBuffer::AppendSingleLine(const char *szLine, int iLineLength, const char *szIndent, CStdFont *pFont, DWORD dwClr, bool fNewPar)
{
	// security: do not append empty line
	if (!szLine || !iLineLength || !*szLine) return;
	// discard first line or grow buffer if data buffer is full
	if (iLineCount == iMaxLineCount)
	{
		if (fDynamicGrow)
			GrowLineCountBuffer(4 + iMaxLineCount/2);
		else
			DiscardFirstLine();
	}
	// include trailing zero-character
	++iLineLength;
	// include indent
	if (szIndent) iLineLength += strlen(szIndent);
	// but do not add a message that is longer than the buffer (shouldn't happen anyway)
	if (iLineLength > iBufSize && !fDynamicGrow)
	{
		// cut from beginning then
		szLine += iLineLength - iBufSize;
		iLineLength = iBufSize;
	}
	// check if the rest of the buffer is sufficient
	if (iAfterLastLinePos + iLineLength > iBufSize)
	{
		if (fDynamicGrow)
		{
			// insufficient buffer in grow mode: grow text buffer
			GrowTextBuffer(std::max(iLineLength, iBufSize/2));
		}
		else
		{
			// insufficient buffer in non-grow mode: wrap to beginning
			// discard any messages in rest of buffer
			// if there are no messages, iFirstLinePos is always zero
			// and iAfterLastLinePos cannot be zero here
			while (iFirstLinePos >= iAfterLastLinePos) DiscardFirstLine();
			// add delimeter to mark end of used buffer
			// if the buffer is exactly full by the last line, no delimeter is needed
			if (iAfterLastLinePos < iBufSize) szBuf[iAfterLastLinePos] = 0;
			// wrap insertion pos to beginning
			iAfterLastLinePos = 0;
		}
	}
	// discard any messages within insertion range of new message
	if (!fDynamicGrow)
		while (iLineCount && Inside(iFirstLinePos, iAfterLastLinePos, iAfterLastLinePos+iLineLength-1))
			DiscardFirstLine();
	// copy indent
	int iIndentLen = 0;
	if (szIndent)
	{
		iIndentLen = strlen(szIndent);
		memcpy(szBuf + iAfterLastLinePos, szIndent, iIndentLen);
	}
	// copy message
	if (iLineLength - iIndentLen > 1)
		memcpy(szBuf + iAfterLastLinePos + iIndentLen, szLine, iLineLength-iIndentLen-1);
	// add delimeter
	iAfterLastLinePos += iLineLength;
	szBuf[iAfterLastLinePos - 1] = 0;
	// no need to add any double delimeters, because this is currently the end of the message list
	// also no need to check for end of buffer here, because that will be done when the next message is inserted
	// add line data
	LineData &rData = pLineDataBuf[iNextLineDataPos];
	rData.pFont = pFont;
	rData.dwClr = dwClr;
	rData.fNewParagraph = fNewPar;
	// new message successfully added; count it
	++iLineCount;
	if (++iNextLineDataPos == iMaxLineCount)
	{
		if (fDynamicGrow)
			GrowLineCountBuffer(4 + iMaxLineCount/2);
		else
			iNextLineDataPos = 0;
	}
}

void C4LogBuffer::AppendLines(const char *szLine, CStdFont *pFont, DWORD dwClr, CStdFont *pFirstLineFont)
{
	char LineBreakChars [] = { 0x0D, 0x0A, '|' };
	int32_t iLineBreakCharCount = 2 + fMarkup;
	// safety
	if (!szLine) return;
	// split '|'/CR/LF-separations first, if there are any
	bool fAnyLineBreakChar = false;
	for (int i = 0; i < iLineBreakCharCount; ++i)
		if (strchr(szLine, LineBreakChars[i]))
		{
			fAnyLineBreakChar = true;
			break;
		}
	if (fAnyLineBreakChar)
	{
		char *szBuf = new char[strlen(szLine)+1];
		char *szBufPos, *szPos2 = szBuf, *szBufFind;
		strcpy(szBuf, szLine);
		while ((szBufPos = szPos2))
		{
			// find first occurance of any line break char
			szPos2 = nullptr;
			for (int i = 0; i < iLineBreakCharCount; ++i)
				if ((szBufFind = strchr(szBufPos, LineBreakChars[i])))
					if (!szPos2 || szBufFind < szPos2)
						szPos2 = szBufFind;
			// split string at linebreak char
			if (szPos2) *szPos2++ = '\0';
			// output current line if not empty
			if (!*szBufPos) continue;
			// first line in caption font
			if (pFirstLineFont)
			{
				AppendLines(szBufPos, pFirstLineFont, dwClr);
				pFirstLineFont = nullptr;
			}
			else
				AppendLines(szBufPos, pFont, dwClr);
		}
		delete [] szBuf;
		return;
	}
	// no line breaks desired: Output all in one line
	if (!iLineBreakWidth || !pFont)
	{
		AppendSingleLine(szLine, strlen(szLine), nullptr, pFont, dwClr, true);
	}
	else
	{
		C4Markup markup(false);
		const char *markupPos = szLine;
		std::string rline;
		// output broken lines until there are any
		int iLineIndex = 0;
		while (*szLine)
		{
			// get line width of this line
			int iBreakWdt = iLineBreakWidth;
			if (iLineIndex && szIndent)
			{
				int32_t iIndentWdt, Q;
				pFont->GetTextExtent(szIndent, iIndentWdt, Q, true);
				iBreakWdt -= iIndentWdt;
			}
			// get number of characters printable into this line
			const char *szNextLine;
			int iNumChars = pFont->GetMessageBreak(szLine, &szNextLine, iBreakWdt);
			// make sure not to break markup
			if (fMarkup)
			{
				std::string opening = markup.OpeningTags();
				while (markupPos < szNextLine)
				{
					if (*markupPos == '<')
					{
						if (markup.Read(&markupPos))
						{
							if (markupPos > szNextLine)
							{
								// The message break is within a tag.
								iNumChars += markupPos - szNextLine + 1;
								szNextLine = markupPos;
							}
							// Read already moved us over a valid tag.
							continue;
						}
					}
					markupPos++;
				}
				std::string closing = markup.ClosingTags();
				if (!opening.empty() || !closing.empty())
				{
					rline = std::move(opening);
					rline.append(szLine, iNumChars);
					rline.append(closing);
					szLine = rline.c_str();
					iNumChars = rline.size();
				}
			}
			// add them
			AppendSingleLine(szLine, iNumChars, iLineIndex ? szIndent : nullptr, pFont, dwClr, !iLineIndex);
			// next line
			szLine = szNextLine;
			++iLineIndex;
			rline.clear();
		}
	}
}

const char *C4LogBuffer::GetLine(int iLineIndex, CStdFont **ppFont, DWORD *pdwClr, bool *pfNewPar) const
{
	// evaluate negative indices
	if (iLineIndex < 0)
	{
		iLineIndex += iLineCount;
		if (iLineIndex < 0) return nullptr;
	}
	// range check
	if (iLineIndex >= iLineCount) return nullptr;
	// assign data
	LineData &rData = pLineDataBuf[(iLineDataPos + iLineIndex) % iMaxLineCount];
	if (ppFont) *ppFont = rData.pFont;
	if (pdwClr) *pdwClr = rData.dwClr;
	if (pfNewPar) *pfNewPar = rData.fNewParagraph;
	// advance in lines until desired line is found
	char *szResult = szBuf + iFirstLinePos;
	while (iLineIndex--)
	{
		// skip this line
		while (*szResult++) ;
		// double delimeter or end of buffer: reset searching to front of buffer
		if (szResult == (szBuf+iBufSize) || !*szResult) szResult = szBuf;
	}
	// return found buffer pos
	return szResult;
}

void C4LogBuffer::Clear()
{
	// clear buffer usage
	iFirstLinePos = iAfterLastLinePos = iLineCount = iNextLineDataPos = iLineDataPos = 0;
}

void C4LogBuffer::SetLBWidth(int iToWidth)
{
	iLineBreakWidth = iToWidth;
}
