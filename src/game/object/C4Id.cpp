/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2001  Sven Eberhardt
 * Copyright (c) 2008  Günther Brammer
 * Copyright (c) 2009-2010  Nicolas Hake
 * Copyright (c) 2010  Benjamin Herr
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

#include "C4Include.h"
#include "C4Id.h"
#include "StdCompiler.h"

#include <utility>

C4ID::NamesList C4ID::names;
C4ID::LookupTable C4ID::lookup;

const C4ID C4ID::None(std::string("None"));
const C4ID C4ID::Contents(std::string("Contents"));

// TODO: Remove these eventually, since they are deprecated.
const C4ID C4ID::StructuresSnowIn(std::string("STSN"));
const C4ID C4ID::CnMaterial(std::string("CNMT"));
const C4ID C4ID::Flag(std::string("FLAG"));
const C4ID C4ID::FlagRemvbl(std::string("FGRV"));
const C4ID C4ID::Linekit(std::string("CableReel"));
const C4ID C4ID::Conkit(std::string("CNKT"));
const C4ID C4ID::SourcePipe(std::string("SPIP"));
const C4ID C4ID::DrainPipe(std::string("DPIP"));
const C4ID C4ID::Clonk(std::string("Clonk"));
const C4ID C4ID::Flame(std::string("FLAM"));
const C4ID C4ID::Meteor(std::string("METO"));
const C4ID C4ID::Blast(std::string("FXB1"));
const C4ID C4ID::Melee(std::string("MELE"));
const C4ID C4ID::TeamworkMelee(std::string("MEL2"));
const C4ID C4ID::Rivalry(std::string("RVLR"));
const C4ID C4ID::Bubble(std::string("Fx_Bubble"));

C4ID::C4ID(const std::string &s) { assign(s); }

void C4ID::assign(const std::string &s)
{
	LookupTable::const_iterator it = lookup.find(s);
	if (it != lookup.end())
		v = it->second;
	else
	{
		v = names.size();
		names.push_back(s);
		lookup.insert(std::make_pair(s, v));
	}
}

void C4ID::CompileFunc(StdCompiler *pComp)
{
	if (pComp->isDecompiler())
	{
		assert(v < names.size());
		pComp->String(&names[v][0], names[v].size(), StdCompiler::RCT_Idtf);
	}
	else
	{
		char *data;
		pComp->String(&data, StdCompiler::RCT_Idtf);
		v = C4ID(data).v;
		StdBuf::DeletePointer(data);
	}
}
