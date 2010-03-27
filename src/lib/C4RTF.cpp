/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005, 2007  Sven Eberhardt
 * Copyright (c) 2008  Matthes Bender
 * Copyright (c) 2008  Günther Brammer
 * Copyright (c) 2005-2009, RedWolf Design GmbH, http://www.clonk.de
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
// RTF file parsing functionality

#include "C4Include.h"
#include "C4RTF.h"

#include <cctype>

extern C4RTFFile::KeywordTableEntry RTFKeywordTable[];

C4RTFFile::C4RTFFile() : pState(NULL)
	{
	}

C4RTFFile::~C4RTFFile()
	{
	ClearState();
	}

void C4RTFFile::ClearState()
	{
	PropertyState *psNext = pState, *ps;
	while ((ps=psNext))
		{
		psNext = ps->pNext;
		delete ps;
		}
	pState = NULL;
	fSkipDestIfUnknownKeyword = false;
	}

void C4RTFFile::AssertNoEOF(size_t iPos)
	{
	if (iPos >= sRTF.getSize()) throw new ParserError("Unexpected end of file");
	}

void C4RTFFile::ChangeDest(StdStrBuf &sResult, int iDest)
	{
	// nothing to do if text is already skipped
	if (pState->dest == dsSkip) return;
	// otherwise, set new dest (always skip)
	pState->dest = (DestState) iDest;
	}

void C4RTFFile::SpecialKeyword(StdStrBuf &sResult, int iKeyw, int iParam)
	{
	switch (iKeyw)
		{
		case specBin:
			if (iParam>0)
				{
				pState->eState = psBinary;
				pState->iHexBinCnt = iParam;
				}
			break;
		case specHex:
			pState->eState = psHex;
			pState->iHexBinCnt = 2;
			break;
		case specSkipDest:
			fSkipDestIfUnknownKeyword = true;
			break;
		case specAnsicpg:
			pState->charset = iParam;
			break;
		};
	}

void C4RTFFile::TranslateKeyword(StdStrBuf &sResult, const char *szKeyword, int iParam, bool fHasIntParam)
	{
	// get keyword from table
	KeywordTableEntry *pKw = RTFKeywordTable;
	while (pKw->szKeyword) if (!strcmp(szKeyword, pKw->szKeyword)) break; else ++pKw;
	// no found?
	if (!pKw->szKeyword)
		{
		// unknown destination: Skip
		if (fSkipDestIfUnknownKeyword)
			{
			pState->dest = dsSkip;
			fSkipDestIfUnknownKeyword = false;
			}
		return;
		}
	fSkipDestIfUnknownKeyword = false;
	// keyword known - handle it
	switch (pKw->eType)
		{
		case KeywordTableEntry::kwdProp:
			// property: Use default param if none given or forced
			ApplyPropChange(pKw->idx, (pKw->fForceDefaultParam || !fHasIntParam) ? pKw->iDefaultParam : iParam);
			break;
		case KeywordTableEntry::kwdChars:
			// direct chars
			ParseChars(sResult, pKw->szChars);
			break;
		case KeywordTableEntry::kwdDest:
			ChangeDest(sResult, pKw->idx);
			break;
		case KeywordTableEntry::kwdSpec:
			SpecialKeyword(sResult, pKw->idx, iParam);
			break;
		}
	}

void C4RTFFile::ParseKeyword(StdStrBuf &sResult, size_t &iPos)
	{
	bool fHasIntParam = false;
	int iSign = +1;
	int iParamInt = 0; // parameter as integer
	char szKeyword[30+1]; *szKeyword = 0;
	char szParameter[20+1]; *szParameter = 0;

	AssertNoEOF(iPos);
	char c = ((const char *) sRTF.getData())[iPos++];
	if (!isalpha((unsigned char)c))
		{
		// parse direct control symbol
		szKeyword[0] = c;
		szKeyword[1] = 0;
		}
	else
		{
		// get keyword string
		char *szWrite = szKeyword;
		do
			{
			*szWrite = c;
			// do not overflow buffer - longer keywords will be read, not recognized and silently discarded
			if (szWrite - szKeyword < 30) ++szWrite;
			// do not go past rtf file
			if (iPos >= sRTF.getSize()) break;
			// next char
			c = ((const char *) sRTF.getData())[iPos++];
			}
		while (isalpha((unsigned char)c));
		*szWrite = 0;
		// parameter is a negative number?
		if (c == '-')
			{
			iSign = -1;
			if (iPos < sRTF.getSize()) c = ((const char *) sRTF.getData())[iPos++];
			}
		if (isdigit((unsigned char)c))
			{
			// get parameter as number
			char *szWrite = szParameter;
			do
				{
				*szWrite = c;
				// do not overflow buffer - longer parameters will be read, not recognized and silently discarded
				if (szWrite - szParameter < 20) ++szWrite;
				// do not go past rtf file
				if (iPos >= sRTF.getSize()) break;
				// next char
				c = ((const char *) sRTF.getData())[iPos++];
				}
			while (isdigit((unsigned char)c));
			*szWrite = 0;
			iParamInt = atoi(szParameter) * iSign;
			fHasIntParam = true;
			}
		// if next char is not a spacing for the command, it does not belong to the keyword and must be re-parsed
		if (c != ' ') --iPos;
		}
	// execute keyword action
	TranslateKeyword(sResult, szKeyword, iParamInt, fHasIntParam);
	}

