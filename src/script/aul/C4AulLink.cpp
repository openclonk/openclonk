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
// links aul scripts; i.e. resolves includes & appends, etc

#include <C4Include.h>
#include <C4Aul.h>

#include <C4Def.h>
#include <C4DefList.h>
#include <C4Material.h>
#include <C4Game.h>
#include <C4GameObjects.h>

bool C4AulScript::ResolveIncludes(C4DefList *rDefs)
{
	return false;
}

bool C4AulScript::ResolveAppends(C4DefList *rDefs)
{
	return false;
}

// ResolveAppends and ResolveIncludes must be called both
// for each script. ResolveAppends has to be called first!
bool C4ScriptHost::ResolveAppends(C4DefList *rDefs)
{
	// resolve local appends
	if (State != ASS_PREPARSED) return false;
	for (std::list<StdCopyStrBuf>::iterator a = Appends.begin(); a != Appends.end(); ++a)
	{
		if (*a != "*")
		{
			C4Def *Def = rDefs->GetByName(*a);
			if (Def)
			{
				if (std::find(Def->Script.SourceScripts.begin(), Def->Script.SourceScripts.end(), GetScriptHost()) == Def->Script.SourceScripts.end())
					Def->Script.SourceScripts.push_back(GetScriptHost());
			}
			else
			{
				// save id in buffer because AulWarn will use the buffer of C4IdText
				// to get the id of the object in which the error occurs...
				// (stupid static buffers...)
				Warn("#appendto %s not found", a->getData());
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
				if (std::find(pDef->Script.SourceScripts.begin(), pDef->Script.SourceScripts.end(), GetScriptHost()) == pDef->Script.SourceScripts.end())
					pDef->Script.SourceScripts.push_back(GetScriptHost());
			}
		}
	}
	return true;
}

bool C4ScriptHost::ResolveIncludes(C4DefList *rDefs)
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
	for (std::list<StdCopyStrBuf>::reverse_iterator i = Includes.rbegin(); i != Includes.rend(); ++i)
	{
		C4Def *Def = rDefs->GetByName(*i);
		if (Def)
		{
			// resolve #includes in included script first (#include-chains :( )
			if (!Def->Script.IncludesResolved)
				if (!Def->Script.ResolveIncludes(rDefs))
					continue; // skip this #include

			for (std::list<C4ScriptHost *>::reverse_iterator s = Def->Script.SourceScripts.rbegin(); s != Def->Script.SourceScripts.rend(); ++s)
			{
				if (std::find(GetScriptHost()->SourceScripts.begin(), GetScriptHost()->SourceScripts.end(), *s) == GetScriptHost()->SourceScripts.end())
					GetScriptHost()->SourceScripts.push_front(*s);
			}
		}
		else
		{
			// save id in buffer because AulWarn will use the buffer of C4IdText
			// to get the id of the object in which the error occurs...
			// (stupid static buffers...)
			Warn("#include %s not found", i->getData());
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

}

void C4ScriptHost::UnLink()
{
	C4PropList * p = GetPropList();
	if (p)
	{
		p->C4PropList::Clear();
		p->SetProperty(P_Prototype, C4VPropList(Engine->GetPropList()));
	}

	// includes will have to be re-resolved now
	IncludesResolved = false;

	if (State > ASS_PREPARSED) State = ASS_PREPARSED;
}

void C4AulScriptEngine::UnLink()
{
	// unlink scripts
	for (C4AulScript *s = Child0; s; s = s->Next)
		s->UnLink();
	GetPropList()->Thaw();
	if (State > ASS_PREPARSED) State = ASS_PREPARSED;
	// Do not clear global variables and constants, because they are registered by the
	// preparser or other parts. Note that keeping those fields means that you cannot delete a global
	// variable or constant at runtime by removing it from the script.
	//GlobalNamedNames.Reset();
	//GlobalConstNames.Reset();
}

bool C4AulScript::ReloadScript(const char *szPath, const char *szLanguage)
{
	return false;
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

		// parse the scripts to byte code
		for (C4AulScript *s = Child0; s; s = s->Next)
			s->Parse();

		// engine is always parsed (for global funcs)
		State = ASS_PARSED;

		// update material pointers
		::MaterialMap.UpdateScriptPointers();

		rDefs->CallEveryDefinition();

		// Done modifying the proplists now
		for (C4AulScript *s = Child0; s; s = s->Next)
			s->GetPropList()->Freeze();
		GetPropList()->Freeze();

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

