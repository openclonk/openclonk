/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005-2009, RedWolf Design GmbH, http://www.clonk.de/
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
// Input to player control mapping

#include "C4Include.h"
#include "C4ForbidLibraryCompilation.h"
#include "control/C4PlayerControl.h"

#include "c4group/C4LangStringTable.h"
#include "control/C4Control.h"
#include "control/C4Record.h"
#include "game/C4GraphicsSystem.h"
#include "game/C4Viewport.h"
#include "graphics/C4GraphicsResource.h"
#include "gui/C4MouseControl.h"
#include "object/C4DefList.h"
#include "object/C4Object.h"
#include "object/C4ObjectMenu.h"
#include "platform/C4GamePadCon.h"
#include "player/C4Player.h"
#include "player/C4PlayerList.h"
#include "script/C4Aul.h"

/* C4PlayerControlDef */

void C4PlayerControlDef::CompileFunc(StdCompiler *pComp)
{
	if (!pComp->Name("ControlDef")) { pComp->NameEnd(); pComp->excNotFound("ControlDef"); }
	pComp->Value(mkNamingAdapt(mkParAdapt(sIdentifier, StdCompiler::RCT_Idtf), "Identifier", "None"));
	pComp->Value(mkNamingAdapt(mkParAdapt(sGUIName, StdCompiler::RCT_All), "GUIName", ""));
	pComp->Value(mkNamingAdapt(mkParAdapt(sGUIDesc, StdCompiler::RCT_All), "GUIDesc", ""));
	pComp->Value(mkNamingAdapt(fGlobal, "Global", false));
	pComp->Value(mkNamingAdapt(fIsHoldKey, "Hold", false));
	pComp->Value(mkNamingAdapt(iRepeatDelay, "RepeatDelay", 0));
	pComp->Value(mkNamingAdapt(iInitialRepeatDelay, "InitialRepeatDelay", 0));
	pComp->Value(mkNamingAdapt(fDefaultDisabled, "DefaultDisabled", false));
	pComp->Value(mkNamingAdapt(idControlExtraData, "ExtraData", C4ID::None));
	const StdEnumEntry<CoordinateSpace> CoordSpaceNames[] =
	{
		{ "Game",        COS_Game        },
		{ "Viewport",    COS_Viewport    },
		{ nullptr, COS_Game }
	};
	pComp->Value(mkNamingAdapt(mkEnumAdapt<CoordinateSpace, int32_t>(eCoordSpace, CoordSpaceNames), "CoordinateSpace", COS_Game));
	pComp->Value(mkNamingAdapt(fSendCursorPos, "SendCursorPos", false));
	const StdEnumEntry<Actions> ActionNames[] =
	{
		{ "None",        CDA_None        },
		{ "Script",      CDA_Script      },
		{ "Menu",        CDA_Menu        },
		{ "MenuOK",      CDA_MenuOK      },
		{ "MenuCancel",  CDA_MenuCancel  },
		{ "MenuLeft",    CDA_MenuLeft    },
		{ "MenuUp",      CDA_MenuUp      },
		{ "MenuRight",   CDA_MenuRight   },
		{ "MenuDown",    CDA_MenuDown    },
		{ "ObjectMenuTextComplete", CDA_ObjectMenuTextComplete },
		{ "ObjectMenuOK",    CDA_ObjectMenuOK      },
		{ "ObjectMenuOKAll", CDA_ObjectMenuOKAll   },
		{ "ObjectMenuSelect",CDA_ObjectMenuSelect      },
		{ "ObjectMenuCancel",CDA_ObjectMenuCancel  },
		{ "ObjectMenuLeft",  CDA_ObjectMenuLeft    },
		{ "ObjectMenuUp",    CDA_ObjectMenuUp      },
		{ "ObjectMenuRight", CDA_ObjectMenuRight   },
		{ "ObjectMenuDown",  CDA_ObjectMenuDown    },
		{ "ZoomIn",      CDA_ZoomIn      },
		{ "ZoomOut",     CDA_ZoomOut     },
		{ nullptr, CDA_None }
	};
	pComp->Value(mkNamingAdapt(mkEnumAdapt<Actions, int32_t>(eAction, ActionNames), "Action", CDA_Script));
	pComp->NameEnd();
}

bool C4PlayerControlDef::operator ==(const C4PlayerControlDef &cmp) const
{
	return sIdentifier == cmp.sIdentifier
	       && sGUIName == cmp.sGUIName
	       && sGUIDesc == cmp.sGUIDesc
	       && fGlobal == cmp.fGlobal
	       && fIsHoldKey == cmp.fIsHoldKey
	       && iRepeatDelay == cmp.iRepeatDelay
	       && iInitialRepeatDelay == cmp.iInitialRepeatDelay
	       && fDefaultDisabled == cmp.fDefaultDisabled
	       && idControlExtraData == cmp.idControlExtraData
	       && fSendCursorPos == cmp.fSendCursorPos
	       && eAction == cmp.eAction;
}


/* C4PlayerControlDefs */

void C4PlayerControlDefs::UpdateInternalCons()
{
	InternalCons.CON_ObjectMenuSelect   = GetControlIndexByIdentifier("ObjectMenuSelect");
	InternalCons.CON_ObjectMenuOK       = GetControlIndexByIdentifier("ObjectMenuOK");
	InternalCons.CON_ObjectMenuOKAll    = GetControlIndexByIdentifier("ObjectMenuOKAll");
	InternalCons.CON_ObjectMenuCancel   = GetControlIndexByIdentifier("ObjectMenuCancel");
	InternalCons.CON_CursorPos          = GetControlIndexByIdentifier("CursorPos");
}

void C4PlayerControlDefs::Clear()
{
	clear_previous = false;
	Defs.clear();
	UpdateInternalCons();
}

void C4PlayerControlDefs::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(clear_previous, "ClearPrevious", false));
	pComp->Value(mkSTLContainerAdapt(Defs, StdCompiler::SEP_NONE));
	if (pComp->isDeserializer()) UpdateInternalCons();
}

