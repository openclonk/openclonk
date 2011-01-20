/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2004  Matthes Bender
 * Copyright (c) 2001, 2006  Peter Wortmann
 * Copyright (c) 2001-2002, 2004-2007, 2010  Sven Eberhardt
 * Copyright (c) 2004, 2006-2007, 2009-2011  Günther Brammer
 * Copyright (c) 2005, 2009-2010  Armin Burgmeier
 * Copyright (c) 2009-2010  Nicolas Hake
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

#include <C4Include.h>

#include <C4Aul.h>
#include <C4AulDefFunc.h>

//=========================== C4Script Function Map ===================================

C4ScriptConstDef C4ScriptObjectConstMap[]=
{
	{ NULL, C4V_Any, 0}
};

#define MkFnC4V (C4Value (*)(C4AulContext *cthr, C4Value*, C4Value*, C4Value*, C4Value*, C4Value*,\
                                                 C4Value*, C4Value*, C4Value*, C4Value*, C4Value*))

C4ScriptFnDef C4ScriptObjectFnMap[]=
{
	{ NULL,                   0  ,C4V_Any      ,{ C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any     ,C4V_Any    ,C4V_Any    ,C4V_Any    ,C4V_Any}   ,0,                                   0 }
};

void InitObjectFunctionMap(C4AulScriptEngine *pEngine)
{
	// add all def constants (all Int)
	for (C4ScriptConstDef *pCDef = &C4ScriptObjectConstMap[0]; pCDef->Identifier; pCDef++)
	{
		assert(pCDef->ValType == C4V_Int); // only int supported currently
		pEngine->RegisterGlobalConstant(pCDef->Identifier, C4VInt(pCDef->Data));
	}

	// add all def script funcs
	for (C4ScriptFnDef *pDef = &C4ScriptObjectFnMap[0]; pDef->Identifier; pDef++)
		pEngine->AddFunc(pDef->Identifier, pDef);
}
