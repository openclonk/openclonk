/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2011-2013, The OpenClonk Team and contributors
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

#include "../../include/c4script/c4script.h"

#include <C4Include.h>

#include <C4AulDebug.h>
#include <C4Def.h>
#include <C4ScriptHost.h>
#include <C4DefList.h>
#include <C4Log.h>
#include <C4Config.h>
#include <C4GameObjects.h>
#include <C4Material.h>
#include <C4Reloc.h>
#include <C4Record.h>
#include <C4MapScript.h>
#include <C4Random.h>

#ifdef _DEBUG
C4Set<C4PropList *> C4PropList::PropLists;
#endif
C4Set<C4PropListNumbered *> C4PropListNumbered::PropLists;
std::vector<C4PropListNumbered *> C4PropListNumbered::ShelvedPropLists;
int32_t C4PropListNumbered::EnumerationIndex = 0;
C4StringTable Strings;
C4AulScriptEngine ScriptEngine;

C4Config Config;
C4Config::C4Config() {}
C4Config::~C4Config() {}
const char * C4Config::AtRelativePath(char const*s) {return s;}

C4DefList Definitions;
C4DefList::C4DefList() {}
C4DefList::~C4DefList() {}
C4Def* C4DefList::GetByName(const StdStrBuf &) {return NULL;}
C4Def * C4DefList::GetDef(int) {return 0;}
int C4DefList::GetDefCount() {return 0;}
void C4DefList::CallEveryDefinition() {}
void C4DefList::ResetIncludeDependencies() {}
bool C4DefList::DrawFontImage(const char* szImageTag, C4Facet& rTarget, C4DrawTransform* pTransform) { return false; }
float C4DefList::GetFontImageAspect(const char* szImageTag) { return -1.0f; }

C4MaterialMap MaterialMap;
C4MaterialMap::C4MaterialMap() {}
C4MaterialMap::~C4MaterialMap() {}
void C4MaterialMap::UpdateScriptPointers() {}

C4AulDebug *C4AulDebug::pDebug;
void C4AulDebug::DebugStep(C4AulBCC*,C4Value*) {}

C4GameObjects Objects;
C4GameObjects::C4GameObjects() {}
C4GameObjects::~C4GameObjects() {}
void C4GameObjects::UpdateScriptPointers() {}
void C4GameObjects::Clear(bool) {}
void C4GameObjects::Default() {}
bool C4GameObjects::Remove(C4Object*) {return 0;}
bool C4GameObjects::AssignInfo() {return 0;}
bool C4GameObjects::ValidateOwners() {return 0;}
C4Value C4GameObjects::GRBroadcast(char const*, C4AulParSet*, bool, bool) {return C4Value();}

C4ObjectList::C4ObjectList() {}
C4ObjectList::~C4ObjectList() {}
void C4ObjectList::Default() {}
void C4ObjectList::Clear() {}
void C4ObjectList::InsertLinkBefore(C4ObjectLink*, C4ObjectLink*) {}
void C4ObjectList::InsertLink(C4ObjectLink*, C4ObjectLink*) {}
void C4ObjectList::RemoveLink(C4ObjectLink*) {}
bool C4ObjectList::Add(C4Object*, C4ObjectList::SortType, C4ObjectList*) {return 0;}
bool C4ObjectList::Remove(C4Object*) {return 0;}
bool C4ObjectList::AssignInfo() {return 0;}
bool C4ObjectList::ValidateOwners() {return 0;}

void C4NotifyingObjectList::InsertLinkBefore(C4ObjectLink *pLink, C4ObjectLink *pBefore) {}
void C4NotifyingObjectList::InsertLink(C4ObjectLink*, C4ObjectLink*) {}
void C4NotifyingObjectList::RemoveLink(C4ObjectLink*) {}

C4Reloc Reloc;
bool C4Reloc::Open(C4Group&, char const*) const {return false;}

void C4LSector::Clear() {}
void C4Def::IncludeDefinition(C4Def*) {}
bool EraseItemSafe(const char *szFilename) {return false;}
void AddDbgRec(C4RecordChunkType, const void *, int) {}

C4MapScriptHost MapScript;
C4MapScriptHost::C4MapScriptHost() {}
C4MapScriptHost::~C4MapScriptHost() {}
void C4MapScriptHost::Clear() {}
C4PropListStatic *C4MapScriptHost::GetPropList() {return NULL;}
bool C4MapScriptHost::Load(C4Group &, const char *, const char *, C4LangStringTable *) { return false; }
bool C4MapScriptHost::LoadData(const char *, const char *, C4LangStringTable *) { return false; }
void C4MapScriptHost::AddEngineFunctions() {}

int c4s_runscript(const char * filename)
{
	InitCoreFunctionMap(&ScriptEngine);

	// Seed PRNG
	FixedRandom(time(NULL));

	C4Group File;
	if (!File.Open(GetWorkingDirectory()))
	{
		fprintf(stderr, "Open failed: %s\n", File.GetError());
		return 1;
	}

	// get scripts
	StdStrBuf fn;
	File.ResetSearch();
	if (!File.FindNextEntry(filename, &fn))
	{
		fprintf(stderr, "FindNextEntry failed: %s\n", File.GetError());
		return 1;
	}
	GameScript.Load(File, fn.getData(), NULL, NULL);

	// Link script engine (resolve includes/appends, generate code)
	ScriptEngine.Link(&::Definitions);

	// Set name list for globals
	ScriptEngine.GlobalNamed.SetNameList(&ScriptEngine.GlobalNamedNames);
	GameScript.Call("Main");
	GameScript.Clear();
	ScriptEngine.Clear();
	return 0;
}