void C4PlayerControlDefs::MergeFrom(const C4PlayerControlDefs &Src)
{
	// Clear previous defs if specified in merge set
	if (Src.clear_previous) Defs.clear();
	// copy all defs from source file; overwrite defs of same name if found
	for (const auto & SrcDef : Src.Defs)
	{
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
	UpdateInternalCons();
}

const C4PlayerControlDef *C4PlayerControlDefs::GetControlByIndex(int32_t idx) const
{
	// safe index
	if (idx<0 || idx>=int32_t(Defs.size())) return nullptr;
	return &(Defs[idx]);
}

int32_t C4PlayerControlDefs::GetControlIndexByIdentifier(const char *szIdentifier) const
{
	for (DefVecImpl::const_iterator i = Defs.begin(); i != Defs.end(); ++i)
		if (SEqual((*i).GetIdentifier(), szIdentifier))
			return i-Defs.begin();
	return CON_None;
}

void C4PlayerControlDefs::FinalInit()
{
	// Assume all defs have been loaded
	// Register scritp constants
	for (DefVecImpl::const_iterator i = Defs.begin(); i != Defs.end(); ++i)
	{
		const char *szIdtf  = (*i).GetIdentifier();
		if (szIdtf && *szIdtf && !SEqual(szIdtf, "None"))
		{
			::ScriptEngine.RegisterGlobalConstant(FormatString("CON_%s", szIdtf).getData(), C4VInt(i-Defs.begin()));
		}
	}
}


/* C4PlayerControlAssignment */

void C4PlayerControlAssignment::KeyComboItem::CompileFunc(StdCompiler *pComp)
{
	// if key is compiled, also store as a string into KeyName for later resolving
	if (pComp->isDeserializer())
	{
		Key.dwShift = 0;
		sKeyName.Clear();
		pComp->Value(mkParAdapt(Key, &sKeyName));
	}
	else
	{
		// decompiler: If there's a stored key name, just write it. Regardless of whether it's a key, undefined or a reference
		// If no key name is stored, it was probably assigned at runtime and sKeyName needs to be recreated
		if (!sKeyName) UpdateKeyName();
		pComp->Value(mkParAdapt(sKeyName, StdCompiler::RCT_Idtf));
	}
}

void C4PlayerControlAssignment::KeyComboItem::UpdateKeyName()
{
	// update key name from key
	sKeyName.Copy(Key.ToString(false, false));
}

void C4PlayerControlAssignment::CompileFunc(StdCompiler *pComp)
{
	if (!pComp->Name("Assignment")) { pComp->NameEnd(); pComp->excNotFound("Assignment"); }
	pComp->Value(mkNamingAdapt(mkSTLContainerAdapt(KeyCombo), "Key", KeyComboVec()));
	pComp->Value(mkNamingAdapt(fComboIsSequence, "ComboIsSequence", false));
	pComp->Value(mkNamingAdapt(mkParAdapt(sControlName, StdCompiler::RCT_Idtf), "Control", "None"));
	pComp->Value(mkNamingAdapt(mkParAdapt(sGUIName, StdCompiler::RCT_All), "GUIName", ""));
	pComp->Value(mkNamingAdapt(mkParAdapt(sGUIDesc, StdCompiler::RCT_All), "GUIDesc", ""));
	pComp->Value(mkNamingAdapt(iGUIGroup,"GUIGroup",0));
	pComp->Value(mkNamingAdapt(fGUIDisabled, "GUIDisabled", false));
	pComp->Value(mkNamingAdapt(iPriority, "Priority", 0));
	const StdBitfieldEntry<int32_t> TriggerModeNames[] =
	{
		{ "Default",      CTM_Default  },
		{ "Hold",         CTM_Hold     },
		{ "Release",      CTM_Release  },
		{ "AlwaysUnhandled", CTM_AlwaysUnhandled  },
		{ "ClearRecentKeys", CTM_ClearRecentKeys  },
		{ nullptr, 0 }
	};
	pComp->Value(mkNamingAdapt(mkBitfieldAdapt< int32_t>(iTriggerMode, TriggerModeNames), "TriggerMode", CTM_Default));
	pComp->Value(mkNamingAdapt(fOverrideAssignments, "OverrideAssignments", false));
	pComp->NameEnd();
	// newly loaded structures are not resolved
	if (pComp->isDeserializer()) fRefsResolved = false;
}

void C4PlayerControlAssignment::ResetKeyToInherited()
{
	if (inherited_assignment) CopyKeyFrom(*inherited_assignment);
}

bool C4PlayerControlAssignment::IsKeyChanged() const
{
	// no inherited assignment? Then the key is always custom
	if (!inherited_assignment) return true;
	// otherwise, compare
	return KeyCombo != inherited_assignment->KeyCombo || fComboIsSequence != inherited_assignment->fComboIsSequence;
}

void C4PlayerControlAssignment::SetKey(const C4KeyCodeEx &key)
{
	// set as one-key-combo
	KeyCombo.resize(1);
	KeyCombo[0].Key = key;
	KeyCombo[0].Key.fRepeated = false;
	KeyCombo[0].sKeyName.Clear();
	fComboIsSequence = false;
	TriggerKey = key;
}

void C4PlayerControlAssignment::CopyKeyFrom(const C4PlayerControlAssignment &src_assignment)
{
	// just copy key settings; keep control and priorities
	KeyCombo = src_assignment.KeyCombo;
	TriggerKey = src_assignment.TriggerKey;
	fComboIsSequence = src_assignment.fComboIsSequence;
	if (!src_assignment.fRefsResolved) fRefsResolved = false;
}

bool C4PlayerControlAssignment::ResolveRefs(C4PlayerControlAssignmentSet *pParentSet, C4PlayerControlDefs *pControlDefs)
{
	// avoid circular chains
	static int32_t recursion_check = 0;
	if (recursion_check > 10)
	{
		LogFatal(FormatString("Maximum recursion limit reached while resolving player control assignments of set %s in assignment for key %s. This is probably due to a circular control chain.", pParentSet->GetName(), GetControlName()).getData());
		return false;
	}
	++recursion_check;
	// resolve control name
	iControl = pControlDefs->GetControlIndexByIdentifier(sControlName.getData());
	// resolve keys
	KeyComboVec NewCombo;
	for (auto & rKeyComboItem : KeyCombo)
	{
		const char *szKeyName = rKeyComboItem.sKeyName.getData();
		// check if this is a key reference. A key reference must be preceded by CON_
		// it may also be preceded by modifiers (Shift+), which are already set in rKeyComboItem.Key.dwShift
		bool is_key_reference = false;
		int last_shift_delim_pos;
		if (szKeyName && *szKeyName)
		{
			if ((last_shift_delim_pos=SCharLastPos('+', szKeyName)) > -1) szKeyName += last_shift_delim_pos+1;
			if (SEqual2(szKeyName, "CON_"))
			{
				is_key_reference = true;
				szKeyName +=4;
			}
		}
		if (is_key_reference)
		{
			// this is a key reference
			// - find referenced target assignment
			C4PlayerControlAssignment *pRefAssignment = pParentSet->GetAssignmentByControlName(szKeyName);
			if (pRefAssignment)
			{
				// resolve itself if necessary
				if (!pRefAssignment->IsRefsResolved()) if (!pRefAssignment->ResolveRefs(pParentSet, pControlDefs)) { --recursion_check; return false; }
				// insert all keys of that combo into own combo
				// add any extra shift states from reference
				DWORD ref_shift = rKeyComboItem.Key.dwShift;
				if (ref_shift)
				{
					for (auto assignment_combo_item : pRefAssignment->KeyCombo)
					{
						assignment_combo_item.Key.dwShift |= ref_shift;
						NewCombo.push_back(assignment_combo_item);
					}
				}
				else
				{
					NewCombo.insert(NewCombo.end(), pRefAssignment->KeyCombo.begin(), pRefAssignment->KeyCombo.end());
				}
			}
			else
			{
				// undefined reference? Not fatal, but inform user
				LogF("WARNING: Control %s of set %s contains reference to unassigned control %s.", GetControlName(), pParentSet->GetName(), rKeyComboItem.sKeyName.getData());
				NewCombo.clear();
			}
		}
		else
		{
			// non-reference: check if the assignment was valid
#ifndef USE_CONSOLE
			if (rKeyComboItem.Key == KEY_Default)
				LogF(R"(WARNING: Control %s of set %s contains undefined key "%s".)", GetControlName(), pParentSet->GetName(), szKeyName);
#endif
			// ...and just keep this item.
			NewCombo.push_back(rKeyComboItem);
		}
	}
	KeyCombo = NewCombo;
	// adjust Control and Shift into key states for non-sequence combo keys
	// e.g. LeftControl,A should become LeftControl,Ctrl+A.
	if (KeyCombo.size() > 1 && !fComboIsSequence)
	{
		int32_t shift = 0;
		for (auto & i : KeyCombo)
		{
			if (i.Key.Key == K_CONTROL_L || i.Key.Key == K_CONTROL_R) shift |= KEYS_Control;
			if (i.Key.Key == K_SHIFT_L || i.Key.Key == K_SHIFT_R) shift |= KEYS_Shift;
			shift |= i.Key.dwShift;
		}
		for (auto & i : KeyCombo) i.Key.dwShift |= shift;
	}
	// remove control/shift duplications
	for (auto & i : KeyCombo) i.Key.FixShiftKeys();
	// the trigger key is always last of the chain
	if (KeyCombo.size()) TriggerKey = KeyCombo.back().Key; else TriggerKey = C4KeyCodeEx();
	// done
	fRefsResolved = true;
	--recursion_check;
	return true;
}

bool C4PlayerControlAssignment::IsComboMatched(const C4PlayerControlRecentKeyList &DownKeys, const C4PlayerControlRecentKeyList &RecentKeys) const
{
	assert(HasCombo());
	// check if combo is currently fulfilled (assuming TriggerKey is already matched)
	if (fComboIsSequence)
	{
		C4TimeMilliseconds tKeyLast = C4TimeMilliseconds::Now();
		// combo is a sequence: The last keys of RecentKeys must match the sequence
		// the last ComboKey is the TriggerKey, which is omitted because it has already been matched and is not to be found in RecentKeys yet
		auto i = KeyCombo.rbegin()+1;
		for (auto ri = RecentKeys.rbegin(); i!=KeyCombo.rend(); ++ri)
		{
			// no more keys pressed but combo didn't end? -> no combo match
			if (ri == RecentKeys.rend()) return false;
			const C4PlayerControlRecentKey &rk = *ri;
			// user waited for too long?
			C4TimeMilliseconds tKeyRecent = rk.tTime;
			if (tKeyLast - tKeyRecent > C4PlayerControl::MaxSequenceKeyDelay) return false;
			// key doesn't match?
			const KeyComboItem &k = *i;
			if (!(rk.matched_key == k.Key))
			{
				// mouse movement commands do not break sequences
				if (Key_IsMouse(rk.matched_key.Key) && Key_GetMouseEvent(rk.matched_key.Key) == KEY_MOUSE_Move) continue;
				return false;
			}
			// key OK
			++i;
		}
	}
	else
	{
		// combo requires keys to be down simultanuously: check that all keys of the combo are in the down-list
		for (const auto & k : KeyCombo)
		{
			bool fFound = false;
			for (const auto & dk : DownKeys)
			{
				if (dk.matched_key == k.Key) { fFound = true; break; }
			}
			if (!fFound) return false;
		}
	}
	// combo OK!
	return true;
}

bool C4PlayerControlAssignment::operator ==(const C4PlayerControlAssignment &cmp) const
{
	// doesn't compare resolved TriggerKey/iControl
	return KeyCombo == cmp.KeyCombo
	       && sControlName == cmp.sControlName
	       && sGUIName == cmp.sGUIName
	       && sGUIDesc == cmp.sGUIDesc
		   && fGUIDisabled == cmp.fGUIDisabled
	       && iTriggerMode == cmp.iTriggerMode
	       && iPriority == cmp.iPriority;
}

StdStrBuf C4PlayerControlAssignment::GetKeysAsString(bool human_readable, bool short_name) const
{
	// create a short, human-readable string of the assigned key
	// to be displayed e.g. in tutorial messages explaining controls
	StdStrBuf result;
	if (!KeyCombo.size()) return result;
	// trigger key
	KeyComboVec::const_iterator i=KeyCombo.begin();
	result.Take(i->Key.ToString(human_readable, short_name));
	// extra keys of combo
	while (++i != KeyCombo.end())
	{
		result.AppendChar(fComboIsSequence ? ',' : '+');
		result.Append(i->Key.ToString(human_readable, short_name));
	}
	return result;
}

const char *C4PlayerControlAssignment::GetGUIName(const C4PlayerControlDefs &defs) const
{
	// local name?
	if (sGUIName.getLength())
	{
		// special: None defaults to empty name
		if (sGUIName == "None") return "";
		return sGUIName.getData();
	}
	// otherwise, fall back to def
	const C4PlayerControlDef *def = defs.GetControlByIndex(GetControl());
	if (def) return def->GetGUIName();
	// no def and no name...
	return nullptr;
}

const char *C4PlayerControlAssignment::GetGUIDesc(const C4PlayerControlDefs &defs) const
{
	// local desc?
	if (sGUIDesc.getLength()) return sGUIDesc.getData();
	// otherwise, fall back to def
	const C4PlayerControlDef *def = defs.GetControlByIndex(GetControl());
	if (def) return def->GetGUIDesc();
	// no def and no desc...
	return nullptr;
}

bool C4PlayerControlAssignment::IsGUIDisabled() const
{
	return fGUIDisabled;
}

int32_t C4PlayerControlAssignment::GetGUIGroup() const
{
	return iGUIGroup;
}

/* C4PlayerControlAssignmentSet */

void C4PlayerControlAssignmentSet::InitEmptyFromTemplate(const C4PlayerControlAssignmentSet &template_set)
{
	// copy all fields except assignments
	sName.Copy(template_set.sName);
	sGUIName.Copy(template_set.sGUIName);
	sParentSetName.Copy(template_set.sParentSetName);
	has_keyboard = template_set.has_keyboard;
	has_mouse = template_set.has_mouse;
	has_gamepad = template_set.has_gamepad;
}

void C4PlayerControlAssignmentSet::CompileFunc(StdCompiler *pComp)
{
	if (!pComp->Name("ControlSet")) { pComp->NameEnd(); pComp->excNotFound("ControlSet"); }
	pComp->Value(mkNamingAdapt(mkParAdapt(sName, StdCompiler::RCT_All), "Name", "None")); // can't do RCT_Idtf because of wildcards
	pComp->Value(mkNamingAdapt(mkParAdapt(sGUIName, StdCompiler::RCT_All), "GUIName", "undefined"));
	pComp->Value(mkNamingAdapt(mkParAdapt(sParentSetName, StdCompiler::RCT_Idtf), "Parent", ""));
	pComp->Value(mkNamingAdapt(has_keyboard, "Keyboard", true));
	pComp->Value(mkNamingAdapt(has_mouse, "Mouse", true));
	pComp->Value(mkNamingAdapt(has_gamepad, "Gamepad", false));
	pComp->Value(mkSTLContainerAdapt(Assignments, StdCompiler::SEP_NONE));
	pComp->NameEnd();
}



void C4PlayerControlAssignmentSet::MergeFrom(const C4PlayerControlAssignmentSet &Src, MergeMode merge_mode)
{
	// take over all assignments defined in Src
	for (const auto & SrcAssignment : Src.Assignments)
	{
		bool fIsReleaseKey = !!(SrcAssignment.GetTriggerMode() & C4PlayerControlAssignment::CTM_Release);
		// overwrite same def and release key state
		if (merge_mode != MM_LowPrio && SrcAssignment.IsOverrideAssignments())
		{
			// high priority override control clears all previous (very inefficient method...might as well recreate the whole list)
			bool any_remaining = true;
			while (any_remaining)
			{
				any_remaining = false;
				for (C4PlayerControlAssignmentVec::iterator j = Assignments.begin(); j != Assignments.end(); ++j)
					if (SEqual((*j).GetControlName(), SrcAssignment.GetControlName()))
					{
						bool fSelfIsReleaseKey = !!((*j).GetTriggerMode() & C4PlayerControlAssignment::CTM_Release);
						if (fSelfIsReleaseKey == fIsReleaseKey)
						{
							Assignments.erase(j);
							any_remaining = true;
							break;
						}
					}
			}
		}
		else if (merge_mode == MM_LowPrio || merge_mode == MM_ConfigOverload)
		{
			// if this is low priority, another override control kills this
			bool any_override = false;
			for (auto & Assignment : Assignments)
				if (SEqual(Assignment.GetControlName(), SrcAssignment.GetControlName()))
				{
					bool fSelfIsReleaseKey = !!(Assignment.GetTriggerMode() & C4PlayerControlAssignment::CTM_Release);
					if (fSelfIsReleaseKey == fIsReleaseKey)
					{
						any_override = true;
						// config overloads just change the key of the inherited assignment
						if (merge_mode == MM_ConfigOverload)
						{
							Assignment.CopyKeyFrom(SrcAssignment);
							Assignment.SetInherited(false);
						}
						break;
					}
				}
			if (any_override) continue;
		}
		// new def: Append a copy
		Assignments.push_back(SrcAssignment);
		// inherited marker
		if (merge_mode == MM_Inherit)
		{
			Assignments.back().SetInherited(true);
			Assignments.back().SetInheritedAssignment(&SrcAssignment);
		}
	}
}

C4PlayerControlAssignment *C4PlayerControlAssignmentSet::CreateAssignmentForControl(const char *control_name)
{
	Assignments.emplace_back();
	Assignments.back().SetControlName(control_name);
	return &Assignments.back();
}

void C4PlayerControlAssignmentSet::RemoveAssignmentByControlName(const char *control_name)
{
	for (C4PlayerControlAssignmentVec::iterator i = Assignments.begin(); i != Assignments.end(); ++i)
		if (SEqual((*i).GetControlName(), control_name))
		{
			Assignments.erase(i);
			return;
		}
}

bool C4PlayerControlAssignmentSet::ResolveRefs(C4PlayerControlDefs *pDefs)
{
	// reset all resolved flags to allow re-resolve after overloads
	for (auto & Assignment : Assignments)
		Assignment.ResetRefsResolved();
	// resolve in order; ignore already resolved because they might have been resolved by cross reference
	for (auto & Assignment : Assignments)
		if (!Assignment.IsRefsResolved())
			if (!Assignment.ResolveRefs(this, pDefs))
				return false;
	return true;
}

void C4PlayerControlAssignmentSet::SortAssignments()
{
	// final init: sort assignments by priority
	// note this screws up sorting for config dialog
	std::sort(Assignments.begin(), Assignments.end());
}

C4PlayerControlAssignment *C4PlayerControlAssignmentSet::GetAssignmentByIndex(int32_t index)
{
	if (index<0 || index>=int32_t(Assignments.size())) return nullptr;
	return &Assignments[index];
}

C4PlayerControlAssignment *C4PlayerControlAssignmentSet::GetAssignmentByControlName(const char *szControlName)
{
	for (auto & Assignment : Assignments)
		if (SEqual(Assignment.GetControlName(), szControlName))
			// We don't like release keys... (2do)
			if (!(Assignment.GetTriggerMode() & C4PlayerControlAssignment::CTM_Release))
				return &Assignment;
	return nullptr;
}

C4PlayerControlAssignment *C4PlayerControlAssignmentSet::GetAssignmentByControl(int32_t control)
{
	// TODO: Might want to stuff this into a vector indexed by control for faster lookup
	for (auto & Assignment : Assignments)
		if (Assignment.GetControl() == control)
			// We don't like release keys... (2do)
			if (!(Assignment.GetTriggerMode() & C4PlayerControlAssignment::CTM_Release))
				return &Assignment;
	return nullptr;
}

bool C4PlayerControlAssignmentSet::operator ==(const C4PlayerControlAssignmentSet &cmp) const
{
	return Assignments == cmp.Assignments
	       && sName == cmp.sName;
}

void C4PlayerControlAssignmentSet::GetAssignmentsByKey(const C4PlayerControlDefs &rDefs, const C4KeyCodeEx &key, bool fHoldKeysOnly, C4PlayerControlAssignmentPVec *pOutVec, const C4PlayerControlRecentKeyList &DownKeys, const C4PlayerControlRecentKeyList &RecentKeys) const
{
	assert(pOutVec);
	// primary match by TriggerKey (todo: Might use a hash map here if matching speed becomes an issue due to large control sets)
	for (const auto & rAssignment : Assignments)
	{
		const C4KeyCodeEx &rAssignmentTriggerKey = rAssignment.GetTriggerKey();
		if (!(rAssignmentTriggerKey.Key == key.Key)) continue;
		// special: hold-keys-only ignore shift, because shift state might have been release during hold
		if (!fHoldKeysOnly) if (rAssignmentTriggerKey.dwShift != key.dwShift) continue;
		// check linked control def
		const C4PlayerControlDef *pCtrl = rDefs.GetControlByIndex(rAssignment.GetControl());
		if (!pCtrl) continue;
		// only want hold keys?
		if (fHoldKeysOnly)
		{
			// a hold/release-trigger key is not a real hold key, even if the underlying control is
			if (!pCtrl->IsHoldKey() || (rAssignment.GetTriggerMode() & (C4PlayerControlAssignment::CTM_Hold | C4PlayerControlAssignment::CTM_Release))) continue;
		}
		else if (rAssignment.HasCombo())
		{
			// hold-only events match the trigger key only (i.e., Release-events are generated as soon as the trigger key goes up)
			// other events must match either the sequence or the down-key-combination
			if (!rAssignment.IsComboMatched(DownKeys, RecentKeys)) continue;
		}
		// we got  match! Store it
		pOutVec->push_back(&rAssignment);
	}
}

void C4PlayerControlAssignmentSet::GetTriggerKeys(const C4PlayerControlDefs &rDefs, C4KeyCodeExVec *pRegularKeys, C4KeyCodeExVec *pHoldKeys) const
{
	// put all trigger keys of keyset into output vectors
	// first all hold keys
	for (const auto & rAssignment : Assignments)
	{
		const C4PlayerControlDef *pDef = rDefs.GetControlByIndex(rAssignment.GetControl());
		if (pDef && pDef->IsHoldKey())
		{
			const C4KeyCodeEx &rKey = rAssignment.GetTriggerKey();
			if (std::find(pHoldKeys->begin(), pHoldKeys->end(), rKey) == pHoldKeys->end()) pHoldKeys->push_back(rKey);
		}
	}
	// then all regular keys that aren't in the hold keys list yet
	for (const auto & rAssignment : Assignments)
	{
		const C4PlayerControlDef *pDef = rDefs.GetControlByIndex(rAssignment.GetControl());
		if (pDef && !pDef->IsHoldKey())
		{
			const C4KeyCodeEx &rKey = rAssignment.GetTriggerKey();
			if (std::find(pHoldKeys->begin(), pHoldKeys->end(), rKey) == pHoldKeys->end())
				if (std::find(pRegularKeys->begin(), pRegularKeys->end(), rKey) == pRegularKeys->end())
					pRegularKeys->push_back(rKey);
		}
	}
}

C4Facet C4PlayerControlAssignmentSet::GetPicture() const
{
	// get image to be drawn to represent this control set
	// picture per set not implemented yet. So just default to out standard images
	if (HasGamepad()) return ::GraphicsResource.fctGamepad.GetPhase(0);
//	if (HasMouse()) return ::GraphicsResource.fctMouse; // might be useful again with changing control sets
	if (HasKeyboard()) return ::GraphicsResource.fctKeyboard.GetPhase(Game.PlayerControlUserAssignmentSets.GetSetIndex(this));
	return C4Facet();
}

bool C4PlayerControlAssignmentSet::IsMouseControlAssigned(int32_t mouseevent) const
{
	// TODO
	return true;
}


/* C4PlayerControlAssignmentSets */

void C4PlayerControlAssignmentSets::Clear()
{
	Sets.clear();
}

void C4PlayerControlAssignmentSets::CompileFunc(StdCompiler *pComp)
{
	if (pComp->isSerializer() && pComp->isRegistry())
	{
		pComp->Default("ControlSets"); // special registry compiler: Clean out everything before
	}
	pComp->Value(mkNamingAdapt(clear_previous, "ClearPrevious", false));
	pComp->Value(mkSTLContainerAdapt(Sets, StdCompiler::SEP_NONE));
}

bool C4PlayerControlAssignmentSets::operator ==(const C4PlayerControlAssignmentSets &cmp) const
{
	return Sets == cmp.Sets && clear_previous == cmp.clear_previous;
}

void C4PlayerControlAssignmentSets::MergeFrom(const C4PlayerControlAssignmentSets &Src, C4PlayerControlAssignmentSet::MergeMode merge_mode)
{
	// if source set is flagged to clear previous, do this!
	if (Src.clear_previous) Sets.clear();
	// take over all assignments in known sets and new sets defined in Src
	for (const auto & SrcSet : Src.Sets)
	{
		// overwrite if def of same name existed if it's not low priority anyway
		bool fIsWildcardSet = SrcSet.IsWildcardName();
		if (!fIsWildcardSet)
		{
			C4PlayerControlAssignmentSet *pPrevSet = GetSetByName(SrcSet.GetName());
			if (!pPrevSet && merge_mode == C4PlayerControlAssignmentSet::MM_Inherit)
			{
				// inherited sets must go through merge procedure to set inherited links
				pPrevSet = CreateEmptySetByTemplate(SrcSet);
			}
			if (pPrevSet)
			{
				pPrevSet->MergeFrom(SrcSet, merge_mode);
			}
			else
			{
				// new def: Append a copy
				Sets.push_back(SrcSet);
			}
		}
		else
		{
			// source is a wildcard: Merge with all matching sets
			for (auto & DstSet : Sets)
			{
				if (WildcardMatch(SrcSet.GetName(), DstSet.GetName()))
				{
					DstSet.MergeFrom(SrcSet, merge_mode);
				}
			}
		}
	}
}

bool C4PlayerControlAssignmentSets::ResolveRefs(C4PlayerControlDefs *pDefs)
{
	for (auto & Set : Sets)
		if (!Set.ResolveRefs(pDefs)) return false;
	return true;
}

void C4PlayerControlAssignmentSets::SortAssignments()
{
	for (auto & Set : Sets)
		Set.SortAssignments();
}

C4PlayerControlAssignmentSet *C4PlayerControlAssignmentSets::GetSetByName(const char *szName)
{
	for (auto & Set : Sets)
		if (WildcardMatch(szName, Set.GetName()))
			return &Set;
	return nullptr;
}

C4PlayerControlAssignmentSet *C4PlayerControlAssignmentSets::GetDefaultSet()
{
	// default set is first defined control set
	if (Sets.empty()) return nullptr; // nothing defined :(
	return &Sets.front();
}

int32_t C4PlayerControlAssignmentSets::GetSetIndex(const C4PlayerControlAssignmentSet *set) const
{
	// find set in list; return index
	int32_t index = 0;
	for (AssignmentSetList::const_iterator i = Sets.begin(); i != Sets.end(); ++i,++index)
		if (&*i == set)
			return index;
	return -1; // not found
}

C4PlayerControlAssignmentSet *C4PlayerControlAssignmentSets::GetSetByIndex(int32_t index)
{
	// bounds check
	if (index < 0 || index >= (int32_t)Sets.size()) return nullptr;
	// return indexed set
	AssignmentSetList::iterator i = Sets.begin();
	while (index--) ++i;
	return &*i;
}

C4PlayerControlAssignmentSet *C4PlayerControlAssignmentSets::CreateEmptySetByTemplate(const C4PlayerControlAssignmentSet &template_set)
{
	Sets.emplace_back();
	Sets.back().InitEmptyFromTemplate(template_set);
	return &Sets.back();
}

void C4PlayerControlAssignmentSets::RemoveSetByName(const char *set_name)
{
	for (AssignmentSetList::iterator i = Sets.begin(); i != Sets.end(); ++i)
		if (SEqual(set_name, (*i).GetName()))
		{
			Sets.erase(i);
			return;
		}
}


/* C4PlayerControlFile */

void C4PlayerControlFile::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(ControlDefs, "ControlDefs", C4PlayerControlDefs()));
	pComp->Value(mkNamingAdapt(AssignmentSets, "ControlSets", C4PlayerControlAssignmentSets()));
}

