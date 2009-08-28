/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2001-2002, 2006-2007  Sven Eberhardt
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

/* Rank list for players or crew members */

#ifndef INC_C4RankSystem
#define INC_C4RankSystem

#include "C4InputValidation.h"

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
		char **pszRankNames;			// loaded rank names for non-registry ranks
		char *szRankNames;				// loaded rank-name buffer
		int iRankNum;							// number of ranks for loaded rank-names
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
		//void Reset(const char *szDefRanks);
		static bool DrawRankSymbol(C4FacetSurface *fctSymbol, int32_t iRank, C4Facet *pfctRankSymbols, int32_t iRankSymbolCount, bool fOwnSurface, int32_t iXOff=0, C4Facet *cgoDrawDirect=NULL); // create facet from rank symbol for definition - use custom rank facets if present
  };

extern C4RankSystem DefaultRanks;

#endif
