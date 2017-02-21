/*
 * OpenClonk, http://www.openclonk.org
 *
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
// statistics

#include "C4Include.h"
#include "lib/C4Stat.h"

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
	if (!pFirst)
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
	if (!pStat->pPrev)
	{
		pFirst = pStat->pNext;
		pStat->pNext = 0;
	}
	// last item?
	else if (!pStat->pNext)
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
	for (C4Stat* pAkt = pFirst; pAkt; pAkt = pAkt->pNext)
		pAkt->Reset();
}

void C4MainStat::ResetPart()
{
	for (C4Stat* pAkt = pFirst; pAkt; pAkt = pAkt->pNext)
		pAkt->ResetPart();
}


void C4MainStat::Show()
{

	// count stats
	unsigned int iCnt = 0;
	C4Stat* pAkt;
	for (pAkt = pFirst; pAkt; pAkt = pAkt->pNext)
		iCnt++;

	// create array
	C4Stat** StatArray = new C4Stat*[iCnt];
	bool* bHS = new bool[iCnt];

	// sort it
	unsigned int i,ii;
	for (ii=0; ii<iCnt; ii++) bHS[ii] = false;
	for (i=0; i<iCnt; i++)
	{
		C4Stat* pBestStat = nullptr;
		unsigned int iBestNr = ~0;

		for (ii=0, pAkt = pFirst; ii<iCnt; ii++, pAkt = pAkt->pNext)
			if (!bHS[ii])
			{
				if (iBestNr == ~0u)
				{
					iBestNr = ii;
					pBestStat = pAkt;
				}
				else if (stricmp(pBestStat->strName, pAkt->strName) > 0)
				{
					iBestNr = ii;
					pBestStat = pAkt;
				}
			}

		if (iBestNr == (unsigned int) -1)
			break;
		bHS[iBestNr] = true;

		StatArray[i] = pBestStat;
	}

	delete [] bHS;

	LogSilent("** Stat");

	// output in order
	for (i=0; i<iCnt; i++)
	{
		pAkt = StatArray[i];

		// output it!
		if (pAkt->iCount)
			LogSilentF("%s: n = %u, t = %u, td = %.2f",
			           pAkt->strName, pAkt->iCount, pAkt->tTimeSum,
			           double(pAkt->tTimeSum) / pAkt->iCount * 1000);
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
	for (pAkt = pFirst; pAkt; pAkt = pAkt->pNext)
		LogSilentF("%s: n=%u, t=%u", pAkt->strName, pAkt->iCountPart, pAkt->tTimeSumPart);

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

	tTimeSum = 0;
	iCount = 0;

	ResetPart();
}

void C4Stat::ResetPart()
{
	tTimeSumPart = 0;
	iCountPart = 0;
}

C4MainStat *C4Stat::getMainStat()
{
	static C4MainStat *pMainStat = new C4MainStat();
	return pMainStat;
}
