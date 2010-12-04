/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2001  Sven Eberhardt
 * Copyright (c) 2005  Peter Wortmann
 * Copyright (c) 2005, 2008  Günther Brammer
 * Copyright (c) 2009  Nicolas Hake
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

/* Value to identify object definitions */

#ifndef INC_C4Id
#define INC_C4Id

#include "StdAdaptors.h"
#include <map>
#include <string>
#include <vector>

//#include <boost/operators.hpp>

class StdCompiler;
class C4ID //: boost::totally_ordered<C4ID, boost::equivalent<C4ID> >
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
	explicit C4ID(const char (&s)[N]) { assign(s); }
public:
	static const C4ID None; // Invalid ID
	static const C4ID Contents; // Not-ID for funny stuff
	DEPRECATED static const C4ID Energy; // Buildings need energy
	DEPRECATED static const C4ID CnMaterial; // Buildings need construction material
	DEPRECATED static const C4ID StructuresSnowIn;
	DEPRECATED static const C4ID Flag;
	DEPRECATED static const C4ID FlagRemvbl; // Flag removable
	DEPRECATED static const C4ID Linekit;
	DEPRECATED static const C4ID Conkit; // Construction kit
	DEPRECATED static const C4ID SourcePipe;
	DEPRECATED static const C4ID DrainPipe;
	DEPRECATED static const C4ID PowerLine;
	DEPRECATED static const C4ID Clonk;
	DEPRECATED static const C4ID Flame;
	DEPRECATED static const C4ID Meteor;
	DEPRECATED static const C4ID Blast;
	DEPRECATED static const C4ID Melee;
	DEPRECATED static const C4ID TeamworkMelee;
	DEPRECATED static const C4ID Rivalry;
	DEPRECATED static const C4ID Bubble;

	C4ID(): v(None.v) {}
	C4ID(const C4ID &other): v(other.v) {}
	C4ID &operator =(const C4ID &other) { v = other.v; return *this; }

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

	inline bool operator ==(const C4ID &other) const { return v == other.v; }
	inline bool operator !=(const C4ID &other) const { return v != other.v; }
	inline bool operator <(const C4ID &other) const { return v < other.v; }
	inline bool operator >(const C4ID &other) const { return v > other.v; }
	inline bool operator <=(const C4ID &other) const { return v <= other.v; }
	inline bool operator >=(const C4ID &other) const { return v >= other.v; }

	// Safe bool
	typedef size_t C4ID::*safe_bool_type;
	inline operator safe_bool_type() const { return v == None.v ? 0 : &C4ID::v; }
};

#endif
