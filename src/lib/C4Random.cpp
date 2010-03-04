/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
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

/* Buffered fast and network-safe random */

#include "C4Include.h"
#include <C4Random.h>
#include <C4Record.h>

int RandomCount = 0;
unsigned int RandomHold = 0;

//--------------------------------------- Random3 -------------------------------------------------------

const int FRndRes=500;

int32_t FRndBuf3[FRndRes];
int32_t FRndPtr3;

void Randomize3()
  {
  FRndPtr3 = 0;
  for (int cnt=0; cnt<FRndRes; cnt++) FRndBuf3[cnt]=Random(3)-1;
  }

int Rnd3()
  {
	FRndPtr3++; if (FRndPtr3==FRndRes) FRndPtr3=0;
#ifdef DEBUGREC
	AddDbgRec(RCT_Rn3, &FRndPtr3, sizeof(int));
#endif
  return FRndBuf3[FRndPtr3];
  }

#ifdef DEBUGREC
int Random(int iRange)
  {
	// next pseudorandom value
	RandomCount++;
	C4RCRandom rc;
	rc.Cnt=RandomCount;
	rc.Range=iRange;
	if (iRange==0)
		rc.Val=0;
	else
		{
		RandomHold = RandomHold * 214013L + 2531011L;
		rc.Val=(RandomHold >> 16) % iRange;
		}
	AddDbgRec(RCT_Random, &rc, sizeof(rc));
	return rc.Val;
  }
#endif

