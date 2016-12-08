/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2016, The OpenClonk Team and contributors
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

/* Rank list for players or crew members */

#include "C4Include.h"
#include "player/C4RankSystem.h"

#include "lib/C4Log.h"
#include "c4group/C4Group.h"
#include "c4group/C4ComponentHost.h"
#include "graphics/C4FacetEx.h"
#include "game/C4Game.h"
#include "graphics/C4GraphicsResource.h"

#include "platform/StdRegistry.h"

C4RankSystem::C4RankSystem()
	: pszRankNames(nullptr), szRankNames(nullptr), pszRankExtensions(nullptr)
{
	Default();
}

int C4RankSystem::Init(const char *szRegister,
                       const char *szDefRanks,
                       int iRankBase)
{

	// Init
	SCopy(szRegister,Register,256);
	RankBase=iRankBase;

	// Check registry for present rank names and set defaults
#ifdef _WIN32
	int crank=0;
	char keyname[30];
	StdCopyStrBuf rankname;
	bool Checking=true;
	while (Checking)
	{
		sprintf(keyname,"Rank%03d",crank+1);
		rankname = GetRegistryString(Register,keyname);
		if (!rankname.isNull())
		{
			// Rank present
			crank++;
		}
		else
		{
			// Rank not defined, check for default
			rankname.AppendChars('\0', C4MaxName);
			if (SCopySegment(szDefRanks,crank,rankname.getMData(),'|',C4MaxName)
			    && SetRegistryString(Register,keyname,rankname.getData()))
				crank++;
			else
				Checking=false;
		}
	}
	return crank;
#else
	// clear any loaded rank names
	Clear();
	if (!szDefRanks) return 0;
	// make a copy
	szRankNames = new char[strlen(szDefRanks) + 1];
	strcpy (szRankNames, szDefRanks);
	// split into substrings by replacing the | with zeros
	for (char * p = szRankNames; *p; ++p) if (*p == '|')
		{
			*p = 0;
			++iRankNum;
		}
	++ iRankNum; // The last rank is already terminated by zero
	// build a list of substrings
	pszRankNames = new char *[iRankNum];
	char * p = szRankNames;
	for (int i = 0; i < iRankNum; ++i)
	{
		pszRankNames[i] = p;
		p += strlen(p) + 1;
	}
	return iRankNum;
#endif
}

bool C4RankSystem::Load(C4Group &hGroup, const char *szFilenames, int DefRankBase, const char *szLanguage)
{
	// clear any loaded rank names
	Clear();
	assert(szFilenames); assert(szLanguage);
	// load new
	C4ComponentHost Ranks;
	if (!C4Language::LoadComponentHost(&Ranks, hGroup, szFilenames, szLanguage)) return false;
	size_t iSize = Ranks.GetDataSize();
	if (!iSize) return false;
	szRankNames=new char[iSize+1];
	memcpy(szRankNames, Ranks.GetData(), iSize * sizeof(char));
	szRankNames[iSize]=0;
	// replace line breaks by zero-chars
	unsigned int i=0;
	for (; i<iSize; ++i)
		if (szRankNames[i] == 0x0A || szRankNames[i] == 0x0D) szRankNames[i]=0;
	// count names
	char *pRank0=szRankNames, *pPos=szRankNames;
	for (i=0; i<iSize; ++i,++pPos)
		if (!*pPos)
		{
			// zero-character found: content?
			if (pPos-pRank0>0)
			{
				// rank extension?
				if (*pRank0 == '*')
					++iRankExtNum;
				// no comment?
				else if (*pRank0 != '#')
					// no setting?
					if (SCharPos('=', pRank0) < 0)
						// count as name!
						++iRankNum;
			}
			// advance pos
			pRank0=pPos+1;
		}
	// safety
	if (!iRankNum) { Clear(); return false; }
	// set default rank base
	RankBase=DefRankBase;
	// alloc lists
	pszRankNames = new char *[iRankNum];
	if (iRankExtNum) pszRankExtensions = new char *[iRankExtNum];
	// fill list with names
	// count names
	pRank0=szRankNames; pPos=szRankNames;
	char **pszCurrRank=pszRankNames;
	char **pszCurrRankExt=pszRankExtensions;
	for (i=0; i<iSize; ++i,++pPos)
		if (!*pPos)
		{
			// zero-character found: content?
			if (pPos-pRank0>0)
			{
				// extension?
				if (*pRank0 == '*')
				{
					*pszCurrRankExt++ = pRank0+1;
				}
				// no comment?
				else if (*pRank0 != '#')
				{
					// check if it's a setting
					int iEqPos=SCharPos('=', pRank0);
					if (iEqPos >= 0)
					{
						// get name and value of setting
						pRank0[iEqPos]=0; char *szValue=pRank0+iEqPos+1;
						if (SEqual(pRank0, "Base"))
							// get rankbase
							// note that invalid numbers may cause desyncs here...not very likely though :)
							sscanf(szValue, "%d", &RankBase);
					}
					else
						// yeeehaa! it's a name! store it, store it!
						*pszCurrRank++=pRank0;
				}
			}
			// advance pos
			pRank0=pPos+1;
		}
	// check rankbase
	if (!RankBase) RankBase=1000;
	// ranks read, success
	return true;
}

