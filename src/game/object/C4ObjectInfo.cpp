/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2003-2005  Matthes Bender
 * Copyright (c) 2002, 2004, 2006-2007  Sven Eberhardt
 * Copyright (c) 2006, 2009  Günther Brammer
 * Copyright (c) 2006  Peter Wortmann
 * Copyright (c) 2009  Nicolas Hake
 * Copyright (c) 2010  Benjamin Herr
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

/* Holds crew member information */

#include <C4Include.h>
#include <C4ObjectInfo.h>

#include <C4DefList.h>
#include <C4Random.h>
#include <C4Components.h>
#include <C4Game.h>
#include <C4Config.h>
#include <C4Application.h>
#include <C4RankSystem.h>
#include <C4Log.h>
#include <C4Player.h>
#include <C4GraphicsResource.h>
#include <C4PlayerList.h>

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

C4ObjectInfo::C4ObjectInfo()
{
	Default();
}

C4ObjectInfo::~C4ObjectInfo()
{
	Clear();
}

void C4ObjectInfo::Default()
{
	WasInAction=false;
	InAction=false;
	InActionTime=0;
	HasDied=false;
	ControlCount=0;
	Filename[0]=0;
	Next=NULL;
	pDef = NULL;
}

bool C4ObjectInfo::Load(C4Group &hMother, const char *szEntryname)
{

	// New version
	if (SEqualNoCase(GetExtension(szEntryname),"oci"))
	{
		C4Group hChild;
		if (hChild.OpenAsChild(&hMother,szEntryname))
		{
			if (!C4ObjectInfo::Load(hChild))
				{ hChild.Close(); return false; }
			// resolve definition, if possible
			// only works in game, but is not needed in frontend or startup editing anyway
			pDef = C4Id2Def(id);
			hChild.Close();
			return true;
		}
	}

	return false;
}

bool C4ObjectInfo::Load(C4Group &hGroup)
{
	// Store group file name
	SCopy(GetFilename(hGroup.GetName()),Filename,_MAX_FNAME);
	// Load core
	if (!C4ObjectInfoCore::Load(hGroup)) return false;
	return true;
}

bool C4ObjectInfo::Save(C4Group &hGroup, bool fStoreTiny, C4DefList *pDefs)
{
	// Set group file name; rename if necessary
	char szTempGroup[_MAX_PATH+1];
	SCopy(Name, szTempGroup, _MAX_PATH);
	MakeFilenameFromTitle(szTempGroup);
	SAppend(".oci",szTempGroup, _MAX_PATH);
	if (!SEqualNoCase(Filename, szTempGroup))
	{
		if (!Filename[0])
		{
			// first time creation of file - make sure it's not a duplicate
			SCopy(szTempGroup, Filename, _MAX_PATH);
			while (hGroup.FindEntry(Filename))
			{
				// if a crew info of that name exists already, rename!
				RemoveExtension(Filename);
				int32_t iFinNum = GetTrailingNumber(Filename), iLen = SLen(Filename);
				while (iLen && Inside(Filename[iLen-1], '0', '9')) --iLen;
				if (iLen>_MAX_PATH-22) { LogF("Error generating unique filename for %s(%s): Path overflow", Name, hGroup.GetFullName().getData()); break; }
				snprintf(Filename+iLen, 22, "%d", iFinNum+1);
				EnforceExtension(Filename, "oci");
			}
		}
		else
		{
			// Crew was renamed; file rename necessary, if the name is not blocked by another crew info
			if (!hGroup.FindEntry(szTempGroup))
			{
				if (hGroup.Rename(Filename, szTempGroup))
					SCopy(szTempGroup, Filename, _MAX_PATH);
				else
				{
					// could not rename. Not fatal; just use old file
					LogF("Error adjusting crew info for %s into %s: Rename error from %s to %s!", Name, hGroup.GetFullName().getData(), Filename, szTempGroup);
				}
			}
		}
	}
	// Open group
	C4Group hTemp;
	if (!hTemp.OpenAsChild(&hGroup, Filename, false, true))
		return false;
	// custom rank image present?
	if (pDefs && !fStoreTiny)
	{
		C4Def *pDef = pDefs->ID2Def(id);
		if (pDef)
		{
			if (pDef->pRankSymbols)
			{
				C4FacetSurface fctRankSymbol;
				if (C4RankSystem::DrawRankSymbol(&fctRankSymbol, Rank, pDef->pRankSymbols, pDef->iNumRankSymbols, true))
				{
					fctRankSymbol.GetFace().SavePNG(hTemp, C4CFN_ClonkRank);
				}
			}
			else
			{
				// definition does not have custom rank symbols: Remove any rank image from Clonk
				hTemp.Delete(C4CFN_ClonkRank);
			}
		}
	}

	// Save info to temp group
	if (!C4ObjectInfoCore::Save(hTemp, pDefs))
		{ hTemp.Close(); return false; }
	// Close temp group
	hTemp.Close();
	// Success
	return true;
}

void C4ObjectInfo::Evaluate()
{
	Retire();
	if (WasInAction) Rounds++;
}

void C4ObjectInfo::Clear()
{
	pDef=NULL;
}

void C4ObjectInfo::Recruit()
{
	// already recruited?
	if (InAction) return;
	WasInAction=true;
	InAction=true;
	InActionTime=Game.Time;
	// rank name overload by def?
	C4Def *pDef=::Definitions.ID2Def(id);
	if (pDef) if (pDef->pRankNames)
		{
			StdStrBuf sRank(pDef->pRankNames->GetRankName(Rank, true));
			if (sRank) sRankName.Copy(sRank);
		}
}

void C4ObjectInfo::Retire()
{
	// not recruited?
	if (!InAction) return;
	// retire
	InAction=false;
	TotalPlayingTime+=(Game.Time-InActionTime);
}

void C4ObjectInfo::SetBirthday()
{
	Birthday=time(NULL);
}