void C4RTFFile::ParseChar(StdStrBuf &sResult, char c)
	{
	// parse as 1-char-string
	char buf[2];
	buf[0] = c; buf[1] = '\0';
	ParseChars(sResult, buf);
	}

void C4RTFFile::ParseChars(StdStrBuf &sResult, const char *szChars)
	{
	// route the characters to the appropriate destination stream.
	switch (pState->dest)
		{
		case dsNormal:
			// process characters: Append to result buffer
			sResult.Append(szChars);
			break;

		case dsSkip:
			// skip character
			break;
		}
	}

void C4RTFFile::ParseHexChar(StdStrBuf &sResult, char c)
	{
	pState->bHex = pState->bHex << 4;
	if (isdigit((unsigned char)c))
		pState->bHex += c - '0';
	else if (Inside<char>(c, 'a', 'f'))
		pState->bHex += c - 'a' + 10;
	else if (Inside<char>(c, 'A', 'F'))
		pState->bHex += c - 'A' + 10;
	else
		throw new ParserError("Invalid hex character");
	if (!--pState->iHexBinCnt)
		{
		pState->eState = psNormal;
		ParseChar(sResult, pState->bHex);
		}
	}

void C4RTFFile::PushState()
	{
	// store current state to new
	PropertyState *pNew = new PropertyState(*pState);
	pNew->pNext = pState;
	// update current state to new; beginning in default parser mode
	pState = pNew;
	pState->eState = psNormal;
	}

void C4RTFFile::PopState()
	{
	if (!pState->pNext) throw new ParserError("Too many brackets closed");
	// if the destination ends, finish it
	if (pState->dest != pState->pNext->dest) EndGroupAction();
	// return to last state
	PropertyState *pKill = pState;
	pState = pState->pNext;
	delete pKill;
	pState->eState = psNormal;
	}

StdStrBuf C4RTFFile::GetPlainText()
	{
	// clear any previous crap
	ClearState();
	// start with a fresh state
	pState = new PropertyState();
	pState->eState = psNormal;
	StdStrBuf sResult;
	// nothing to do for empty RTFs
	if (sRTF.getSize()<=0) return sResult;
	// parse through all chars
	try
		{
		char c; size_t iPos = 0;
		while (iPos < sRTF.getSize())
			{
			c = ((const char *) sRTF.getData())[iPos++];
			// binary parsing?
			if (pState->eState == psBinary)
				{
				if (!--pState->iHexBinCnt) pState->eState = psNormal;
				ParseChar(sResult, c);
				continue;
				}
			// normal parsing: Handle state blocks
			switch (c)
				{
				case '{': PushState(); break;
				case '}': PopState(); break;
				case '\\':
					ParseKeyword(sResult, iPos);
					break;
				case 0x0d: case 0x0a: // ignored chars
					break;
				default:
					// regular char parsing
					if (pState->eState == psNormal)
						// normal mode
						ParseChar(sResult, c);
					else if (pState->eState == psHex)
						ParseHexChar(sResult, c);
					else
						throw new ParserError("Invalid State");
					break;
				}
			// next char
			}
		// all states must be closed in the end
		if (pState->pNext) throw new ParserError("Block not closed");
		}
	catch (ParserError *pe)
		{
		// invalid RTF file: Display error message instead
		sResult = "Invalid RTF file: ";
		sResult.Append(pe->ErrorText);
		delete pe;
		}
	// cleanup
	ClearState();
	// FIXME: This is wrong, RTF contains charset information which should be used
	sResult.EnsureUnicode();
	// return result
	return sResult;
	}


