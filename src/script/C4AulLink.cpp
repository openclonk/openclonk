/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001, 2006-2007  Sven Eberhardt
 * Copyright (c) 2001-2002, 2004, 2007  Peter Wortmann
 * Copyright (c) 2006-2009, 2011  Günther Brammer
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
#include <C4DefList.h>
#include <C4Material.h>
#include <C4Game.h>
#include <C4GameObjects.h>

// ResolveAppends and ResolveIncludes must be called both
// for each script. ResolveAppends has to be called first!
bool C4AulScript::ResolveAppends(C4DefList *rDefs)
{
	// resolve local appends
	if (State != ASS_PREPARSED) return false;
	for (std::list<C4ID>::iterator a = Appends.begin(); a != Appends.end(); ++a)
	{
		if (*a)
		{
			C4Def *Def = rDefs->ID2Def(*a);
			if (Def)
				Def->Script.SourceScripts.push_back(GetScriptHost());
			else
			{
				// save id in buffer because AulWarn will use the buffer of C4IdText
				// to get the id of the object in which the error occurs...
				// (stupid static buffers...)
				Warn("script to #appendto not found: ", a->ToString());
			}
		}
		else
		{
			// append to all defs
			for (int i = 0; i < rDefs->GetDefCount(); i++)
			{
				C4Def *pDef = rDefs->GetDef(i);
				if (!pDef) break;
				if (pDef == GetPropList()) continue;
				// append
				pDef->Script.SourceScripts.push_back(GetScriptHost());
			}
		}
	}
	return true;
}

bool C4AulScript::ResolveIncludes(C4DefList *rDefs)
{
	// Had been preparsed?
	if (State != ASS_PREPARSED) return false;
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
	for (std::list<C4ID>::reverse_iterator i = Includes.rbegin(); i != Includes.rend(); ++i)
	{
		C4Def *Def = rDefs->ID2Def(*i);
		if (Def)
		{
			// resolve #includes in included script first (#include-chains :( )
			if (!((C4AulScript &)Def->Script).IncludesResolved)
				if (!Def->Script.ResolveIncludes(rDefs))
					continue; // skip this #include

			for (std::list<C4ScriptHost *>::reverse_iterator s = Def->Script.SourceScripts.rbegin(); s != Def->Script.SourceScripts.rend(); ++s)
				GetScriptHost()->SourceScripts.push_front(*s);
		}
		else
		{
			// save id in buffer because AulWarn will use the buffer of C4IdText
			// to get the id of the object in which the error occurs...
			// (stupid static buffers...)
			Warn("script to #include not found: ", i->ToString());
		}
	}
	IncludesResolved = true;
	// includes/appends are resolved now (for this script)
	Resolving=false;
	State = ASS_LINKED;
	return true;
}

void C4AulScript::UnLink()
{
	// do not unlink temporary (e.g., DirectExec-script in ReloadDef)
	if (Temporary) return;

	C4PropList * p = GetPropList();
	if (p) p->C4PropList::Thaw();

	// delete included/appended functions
	C4AulFunc* pFunc = Func0;
	while (pFunc)
	{
		C4AulFunc* pNextFunc = pFunc->Next;

		// clear stuff that's set in AfterLink
		pFunc->UnLink();

		if (pFunc->SFunc())
			if (pFunc->Owner != pFunc->SFunc()->pOrgScript)
				// do not kill global links; those will be deleted if corresponding sfunc in script is deleted
				if (!pFunc->LinkedTo || pFunc->LinkedTo->SFunc())
				{
					if (p) p->ResetProperty(pFunc->Name);
					delete pFunc;
				}

		pFunc = pNextFunc;
	}

	// includes will have to be re-resolved now
	IncludesResolved = false;

	if (State > ASS_PREPARSED) State = ASS_PREPARSED;
}

void C4AulScriptFunc::UnLink()
{
	OwnerOverloaded = NULL;

	C4AulFunc::UnLink();
}

void C4AulScript::AfterLink() { }

bool C4AulScript::ReloadScript(const char *szPath, const char *szLanguage)
{
	return false;
}

void C4AulScriptEngine::AfterLink()
{
	GlobalPropList->Freeze();
}

void C4AulScriptEngine::Link(C4DefList *rDefs)
{
	try
	{

		// resolve appends
		for (C4AulScript *s = Child0; s; s = s->Next)
			s->ResolveAppends(rDefs);

		// resolve includes
		for (C4AulScript *s = Child0; s; s = s->Next)
			s->ResolveIncludes(rDefs);

		// put script functions into the proplist
		LinkFunctions();

		// parse the scripts to byte code
		for (C4AulScript *s = Child0; s; s = s->Next)
			s->Parse();

		// engine is always parsed (for global funcs)
		State = ASS_PARSED;

		// get common funcs
		for (C4AulScript *s = Child0; s; s = s->Next)
			s->AfterLink();
		AfterLink();

		// update material pointers
		::MaterialMap.UpdateScriptPointers();

		rDefs->CallEveryDefinition();
		// display state
		LogF("C4AulScriptEngine linked - %d line%s, %d warning%s, %d error%s",
		     lineCnt, (lineCnt != 1 ? "s" : ""), warnCnt, (warnCnt != 1 ? "s" : ""), errCnt, (errCnt != 1 ? "s" : ""));

		// reset counters
		warnCnt = errCnt = lineCnt = 0;
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

	// re-link
	Link(rDefs);

	// update effect pointers
	::Objects.UpdateScriptPointers();

	// update material pointers
	::MaterialMap.UpdateScriptPointers();
}

bool C4AulScriptEngine::ReloadScript(const char *szScript, C4DefList *pDefs, const char *szLanguage)
{
	C4AulScript * s;
	for (s = Child0; s; s = s->Next)
		if (s->ReloadScript(szScript, szLanguage))
			break;
	if (!s)
		return false;
	// relink
	ReLink(pDefs);
	// ok
	return true;
}