StdStrBuf C4RankSystem::GetRankName(int iRank, bool fReturnLastIfOver)
{
	if (iRank<0) return StdStrBuf();
	// if a new-style ranklist is loaded, seek there
	if (pszRankNames)
	{
		if (iRankNum<=0) return StdStrBuf();
		// overflow check
		if (iRank>=iRankNum*(iRankExtNum+1))
		{
			// rank undefined: Fallback to last rank
			if (!fReturnLastIfOver) return StdStrBuf();
			iRank = iRankNum*(iRankExtNum+1)-1;
		}
		StdStrBuf sResult;
		if (iRank >= iRankNum)
		{
			// extended rank composed of two parts
			int iExtension = iRank / iRankNum - 1;
			iRank = iRank % iRankNum;
			sResult.Format(pszRankExtensions[iExtension], pszRankNames[iRank]);
		}
		else
		{
			// simple rank
			sResult.Ref(pszRankNames[iRank]);
		}
		return sResult;
	}
#ifdef _WIN32
	// old-style registry fallback
	while (iRank>=0)
	{
		char keyname[30];
		StdCopyStrBuf rankname;
		sprintf(keyname,"Rank%03d",iRank+1);
		rankname = GetRegistryString(Register,keyname);
		if (!rankname.isNull())
			return rankname;
		if (!fReturnLastIfOver) return StdStrBuf();
		--iRank;
	}
#endif
	return StdStrBuf();
}

int C4RankSystem::Experience(int iRank)
{
	if (iRank<0) return 0;
	return (int) ( pow (double(iRank), 1.5) * RankBase );
}

int C4RankSystem::RankByExperience(int iExp)
{
	int iRank=0;
	while (Experience(iRank+1)<=iExp) ++iRank;
	return iRank;
}

bool C4RankSystem::Check(int iRank, const char  *szDefRankName)
{
#ifdef _WIN32
	char keyname[30];
	sprintf(keyname,"Rank%03d",iRank);
	if (!GetRegistryString(Register,keyname).isNull())
		return false;
	if (!szDefRankName || (SLen(szDefRankName)>C4MaxName))
		return false;
	return SetRegistryString(Register,keyname,szDefRankName);
#else
	return true;
#endif
}

void C4RankSystem::Clear()
{
	// clear any loaded rank names
	if (pszRankNames) { delete [] pszRankNames; pszRankNames=nullptr; }
	if (pszRankExtensions) { delete [] pszRankExtensions; pszRankExtensions = nullptr; }
	if (szRankNames) { delete [] szRankNames; szRankNames=nullptr; }
	// reset number of ranks
	iRankNum=0;
	iRankExtNum=0;
}

void C4RankSystem::Default()
{
	Clear();
	Register[0]=0;
	RankName[0]=0;
	RankBase=1000;
	iRankExtNum=0;
}

bool C4RankSystem::DrawRankSymbol(C4FacetSurface *fctSymbol, int32_t iRank, C4Facet *pfctRankSymbols, int32_t iRankSymbolCount, bool fOwnSurface, int32_t iXOff, C4Facet *cgoDrawDirect)
{
	// safety
	if (iRank<0) iRank = 0;
	// symbol by rank
	int32_t iMaxRankSym,Q;
	if (pfctRankSymbols->GetPhaseNum(iMaxRankSym, Q))
	{
		if (!iMaxRankSym) iMaxRankSym=1;
		int32_t iBaseRank=iRank%iRankSymbolCount;
		if (iRank/iRankSymbolCount)
		{
			// extended rank: draw
			// extension star defaults to captain star; but use extended symbols if they are in the gfx
			C4Facet fctExtended = ::GraphicsResource.fctCaptain;
			if (iMaxRankSym > iRankSymbolCount)
			{
				int32_t iExtended = iRank / iRankSymbolCount - 1 + iRankSymbolCount;
				if (iExtended >= iMaxRankSym)
				{
					// max rank exceeded
					iExtended = iMaxRankSym - 1;
					iBaseRank = iRankSymbolCount - 1;
				}
				fctExtended = pfctRankSymbols->GetPhase(iExtended);
			}
			int32_t iSize = pfctRankSymbols->Wdt;
			if (!cgoDrawDirect)
			{
				fctSymbol->Create(iSize,iSize);
				pfctRankSymbols->DrawX(fctSymbol->Surface, 0,0,iSize,iSize,iBaseRank);
				fctExtended.DrawX(fctSymbol->Surface, 0,0,iSize*2/3,iSize*2/3);
			}
			else
			{
				pfctRankSymbols->Draw(cgoDrawDirect->Surface,cgoDrawDirect->X+iXOff,cgoDrawDirect->Y,iBaseRank);
				fctExtended.Draw(cgoDrawDirect->Surface, cgoDrawDirect->X+iXOff-4,cgoDrawDirect->Y-3);
			}
		}
		else
		{
			// regular rank: copy facet
			if (cgoDrawDirect)
			{
				pfctRankSymbols->Draw(cgoDrawDirect->Surface, cgoDrawDirect->X+iXOff,cgoDrawDirect->Y,iBaseRank);
			}
			else if (fOwnSurface)
			{
				int32_t iSize = pfctRankSymbols->Wdt;
				fctSymbol->Create(iSize,iSize);
				pfctRankSymbols->DrawX(fctSymbol->Surface, 0,0,iSize,iSize,iBaseRank);
			}
			else
			{
				fctSymbol->Set(pfctRankSymbols->GetPhase(iBaseRank));
			}
		}
		return true;
	}
	return false;
}

C4RankSystem DefaultRanks;
