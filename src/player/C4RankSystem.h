/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
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

/* Rank list for players or crew members */

#ifndef INC_C4RankSystem
#define INC_C4RankSystem

#include "lib/C4InputValidation.h"

class C4RankSystem
{
public:
	C4RankSystem();
	~C4RankSystem() { Clear(); }

	enum { EXP_NoPromotion = -1 }; // experience value for NextRankExp: No more promotion possible

protected:
	char Register[256+1];
	char RankName[C4MaxName+1];
	int RankBase;
	char **pszRankNames;      // loaded rank names for non-registry ranks
	char *szRankNames;        // loaded rank-name buffer
	int iRankNum;             // number of ranks for loaded rank-names
	char **pszRankExtensions; // rank extensions (e.g. "%s First class") for even more ranks!
	int iRankExtNum;          // number of rank extensions
public:
	void Default();
	void Clear();
	int Init(const char *szRegister, const char *szDefRanks, int iRankBase);
	bool Load(C4Group &hGroup, const char *szFilenames, int DefRankBase, const char *szLanguage); // init based on nk file in group
	int Experience(int iRank);
	int RankByExperience(int iExp);  // get rank by experience
	StdStrBuf GetRankName(int iRank, bool fReturnLastIfOver);
	bool Check(int iRank, const char  *szDefRankName);
	int32_t GetExtendedRankNum() const { return iRankExtNum; }
	int32_t GetBaseRankNum() const { return iRankNum; }
	static bool DrawRankSymbol(C4FacetSurface *fctSymbol, int32_t iRank, C4Facet *pfctRankSymbols, int32_t iRankSymbolCount, bool fOwnSurface, int32_t iXOff=0, C4Facet *cgoDrawDirect=nullptr); // create facet from rank symbol for definition - use custom rank facets if present
};

extern C4RankSystem DefaultRanks;

#endif
