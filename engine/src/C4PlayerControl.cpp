/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005-2009, RedWolf Design GmbH, http://www.clonk.de
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
// Input to player control mapping

#include <C4Include.h>
#include <C4PlayerControl.h>


/* C4PlayerControlDef */

void C4PlayerControlDef::CompileFunc(StdCompiler *pComp)
	{
	if (!pComp->Name("ControlDef")) pComp->excNotFound("ControlDef");
	pComp->Value(mkNamingAdapt(mkParAdapt(sIdentifier, StdCompiler::RCT_Idtf), "Identifier", "None"));
	pComp->Value(mkNamingAdapt(mkParAdapt(sGUIName, StdCompiler::RCT_All), "GUIName", "undefined"));
	pComp->Value(mkNamingAdapt(mkParAdapt(sGUIDesc, StdCompiler::RCT_All), "GUIDesc", ""));
	pComp->Value(mkNamingAdapt(fIsHoldKey, "IsHoldKey", false));
	pComp->Value(mkNamingAdapt(iRepeat, "Repeat", 0));
	pComp->Value(mkNamingAdapt(fDefaultDisabled, "DefaultDisabled", false));
	pComp->NameEnd();
	}

bool C4PlayerControlDef::operator ==(const C4PlayerControlDef &cmp) const
	{
	return sIdentifier == cmp.sIdentifier
	    && sGUIName == cmp.sGUIName
	    && sGUIDesc == cmp.sGUIDesc
	    && fIsHoldKey == cmp.fIsHoldKey
	    && iRepeat == cmp.iRepeat
	    && fDefaultDisabled == cmp.fDefaultDisabled;
	}


/* C4PlayerControlDefs */

void C4PlayerControlDefs::CompileFunc(StdCompiler *pComp)
	{
	pComp->Value(mkNamingAdapt(mkSTLContainerAdapt(Defs, StdCompiler::SEP_NONE), "ControlDefs", DefVecImpl()));
	}

void C4PlayerControlDefs::MergeFrom(const C4PlayerControlDefs &Src)
	{
	// copy all defs from source file; overwrite defs of same name if found
	for (DefVecImpl::const_iterator i = Src.Defs.begin(); i != Src.Defs.end(); ++i)
		{
		const C4PlayerControlDef &SrcDef = *i;
		// overwrite if def of same name existed
		int32_t iPrevIdx = GetControlIndexByIdentifier(SrcDef.GetIdentifier());
		if (iPrevIdx != CON_None)
			{
			Defs[iPrevIdx] = SrcDef;
			}
		else
			{
			// new def: Append a copy
			Defs.push_back(SrcDef);
			}
		}
	}

C4PlayerControlDef *C4PlayerControlDefs::GetControlByIndex(int32_t idx)
	{
	// safe index
	if (idx<0 || idx>=Defs.size()) return NULL;
	return &(Defs[idx]);
	}

int32_t C4PlayerControlDefs::GetControlIndexByIdentifier(const char *szIdentifier) const
	{
	for (DefVecImpl::const_iterator i = Defs.begin(); i != Defs.end(); ++i)
		if (SEqual((*i).GetIdentifier(), szIdentifier))
			return i-Defs.begin();
	return CON_None;
	}


/* C4PlayerControlAssignment */

void C4PlayerControlAssignment::KeyComboItem::CompileFunc(StdCompiler *pComp)
	{
	// if key is compiled, also store as a string into KeyName for later resolving
	if (pComp->isCompiler())
		{
		sKeyName.Clear();
		pComp->Value(mkParAdapt(Key, &sKeyName));
		if (!sKeyName)
			{
			// key was not assigned during compilation - this means it's a regular key (or undefined)
			// store this as the name
			sKeyName.Copy(Key.ToString(false, false));
			}
		}
	else
		{
		// decompiler: Just write the stored key name; regardless of whether it's a key, undefined or a reference
		pComp->Value(mkParAdapt(sKeyName, StdCompiler::RCT_Idtf));
		}
	}

void C4PlayerControlAssignment::CompileFunc(StdCompiler *pComp)
	{
	if (!pComp->Name("ControlAssignment")) pComp->excNotFound("ControlAssignment");
	pComp->Value(mkNamingAdapt(mkSTLContainerAdapt(KeyCombo), "Key", KeyComboVec()));
	pComp->Value(mkNamingAdapt(mkParAdapt(sControlName, StdCompiler::RCT_Idtf), "Control", "None"));
	pComp->Value(mkNamingAdapt(fAlwaysUnhandled, "AlwaysUnhandled", false));
	const StdEnumEntry<TriggerModes> TriggerModeNames[] = {
		{ "Default",      CTM_Default  },
		{ "Hold",         CTM_Hold     },
		{ "Release",      CTM_Release  },
		{ NULL, CTM_Default } };
	pComp->Value(mkNamingAdapt(mkEnumAdapt<TriggerModes, int32_t>(eTriggerMode, TriggerModeNames), "TriggerMode", CTM_Default));
	pComp->NameEnd();
	}

bool C4PlayerControlAssignment::operator ==(const C4PlayerControlAssignment &cmp) const
	{
	// doesn't compare resolved TriggerKey/iControl
	return KeyCombo == cmp.KeyCombo
	    && sControlName == cmp.sControlName
	    && fAlwaysUnhandled == cmp.fAlwaysUnhandled
		&& eTriggerMode == cmp.eTriggerMode;
	}


/* C4PlayerControlAssignmentSet */

void C4PlayerControlAssignmentSet::CompileFunc(StdCompiler *pComp)
	{
	if (!pComp->Name("ControlAssignmentSet")) pComp->excNotFound("ControlAssignmentSet");
	pComp->Value(mkNamingAdapt(mkParAdapt(sName, StdCompiler::RCT_Idtf), "Name", "None"));
	pComp->Value(mkNamingAdapt(mkSTLContainerAdapt(Assignments, StdCompiler::SEP_NONE), "Key", AssignmentsVec()));
	pComp->NameEnd();
	}

void C4PlayerControlAssignmentSet::MergeFrom(const C4PlayerControlAssignmentSet &Src, bool fLowPrio)
	{
	// take over all assignments defined in Src
	for (AssignmentsVec::const_iterator i = Src.Assignments.begin(); i != Src.Assignments.end(); ++i)
		{
		const C4PlayerControlAssignment &SrcAssignment = *i;
		// overwrite if def of same name existed if it's not low priority anyway
		C4PlayerControlAssignment *pPrevAssignment = GetAssignmentByControlName(SrcAssignment.GetControlName());
		if (pPrevAssignment)
			{
			if (!fLowPrio) *pPrevAssignment = SrcAssignment;
			}
		else
			{
			// new def: Append a copy
			Assignments.push_back(SrcAssignment);
			}
		}
	}

C4PlayerControlAssignment *C4PlayerControlAssignmentSet::GetAssignmentByControlName(const char *szControlName) const
	{
	for (AssignmentsVec::const_iterator i = Assignments.begin(); i != Assignments.end(); ++i)
		if (SEqual((*i).GetControlName(), szControlName))
			return &*i;
	return NULL;
	}


/* C4PlayerControlAssignmentSets */

