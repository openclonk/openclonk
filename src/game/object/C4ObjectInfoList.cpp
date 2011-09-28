/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2003  Matthes Bender
 * Copyright (c) 2002, 2004-2005  Sven Eberhardt
 * Copyright (c) 2002, 2008  Peter Wortmann
 * Copyright (c) 2006, 2008-2009  Günther Brammer
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

/* Dynamic list for crew member info */

#include <C4Include.h>
#include <C4ObjectInfoList.h>

#include <C4DefList.h>
#include <C4ObjectInfo.h>
#include <C4Components.h>
#include <C4Game.h>
#include <C4RankSystem.h>
#include <C4Config.h>
#include <C4GameObjects.h>

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
	First=NULL;
	iNumCreated=0;
}

void C4ObjectInfoList::Clear()
{
	C4ObjectInfo *next;
	while (First)
	{
		next=First->Next;
		delete First;
		First=next;
	}
}

int32_t C4ObjectInfoList::Load(C4Group &hGroup)
{
	C4ObjectInfo *ninf;
	int32_t infn=0;
	char entryname[256+1];

	// Search all oci files
	hGroup.ResetSearch();
	while (hGroup.FindNextEntry(C4CFN_ObjectInfoFiles,entryname))
		if ((ninf=new C4ObjectInfo))
		{
			if (ninf->Load(hGroup,entryname))  { Add(ninf); infn++; }
			else delete ninf;
		}

	// Search all c4o files
	/*hGroup.ResetSearch();
	while (hGroup.FindNextEntry("*.c4o",entryname))
	  if (ninf=new C4ObjectInfo)
	    if (ninf->Load(hGroup,entryname)) { Add(ninf); infn++; }
	    else delete ninf;*/

	// Search subfolders
	hGroup.ResetSearch();
	while (hGroup.FindNextEntry("*", entryname))
	{
		C4Group ItemGroup;
		if (ItemGroup.OpenAsChild(&hGroup, entryname))
			Load(ItemGroup);
	}

	return infn;
}

bool C4ObjectInfoList::Add(C4ObjectInfo *pInfo)
{
	if (!pInfo) return false;
	pInfo->Next=First;
	First=pInfo;
	return true;
}

void C4ObjectInfoList::MakeValidName(char *sName)
{
	char tstr[_MAX_PATH];
	int32_t iname,namelen=SLen(sName);
	for (iname=2; NameExists(sName); iname++)
	{
		sprintf(tstr," %d",iname);
		SCopy(tstr,sName+Min<int32_t>(namelen,C4MaxName-SLen(tstr)));
	}
}

bool C4ObjectInfoList::NameExists(const char *szName)
{
	C4ObjectInfo *cinf;
	for (cinf=First; cinf; cinf=cinf->Next)
		if (SEqualNoCase(szName,cinf->Name))
			return true;
	return false;
}

C4ObjectInfo* C4ObjectInfoList::GetIdle(C4ID c_id, C4DefList &rDefs)
{
	C4Def *pDef;
	C4ObjectInfo *pInfo;
	C4ObjectInfo *pHiExp=NULL;

	// Search list
	for (pInfo=First; pInfo; pInfo=pInfo->Next)
		// Valid only
		if ((pDef = rDefs.ID2Def(pInfo->id)))
			// Use standard crew or matching id
			if ( (!c_id && !pDef->NativeCrew) || (pInfo->id==c_id) )
				// Participating and not in action
				if (pInfo->Participation) if (!pInfo->InAction)
						// Not dead
						if (!pInfo->HasDied)
							// Highest experience
							if (!pHiExp || (pInfo->Experience>pHiExp->Experience))
								// Set this
								pHiExp=pInfo;

	// Found
	if (pHiExp)
	{
		pHiExp->Recruit();
		return pHiExp;
	}

	return NULL;
}

C4ObjectInfo* C4ObjectInfoList::New(C4ID n_id, C4DefList *pDefs)
{
	C4ObjectInfo *pInfo;
	// Create new info object
	if (!(pInfo = new C4ObjectInfo)) return NULL;
	// Default type clonk if none specified
	if (n_id == C4ID::None) n_id = C4ID::Clonk;
	// Check type valid and def available
	C4Def *pDef = NULL;
	if (pDefs)
		if (!(pDef = pDefs->ID2Def(n_id)))
			{ delete pInfo; return NULL; }
	// Set name source
	const char *cpNames = Game.Names.GetData();
	if (pDef->pClonkNames) cpNames = pDef->pClonkNames->GetData();
	// Default by type
	((C4ObjectInfoCore*)pInfo)->Default(n_id, pDefs, cpNames);
	// Set birthday
	pInfo->Birthday=time(NULL);
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
	for (cinf=First; cinf; cinf=cinf->Next)
		cinf->Evaluate();
}

bool C4ObjectInfoList::Save(C4Group &hGroup, bool fSavegame, bool fStoreTiny, C4DefList *pDefs)
{
	// Save in opposite order (for identical crew order on load)
	C4ObjectInfo *pInfo;
	for (pInfo = GetLast(); pInfo; pInfo = GetPrevious(pInfo))
	{
		// don't safe TemporaryCrew in regular player files
		if (!fSavegame)
		{
			C4Def *pDef=C4Id2Def(pInfo->id);
			if (pDef) if (pDef->TemporaryCrew) continue;
		}
		// save
		if (!pInfo->Save(hGroup, fStoreTiny, pDefs))
			return false;
	}
	return true;
}

C4ObjectInfo* C4ObjectInfoList::GetIdle(const char *szByName)
{
	C4ObjectInfo *pInfo;
	// Find matching name, participating, alive and not in action
	for (pInfo=First; pInfo; pInfo=pInfo->Next)
		if (SEqualNoCase(pInfo->Name,szByName))
			if (pInfo->Participation) if (!pInfo->InAction)
					if (!pInfo->HasDied)
					{
						pInfo->Recruit();
						return pInfo;
					}
	return NULL;
}

void C4ObjectInfoList::DetachFromObjects()
{
	C4ObjectInfo *cinf;
	for (cinf=First; cinf; cinf=cinf->Next)
		::Objects.ClearInfo(cinf);
}

C4ObjectInfo* C4ObjectInfoList::GetLast()
{
	C4ObjectInfo *cInfo = First;
	while (cInfo && cInfo->Next) cInfo=cInfo->Next;
	return cInfo;
}

C4ObjectInfo* C4ObjectInfoList::GetPrevious(C4ObjectInfo *pInfo)
{
	for (C4ObjectInfo *cInfo = First; cInfo; cInfo=cInfo->Next)
		if (cInfo->Next == pInfo)
			return cInfo;
	return NULL;
}

bool C4ObjectInfoList::IsElement(C4ObjectInfo *pInfo)
{
	for (C4ObjectInfo *cInfo = First; cInfo; cInfo=cInfo->Next)
		if (cInfo == pInfo) return true;
	return false;
}

void C4ObjectInfoList::Strip(C4DefList &rDefs)
{
	C4ObjectInfo *pInfo, *pPrev;
	// Search list
	for (pInfo=First, pPrev=NULL; pInfo; )
	{
		// Invalid?
		if (!rDefs.ID2Def(pInfo->id))
		{
			C4ObjectInfo *pDelete = pInfo;
			if (pPrev) pPrev->Next = pInfo->Next; else First = pInfo->Next;
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
