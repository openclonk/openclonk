/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2005  Sven Eberhardt
 * Copyright (c) 2005  Peter Wortmann
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

/* A static list of strings and integer values, i.e. for material amounts */

#include <Standard.h>
#include <C4NameList.h>

C4NameList::C4NameList()
  {
  Clear();
  }

void C4NameList::Clear()
  	{
 	ZeroMem(this,sizeof(C4NameList));
	}

bool C4NameList::Set(const char *szName, int32_t iCount)
	{
	int32_t cnt;
	// Find existing name, set count
	for (cnt=0; cnt<C4MaxNameList; cnt++)
		if (SEqual(Name[cnt],szName))
			{
			Count[cnt]=iCount;
			return true;
			}
	// Find empty spot, set name and count
	for (cnt=0; cnt<C4MaxNameList; cnt++)
		if (Name[cnt][0]==0)
			{
			SCopy(szName,Name[cnt],C4MaxName);
			Count[cnt]=iCount;
			return true;
			}
	// No empty spots
	return false;
	}

bool C4NameList::Read(const char *szSource, int32_t iDefValue)
	{
	char buf[50];
	if (!szSource) return false;
	Clear();
	for (int32_t cseg=0; SCopySegment(szSource,cseg,buf,';',50); cseg++)
		{
		SClearFrontBack(buf);
		int32_t value = iDefValue;
		if (SCharCount('=',buf))
			{
			value = strtol(buf + SCharPos('=',buf) + 1, NULL, 10);
			buf[SCharPos('=',buf)]=0;
			SClearFrontBack(buf);
			}
		if (!Set(buf,value)) return false;
		}
	return true;
	}

bool C4NameList::Write(char *szTarget, bool fValues)
	{
	char buf[50];
	if (!szTarget) return false;
	szTarget[0]=0;
	for (int32_t cnt=0; cnt<C4MaxNameList; cnt++)
		if (Name[cnt][0])
			{
			if (fValues) sprintf(buf,"%s=%d; ",Name[cnt],Count[cnt]);
			else sprintf(buf,"%s; ",Name[cnt]);
			SAppend(buf,szTarget);
			}
	return true;
	}

bool C4NameList::Add(const char *szName, int32_t iCount)
	{
	// Find empty spot, set name and count
	for (int32_t cnt=0; cnt<C4MaxNameList; cnt++)
		if (!Name[cnt][0])
			{
			SCopy(szName,Name[cnt],C4MaxName);
			Count[cnt]=iCount;
			return true;
			}
	// No empty spots
	return false;
	}

bool C4NameList::IsEmpty()
	{
	for (int32_t cnt=0; cnt<C4MaxNameList; cnt++)
		if (Name[cnt][0])
			return false;
	return true;
	}

void C4NameList::CompileFunc(StdCompiler *pComp, bool fValues)
  {
  bool fCompiler = pComp->isCompiler();
	for (int32_t cnt=0; cnt<C4MaxNameList; cnt++)
    if(fCompiler || Name[cnt][0])
      {
      if(cnt) pComp->Seperator(StdCompiler::SEP_SEP2);
      // Name
      pComp->Value(mkDefaultAdapt(mkStringAdapt(Name[cnt], C4MaxName, StdCompiler::RCT_Idtf), ""));
      // Value
      if(fValues)
        {
        pComp->Seperator(StdCompiler::SEP_SET);
        pComp->Value(mkDefaultAdapt(Count[cnt], 0));
        }
      }
  }