bool C4PlayerControlFile::Load(C4Group &hGroup, const char *szFilename, C4LangStringTable *pLang)
{
	// clear previous
	Clear();
	// load and prepare file contents
	StdStrBuf Buf;
	if (!hGroup.LoadEntryString(szFilename, &Buf)) return false;
	if (pLang) pLang->ReplaceStrings(Buf);
	// parse it!
	if (!CompileFromBuf_LogWarn<StdCompilerINIRead>(*this, Buf, szFilename)) return false;
	return true;
}

bool C4PlayerControlFile::Save(C4Group &hGroup, const char *szFilename)
{
	// decompile to buffer and save buffer to group
	StdStrBuf Buf;
	if (!DecompileToBuf_Log<StdCompilerINIWrite>(*this, &Buf, szFilename)) return false;
	hGroup.Add(szFilename, Buf, false, true);
	return true;
}

void C4PlayerControlFile::Clear()
{
	ControlDefs.Clear();
	AssignmentSets.Clear();
}


/* C4PlayerControl */

void C4PlayerControl::CSync::ControlDownState::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(DownState);
	pComp->Separator();
	pComp->Value(MovedState);
	pComp->Separator();
	pComp->Value(iDownFrame);
	pComp->Separator();
	pComp->Value(iMovedFrame);
	pComp->Separator();
	pComp->Value(fDownByUser);
}

