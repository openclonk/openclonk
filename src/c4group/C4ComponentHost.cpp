/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2004  Matthes Bender
 * Copyright (c) 2002-2004  Peter Wortmann
 * Copyright (c) 2005, 2007-2009  Günther Brammer
 * Copyright (c) 2005, 2007  Sven Eberhardt
 * Copyright (c) 2009  David Dormagen
 * Copyright (c) 2010  Armin Burgmeier
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

/* Holds a single text file component from a group */

#include <C4Include.h>
#include <C4ComponentHost.h>
#include <C4Application.h>
#include <C4Language.h>
#include <StdRegistry.h>

C4ComponentHost::C4ComponentHost()
{
	Default();
}

C4ComponentHost::~C4ComponentHost()
{
	Clear();
}

void C4ComponentHost::Default()
{
	Data.Clear();
	Name[0]=0;
	Filename[0]=0;
	FilePath[0]=0;
}

void C4ComponentHost::Clear()
{
	Data.Clear();
	OnLoad();
}

bool C4ComponentHost::Load(const char *szName,
                           C4Group &hGroup,
                           const char *szFilename,
                           const char *szLanguage)
{
	// Clear any old stuff
	Clear();
	// Store name & filename
	SCopy(szName, Name);
	SCopy(szFilename, Filename);
	// Load component - try all segmented filenames
	char strEntry[_MAX_FNAME+1], strEntryWithLanguage[_MAX_FNAME+1];
	for (int iFilename = 0; SCopySegment(Filename, iFilename, strEntry, '|'); iFilename++)
	{
		// Try to insert all language codes provided into the filename
		char strCode[3] = "";
		for (int iLang = 0; SCopySegment(szLanguage ? szLanguage : "", iLang, strCode, ',', 2); iLang++)
		{
			// Insert language code
			sprintf(strEntryWithLanguage, strEntry, strCode);
			if (hGroup.LoadEntryString(strEntryWithLanguage, Data))
			{
				Data.EnsureUnicode();
				// Skip those stupid "zero width no-break spaces" (also known as Byte Order Marks)
				if (Data[0] == '\xEF' && Data[1] == '\xBB' && Data[2] == '\xBF')
				{
					Data.Move(3,Data.getSize()-3);
					Data.Shrink(3);
				}
				// Store actual filename
				hGroup.FindEntry(strEntryWithLanguage, Filename);
				CopyFilePathFromGroup(hGroup);
				// Notify
				OnLoad();
				// Got it
				return true;
			}
			// Couldn't insert language code anyway - no point in trying other languages
			if (!SSearch(strEntry, "%s")) break;
		}
	}
	// Truncate any additional segments from stored filename
	SReplaceChar(Filename, '|', 0);
	CopyFilePathFromGroup(hGroup);
	// Not loaded
	return false;
}

bool C4ComponentHost::Load(const char *szName,
                           C4GroupSet &hGroupSet,
                           const char *szFilename,
                           const char *szLanguage)
{
	// Clear any old stuff
	Clear();
	// Store name & filename
	SCopy(szName, Name);
	SCopy(szFilename, Filename);
	// Load component - try all segmented filenames
	char strEntry[_MAX_FNAME+1], strEntryWithLanguage[_MAX_FNAME+1];
	for (int iFilename = 0; SCopySegment(Filename, iFilename, strEntry, '|'); iFilename++)
	{
		// Try to insert all language codes provided into the filename
		char strCode[3] = "";
		for (int iLang = 0; SCopySegment(szLanguage ? szLanguage : "", iLang, strCode, ',', 2); iLang++)
		{
			// Insert language code
			sprintf(strEntryWithLanguage, strEntry, strCode);
			if (hGroupSet.LoadEntryString(strEntryWithLanguage, Data))
			{
				Data.EnsureUnicode();
				// Store actual filename
				C4Group *pGroup = hGroupSet.FindEntry(strEntryWithLanguage);
				pGroup->FindEntry(strEntryWithLanguage, Filename);
				CopyFilePathFromGroup(*pGroup);
				// Notify
				OnLoad();
				// Got it
				return true;
			}
			// Couldn't insert language code anyway - no point in trying other languages
			if (!SSearch(strEntry, "%s")) break;
		}
	}
	// Truncate any additional segments from stored filename
	SReplaceChar(Filename, '|', 0);
	// skip full path (unknown)
	FilePath[0] = 0;
	// Not loaded
	return false;
}

