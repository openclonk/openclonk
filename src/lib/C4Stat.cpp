/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001, 2007-2008  Peter Wortmann
 * Copyright (c) 2006  Günther Brammer
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
// statistics

#include "C4Include.h"
#include <C4Stat.h>

// ** implemetation of C4MainStat

C4MainStat::C4MainStat()
: pFirst(0)
{
}

C4MainStat::~C4MainStat()
{
}

void C4MainStat::RegisterStat(C4Stat* pStat)
{
	// add to list
	if(!pFirst)
	{
		pFirst = pStat;
		pStat->pNext = 0;
		pStat->pPrev = 0;
	}
	else
	{
		pStat->pNext = pFirst;
		pFirst->pPrev = pStat;
		pStat->pPrev = 0;
		pFirst = pStat;
	}
}

void C4MainStat::UnRegStat(C4Stat* pStat)
{
	// first item?
	if(!pStat->pPrev)
	{
		pFirst = pStat->pNext;
		pStat->pNext = 0;
	}
	// last item?
	else if(!pStat->pNext)
	{
		pStat->pPrev->pNext = 0;
		pStat->pPrev = 0;
	}
	else
	{
		pStat->pNext->pPrev = pStat->pPrev;
		pStat->pPrev->pNext = pStat->pNext;
		pStat->pNext = 0;
		pStat->pPrev = 0;
	}
}

void C4MainStat::Reset()
{
	for(C4Stat* pAkt = pFirst; pAkt; pAkt = pAkt->pNext)
		pAkt->Reset();
}

void C4MainStat::ResetPart()
{
	for(C4Stat* pAkt = pFirst; pAkt; pAkt = pAkt->pNext)
		pAkt->ResetPart();
}


void C4MainStat::Show()
{

	// count stats
	unsigned int iCnt = 0;
	C4Stat* pAkt;
	for(pAkt = pFirst; pAkt; pAkt = pAkt->pNext)
		iCnt++;

	// create array
	C4Stat** StatArray = new C4Stat*[iCnt];
	bool* bHS = new bool[iCnt];

	// sort it
	unsigned int i,ii;
	for(ii=0;ii<iCnt;ii++) bHS[ii] = false;
	for(i=0;i<iCnt;i++)
	{
		C4Stat* pBestStat = NULL;
		unsigned int iBestNr = ~0;

		for(ii=0, pAkt = pFirst; ii<iCnt; ii++, pAkt = pAkt->pNext)
			if(!bHS[ii])
			{
				if(iBestNr == ~0u)
				{
					iBestNr = ii;
					pBestStat = pAkt;
				}
				else if(stricmp(pBestStat->strName, pAkt->strName) > 0)
				{
					iBestNr = ii;
					pBestStat = pAkt;
				}
			}

		if(iBestNr == (unsigned int) -1)
			break;
		bHS[iBestNr] = true;

		StatArray[i] = pBestStat;
	}

	delete bHS;

	LogSilent("** Stat");

	// output in order
	for(i=0; i<iCnt; i++)
	{
		pAkt = StatArray[i];

		// output it!
		if(pAkt->iCount)
			LogSilentF("%s: n = %d, t = %d, td = %.2f",
				pAkt->strName, pAkt->iCount, pAkt->iTimeSum,
				double(pAkt->iTimeSum) / /*Max<int>(1,*/ pAkt->iCount /*- 100)*/ * 1000);

	}

	// delete...
	delete[] StatArray;

	// ok. job done
	LogSilent("** Stat end");
}

void C4MainStat::ShowPart(int FrameCounter)
{
	C4Stat* pAkt;

	// insert tick nr
	LogSilentF("** PartStat begin %d", FrameCounter);

	// insert all stats
	for(pAkt = pFirst; pAkt; pAkt = pAkt->pNext)
		LogSilentF("%s: n=%d, t=%d", pAkt->strName, pAkt->iCountPart, pAkt->iTimeSumPart);

	// insert part stat end idtf
	LogSilentF("** PartStat end\n");
}

// ** implemetation of C4Stat

C4Stat::C4Stat(const char* strnName)
: strName(strnName)
{
	Reset();
	getMainStat()->RegisterStat(this);
}

C4Stat::~C4Stat()
{
	getMainStat()->UnRegStat(this);
}

void C4Stat::Reset()
{
	iStartCalled = 0;

	iTimeSum = 0;
	iCount = 0;

	ResetPart();
}

void C4Stat::ResetPart()
{
	iTimeSumPart = 0;
	iCountPart = 0;
}

C4MainStat *C4Stat::getMainStat()
{
	static C4MainStat *pMainStat = new C4MainStat();
	return pMainStat;
}