bool C4PlayerControl::CSync::ControlDownState::operator ==(const ControlDownState &cmp) const
{
	return DownState == cmp.DownState && MovedState == cmp.MovedState && iDownFrame == cmp.iDownFrame && iMovedFrame == cmp.iMovedFrame && fDownByUser == cmp.fDownByUser;
}

const C4PlayerControl::CSync::ControlDownState *C4PlayerControl::CSync::GetControlDownState(int32_t iControl) const
{
	// safe access
	if (iControl < 0 || iControl >= int32_t(ControlDownStates.size())) return nullptr;
	return &ControlDownStates[iControl];
}

int32_t C4PlayerControl::CSync::GetControlDisabled(int32_t iControl) const
{
	// safe access
	if (iControl < 0 || iControl >= int32_t(ControlDisableStates.size())) return 0;
	return ControlDisableStates[iControl];
}

void C4PlayerControl::CSync::SetControlDownState(int32_t iControl, const C4KeyEventData &rDownState, int32_t iDownFrame, bool fDownByUser)
{
	// update state
	if (iControl < 0) return;
	if (iControl >= int32_t(ControlDownStates.size())) ControlDownStates.resize(iControl+1);
	ControlDownState &rState = ControlDownStates[iControl];
	rState.DownState = rDownState;
	rState.iDownFrame = iDownFrame;
	rState.fDownByUser = fDownByUser;
}

