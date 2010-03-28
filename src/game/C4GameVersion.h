/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2007  Peter Wortmann
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
#ifndef C4GAMEVERSION_H

#define C4GAMEVERSION_H

#include "C4Version.h"
#include "C4InputValidation.h"

struct C4GameVersion
{
	ValidatedStdCopyStrBuf<C4InVal::VAL_NameAllowEmpty> sEngineName; // status only - not used for comparison
	int32_t iVer[4];
	int32_t iBuild;

	C4GameVersion(const char *szEngine=C4ENGINENAME, int32_t iVer1=C4XVER1, int32_t iVer2=C4XVER2, int32_t iVer3=C4XVER3, int32_t iVer4=C4XVER4, int32_t iVerBuild=C4XVERBUILD)
	{ Set(szEngine, iVer1, iVer2, iVer3, iVer4, iVerBuild); }
	void Set(const char *szEngine=C4ENGINENAME, int32_t iVer1=C4XVER1, int32_t iVer2=C4XVER2, int32_t iVer3=C4XVER3, int32_t iVer4=C4XVER4, int32_t iVerBuild=C4XVERBUILD)
	{ sEngineName.CopyValidated(szEngine); iVer[0]=iVer1; iVer[1]=iVer2; iVer[2]=iVer3; iVer[3]=iVer4; iBuild=iVerBuild; }
	StdStrBuf GetString() const
	{ return FormatString("%s %d.%d.%d.%d [%d]", sEngineName.getData(), (int)iVer[0], (int)iVer[1], (int)iVer[2], (int)iVer[3], (int)iBuild); }
	bool operator == (const C4GameVersion &rCmp) const
	{ return /*sEngineName==rCmp.sEngineName &&*/ iVer[0]==rCmp.iVer[0] && iVer[1]==rCmp.iVer[1] && iVer[2]==rCmp.iVer[2] && iVer[3]==rCmp.iVer[3] && iBuild==rCmp.iBuild; }

	void CompileFunc(StdCompiler *pComp, bool fEngineName)
	{
		if (fEngineName)
		{
			pComp->Value(mkDefaultAdapt(sEngineName, ""));
			pComp->Seperator();
		}
		else if (pComp->isCompiler())
			sEngineName = "";
		pComp->Value(mkArrayAdapt(iVer,4,0));
		pComp->Seperator();
		pComp->Value(mkDefaultAdapt(iBuild, 0));
	}
};

// helper
inline int CompareVersion(int iVer1, int iVer2, int iVer3, int iVer4,
                          int iRVer1 = C4XVER1, int iRVer2 = C4XVER2, int iRVer3 = C4XVER3, int iRVer4 = C4XVER4)
{
	if (iVer1 > iRVer1) return 1; if (iVer1 < iRVer1) return -1;
	if (iVer2 > iRVer2) return 1; if (iVer2 < iRVer2) return -1;
	if (iVer3 > iRVer3) return 1; if (iVer3 < iRVer3) return -1;
	if (iVer4 > iRVer4) return 1; if (iVer4 < iRVer4) return -1;
	return 0;
}

#endif // C4GAMEVERSION_H
