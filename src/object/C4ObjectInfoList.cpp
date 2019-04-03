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

/* Dynamic list for crew member info */

#include "C4Include.h"
#include "object/C4ObjectInfoList.h"

#include "c4group/C4Components.h"
#include "object/C4Def.h"
#include "object/C4DefList.h"
#include "object/C4GameObjects.h"
#include "object/C4ObjectInfo.h"
#include "player/C4RankSystem.h"

C4ObjectInfoList::C4ObjectInfoList()
{
	Default();
}

C4ObjectInfoList::~C4ObjectInfoList()
{
	Clear();
}

void C4ObjectInfoList::Default()
{
	First = nullptr;
	iNumCreated = 0;
}

void C4ObjectInfoList::Clear()
{
	C4ObjectInfo *next;
	while (First)
	{
		next = First->Next;
		delete First;
		First = next;
	}
}

int32_t C4ObjectInfoList::Load(C4Group &hGroup)
{
	C4ObjectInfo *ninf;
	int32_t infn = 0;
	char entryname[256 + 1];

	// Search all oci files
	hGroup.ResetSearch();
	while (hGroup.FindNextEntry(C4CFN_ObjectInfoFiles, entryname))
	{
		if ((ninf = new C4ObjectInfo))
		{
			if (ninf->Load(hGroup,entryname))
			{
				Add(ninf);
				infn++;
			}
			else
			{
				delete ninf;
			}
		}
	}

	// Search subfolders
	hGroup.ResetSearch();
	while (hGroup.FindNextEntry("*", entryname))
	{
		C4Group ItemGroup;
		if (ItemGroup.OpenAsChild(&hGroup, entryname))
		{
			Load(ItemGroup);
		}
	}

	return infn;
}

bool C4ObjectInfoList::Add(C4ObjectInfo *pInfo)
{
	if (!pInfo)
	{
		return false;
	}
	pInfo->Next = First;
	First = pInfo;
	return true;
}

void C4ObjectInfoList::MakeValidName(char *sName)
{
	// Append a number to the name if it already exists, starting at 2

	// 1 space, up to 10 characters for integer max size,
	// 1 terminating zero, 1 extra space for overflow (negative value) = 13 characters
	char number_suffix[13];
	size_t namelen = SLen(sName);
	for (int32_t identifier = 2; NameExists(sName); identifier++)
	{
		sprintf(number_suffix, " %d", identifier);
		SCopy(number_suffix, sName + std::min(namelen, C4MaxName - SLen(number_suffix)));
	}
}

bool C4ObjectInfoList::NameExists(const char *szName)
{
	C4ObjectInfo *cinf;
	for (cinf = First; cinf; cinf = cinf->Next)
	{
		if (SEqualNoCase(szName, cinf->Name))
		{
			return true;
		}
	}
	return false;
}

C4ObjectInfo* C4ObjectInfoList::GetIdle(C4ID c_id, C4DefList &rDefs)
{
	C4Def *pDef;
	C4ObjectInfo *pInfo;
	C4ObjectInfo *pHiExp = nullptr;

	// Search list
	for (pInfo = First; pInfo; pInfo = pInfo->Next)
	{
		// Valid only
		if ((pDef = rDefs.ID2Def(pInfo->id)))
		{
			// Use standard crew or matching id
			if ( (!c_id && !pDef->NativeCrew) || (pInfo->id == c_id) )
			{
				// Participating and not in action
				if (pInfo->Participation && !pInfo->InAction && !pInfo->HasDied)
				{
					// Highest experience
					if (!pHiExp || (pInfo->Experience > pHiExp->Experience))
					{
						// Set this
						pHiExp = pInfo;
					}
				}
			}
		}
	}

	// Found
	if (pHiExp)
	{
		pHiExp->Recruit();
		return pHiExp;
	}

	return nullptr;
}

