/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2005  Peter Wortmann
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

/* The C4Action class is merely a simple data structure */

#include <C4Include.h>
#include <C4Object.h>

C4Action::C4Action()
	{
	Default();
	}

C4Action::~C4Action()
	{

	}

void C4Action::Default()
	{
	//pActionDef = 0;
	Dir=DIR_None;
	DrawDir=Dir;
	ComDir=COMD_None;
	Time=0;
	Data=0;
	Target=Target2=NULL;
	Phase=PhaseDelay=0;
	Facet.Default();
	FacetX=FacetY=0;
	t_attach=CNAT_None;
	Animation = NULL;
	}

void C4Action::CompileFunc(StdCompiler *pComp)
	{
	// Note: Compiled directly into "Object"-categories, so beware of name clashes (see C4Object::CompileFunc)
	// FIXME pComp->Value(mkNamingAdapt( toC4CStr(Name),						"Action",							""								));
	pComp->Value(mkNamingAdapt( Dir,											"Dir",								DIR_None					));
	pComp->Value(mkNamingAdapt( ComDir,										"ComDir",							COMD_None					));
	pComp->Value(mkNamingAdapt( Time,											"ActionTime",					0									));
	pComp->Value(mkNamingAdapt( Data,											"ActionData",					0									));
	pComp->Value(mkNamingAdapt( Phase,										"Phase",							0									));
	pComp->Value(mkNamingAdapt( PhaseDelay,								"PhaseDelay",					0									));
	}

