/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2011-2015, The OpenClonk Team and contributors
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

#include "lib/C4Random.h"
#include "c4group/C4Group.h"
#include "script/C4Aul.h"
#include "script/C4ScriptHost.h"
#include <C4DefList.h>

/* StandaloneStubs.cpp is shared with mape, which has a real implementation of these */
C4Def* C4DefList::GetByName(const StdStrBuf &) {return NULL;}
C4Def * C4DefList::GetDef(int) {return 0;}
int C4DefList::GetDefCount() {return 0;}
void C4DefList::CallEveryDefinition() {}
void C4DefList::ResetIncludeDependencies() {}

void InitializeC4Script()
{
	InitCoreFunctionMap(&ScriptEngine);

	// Seed PRNG
	FixedRandom(time(NULL));
}

C4Value RunLoadedC4Script()
{
	// Link script engine (resolve includes/appends, generate code)
	ScriptEngine.Link(NULL);

	C4Value result = GameScript.Call("Main");
	GameScript.Clear();
	ScriptEngine.Clear();
	return result;
}

int c4s_runfile(const char * filename)
{
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

	InitializeC4Script();
	GameScript.Load(File, fn.getData(), NULL, NULL);
	RunLoadedC4Script();
	return 0;
}

int c4s_runstring(const char *script)
{
	InitializeC4Script();
	GameScript.LoadData("<memory>", script, NULL);
	RunLoadedC4Script();
	return 0;
}