void C4PlayerControl::CSync::SetControlMovedState(int32_t iControl, const C4KeyEventData &rMovedState, int32_t iMovedFrame)
{
	// update state
	if (iControl < 0) return;
	if (iControl >= int32_t(ControlDownStates.size())) ControlDownStates.resize(iControl+1);
	ControlDownState &rState = ControlDownStates[iControl];
	rState.MovedState = rMovedState;
	rState.iMovedFrame = iMovedFrame;
}

bool  C4PlayerControl::CSync::SetControlDisabled(int32_t iControl, int32_t iVal)
{
	// disable control
	if (iControl < 0) return false;
	if (iControl >= int32_t(ControlDisableStates.size())) ControlDisableStates.resize(iControl+1);
	ControlDisableStates[iControl] = iVal;
	// if a control is disabled, its down-state is reset silently
	ResetControlDownState(iControl);
	return true;
}

void C4PlayerControl::CSync::ResetControlDownState(int32_t iControl)
{
	// silently reset down state of control
	const ControlDownState *pDownState = GetControlDownState(iControl);
	if (pDownState && pDownState->IsDown())
	{
		C4KeyEventData KeyDownState = pDownState->DownState;
		KeyDownState.iStrength = 0;
		SetControlDownState(iControl, KeyDownState, 0, false);
		SetControlMovedState(iControl, KeyDownState, 0);
	}
}

void C4PlayerControl::CSync::InitDefaults(const C4PlayerControlDefs &ControlDefs)
{
	const C4PlayerControlDef *def;
	int32_t i=0;
	while ((def = ControlDefs.GetControlByIndex(i)))
	{
		if (def->IsDefaultDisabled()) SetControlDisabled(i, true);
		++i;
	}
}

void C4PlayerControl::CSync::Clear()
{
	ControlDownStates.clear();
	ControlDisableStates.clear();
}

void C4PlayerControl::CSync::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(mkSTLContainerAdapt(ControlDownStates), "Down", DownStateVec()));
	pComp->Value(mkNamingAdapt(mkSTLContainerAdapt(ControlDisableStates), "Disabled", DisableStateVec()));
}

bool C4PlayerControl::CSync::operator ==(const CSync &cmp) const
{
	return ControlDownStates == cmp.ControlDownStates
	       && ControlDisableStates == cmp.ControlDisableStates;
}

void C4PlayerControl::Init()
{
	// defaultdisabled controls
	Sync.InitDefaults(ControlDefs);
}

void C4PlayerControl::CompileFunc(StdCompiler *pComp)
{
	// compile sync values only
	CSync DefaultSync;
	DefaultSync.InitDefaults(ControlDefs);
	pComp->Value(mkNamingAdapt(Sync, "PlayerControl", DefaultSync));
}

