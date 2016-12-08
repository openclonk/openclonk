/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2013-2016, The OpenClonk Team and contributors
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

/* The C4Action class is merely a simple data structure */

#include "C4Include.h"
#include "object/C4Object.h"

C4Action::C4Action()
{
	Default();
}

C4Action::~C4Action()
{

}

void C4Action::Default()
{
	Dir=DIR_None;
	DrawDir=Dir;
	ComDir=COMD_Stop;
	Time=0;
	Data=0;
	Target=Target2=nullptr;
	Phase=PhaseDelay=0;
	Facet.Default();
	FacetX=FacetY=0;
	t_attach=CNAT_None;
	Animation = nullptr;
}

void C4Action::CompileFunc(StdCompiler *pComp)
{
	// Note: Compiled directly into "Object"-categories, so beware of name clashes (see C4Object::CompileFunc)
	pComp->Value(mkNamingAdapt( Dir,                      "Dir",                DIR_None          ));
	pComp->Value(mkNamingAdapt( ComDir,                   "ComDir",             COMD_Stop         ));
	pComp->Value(mkNamingAdapt( Time,                     "ActionTime",         0                 ));
	pComp->Value(mkNamingAdapt( Data,                     "ActionData",         0                 ));
	pComp->Value(mkNamingAdapt( Phase,                    "Phase",              0                 ));
	pComp->Value(mkNamingAdapt( PhaseDelay,               "PhaseDelay",         0                 ));
}

