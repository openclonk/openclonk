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

/* Value to identify object definitions */

#ifndef INC_C4Id
#define INC_C4Id

#include "lib/StdAdaptors.h"

class C4ID
{
public:
	typedef size_t Handle;
private:
	Handle v;
	typedef std::map<std::string, Handle> LookupTable;
	typedef std::vector<std::string> NamesList;
	static LookupTable lookup;
	static NamesList names;
	void assign(const std::string &s);
	template<size_t N>
	explicit C4ID(const char (&s)[N]) { assign(s); } // @suppress("Class members should be properly initialized"): The call to assign() initializes 'v'
public:
	static const C4ID None; // Invalid ID
	static const C4ID Clonk;
	static const C4ID Bubble;
	static const C4ID EditorBase;

	C4ID(): v(None.v) {}
	C4ID(const C4ID &other) = default;
	C4ID &operator =(const C4ID &other) = default;

	explicit C4ID(const std::string &s);
	explicit C4ID(const StdStrBuf &s) { assign(s.getData()); }

	explicit inline C4ID(Handle i): v(i)
	{
		assert(v < names.size());
	}

	inline const char *ToString() const
	{
		assert(v < names.size());
		return names[v].c_str();
	}
	inline operator std::string () const
	{
		assert(v < names.size());
		return names[v];
	}
	inline Handle GetHandle() const
	{
		return v;
	}

	void CompileFunc(StdCompiler *pComp);

	// Compare names instead of v directly so that a sequence of IDs is synchronous
	inline bool operator ==(const C4ID &other) const { return names[v] == names[other.v]; }
	inline bool operator !=(const C4ID &other) const { return names[v] != names[other.v]; }
	inline bool operator <(const C4ID &other)  const { return names[v] < names[other.v];  }
	inline bool operator >(const C4ID &other)  const { return names[v] > names[other.v];  }
	inline bool operator <=(const C4ID &other) const { return names[v] <= names[other.v]; }
	inline bool operator >=(const C4ID &other) const { return names[v] >= names[other.v]; }

	// Safe bool
	typedef size_t C4ID::*safe_bool_type;
	inline operator safe_bool_type() const { return v == None.v ? nullptr : &C4ID::v; }
};

#endif