bool C4PlayerControl::ProcessKeyEvent(const C4KeyCodeEx &pressed_key, const C4KeyCodeEx &matched_key, ControlState state, const C4KeyEventData &rKeyExtraData, bool reset_down_states_only, bool *clear_recent_keys)
{
	if (Key_IsGamepad(pressed_key.Key))
	{
		// We have to filter gamepad events here.
		C4Player *plr = ::Players.Get(iPlr);
		if (!plr || !plr->pGamepad || plr->pGamepad->GetID() != pressed_key.deviceId)
			return false;
	}
	// collect all matching keys
	C4PlayerControlAssignmentPVec Matches;
	assert(pControlSet); // shouldn't get this callback for players without control set
	pControlSet->GetAssignmentsByKey(ControlDefs, matched_key, state != CONS_Down, &Matches, DownKeys, RecentKeys);
	// process async controls
	bool cursor_pos_added = false;
	C4ControlPlayerControl *pControlPacket = nullptr;
	for (C4PlayerControlAssignmentPVec::const_iterator i = Matches.begin(); i != Matches.end(); ++i)
	{
		const C4PlayerControlAssignment *pAssignment = *i;
		assert(pAssignment);
		int32_t iControlIndex = pAssignment->GetControl();
		const C4PlayerControlDef *pControlDef = ControlDefs.GetControlByIndex(iControlIndex);
		if (pControlDef && pControlDef->IsValid() && !Sync.IsControlDisabled(iControlIndex) && (state == CONS_Down || pControlDef->IsHoldKey()))
		{
			// clear RecentKeys if requested by this assignment. Must be done before sync queue, so multiple combos can be issued in a single control frame.
			if (clear_recent_keys && (pAssignment->GetTriggerMode() & C4PlayerControlAssignment::CTM_ClearRecentKeys)) *clear_recent_keys = true;
			// extra data from key or overwrite by current cursor pos if definition requires it
			if (pControlDef->IsAsync() && !pControlPacket)
			{
				if (pControlDef->IsSendCursorPos()) IsCursorPosRequested = true; // async cursor pos request - doesn't really make sense to set this flag for async controls
				if (ExecuteControl(iControlIndex, state, rKeyExtraData, pAssignment->GetTriggerMode(), pressed_key.IsRepeated(), reset_down_states_only))
					return true;
			}
			else
			{
				// sync control
				// ignore key repeats, because we do our own key repeat for sync controls
				if (pressed_key.IsRepeated()) return false;
				// sync control has higher priority - no more async execution then
				// build a control packet and add control data instead. even for async controls later in chain, as they may be blocked by a sync handler
				if (!pControlPacket) pControlPacket = new C4ControlPlayerControl(iPlr, state, rKeyExtraData);
				int32_t extra_trigger_mode = 0;
				if (reset_down_states_only) extra_trigger_mode |= C4PlayerControlAssignment::CTM_HandleDownStatesOnly;
				pControlPacket->AddControl(iControlIndex, pAssignment->GetTriggerMode() | extra_trigger_mode);
				// sync cursor pos request; pos will be added to control before it is synced/executed
				if (pControlDef->IsSendCursorPos() && !cursor_pos_added)
				{
					int32_t x, y, game_x, game_y;
					// Add current cursor pos in GUI and game coordinates to input
					if (GetCurrentPlayerCursorPos(&x, &y, &game_x, &game_y))
					{
						C4KeyEventData cursor_key_data(rKeyExtraData);
						cursor_key_data.vp_x = x; cursor_key_data.vp_y = y;
						cursor_key_data.game_x = game_x; cursor_key_data.game_y = game_y;
						pControlPacket->SetExtraData(cursor_key_data);
					}
					// Will also send a CON_CursorPos packet separately
					IsCursorPosRequested = true;
					cursor_pos_added = true;
				}
			}
		}
	}
	// push sync control to input
	if (pControlPacket)
	{
		Game.Input.Add(CID_PlrControl, pControlPacket);
		// assume processed (although we can't really know that yet)
		return true;
	}
	return false;
}

bool C4PlayerControl::ProcessKeyDown(const C4KeyCodeEx &pressed_key, const C4KeyCodeEx &matched_key)
{
	// add key to local "down" list if it's not already in there
	// except for some mouse events for which a down state does not make sense
	C4PlayerControlRecentKey RKey(pressed_key,matched_key,C4TimeMilliseconds::Now());
	if (!Key_IsMouse(pressed_key.Key) || Inside<uint8_t>(Key_GetMouseEvent(pressed_key.Key), KEY_MOUSE_Button1, KEY_MOUSE_ButtonMax))
	{
		if (std::find(DownKeys.begin(), DownKeys.end(), pressed_key) == DownKeys.end()) DownKeys.push_back(RKey);
	}
	// process!
	bool clear_recent_keys = false;
	bool fResult = ProcessKeyEvent(pressed_key, matched_key, CONS_Down, Game.KeyboardInput.GetLastKeyExtraData(), false, &clear_recent_keys);
	// unless assignment requests a clear, always add keys to recent list even if not handled
	if (clear_recent_keys)
		RecentKeys.clear();
	else if (!pressed_key.IsRepeated()) // events caused by holding down the key are not added to recent list (so you cannot cause "double-Q" just by holding down Q)
		RecentKeys.push_back(RKey);
	return fResult;
}

bool C4PlayerControl::ProcessKeyUp(const C4KeyCodeEx &pressed_key, const C4KeyCodeEx &matched_key)
{
	// remove key from "down" list
	// except for some mouse events for which a down state does not make sense
	if (!Key_IsMouse(pressed_key.Key) || Inside<uint8_t>(Key_GetMouseEvent(pressed_key.Key), KEY_MOUSE_Button1, KEY_MOUSE_ButtonMax))
	{
		C4PlayerControlRecentKeyList::iterator i = find(DownKeys.begin(), DownKeys.end(), pressed_key);
		if (i != DownKeys.end()) DownKeys.erase(i);
	}
	// process!
	return ProcessKeyEvent(pressed_key, matched_key, CONS_Up, Game.KeyboardInput.GetLastKeyExtraData());
}

bool C4PlayerControl::ProcessKeyMoved(const C4KeyCodeEx &pressed_key, const C4KeyCodeEx &matched_key)
{
	// process!
	return ProcessKeyEvent(pressed_key, matched_key, CONS_Moved, Game.KeyboardInput.GetLastKeyExtraData());
}

void C4PlayerControl::ExecuteControlPacket(const class C4ControlPlayerControl *pCtrl)
{
	// callback from control queue. Execute controls in packet until one of them gets processed
	// assume async packets always as not processed to ensure sync safety (usually, sync commands should better not ovberride async commands anyway)
	bool fHandleDownStateOnly = false;
	for (C4ControlPlayerControl::ControlItemVec::const_iterator i = pCtrl->GetControlItems().begin(); i != pCtrl->GetControlItems().end(); ++i)
	{
		const C4ControlPlayerControl::ControlItem &rItem = *i;
		const C4PlayerControlDef *pCtrlDef = ControlDefs.GetControlByIndex(rItem.iControl);
		if (pCtrlDef)
		{
			if (Config.General.DebugRec)
			{
				if (pCtrlDef->IsSync())
				{
					AddDbgRec(RCT_PlrCom, &rItem.iControl, sizeof(rItem.iControl));
				}
			}
			if (ExecuteControl(rItem.iControl, pCtrl->GetState(), pCtrl->GetExtraData(), rItem.iTriggerMode, false, fHandleDownStateOnly))
				if (pCtrlDef->IsSync())
				{
					if (pCtrl->GetState() == CONS_Up)
					{
						// control processed. however, for key releases, overriden keys are released silently so following down events aren't handled as key repeats
						// note this does not affect CTM_Hold/CTM_Release, because they ignore release controls anyway
						fHandleDownStateOnly = true;
					}
					else
					{
						break;
					}
				}
		}
	}
}

bool C4PlayerControl::ExecuteControl(int32_t iControl, ControlState state, const C4KeyEventData &rKeyExtraData, int32_t iTriggerMode, bool fRepeated, bool fHandleDownStateOnly)
{
	// execute single control. return if handled
	const C4PlayerControlDef *pControlDef = ControlDefs.GetControlByIndex(iControl);
	if (!pControlDef || Sync.IsControlDisabled(iControl)) return false;
	C4PlayerControlDef::Actions eAction = pControlDef->GetAction();
	C4KeyEventData KeyExtraData(rKeyExtraData);
	const CSync::ControlDownState *pCtrlDownState = Sync.GetControlDownState(iControl);
	bool fWasDown = pCtrlDownState ? pCtrlDownState->IsDown() : false;
	// global controls only in global context
	if (IsGlobal() != pControlDef->IsGlobal()) return false;
	// down state handling only?
	if (iTriggerMode & C4PlayerControlAssignment::CTM_HandleDownStatesOnly) fHandleDownStateOnly = true;
	// hold-actions only work on script controls with the hold flag
	if (iTriggerMode & (C4PlayerControlAssignment::CTM_Hold | C4PlayerControlAssignment::CTM_Release))
	{
		if (eAction != C4PlayerControlDef::CDA_Script) return false;
		if (!pControlDef->IsHoldKey()) return false;
		if (state == CONS_Up) return false; // hold triggers have no "up"-event
		// perform hold/release
		if (fWasDown)
		{
			// control is currently down: release?
			if (iTriggerMode & C4PlayerControlAssignment::CTM_Release)
			{
				KeyExtraData.iStrength = 0;
				Sync.SetControlDownState(iControl, KeyExtraData, Game.FrameCounter, false);
				// now process as a regular "Up" event
				state = CONS_Up;
				fRepeated = false;
			}
			else
			{
				assert(iTriggerMode & C4PlayerControlAssignment::CTM_Hold);
				// control is down but trigger key is pressed again: Refresh down state
				// (this will restart the KeyRepeat time)
				Sync.SetControlDownState(iControl, KeyExtraData, Game.FrameCounter, false);
				// now process as a regular, repeated "down" event
				fRepeated = true;
			}
		}
		else
		{
			// control is currently up. Put into hold-down-state if this is a hold key
			if (iTriggerMode & C4PlayerControlAssignment::CTM_Hold)
			{
				Sync.SetControlDownState(iControl, KeyExtraData, Game.FrameCounter, false);
				// now process as a regular "down" event
				fRepeated = false;
			}
			else
			{
				//. Ignore if it's only a release key
				return false;
			}
		}
	}
	else if (state == CONS_Up)
	{
		// regular ControlUp: Only valid if that control was down
		if (!fWasDown) return false;
		Sync.SetControlDownState(iControl, KeyExtraData, Game.FrameCounter, true);
	}
	else if (pControlDef->IsHoldKey())
	{
		if (state == CONS_Moved)
		{
			Sync.SetControlMovedState(iControl, KeyExtraData, Game.FrameCounter);
			fRepeated = true;
		}
		else
		{
			// regular ControlDown on Hold Key: Set in down list
			Sync.SetControlDownState(iControl, KeyExtraData, Game.FrameCounter, true);
			fRepeated = fWasDown;
		}
	}
	// down state handling done
	if (fHandleDownStateOnly) return false;
	// perform action for this control
	bool fHandled = ExecuteControlAction(iControl, eAction, pControlDef->GetExtraData(), state, KeyExtraData, fRepeated);
	// handled controls hide control display
	C4Player *pPlr;
	if ((pPlr = ::Players.Get(iPlr))) if (pPlr->ShowStartup) pPlr->ShowStartup = false;
	// return if handled, unless control is defined as always unhandled
	return fHandled && !(iTriggerMode & C4PlayerControlAssignment::CTM_AlwaysUnhandled);
}

