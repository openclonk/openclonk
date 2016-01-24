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

#include <C4Include.h>

#include <C4Aul.h>
#include <C4AulDebug.h>
#include <C4Config.h>
#include <C4Def.h>
#include <C4PropList.h>
#include <C4Record.h>
#include <C4Reloc.h>

/* Parts of the ScriptEngine that are normally in C4Globals for initialization order reasons. */
#ifdef _DEBUG
C4Set<C4PropList *> C4PropList::PropLists;
#endif
C4Set<C4PropListNumbered *> C4PropListNumbered::PropLists;
C4Set<C4PropListScript *> C4PropListScript::PropLists;
std::vector<C4PropListNumbered *> C4PropListNumbered::ShelvedPropLists;
int32_t C4PropListNumbered::EnumerationIndex = 0;
C4StringTable Strings;
C4AulScriptEngine ScriptEngine;

/* Stubs */
C4Config Config;
C4Config::C4Config() {}
C4Config::~C4Config() {}
const char * C4Config::AtRelativePath(char const*s) {return s;}

C4AulDebug *C4AulDebug::pDebug;
void C4AulDebug::DebugStep(C4AulBCC*,C4Value*) {}

C4Reloc Reloc;
bool C4Reloc::Open(C4Group&, char const*) const { return false; }

void C4Def::IncludeDefinition(C4Def*) {}
bool EraseItemSafe(const char *szFilename) {return false;}
void AddDbgRec(C4RecordChunkType, const void *, int) {}
