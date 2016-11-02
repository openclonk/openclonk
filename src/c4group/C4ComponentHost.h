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

#ifndef INC_C4ComponentHost
#define INC_C4ComponentHost

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
	bool Load(C4Group &hGroup, const char *szFilename, const char *szLanguage=nullptr);
	bool Load(C4GroupSet &hGroupSet, const char *szFilename, const char *szLanguage=nullptr);
	bool GetLanguageString(const char *szLanguage, StdStrBuf &rTarget);
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