bool C4PlayerControl::ExecuteControlAction(int32_t iControl, C4PlayerControlDef::Actions eAction, C4ID idControlExtraData, ControlState state, const C4KeyEventData &rKeyExtraData, bool fRepeated)
{
	// moved events don't make sense for menus and are only handled by script
	if (state == CONS_Moved && eAction != C4PlayerControlDef::CDA_Script) return false;
	// get affected player
	C4Player *pPlr = nullptr;
	C4Viewport *pVP;
	C4Object *pCursor = nullptr;
	C4Menu *pCursorMenu = nullptr;
	if (iPlr > -1)
	{
		pPlr = ::Players.Get(iPlr);
		if (!pPlr) return false;
		pCursor = pPlr->Cursor;
		if (pCursor && pCursor->Menu && pCursor->Menu->IsActive()) pCursorMenu = pCursor->Menu;
	}
	bool fUp = state == CONS_Up;
	// exec action (on player)
	switch (eAction)
	{
		// scripted player control
	case C4PlayerControlDef::CDA_Script:
		return ExecuteControlScript(iControl, idControlExtraData, state, rKeyExtraData, fRepeated);

		// menu controls
	case C4PlayerControlDef::CDA_Menu: if (!pPlr || fUp) return false; if (pPlr->Menu.IsActive()) pPlr->Menu.TryClose(false, true); else pPlr->ActivateMenuMain(); return true; // toggle
	case C4PlayerControlDef::CDA_MenuOK:     if (!pPlr || !pPlr->Menu.IsActive() || fUp) return false; pPlr->Menu.Control(COM_MenuEnter,0); return true; // ok on item
	case C4PlayerControlDef::CDA_MenuCancel: if (!pPlr || !pPlr->Menu.IsActive() || fUp) return false; pPlr->Menu.Control(COM_MenuClose,0); return true; // close menu
	case C4PlayerControlDef::CDA_MenuLeft:   if (!pPlr || !pPlr->Menu.IsActive() || fUp) return false; pPlr->Menu.Control(COM_MenuLeft ,0); return true; // navigate
	case C4PlayerControlDef::CDA_MenuUp:     if (!pPlr || !pPlr->Menu.IsActive() || fUp) return false; pPlr->Menu.Control(COM_MenuUp   ,0); return true; // navigate
	case C4PlayerControlDef::CDA_MenuRight:  if (!pPlr || !pPlr->Menu.IsActive() || fUp) return false; pPlr->Menu.Control(COM_MenuRight,0); return true; // navigate
	case C4PlayerControlDef::CDA_MenuDown:  if (!pPlr || !pPlr->Menu.IsActive() || fUp) return false; pPlr->Menu.Control(COM_MenuDown,0); return true; // navigate
	case C4PlayerControlDef::CDA_ObjectMenuTextComplete:   if (!pCursorMenu || fUp || !pCursorMenu->IsTextProgressing()) return false; pCursorMenu->Control(COM_MenuShowText,0); return true; // fast-foward text display
	case C4PlayerControlDef::CDA_ObjectMenuOK:     if (!pCursorMenu || fUp) return false; pCursorMenu->Control(COM_MenuEnter,0); return true; // ok on item
	case C4PlayerControlDef::CDA_ObjectMenuOKAll:  if (!pCursorMenu || fUp) return false; pCursorMenu->Control(COM_MenuEnterAll,0); return true; // alt ok on item
	case C4PlayerControlDef::CDA_ObjectMenuSelect: if (!pCursorMenu || fUp) return false; pCursorMenu->Control(COM_MenuSelect,rKeyExtraData.iStrength); return true; // select an item directly
	case C4PlayerControlDef::CDA_ObjectMenuCancel:       if (!pCursorMenu || fUp) return false; pCursorMenu->Control(COM_MenuClose,0); return true; // close menu
	case C4PlayerControlDef::CDA_ObjectMenuLeft:         if (!pCursorMenu || fUp) return false; pCursorMenu->Control(COM_MenuLeft ,0); return true; // navigate
	case C4PlayerControlDef::CDA_ObjectMenuUp:           if (!pCursorMenu || fUp) return false; pCursorMenu->Control(COM_MenuUp   ,0); return true; // navigate
	case C4PlayerControlDef::CDA_ObjectMenuRight:        if (!pCursorMenu || fUp) return false; pCursorMenu->Control(COM_MenuRight,0); return true; // navigate
	case C4PlayerControlDef::CDA_ObjectMenuDown:         if (!pCursorMenu || fUp) return false; pCursorMenu->Control(COM_MenuDown ,0); return true; // navigate

	case C4PlayerControlDef::CDA_ZoomIn:   if (!pPlr || fUp || !(pVP = ::Viewports.GetViewport(iPlr))) return false; pVP->ChangeZoom(C4GFX_ZoomStep); return true; // viewport zoom
	case C4PlayerControlDef::CDA_ZoomOut:  if (!pPlr || fUp || !(pVP = ::Viewports.GetViewport(iPlr))) return false; pVP->ChangeZoom(1.0f/C4GFX_ZoomStep); return true; // viewport zoom

		//unknown action
	default: return false;
	}
}

bool C4PlayerControl::ExecuteControlScript(int32_t iControl, C4ID idControlExtraData, ControlState state, const C4KeyEventData &rKeyExtraData, bool fRepeated)
{
	C4Player *pPlr = ::Players.Get(iPlr);
	if (pPlr)
	{
		// Not for eliminated (checked again in DirectCom, but make sure no control is generated for eliminated players!)
		if (pPlr->Eliminated) return false;
		// control count for statistics (but don't count analog stick wiggles)
		if (state != CONS_Moved)
			pPlr->CountControl(C4Player::PCID_DirectCom, iControl*2+state);
	}
	else if (iPlr > -1)
	{
		// player lost?
		return false;
	}
	// get coordinates
	int32_t x,y;
	const C4PlayerControlDef *def = ControlDefs.GetControlByIndex(iControl);
	if (def && def->GetCoordinateSpace() == C4PlayerControlDef::COS_Viewport)
	{
		x = rKeyExtraData.vp_x; y = rKeyExtraData.vp_y;
	}
	else
	{
		x = rKeyExtraData.game_x; y = rKeyExtraData.game_y;
	}
	C4Value vx = (x == C4KeyEventData::KeyPos_None) ? C4VNull : C4VInt(x);
	C4Value vy = (y == C4KeyEventData::KeyPos_None) ? C4VNull : C4VInt(y);
	// exec control function
	C4AulParSet Pars(iPlr, iControl, C4Id2Def(idControlExtraData), vx, vy, rKeyExtraData.iStrength, fRepeated, C4VInt(state));
	return ::ScriptEngine.GetPropList()->Call(PSF_PlayerControl, &Pars).getBool();
}


