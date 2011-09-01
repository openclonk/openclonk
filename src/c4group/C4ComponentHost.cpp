/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2004  Matthes Bender
 * Copyright (c) 2002-2004  Peter Wortmann
 * Copyright (c) 2005, 2007  Sven Eberhardt
 * Copyright (c) 2005, 2007-2009, 2011  Günther Brammer
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
#include <StdRegistry.h>

bool C4ComponentHost::Load(C4Group &hGroup,
                           const char *fname,
                           const char *szLanguage)
{
	// Clear any old stuff
	Clear();
	// Store filename
	if (fname)
		Filename.Copy(fname);
	// Load component - try all segmented filenames
	char strEntry[_MAX_FNAME+1];
	StdStrBuf strEntryWithLanguage;
	for (int iFilename = 0; SCopySegment(Filename.getData(), iFilename, strEntry, '|'); iFilename++)
	{
		// Try to insert all language codes provided into the filename
		char strCode[3] = "";
		for (int iLang = 0; SCopySegment(szLanguage ? szLanguage : "", iLang, strCode, ',', 2); iLang++)
		{
			// Insert language code
			strEntryWithLanguage.Format(strEntry, strCode);
			if (hGroup.LoadEntryString(strEntryWithLanguage, &Data))
			{
				FinishLoad(strEntryWithLanguage, hGroup);
				// Got it
				return true;
			}
			// Couldn't insert language code anyway - no point in trying other languages
			if (!SSearch(strEntry, "%s")) break;
		}
	}
	// Truncate any additional segments from stored filename
	SReplaceChar(Filename.getMData(), '|', 0);
	CopyFilePathFromGroup(hGroup);
	// Not loaded
	return false;
}

bool C4ComponentHost::Load(C4GroupSet &hGroupSet,
                           const char *fname,
                           const char *szLanguage)
{
	// Clear any old stuff
	Clear();
	// Store filename
	if (fname)
		Filename.Copy(fname);
	// Load component - try all segmented filenames
	char strEntry[_MAX_FNAME+1];
	StdStrBuf strEntryWithLanguage;
	for (int iFilename = 0; SCopySegment(Filename.getData(), iFilename, strEntry, '|'); iFilename++)
	{
		// Try to insert all language codes provided into the filename
		char strCode[3] = "";
		for (int iLang = 0; SCopySegment(szLanguage ? szLanguage : "", iLang, strCode, ',', 2); iLang++)
		{
			// Insert language code
			strEntryWithLanguage.Format(strEntry, strCode);
			if (hGroupSet.LoadEntryString(strEntryWithLanguage, &Data))
			{
				C4Group *pGroup = hGroupSet.FindEntry(strEntryWithLanguage.getData());
				FinishLoad(strEntryWithLanguage, *pGroup);
				// Got it
				return true;
			}
			// Couldn't insert language code anyway - no point in trying other languages
			if (!SSearch(strEntry, "%s")) break;
		}
	}
	// Truncate any additional segments from stored filename
	SReplaceChar(Filename.getMData(), '|', 0);
	// for error message purposes, the first group failed to provide the desired file
	CopyFilePathFromGroup(*hGroupSet.GetGroup(0));
	// Not loaded
	return false;
}

void C4ComponentHost::FinishLoad(const StdStrBuf & name, C4Group &hGroup)
{
	Data.EnsureUnicode();
	// Skip those stupid "zero width no-break spaces" (also known as Byte Order Marks)
	if (Data[0] == '\xEF' && Data[1] == '\xBB' && Data[2] == '\xBF')
	{
		Data.Move(3,Data.getSize()-3);
		Data.Shrink(3);
	}
	// Store actual filename
	hGroup.FindEntry(name.getData(), &Filename);
	CopyFilePathFromGroup(hGroup);
	// Notify
	OnLoad();
}

// Construct full path
void C4ComponentHost::CopyFilePathFromGroup(const C4Group &hGroup)
{
	FilePath.Copy(Config.AtRelativePath(hGroup.GetFullName().getData()));
	FilePath.AppendBackslash();
	FilePath.Append(Filename);
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
