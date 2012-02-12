/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2004  Matthes Bender
 * Copyright (c) 2005, 2007, 2009, 2011  Günther Brammer
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

#ifndef INC_C4ComponentHost
#define INC_C4ComponentHost

#include "C4GroupSet.h"
#include <C4Language.h>

class C4ComponentHost
{
public:
	C4ComponentHost() { }
	virtual ~C4ComponentHost() { Clear(); }
	const char *GetFilePath() const { return FilePath.getData(); }
	void Clear() { Data.Clear(); OnLoad(); }
	const char *GetData() const { return Data.getData(); }
	const StdStrBuf & GetDataBuf() const { return Data; }
	size_t GetDataSize() const { return Data.getLength(); }
	bool Load(C4Group &hGroup, const char *szFilename, const char *szLanguage=NULL);
	bool Load(C4GroupSet &hGroupSet, const char *szFilename, const char *szLanguage=NULL);
	bool LoadEx(C4Group &hGroup, const char *szFilename, const char *szLanguage=NULL)
	{
		C4GroupSet hGroups = Languages.GetPackGroups(hGroup);
		return Load(hGroups, szFilename, szLanguage);
	}
	bool GetLanguageString(const char *szLanguage, class StdStrBuf &rTarget);
protected:
	// The component host's Data has changed. This callback can be used by
	// derived classes to reload internal structures.
	virtual void OnLoad() {}

	StdCopyStrBuf Data;
	StdCopyStrBuf Filename;
	StdCopyStrBuf FilePath;
	void CopyFilePathFromGroup(const C4Group &hGroup);
	void FinishLoad(const StdStrBuf &, C4Group &hGroup);
};

#endif