void C4PlayerControl::Execute()
{
	// sync execution: Do keyrepeat
	for (size_t i=0; i<ControlDefs.GetCount(); ++i)
	{
		const CSync::ControlDownState *pControlDownState = Sync.GetControlDownState(i);
		if (pControlDownState && pControlDownState->IsDown())
		{
			const C4PlayerControlDef *pCtrlDef = ControlDefs.GetControlByIndex(i);
			assert(pCtrlDef);
			int32_t iCtrlRepeatDelay = pCtrlDef->GetRepeatDelay();
			if (iCtrlRepeatDelay)
			{
				int32_t iFrameDiff = Game.FrameCounter - pControlDownState->iDownFrame;
				int32_t iCtrlInitialRepeatDelay = pCtrlDef->GetInitialRepeatDelay();
				if (iFrameDiff && iFrameDiff >= iCtrlInitialRepeatDelay)
				{
					if (!((iFrameDiff-iCtrlInitialRepeatDelay) % iCtrlRepeatDelay))
					{
						// it's RepeatTime for this key!
						ExecuteControlAction(i, pCtrlDef->GetAction(), pCtrlDef->GetExtraData(), CONS_Down, pControlDownState->DownState, true);
					}
				}
			}
		}
	}
	// cleanup old recent keys
	C4TimeMilliseconds tNow = C4TimeMilliseconds::Now();
	C4PlayerControlRecentKeyList::iterator irk;
	for (irk = RecentKeys.begin(); irk != RecentKeys.end(); ++irk)
	{
		C4PlayerControlRecentKey &rk = *irk;
		if (rk.tTime + MaxRecentKeyLookback > tNow) break;
	}
	if (irk != RecentKeys.begin()) RecentKeys.erase(RecentKeys.begin(), irk);
}

C4PlayerControl::C4PlayerControl() : ControlDefs(Game.PlayerControlDefs)
{
}

void C4PlayerControl::Clear()
{
	iPlr = NO_OWNER;
	pControlSet = nullptr;
	for (auto & KeyBinding : KeyBindings) delete KeyBinding;
	KeyBindings.clear();
	RecentKeys.clear();
	DownKeys.clear();
	Sync.Clear();
	IsCursorPosRequested = false;
}

void C4PlayerControl::RegisterKeyset(int32_t iPlr, C4PlayerControlAssignmentSet *pKeyset)
{
	// setup
	pControlSet = pKeyset;
	this->iPlr = iPlr;
	// register all keys into Game.KeyboardInput creating KeyBindings
	if (pControlSet)
	{
		C4KeyCodeExVec RegularKeys, HoldKeys;
		pControlSet->GetTriggerKeys(ControlDefs, &RegularKeys, &HoldKeys);
		int32_t idx=0;
		for (C4KeyCodeExVec::const_iterator i = RegularKeys.begin(); i != RegularKeys.end(); ++i) AddKeyBinding(*i, false, idx++);
		for (C4KeyCodeExVec::const_iterator i = HoldKeys.begin(); i != HoldKeys.end(); ++i) AddKeyBinding(*i, true, idx++);
	}
}

void C4PlayerControl::AddKeyBinding(const C4KeyCodeEx &key, bool fHoldKey, int32_t idx)
{
	KeyBindings.push_back(new C4KeyBinding(
	                        key, FormatString("PlrKey%02d", idx).getData(), KEYSCOPE_Control,
	                        new C4KeyCBExPassKey<C4PlayerControl, C4KeyCodeEx>(*this, key, &C4PlayerControl::ProcessKeyDown, fHoldKey ? &C4PlayerControl::ProcessKeyUp : nullptr, nullptr, fHoldKey ? &C4PlayerControl::ProcessKeyMoved : nullptr),
	                        C4CustomKey::PRIO_PlrControl));
}

bool C4PlayerControl::DoMouseInput(uint8_t mouse_id, int32_t mouseevent, float game_x, float game_y, float gui_x, float gui_y, DWORD modifier_flags)
{
	// convert moueevent to key code
	bool is_down;
	C4KeyCodeEx mouseevent_keycode = C4KeyCodeEx::FromC4MC(mouse_id, mouseevent, modifier_flags, &is_down);
	// first, try processing it as GUI mouse event. if not assigned, process as Game mous event
	// TODO: May route this through Game.DoKeyboardInput instead - would allow assignment of mouse events in CustomConfig
	//  and would get rid of the Game.KeyboardInput.SetLastKeyExtraData-hack
	C4KeyEventData mouseevent_data;
	mouseevent_data.iStrength = 100*is_down; // TODO: May get pressure from tablet here
	mouseevent_data.vp_x = uint32_t(gui_x);
	mouseevent_data.vp_y = uint32_t(gui_y);
	mouseevent_data.game_x = uint32_t(game_x);
	mouseevent_data.game_y = uint32_t(game_y);
	Game.KeyboardInput.SetLastKeyExtraData(mouseevent_data); // ProcessKeyDown/Up queries it from there...
	bool result;
	if (is_down)
		result = ProcessKeyDown(mouseevent_keycode, mouseevent_keycode);
	else
		result = ProcessKeyUp(mouseevent_keycode, mouseevent_keycode);
	return result;
}

bool C4PlayerControl::GetCurrentPlayerCursorPos(int32_t *x_out, int32_t *y_out, int32_t *game_x_out, int32_t *game_y_out)
{
	// prefer mouse position if this is a mouse control
	if (pControlSet && pControlSet->HasMouse())
	{
		if (MouseControl.GetLastCursorPos(x_out, y_out, game_x_out, game_y_out))
		{
			return true;
		}
		// if getting the mouse position failed, better fall back to cursor pos
	}
	// no mouse position known. Use cursor.
	C4Player *plr = Players.Get(iPlr);
	if (!plr) return false;
	C4Object *cursor_obj = plr->Cursor;
	if (!cursor_obj) return false;
	C4Viewport *vp = ::Viewports.GetViewport(iPlr);
	if (!vp) return false;
	int32_t game_x = cursor_obj->GetX(), game_y=cursor_obj->GetY();
	*game_x_out = game_x; *game_y_out = game_y;
	// game coordinate to screen coordinates...
	float screen_x = (float(game_x) - vp->last_game_draw_cgo.TargetX - vp->last_game_draw_cgo.X) * vp->GetZoom();
	float screen_y = (float(game_y) - vp->last_game_draw_cgo.TargetY - vp->last_game_draw_cgo.Y) * vp->GetZoom();
	// ...and screen coordinates to GUI coordinates (might push this into a helper function of C4Viewport?)
	float gui_x = (screen_x - vp->last_game_draw_cgo.X) / C4GUI::GetZoom() + vp->last_game_draw_cgo.X;
	float gui_y = (screen_y - vp->last_game_draw_cgo.Y) / C4GUI::GetZoom() + vp->last_game_draw_cgo.Y;
	*x_out = int32_t(gui_x); *y_out = int32_t(gui_y);
	return true;
}

void C4PlayerControl::PrepareInput()
{
	if (IsCursorPosRequested)
	{
		int32_t x, y, game_x, game_y;
		// add current cursor pos in GUI coordinates to input
		if (GetCurrentPlayerCursorPos(&x, &y, &game_x, &game_y))
		{
			// CON_CursorPos might not have been defined in definition file
			if (ControlDefs.InternalCons.CON_CursorPos != CON_None)
			{
				C4KeyEventData ev;
				ev.iStrength = 0;
				ev.vp_x = x; ev.vp_y = y;
				ev.game_x = game_x; ev.game_y = game_y;
				C4ControlPlayerControl *pControlPacket = new C4ControlPlayerControl(iPlr, CONS_Down, ev);
				pControlPacket->AddControl(ControlDefs.InternalCons.CON_CursorPos, C4PlayerControlAssignment::CTM_Default);
				// make sure it's added at head, because controls that have SendCursorPos=1 set will follow, which will rely
				// on the cursor pos being known
				Game.Input.AddHead(CID_PlrControl, pControlPacket);
			}
		}
		else
		{
			// no cursor is known (e.g.: Cursor Clonk dead, etc.). Don't create a control.
			// Script will probably fall back to last known cursor pos
		}
		IsCursorPosRequested = false;
	}
}