C4ObjectInfo* C4ObjectInfoList::New(C4ID n_id, C4DefList *pDefs)
{
	C4ObjectInfo *pInfo;
	// Create new info object
	if (!(pInfo = new C4ObjectInfo))
	{
		return nullptr;
	}
	// Default type clonk if none specified
	if (n_id == C4ID::None)
	{
		n_id = C4ID::Clonk;
	}
	// Check type valid and def available
	C4Def *pDef = nullptr;
	if (pDefs && !(pDef = pDefs->ID2Def(n_id)))
	{
		delete pInfo;
		return nullptr;
	}
	// Set name source
	const char *cpNames = Game.Names.GetData();
	if (pDef->pClonkNames)
	{
		cpNames = pDef->pClonkNames->GetData();
	}
	// Default by type
	((C4ObjectInfoCore*) pInfo)->Default(n_id, pDefs, cpNames);
	// Set birthday
	pInfo->Birthday = time(nullptr); // TODO: Use the function from C4ObjectInfo here, instead of assigning it manually
	// Make valid names
	MakeValidName(pInfo->Name);
	// Add
	Add(pInfo);
	++iNumCreated;
	return pInfo;
}

void C4ObjectInfoList::Evaluate()
{
	C4ObjectInfo *cinf;
	for (cinf = First; cinf; cinf = cinf->Next)
	{
		cinf->Evaluate();
	}
}

bool C4ObjectInfoList::Save(C4Group &hGroup, bool fSavegame, bool fStoreTiny, C4DefList *pDefs)
{
	// Save in opposite order (for identical crew order on load)
	C4ObjectInfo *pInfo;
	for (pInfo = GetLast(); pInfo; pInfo = GetPrevious(pInfo))
	{
		// Don't save TemporaryCrew in regular player files
		if (!fSavegame)
		{
			C4Def *pDef = C4Id2Def(pInfo->id);
			if (pDef && pDef->TemporaryCrew)
			{
				continue;
			}
		}
		// save
		if (!pInfo->Save(hGroup, fStoreTiny, pDefs))
		{
			return false;
		}
	}
	return true;
}

C4ObjectInfo* C4ObjectInfoList::GetIdle(const char *szByName)
{
	C4ObjectInfo *pInfo;
	// Find matching name, participating, alive and not in action
	for (pInfo = First; pInfo; pInfo = pInfo->Next)
	{
		if (SEqualNoCase(pInfo->Name, szByName)
		&&  pInfo->Participation
		&& !pInfo->InAction
		&& !pInfo->HasDied)
		{
			pInfo->Recruit();
			return pInfo;
		}
	}
	return nullptr;
}

void C4ObjectInfoList::DetachFromObjects()
{
	C4ObjectInfo *cinf;
	for (cinf = First; cinf; cinf = cinf->Next)
	{
		::Objects.ClearInfo(cinf);
	}
}

C4ObjectInfo* C4ObjectInfoList::GetLast()
{
	C4ObjectInfo *cInfo = First;
	while (cInfo && cInfo->Next)
	{
		cInfo = cInfo->Next;
	}
	return cInfo;
}

C4ObjectInfo* C4ObjectInfoList::GetPrevious(C4ObjectInfo *pInfo)
{
	for (C4ObjectInfo *cInfo = First; cInfo; cInfo=cInfo->Next)
	{
		if (cInfo->Next == pInfo)
		{
			return cInfo;
		}
	}
	return nullptr;
}

bool C4ObjectInfoList::IsElement(C4ObjectInfo *pInfo)
{
	for (C4ObjectInfo *cInfo = First; cInfo; cInfo = cInfo->Next)
	{
		if (cInfo == pInfo)
		{
			return true;
		}
	}
	return false;
}

void C4ObjectInfoList::Strip(C4DefList &rDefs)
{
	C4ObjectInfo *pInfo;
	C4ObjectInfo *pPrev;
	// Search list
	for (pInfo = First, pPrev = nullptr; pInfo; )
	{
		// Invalid?
		if (!rDefs.ID2Def(pInfo->id))
		{
			C4ObjectInfo *pDelete = pInfo;
			if (pPrev)
			{
				pPrev->Next = pInfo->Next;
			}
			else
			{
				First = pInfo->Next;
			}
			pInfo = pInfo->Next;
			delete pDelete;
		}
		else
		{
			pPrev = pInfo;
			pInfo = pInfo->Next;
		}
	}
}
