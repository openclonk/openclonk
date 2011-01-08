/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001, 2006-2007  Sven Eberhardt
 * Copyright (c) 2001-2002, 2004, 2007  Peter Wortmann
 * Copyright (c) 2006-2009  Günther Brammer
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
// links aul scripts; i.e. resolves includes & appends, etc

#include <C4Include.h>
#include <C4Aul.h>

#include <C4Def.h>
#include <C4Material.h>
#include <C4Game.h>
#include <C4GameObjects.h>

// ResolveAppends and ResolveIncludes must be called both
// for each script. ResolveAppends has to be called first!
bool C4AulScript::ResolveAppends(C4DefList *rDefs)
{
	// resolve children appends
	for (C4AulScript *s = Child0; s; s = s->Next) s->ResolveAppends(rDefs);
	// resolve local appends
	if (State != ASS_PREPARSED) return false;
	for (C4AListEntry *a = Appends; a; a = a->next())
	{
		if (a->Var)
		{
			C4Def *Def = rDefs->ID2Def(a->Var);
			if (Def)
				AppendTo(Def->Script, true);
			else
			{
				// save id in buffer because AulWarn will use the buffer of C4IdText
				// to get the id of the object in which the error occurs...
				// (stupid static buffers...)
				Warn("script to #appendto not found: ", a->Var.ToString());
			}
		}
		else
		{
			// append to all defs
			for (int i = 0; i < rDefs->GetDefCount(); i++)
			{
				C4Def *pDef = rDefs->GetDef(i);
				if (!pDef) break;
				if (pDef == Def) continue;
				// append
				AppendTo(pDef->Script, true);
			}
		}
	}
	return true;
}

bool C4AulScript::ResolveIncludes(C4DefList *rDefs)
{
	// resolve children includes
	for (C4AulScript *s = Child0; s; s = s->Next) s->ResolveIncludes(rDefs);
	// Had been preparsed?
	if (State  != ASS_PREPARSED) return false;
	// has already been resolved?
	if (IncludesResolved) return true;
	// catch circular includes
	if (Resolving)
	{
		C4AulParseError(this, "Circular include chain detected - ignoring all includes!").show();
		IncludesResolved = true;
		State = ASS_LINKED;
		return false;
	}
	Resolving=true;
	// append all includes to local script
	for (C4AListEntry *i = Includes; i; i = i->next())
	{
		C4Def *Def = rDefs->ID2Def(i->Var);
		if (Def)
		{
			// resolve #includes in included script first (#include-chains :( )
			if (!((C4AulScript &)Def->Script).IncludesResolved)
				if (!Def->Script.ResolveIncludes(rDefs))
					continue; // skip this #include

			Def->Script.AppendTo(*this, false);
		}
		else
		{
			// save id in buffer because AulWarn will use the buffer of C4IdText
			// to get the id of the object in which the error occurs...
			// (stupid static buffers...)
			Warn("script to #include not found: ", i->Var.ToString());
		}
	}
	IncludesResolved = true;
	// includes/appends are resolved now (for this script)
	Resolving=false;
	State = ASS_LINKED;
	return true;
}

void C4AulScript::AppendTo(C4AulScript &Scr, bool bHighPrio)
{
	// definition appends
	if (Def && Scr.Def) Scr.Def->IncludeDefinition(Def);
	// append all funcs
	// (in reverse order if inserted at begin of list)
	C4AulScriptFunc *sf;
	for (C4AulFunc *f = bHighPrio ? Func0 : FuncL; f; f = bHighPrio ? f->Next : f->Prev)
		// script funcs only
		if ((sf = f->SFunc()))
			// no need to append global funcs
			if (sf->Access != AA_GLOBAL)
			{
				// append: create copy
				// (if high priority, insert at end, otherwise at the beginning)
				C4AulScriptFunc *sfc = new C4AulScriptFunc(&Scr, sf->Name, bHighPrio);
				sfc->CopyBody(*sf);
				// link the copy to a local function
				if (sf->LinkedTo)
				{
					sfc->LinkedTo = sf->LinkedTo;
					sf->LinkedTo = sfc;
				}
				else
				{
					sfc->LinkedTo = sf;
					sf->LinkedTo = sfc;
				}
			}
	// mark as linked
	// increase code size needed
	// append all local vars (if any existing)
	assert(!Def || this == &Def->Script);
	assert(!Scr.Def || &Scr.Def->Script == &Scr);
	if (LocalNamed.iSize == 0)
		return;
	if (!Scr.Def)
	{
		Warn("could not append local variables to global script!", "");
		return;
	}
	// copy local var definitions
	for (int ivar = 0; ivar < LocalNamed.iSize; ivar ++)
		Scr.LocalNamed.AddName(LocalNamed.pNames[ivar]);

}

