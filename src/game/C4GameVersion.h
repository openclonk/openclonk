/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2010-2016, The OpenClonk Team and contributors
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
#ifndef C4GAMEVERSION_H

#define C4GAMEVERSION_H

#include "C4Version.h"
#include "lib/C4InputValidation.h"
#include <utility>

struct C4GameVersion
{
	ValidatedStdCopyStrBuf<C4InVal::VAL_NameAllowEmpty> sEngineName; // status only - not used for comparison
	int32_t iVer[2];

	C4GameVersion(const char *szEngine=C4ENGINENAME, int32_t iVer1=C4XVER1, int32_t iVer2=C4XVER2)
	{ Set(szEngine, iVer1, iVer2); }
	void Set(const char *szEngine=C4ENGINENAME, int32_t iVer1=C4XVER1, int32_t iVer2=C4XVER2)
	{ sEngineName.CopyValidated(szEngine); iVer[0]=iVer1; iVer[1]=iVer2; }
	StdStrBuf GetString() const
	{ return FormatString("%s %d.%d", sEngineName.getData(), (int)iVer[0], (int)iVer[1]); }
	bool operator == (const C4GameVersion &rCmp) const
	{ return iVer[0]==rCmp.iVer[0] && iVer[1]==rCmp.iVer[1]; }

	void CompileFunc(StdCompiler *pComp, bool fEngineName)
	{
		if (fEngineName)
		{
			pComp->Value(mkDefaultAdapt(sEngineName, ""));
			pComp->Separator();
		}
		else if (pComp->isDeserializer())
			sEngineName = "";
		pComp->Value(mkArrayAdapt(iVer,2,0));
	}
};

// helper
inline int CompareVersion(int iVer1, int iVer2,
                          int iRVer1 = C4XVER1, int iRVer2 = C4XVER2)
{
	auto ver = std::make_pair(iVer1, iVer2);
	auto rVer = std::make_pair(iRVer1, iRVer2);

	if (ver < rVer) return -1;
	if (ver > rVer) return 1;
	return 0;
}

#endif // C4GAMEVERSION_H
