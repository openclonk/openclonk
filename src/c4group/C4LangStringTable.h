/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2013, The OpenClonk Team and contributors
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
	int32_t ref_count; // ref counter initialized to 1 on ctor; delete when zero is reached
public:
	C4LangStringTable();
	const std::string &Translate(const std::string &text) const;
	bool HasTranslation(const std::string &text) const;
	// do replacement in buffer
	// if any replacement is done, the buffer will be realloced
	void ReplaceStrings(StdStrBuf &rBuf);
	void ReplaceStrings(const StdStrBuf &rBuf, StdStrBuf &rTarget);

	void AddRef() { ++ref_count;  }
	void DelRef() { if (!--ref_count) delete this; }

	class NoSuchTranslation : public std::runtime_error
	{
	public:
		NoSuchTranslation(const std::string &text) : std::runtime_error("No such translation: \"" + text + "\"") {}
	};

	static inline C4LangStringTable &GetSystemStringTable() { return system_string_table; }
protected:
	virtual void OnLoad() { strings.clear(); } // Make sure we re-populate when the string table is reloaded

private:
	static C4LangStringTable system_string_table;
};

#endif // INC_C4LangStringTable
