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

bool C4PropertyDlg::Update(C4ObjectList &rSelection)
{
	if (!Active) return false;
	// Set new selection
	Selection.Copy(rSelection);
	// Update input control
	UpdateInputCtrl(Selection.GetObject());
	// Update contents
	return Update();
}

bool C4PropertyDlg::Update()
{
	if (!Active) return false;

	StdStrBuf Output;

	idSelectedDef=C4ID::None;

	// Compose info text by selected object(s)
	switch (Selection.ObjectCount())
	{
		// No selection
	case 0:
		Output = LoadResStr("IDS_CNS_NOOBJECT");
		break;
		// One selected object
	case 1:
	{
		C4Object *cobj=Selection.GetObject();
		cobj->EnumeratePointers();
		DecompileToBuf_Log<StdCompilerINIWrite>(mkNamingAdapt(*cobj, "Object"), &Output, "C4PropertyDlg::Update");
		cobj->DenumeratePointers();
		// Type
		Output.AppendFormat(LoadResStr("IDS_CNS_TYPE"),cobj->GetName(),cobj->Def->id.ToString());
		// Owner
		if (ValidPlr(cobj->Owner))
		{
			Output.Append(LineFeed);
			Output.AppendFormat(LoadResStr("IDS_CNS_OWNER"),::Players.Get(cobj->Owner)->GetName());
		}
		// Contents
		if (cobj->Contents.ObjectCount())
		{
			Output.Append(LineFeed);
			Output.Append(LoadResStr("IDS_CNS_CONTENTS"));
			Output.Append(static_cast<const StdStrBuf &>(cobj->Contents.GetNameList(::Definitions)));
		}
		// Action
		if (cobj->GetAction())
		{
			Output.Append(LineFeed);
			Output.Append(LoadResStr("IDS_CNS_ACTION"));
			Output.Append(cobj->GetAction()->GetName());
		}
		// Effects
		for (C4Effect *pEffect = cobj->pEffects; pEffect; pEffect = pEffect->pNext)
		{
			// Header
			if (pEffect == cobj->pEffects)
			{
				Output.Append(LineFeed);
				Output.Append(LoadResStr("IDS_CNS_EFFECTS"));
			}
			Output.Append(LineFeed);
			// Effect name
			Output.AppendFormat(" %s: Interval %d", pEffect->Name, pEffect->iInterval);
		}
		// Store selected def
		idSelectedDef=cobj->id;
		break;
	}
	// Multiple selected objects
	default:
		Output.Format(LoadResStr("IDS_CNS_MULTIPLEOBJECTS"),Selection.ObjectCount());
		break;
	}
	// Update info edit control
	Console.PropertyDlgUpdate(this, Output);
	return true;
}

void C4PropertyDlg::UpdateInputCtrl(C4Object *pObj)
{
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
	if (!::Game.iTick35) Update();
}

void C4PropertyDlg::ClearPointers(C4Object *pObj)
{
	Selection.ClearPointers(pObj);
}