#define kwdChars C4RTFFile::KeywordTableEntry::kwdChars
#define kwdSpec C4RTFFile::KeywordTableEntry::kwdSpec
#define kwdDest C4RTFFile::KeywordTableEntry::kwdDest
// Keyword descriptions
C4RTFFile::KeywordTableEntry RTFKeywordTable [] = {
//    keyword iDefaultPar fForceDef  eType       idx
		{ "par",      0,      false,     kwdChars,   "\n", 0 },
		{ "\0x0a",    0,      false,     kwdChars,   "\n", 0 },
		{ "\0x0d",    0,      false,     kwdChars,   "\n", 0 },
		{ "tab",      0,      false,     kwdChars,   "\t", 0 },
		{ "ldblquote",0,      false,     kwdChars,   "\x93", 0 },
		{ "rdblquote",0,      false,     kwdChars,   "\x94", 0 },
		{ "lquote",   0,      false,     kwdChars,   "\x91", 0 },
		{ "rquote",   0,      false,     kwdChars,   "\x92", 0 },
		{ "bullet",   0,      false,     kwdChars,   "\x95", 0 },
		{ "endash",   0,      false,     kwdChars,   "\x96", 0 },
		{ "emdash",   0,      false,     kwdChars,   "\x97", 0 },
		{ "bin",      0,      false,     kwdSpec,    NULL, C4RTFFile::specBin },
		{ "*",        0,      false,     kwdSpec,    NULL, C4RTFFile::specSkipDest },
		{ "'",        0,      false,     kwdSpec,    NULL, C4RTFFile::specHex },
		{ "ansicpg",  0,      false,     kwdSpec,    NULL, C4RTFFile::specAnsicpg },
		{ "author",   0,      false,     kwdDest,    NULL, C4RTFFile::dsSkip },
		{ "buptim",   0,      false,     kwdDest,    NULL, C4RTFFile::dsSkip },
		{ "colortbl", 0,      false,     kwdDest,    NULL, C4RTFFile::dsSkip },
		{ "comment",  0,      false,     kwdDest,    NULL, C4RTFFile::dsSkip },
		{ "creatim",  0,      false,     kwdDest,    NULL, C4RTFFile::dsSkip },
		{ "doccomm",  0,      false,     kwdDest,    NULL, C4RTFFile::dsSkip },
		{ "fonttbl",  0,      false,     kwdDest,    NULL, C4RTFFile::dsSkip },
		{ "footer",   0,      false,     kwdDest,    NULL, C4RTFFile::dsSkip },
		{ "footerf",  0,      false,     kwdDest,    NULL, C4RTFFile::dsSkip },
		{ "footerl",  0,      false,     kwdDest,    NULL, C4RTFFile::dsSkip },
		{ "footerr",  0,      false,     kwdDest,    NULL, C4RTFFile::dsSkip },
		{ "footnote", 0,      false,     kwdDest,    NULL, C4RTFFile::dsSkip },
		{ "ftncn",    0,      false,     kwdDest,    NULL, C4RTFFile::dsSkip },
		{ "ftnsep",   0,      false,     kwdDest,    NULL, C4RTFFile::dsSkip },
		{ "ftnsepc",  0,      false,     kwdDest,    NULL, C4RTFFile::dsSkip },
		{ "header",   0,      false,     kwdDest,    NULL, C4RTFFile::dsSkip },
		{ "headerf",  0,      false,     kwdDest,    NULL, C4RTFFile::dsSkip },
		{ "headerl",  0,      false,     kwdDest,    NULL, C4RTFFile::dsSkip },
		{ "headerr",  0,      false,     kwdDest,    NULL, C4RTFFile::dsSkip },
		{ "info",     0,      false,     kwdDest,    NULL, C4RTFFile::dsSkip },
		{ "keywords", 0,      false,     kwdDest,    NULL, C4RTFFile::dsSkip },
		{ "operator", 0,      false,     kwdDest,    NULL, C4RTFFile::dsSkip },
		{ "pict",     0,      false,     kwdDest,    NULL, C4RTFFile::dsSkip },
		{ "printim",  0,      false,     kwdDest,    NULL, C4RTFFile::dsSkip },
		{ "private1", 0,      false,     kwdDest,    NULL, C4RTFFile::dsSkip },
		{ "revtim",   0,      false,     kwdDest,    NULL, C4RTFFile::dsSkip },
		{ "rxe",      0,      false,     kwdDest,    NULL, C4RTFFile::dsSkip },
		{ "stylesheet",0,     false,     kwdDest,    NULL, C4RTFFile::dsSkip },
		{ "subject",  0,      false,     kwdDest,    NULL, C4RTFFile::dsSkip },
		{ "tc",       0,      false,     kwdDest,    NULL, C4RTFFile::dsSkip },
		{ "title",    0,      false,     kwdDest,    NULL, C4RTFFile::dsSkip },
		{ "txe",      0,      false,     kwdDest,    NULL, C4RTFFile::dsSkip },
		{ "xe",       0,      false,     kwdDest,    NULL, C4RTFFile::dsSkip },
		{ "{",        0,      false,     kwdChars,   "{", 0 },
		{ "}",        0,      false,     kwdChars,   "}", 0 },
		{ "\\",       0,      false,     kwdChars,   "\\", 0 },
		{ NULL, 0, false, kwdChars, NULL, 0 }
		};