void C4AulScript::UnLink()
{
	// unlink children
	for (C4AulScript *s = Child0; s; s = s->Next) s->UnLink();

	// do not unlink temporary (e.g., DirectExec-script in ReloadDef)
	if (Temporary) return;

	// check if byte code needs to be freed
	ClearCode();

	if (Def) Def->C4PropList::Clear();

	// delete included/appended functions
	C4AulFunc* pFunc = Func0;
	while (pFunc)
	{
		C4AulFunc* pNextFunc = pFunc->Next;

		// clear stuff that's set in AfterLink
		pFunc->UnLink();

		if (pFunc->SFunc()) if (pFunc->Owner != pFunc->SFunc()->pOrgScript)
				if (!pFunc->LinkedTo || pFunc->LinkedTo->SFunc()) // do not kill global links; those will be deleted if corresponding sfunc in script is deleted
					delete pFunc;

		pFunc = pNextFunc;
	}
	// includes will have to be re-resolved now
	IncludesResolved = false;


	if (State > ASS_PREPARSED) State = ASS_PREPARSED;
}

void C4AulScriptFunc::UnLink()
{
	OwnerOverloaded = NULL;

	// clear desc information, ParseDesc will set these later on
	idImage = C4ID::None;
	iImagePhase = 0;
	Condition = NULL;
	ControlMethod = C4AUL_ControlMethod_All;

	C4AulFunc::UnLink();
}

void C4AulScript::AfterLink()
{
	// call for childs
	for (C4AulScript *s = Child0; s; s = s->Next) s->AfterLink();
}

bool C4AulScript::ReloadScript(const char *szPath)
{
	// call for childs
	for (C4AulScript *s = Child0; s; s = s->Next)
		if (s->ReloadScript(szPath))
			return true;
	return false;
}

void C4AulScriptEngine::Link(C4DefList *rDefs)
{


	try
	{

		// resolve appends
		ResolveAppends(rDefs);

		// resolve includes
		ResolveIncludes(rDefs);

		// parse script funcs descs
		ParseDescs();

		// parse the scripts to byte code
		Parse();

		// engine is always parsed (for global funcs)
		State = ASS_PARSED;

		// get common funcs
		AfterLink();

		// non-strict scripts?
		if (nonStrictCnt)
		{
			// warn!
			// find first non-#strict script
			C4AulScript *pNonStrictScr=FindFirstNonStrictScript();
			if (pNonStrictScr)
				pNonStrictScr->Warn("using non-#strict syntax!", NULL);
			else
			{
				Warn("non-#strict script detected, but def is lost", NULL);
				Warn("please contact piracy@treffpunktclonk.net for further instructions", NULL);
			}
			Warn(FormatString("%d script%s use non-#strict syntax!", nonStrictCnt, (nonStrictCnt != 1 ? "s" : "")).getData(), NULL);
		}

		// update material pointers
		::MaterialMap.UpdateScriptPointers();

		rDefs->CallEveryDefinition();
		// display state
		LogF("C4AulScriptEngine linked - %d line%s, %d warning%s, %d error%s",
		     lineCnt, (lineCnt != 1 ? "s" : ""), warnCnt, (warnCnt != 1 ? "s" : ""), errCnt, (errCnt != 1 ? "s" : ""));

		// reset counters
		warnCnt = errCnt = nonStrictCnt = lineCnt = 0;
	}
	catch (C4AulError *err)
	{
		// error??! show it!
		err->show();
		delete err;
	}


}


void C4AulScriptEngine::ReLink(C4DefList *rDefs)
{
	// unlink scripts
	UnLink();

	// unlink defs
	if (rDefs) rDefs->ResetIncludeDependencies();

	// clear string table
	::GameScript.UnLink();

	// re-link
	Link(rDefs);

	// update effect pointers
	::Objects.UpdateScriptPointers();

	// update material pointers
	::MaterialMap.UpdateScriptPointers();
}

bool C4AulScriptEngine::ReloadScript(const char *szScript, C4DefList *pDefs)
{
	// reload
	if (!C4AulScript::ReloadScript(szScript))
		return false;
	// relink
	ReLink(pDefs);
	// ok
	return true;
}

