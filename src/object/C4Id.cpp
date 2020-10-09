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

#include "C4Include.h"
#include "object/C4Id.h"

#include "lib/StdCompiler.h"

C4ID::NamesList C4ID::names;
C4ID::LookupTable C4ID::lookup;

const C4ID C4ID::None(std::string("None"));
const C4ID C4ID::Clonk(std::string("Clonk"));
const C4ID C4ID::Bubble(std::string("Fx_Bubble"));
const C4ID C4ID::EditorBase(std::string("EditorBase"));


C4ID::C4ID(const std::string &s) { assign(s); }

void C4ID::assign(const std::string &s)
{
	LookupTable::const_iterator it = lookup.find(s);
	if (it != lookup.end())
	{
		v = it->second;
	}
	else
	{
		v = names.size();
		names.push_back(s);
		lookup.insert(std::make_pair(s, v));
	}
}

void C4ID::CompileFunc(StdCompiler *comp)
{
	if (comp->isSerializer())
	{
		assert(v < names.size());
		comp->String(&names[v][0], names[v].size(), StdCompiler::RCT_ID);
	}
	else
	{
		char *data;
		comp->String(&data, StdCompiler::RCT_ID);
		v = C4ID(data).v;
		StdBuf::DeletePointer(data);
	}
}
