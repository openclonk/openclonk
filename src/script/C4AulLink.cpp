/*
 * OpenClonk, http://www.openclonk.org
 *
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
// links aul scripts; i.e. resolves includes & appends, etc

#include "C4Include.h"
#include "script/C4Aul.h"

#include "game/C4Game.h"
#include "landscape/C4Material.h"
#include "object/C4Def.h"
#include "object/C4DefList.h"
#include "object/C4GameObjects.h"
#include "script/C4Effect.h"

void C4ScriptHost::DoAppend(C4Def *def)
{
	if (std::find(def->Script.SourceScripts.begin(), def->Script.SourceScripts.end(), this) == def->Script.SourceScripts.end())
	{
		def->Script.SourceScripts.push_back(this);
		if (!Includes.empty())
		{
			Warn("#appendto contains #include");
			// Try to fullfil #include, but it won't work properly: #appendtos
			// are always appended, but #includes are prepended to the script.
			def->Script.Includes.insert(def->Script.Includes.end(), Includes.begin(), Includes.end());
		}

	}
}

// ResolveAppends and ResolveIncludes must be called both
// for each script. ResolveAppends has to be called first!
bool C4ScriptHost::ResolveAppends(C4DefList *rDefs)
{
	// resolve local appends
	if (State != ASS_PREPARSED) return false;
	for (auto & Append : Appends)
	{
		if (Append != "*" || !rDefs)
		{
			C4Def *Def = rDefs ? rDefs->GetByName(Append) : nullptr;
			if (Def)
			{
				DoAppend(Def);
			}
			else
			{
				// save id in buffer because AulWarn will use the buffer of C4IdText
				// to get the id of the object in which the error occurs...
				// (stupid static buffers...)
				Warn("#appendto %s not found", Append.getData());
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
				DoAppend(pDef);
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
		Engine->GetErrorHandler()->OnError(C4AulParseError(this, "Circular include chain detected - ignoring all includes!").what());
		IncludesResolved = true;
		State = ASS_LINKED;
		return false;
	}
	Resolving=true;
	// append all includes to local script
	for (std::list<StdCopyStrBuf>::reverse_iterator i = Includes.rbegin(); i != Includes.rend(); ++i)
	{
		C4Def *Def = rDefs ? rDefs->GetByName(*i) : nullptr;
		if (Def)
		{
			// resolve #includes in included script first (#include-chains :( )
			if (!Def->Script.IncludesResolved)
				if (!Def->Script.ResolveIncludes(rDefs))
					continue; // skip this #include

			for (auto s = Def->Script.SourceScripts.rbegin(); s != Def->Script.SourceScripts.rend(); ++s)
			{
				if (std::find(SourceScripts.begin(), SourceScripts.end(), *s) == SourceScripts.end())
					SourceScripts.push_front(*s);
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

void C4ScriptHost::UnLink()
{
	C4PropList * p = GetPropList();
	if (p)
	{
		p->C4PropList::Clear();
		p->SetProperty(P_Prototype, C4VPropList(Engine->GetPropList()));
	}

	// Delete cyclic references of owned proplists
	DeleteOwnedPropLists();

	// includes will have to be re-resolved now
	IncludesResolved = false;

	if (State > ASS_PREPARSED) State = ASS_PREPARSED;
}

void C4AulScriptEngine::UnLink()
{
	warnCnt = errCnt = lineCnt = 0;

	// Make everything writeable
	GetPropList()->ThawRecursively();
	for (C4ScriptHost *s = Child0; s; s = s->Next)
		s->GetPropList()->ThawRecursively();

	// unlink scripts
	for (C4ScriptHost *s = Child0; s; s = s->Next)
		s->UnLink();
	// Do not clear global variables and constants, because they are registered by the
	// preparser or other parts. Note that keeping those fields means that you cannot delete a global
	// variable or constant at runtime by removing it from the script.
}

void C4AulScriptEngine::Link(C4DefList *rDefs)
{
	try
	{
		// resolve appends
		for (C4ScriptHost *s = Child0; s; s = s->Next)
			s->ResolveAppends(rDefs);

		// resolve includes
		for (C4ScriptHost *s = Child0; s; s = s->Next)
			s->ResolveIncludes(rDefs);

		// parse the scripts to byte code
		for (C4ScriptHost *s = Child0; s; s = s->Next)
			s->Parse();

		if (rDefs)
		{
			rDefs->SortByPriority();
			rDefs->CallEveryDefinition();
		}

		// Done modifying the proplists now
		for (C4ScriptHost *s = Child0; s; s = s->Next)
			s->GetPropList()->FreezeAndMakeStaticRecursively(&s->ownedPropLists);

		GetPropList()->FreezeAndMakeStaticRecursively(&OwnedPropLists);
	}
	catch (C4AulError &err)
	{
		// error??! show it!
		ErrorHandler->OnError(err.what());
	}

	// Set name list for globals (FIXME: is this necessary?)
	ScriptEngine.GlobalNamed.SetNameList(&ScriptEngine.GlobalNamedNames);
}


void C4AulScriptEngine::ReLink(C4DefList *rDefs)
{
	// unlink scripts
	UnLink();

	// unlink defs
	if (rDefs) rDefs->ResetIncludeDependencies();

	// re-link
	Link(rDefs);

	// display state
	LogF("C4AulScriptEngine linked - %d line%s, %d warning%s, %d error%s",
		lineCnt, (lineCnt != 1 ? "s" : ""), warnCnt, (warnCnt != 1 ? "s" : ""), errCnt, (errCnt != 1 ? "s" : ""));

	// adjust global effects
	if (pGlobalEffects) pGlobalEffects->ReAssignAllCallbackFunctions();
	if (GameScript.pScenarioEffects) GameScript.pScenarioEffects->ReAssignAllCallbackFunctions();
}

bool C4AulScriptEngine::ReloadScript(const char *szScript, const char *szLanguage)
{
	C4ScriptHost * s;
	for (s = Child0; s; s = s->Next)
		if (s->ReloadScript(szScript, szLanguage))
			break;
	return !!s;
}

