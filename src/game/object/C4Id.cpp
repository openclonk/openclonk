/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2001  Sven Eberhardt
 * Copyright (c) 2008  Günther Brammer
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

#include "C4Include.h"
#include "C4Id.h"
#include "StdCompiler.h"

#include <utility>

C4ID::NamesList C4ID::names;
C4ID::LookupTable C4ID::lookup;

#ifdef _MSC_VER
#	pragma warning (push)
#	pragma warning (disable: 4996)
#endif
const C4ID C4ID::None("None");
const C4ID C4ID::Contents("Contents");
const C4ID C4ID::Energy("ENRG");
const C4ID C4ID::StructuresSnowIn("STSN");
const C4ID C4ID::CnMaterial("CNMT");
const C4ID C4ID::Flag("FLAG");
const C4ID C4ID::FlagRemvbl("FGRV");
const C4ID C4ID::Linekit("LNKT");
const C4ID C4ID::Conkit("CNKT");
const C4ID C4ID::SourcePipe("SPIP");
const C4ID C4ID::DrainPipe("DPIP");
const C4ID C4ID::PowerLine("PWRL");
const C4ID C4ID::Clonk("CLNK");
const C4ID C4ID::Flame("FLAM");
const C4ID C4ID::Meteor("METO");
const C4ID C4ID::Blast("FXB1");
const C4ID C4ID::Melee("MELE");
const C4ID C4ID::TeamworkMelee("MEL2");
const C4ID C4ID::Rivalry("RVLR");
#ifdef _MSC_VER
#	pragma warning (pop)
#endif

C4ID::C4ID(const std::string &s) { assign(s); }
C4ID::C4ID(const char *s) { assign(s); }
C4ID::C4ID(char *s) { assign(s); }

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
