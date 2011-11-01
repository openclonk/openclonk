/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2011  Günther Brammer
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

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

bool Log(const char *msg)
{
	printf("%s\n", msg);
	return 1;
}
bool DebugLog(const char *strMessage) { return Log(strMessage); }
bool LogFatal(const char *strMessage) { return Log(strMessage); }

#define IMPLEMENT_LOGF(func) \
  bool func(const char *msg, ...) { \
    va_list args; va_start(args, msg); \
    StdStrBuf Buf; \
    Buf.FormatV(msg, args); \
    return Log(Buf.getData()); \
  }

IMPLEMENT_LOGF(DebugLogF)
IMPLEMENT_LOGF(LogF)
IMPLEMENT_LOGF(LogSilentF)

C4Config Config;
C4Config::C4Config() {}
C4Config::~C4Config() {}
const char * C4Config::AtRelativePath(char const*s) {return s;}

C4DefList Definitions;
C4DefList::C4DefList() {}
C4DefList::~C4DefList() {}
C4Def* C4DefList::ID2Def(C4ID id) {return NULL;}
C4Def * C4DefList::GetDef(int) {return 0;}
int C4DefList::GetDefCount() {return 0;}
void C4DefList::CallEveryDefinition() {}
void C4DefList::ResetIncludeDependencies() {}
bool C4DefList::GetFontImage(char const*, CFacet&) {return false;}

C4MaterialMap MaterialMap;
C4MaterialMap::C4MaterialMap() {}
C4MaterialMap::~C4MaterialMap() {}
void C4MaterialMap::UpdateScriptPointers() {}

C4AulDebug *C4AulDebug::pDebug;
void C4AulDebug::DebugStepIn(C4AulBCC*) {}
void C4AulDebug::DebugStepOut(C4AulBCC*, C4AulScriptContext*, C4Value*) {}
void C4AulDebug::DebugStep(C4AulBCC*) {}

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
C4Object * C4GameObjects::ObjectPointer(int) {return 0;}

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

int main(int argc, const char * argv[])
{
	InitCoreFunctionMap(&ScriptEngine);

	C4Group File;
	if (!File.Open(GetWorkingDirectory()))
	{
		fprintf(stderr, "Open failed: %s\n", File.GetError());
		return 1;
	}

	// get scripts
	StdStrBuf fn;
	File.ResetSearch();
	if (!File.FindNextEntry(argv[1], &fn))
	{
		fprintf(stderr, "FindNextEntry failed: %s\n", File.GetError());
		return 1;
	}
	// host will be destroyed by script engine, so drop the references
	GameScript.Reg2List(&ScriptEngine, &ScriptEngine);
	GameScript.Load(File, fn.getData(), NULL, NULL, NULL);
	
	// Link script engine (resolve includes/appends, generate code)
	ScriptEngine.Link(&::Definitions);

	// Set name list for globals
	ScriptEngine.GlobalNamed.SetNameList(&ScriptEngine.GlobalNamedNames);
	GameScript.Call("Main");
	GameScript.Clear();
	return 0;
}
