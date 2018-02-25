/*
* OpenClonk, http://www.openclonk.org
*
* Copyright (c) 2018, The OpenClonk Team and contributors
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

#ifndef INC_C4ScriptLibraries
#define INC_C4ScriptLibraries

#include "script/C4PropList.h"

class C4ScriptLibrary : public C4PropListStaticMember
{
public:
	static void InstantiateAllLibraries(C4AulScriptEngine *engine);

protected:
	explicit C4ScriptLibrary(const char *name);
	virtual void CreateFunctions() = 0;

private:
	void RegisterWithEngine(C4AulScriptEngine *engine);
};

#endif