bool C4ComponentHost::LoadEx(const char *szName,
                             C4Group &hGroup,
                             const char *szFilename,
                             const char *szLanguage)
{
	// Load from a group set containing the provided group and
	// alternative groups for cross-loading from a language pack
	C4GroupSet hGroups;
	hGroups.RegisterGroup(hGroup, false, 1000, C4GSCnt_Component); // Provided group gets highest priority
	hGroups.RegisterGroups(Languages.GetPackGroups(pConfig->AtRelativePath(hGroup.GetFullName().getData())), C4GSCnt_Language);
	// Load from group set
	return Load(szName, hGroups, szFilename, szLanguage);
}

bool C4ComponentHost::LoadAppend(const char *szName,
                                 C4Group &hGroup, const char *szFilename,
                                 const char *szLanguage)
{
	Clear();

	// Store name & filename
	SCopy(szName,Name);
	SCopy(szFilename,Filename);

	// Load component (segmented filename)
	char str1[_MAX_FNAME+1],str2[_MAX_FNAME+1];
	int iFileCnt = 0, iFileSizeSum = 0;
	int cseg, clseg;
	for (cseg=0; SCopySegment(Filename,cseg,str1,'|'); cseg++)
	{
		char szLang[3] = "";
		for (clseg=0; SCopySegment(szLanguage ? szLanguage : "", clseg, szLang, ',', 2); clseg++)
		{
			sprintf(str2,str1,szLang);
			// Check existance
			size_t iFileSize;
			if (hGroup.FindEntry(str2, NULL, &iFileSize))
			{
				iFileCnt++;
				iFileSizeSum += 1 + iFileSize;
				break;
			}
			if (!SSearch(str1, "%s")) break;
		}
	}

	// No matching files found?
	if (!iFileCnt) return false;

	// Allocate memory
	Data.SetLength(iFileSizeSum);

	// Search files to read contents
	char *pPos = Data.getMData(); *pPos = 0;
	for (cseg=0; SCopySegment(Filename,cseg,str1,'|'); cseg++)
	{
		char szLang[3] = "";
		for (clseg=0; SCopySegment(szLanguage ? szLanguage : "", clseg, szLang, ',', 2); clseg++)
		{
			sprintf(str2,str1,szLang);
			// Load data
			char *pTemp;
			if (hGroup.LoadEntry(str2,&pTemp,NULL,1))
			{
				*pPos++ = '\n';
				SCopy(pTemp, pPos, Data.getPtr(Data.getLength()) - pPos);
				pPos += SLen(pPos);
				delete[] pTemp;
				break;
			}
			delete [] pTemp;
			if (!SSearch(str1, "%s")) break;
		}
	}

	SReplaceChar(Filename,'|',0);
	CopyFilePathFromGroup(hGroup);
	OnLoad();
	return !! iFileCnt;
}

// Construct full path
void C4ComponentHost::CopyFilePathFromGroup(const C4Group &hGroup)
{
	SCopy(pConfig->AtRelativePath(hGroup.GetFullName().getData()), FilePath, _MAX_PATH - 1);
	SAppendChar(DirectorySeparator, FilePath);
	SAppend(Filename, FilePath, _MAX_PATH);
}

bool C4ComponentHost::GetLanguageString(const char *szLanguage, StdStrBuf &rTarget)
{
	const char *cptr;
	// No good parameters
	if (!szLanguage || !Data) return false;
	// Search for two-letter language identifier in text body, i.e. "DE:"
	char langindex[4] = "";
	for (int clseg=0; SCopySegment(szLanguage ? szLanguage : "", clseg, langindex, ',', 2); clseg++)
	{
		SAppend(":",langindex);
		if ((cptr = SSearch(Data.getData(),langindex)))
		{
			// Return the according string
			int iEndPos = SCharPos('\r', cptr);
			if (iEndPos<0) iEndPos = SCharPos('\n', cptr);
			if (iEndPos<0) iEndPos = strlen(cptr);
			rTarget.Copy(cptr, iEndPos);
			return true;
		}
	}
	// Language string not found
	return false;
}
