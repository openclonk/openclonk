/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2001, 2006  Peter Wortmann
 * Copyright (c) 2001  Sven Eberhardt
 * Copyright (c) 2005, 2007, 2009  Günther Brammer
 * Copyright (c) 2006-2007  Armin Burgmeier
 * Copyright (c) 2009  Nicolas Hake
 * Copyright (c) 2010  Mortimer
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

/* Console mode dialog for object properties and script interface */

#include <C4Include.h>
#include <C4PropertyDlg.h>

#include <C4Console.h>
#include <C4Application.h>
#include <C4Object.h>
#include <C4Player.h>
#include <C4Game.h>
#include <C4PlayerList.h>
#include <C4GameObjects.h>

#include <StdRegistry.h>

bool C4PropertyDlg::Open()
{
	Console.PropertyDlgOpen(this);
	Active = true;
	return true;
}

void C4PropertyDlg::Update(C4ObjectList &rSelection)
{
	if (!Active) return;
	C4Object * pObj = rSelection.GetObject();

	// Update info edit control
	Console.PropertyDlgUpdate(this, rSelection.GetDataString());

	// Store selected def
	idSelectedDef=pObj->id;

	// Update input control
	// add global and standard functions
	std::vector<char*> functions;
	for (C4AulFunc *pFn = ::ScriptEngine.GetFirstFunc(); pFn; pFn = ::ScriptEngine.GetNextFunc(pFn))
	{
		if (pFn->GetPublic())
		{
			functions.push_back(pFn->Name);
		}
	}
	// Add object script functions
	bool fDivider = false;
	C4AulScriptFunc *pRef;
	// Object script available
	if (pObj && pObj->Def)
	{
		// Scan all functions
		for (int cnt=0; (pRef=pObj->Def->Script.GetSFunc(cnt)); cnt++)
		{
			// Public functions only
			if ((pRef->Access=AA_PUBLIC))
			{
				// Insert divider if necessary
				if (!fDivider) { functions.push_back((char*)C4ConsoleGUI::LIST_DIVIDER); fDivider=true; }
				// Add function
				functions.push_back(pRef->Name);
			}
		}
	}
	Console.PropertyDlgSetFunctions(this, functions);
}

void C4PropertyDlg::Execute()
{
	if (!::Game.iTick35)
		Console.PropertyDlgUpdate(this, Console.EditCursor.GetSelection().GetDataString());
}
