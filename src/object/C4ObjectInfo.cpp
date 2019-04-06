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

/* Holds crew member information */

#include "C4Include.h"
#include "object/C4ObjectInfo.h"

#include "c4group/C4Components.h"
#include "game/C4Application.h"
#include "graphics/C4GraphicsResource.h"
#include "lib/C4Random.h"
#include "object/C4Def.h"
#include "object/C4DefList.h"
#include "player/C4Player.h"
#include "player/C4PlayerList.h"
#include "player/C4RankSystem.h"

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
	WasInAction = false;
	InAction = false;
	InActionTime = 0;
	HasDied = false;
	ControlCount = 0;
	Filename[0] = 0;
	Next = nullptr;
	pDef = nullptr;
}

bool C4ObjectInfo::Load(C4Group &hMother, const char *szEntryname)
{
	// New version
	if (SEqualNoCase(GetExtension(szEntryname), "oci")) // TODO: Define file extensions in a static constant
	{
		C4Group hChild;
		if (hChild.OpenAsChild(&hMother,szEntryname))
		{
			if (!C4ObjectInfo::Load(hChild))
			{
				hChild.Close();
				return false;
			}
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
	SCopy(GetFilename(hGroup.GetName()), Filename, _MAX_FNAME);
	// Load core
	return C4ObjectInfoCore::Load(hGroup);
}

bool C4ObjectInfo::Save(C4Group &hGroup, bool fStoreTiny, C4DefList *pDefs)
{
	// Set group file name; rename if necessary
	char szTempGroup[_MAX_PATH+1];
	SCopy(Name, szTempGroup, _MAX_PATH);
	MakeFilenameFromTitle(szTempGroup);
	SAppend(".oci", szTempGroup, _MAX_PATH); // TODO: File extension again, this time with a dot
	if (!SEqualNoCase(Filename, szTempGroup))
	{
		if (!Filename[0])
		{
			// First time creation of file - make sure it's not a duplicate
			SCopy(szTempGroup, Filename, _MAX_PATH);
			while (hGroup.FindEntry(Filename))
			{
				// If a crew info of that name exists already, rename!
				RemoveExtension(Filename);
				int32_t iFinNum = GetTrailingNumber(Filename);
				int32_t iLen = SLen(Filename);
				while (iLen && Inside(Filename[iLen-1], '0', '9'))
				{
					--iLen;
				}
				if (iLen > _MAX_PATH-22)
				{
					LogF("Error generating unique filename for %s(%s): Path overflow", Name, hGroup.GetFullName().getData());
					break;
				}
				snprintf(Filename+iLen, 22, "%d", iFinNum+1);
				EnforceExtension(Filename, "oci"); // TODO: File extension again
			}
		}
		else
		{
			// Crew was renamed; file rename necessary, if the name is not blocked by another crew info
			if (!hGroup.FindEntry(szTempGroup))
			{
				if (hGroup.Rename(Filename, szTempGroup))
				{
					SCopy(szTempGroup, Filename, _MAX_PATH);
				}
				else
				{
					// Could not rename. Not fatal; just use old file
					LogF("Error adjusting crew info for %s into %s: Rename error from %s to %s!", Name, hGroup.GetFullName().getData(), Filename, szTempGroup);
				}
			}
		}
	}
	// Open group
	C4Group hTemp;
	if (!hTemp.OpenAsChild(&hGroup, Filename, false, true))
	{
		return false;
	}
	// Custom rank image present?
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
	{
		hTemp.Close();
		return false;
	}
	// Close temp group
	hTemp.Close();
	// Success
	return true;
}

void C4ObjectInfo::Evaluate()
{
	Retire();
	if (WasInAction)
	{
		Rounds++;
	}
}

void C4ObjectInfo::Clear()
{
	pDef = nullptr;
}

void C4ObjectInfo::Recruit()
{
	// Already recruited?
	if (InAction)
	{
		return;
	}
	WasInAction = true;
	InAction = true;
	InActionTime = Game.Time;
	// Rank name overload by def?
	C4Def *pDef = ::Definitions.ID2Def(id);
	if (pDef && pDef->pRankNames)
	{
		StdStrBuf sRank(pDef->pRankNames->GetRankName(Rank, true));
		if (sRank)
		{
			sRankName.Copy(sRank);
		}
	}
}

void C4ObjectInfo::Retire()
{
	// Not recruited?
	if (!InAction)
	{
		return;
	}
	// retire
	InAction = false;
	TotalPlayingTime += (Game.Time - InActionTime);
}

void C4ObjectInfo::SetBirthday()
{
	Birthday = time(nullptr);
}
