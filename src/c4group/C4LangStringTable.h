/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005  Sven Eberhardt
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
// Loads StringTbl* and replaces $..$-strings by localized versions

#ifndef INC_C4LangStringTable
#define INC_C4LangStringTable

#include "C4ComponentHost.h"

#include <map>
#include <string>
#include <stdexcept>

class C4LangStringTable : public C4ComponentHost
{
	// Contains the localization string->string mapping. Populated lazily from PopulateStringTable, thus mutable.
	typedef std::map<std::string, std::string> Table;
	mutable Table strings;
	void PopulateStringTable() const;
public:
	C4LangStringTable();
	std::string Translate(const std::string &text) const;
	bool HasTranslation(const std::string &text) const;
	// do replacement in buffer
	// if any replacement is done, the buffer will be realloced
	void ReplaceStrings(StdStrBuf &rBuf);
	void ReplaceStrings(const StdStrBuf &rBuf, StdStrBuf &rTarget, const char *szParentFilePath = NULL);

	class NoSuchTranslation : public std::runtime_error
	{
	public:
		NoSuchTranslation(const std::string &text) : std::runtime_error("No such translation: \"" + text + "\"") {}
	};
};

#endif // INC_C4LangStringTable
