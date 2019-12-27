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

/* Holds a single text file component from a group */

#include "C4Include.h"
#include "c4group/C4ComponentHost.h"

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
	char strEntry[_MAX_FNAME_LEN];
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
	char strEntry[_MAX_FNAME_LEN];
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
	// Store actual filename
	hGroup.FindEntry(name.getData(), &Filename);
	CopyFilePathFromGroup(hGroup);

	if (Data.EnsureUnicode())
	{
		LogF("WARNING: File is not encoded as UTF-8 (%s)", FilePath.getData());
	}
	// Skip those stupid "zero width no-break spaces" (also known as Byte Order Marks)
	if (Data[0] == '\xEF' && Data[1] == '\xBB' && Data[2] == '\xBF')
	{
		Data.Move(3,Data.getSize()-3);
		Data.Shrink(3);
	}
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
