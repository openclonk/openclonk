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
	if (!pComp->Name("ControlDef")) { pComp->NameEnd(); pComp->excNotFound("ControlDef"); }
	pComp->Value(mkNamingAdapt(mkParAdapt(sIdentifier, StdCompiler::RCT_Idtf), "Identifier", "None"));
	pComp->Value(mkNamingAdapt(mkParAdapt(sGUIName, StdCompiler::RCT_All), "GUIName", "undefined"));
	pComp->Value(mkNamingAdapt(mkParAdapt(sGUIDesc, StdCompiler::RCT_All), "GUIDesc", ""));
	pComp->Value(mkNamingAdapt(fGlobal, "Global", false));
	pComp->Value(mkNamingAdapt(fIsHoldKey, "Hold", false));
	pComp->Value(mkNamingAdapt(iRepeatDelay, "RepeatDelay", 0));
	pComp->Value(mkNamingAdapt(iInitialRepeatDelay, "InitialRepeatDelay", 0));
	pComp->Value(mkNamingAdapt(fDefaultDisabled, "DefaultDisabled", false));
	pComp->Value(mkNamingAdapt(mkC4IDAdapt(idControlExtraData), "ExtraData", C4ID_None));
	const StdEnumEntry<Actions> ActionNames[] = {
		{ "None",        CDA_None        },
		{ "Script",      CDA_Script      },
		{ "Menu",        CDA_Menu        },
		{ "MenuOK",      CDA_MenuOK      },
		{ "MenuCancel",  CDA_MenuCancel  },
		{ "MenuLeft",    CDA_MenuLeft    },
		{ "MenuUp",      CDA_MenuUp      },
		{ "MenuRight",   CDA_MenuRight   },
		{ "MenuDown",    CDA_MenuDown    },
		{ NULL, CDA_None } };
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
	    && eAction == cmp.eAction;
	}


/* C4PlayerControlDefs */

void C4PlayerControlDefs::Clear()
	{
	Defs.clear();
	}

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
	if (!pComp->Name("Assignment")) { pComp->NameEnd(); pComp->excNotFound("Assignment"); }
	pComp->Value(mkNamingAdapt(mkSTLContainerAdapt(KeyCombo), "Key", KeyComboVec()));
	pComp->Value(mkNamingAdapt(mkParAdapt(sControlName, StdCompiler::RCT_Idtf), "Control", "None"));
	pComp->Value(mkNamingAdapt(iPriority, "Priority", 0));
	const StdBitfieldEntry<int32_t> TriggerModeNames[] = {
		{ "Default",      CTM_Default  },
		{ "Hold",         CTM_Hold     },
		{ "Release",      CTM_Release  },
		{ "AlwaysUnhandled", CTM_AlwaysUnhandled  },
		{ NULL, 0 } };
	pComp->Value(mkNamingAdapt(mkBitfieldAdapt< int32_t>(iTriggerMode, TriggerModeNames), "TriggerMode", CTM_Default));
	pComp->NameEnd();
	// newly loaded structures are not resolved
	if (pComp->isCompiler()) fRefsResolved = false;
	}

bool C4PlayerControlAssignment::ResolveRefs(C4PlayerControlAssignmentSet *pParentSet, C4PlayerControlDefs *pControlDefs)
	{
	// avoid circular chains
	static C4PlayerControlAssignment *pCircularDetect = NULL;
	if (!pCircularDetect) pCircularDetect = this; else if (pCircularDetect == this)
		{
		LogFatal(FormatString("Circular reference chain detected in player control assignments of set %s in assignment for key %s!", pParentSet->GetName(), GetControlName()).getData());
		return false;
		}
	// resolve control name
	iControl = pControlDefs->GetControlIndexByIdentifier(sControlName.getData());
	// resolve keys
	KeyComboVec NewCombo;
	for (KeyComboVec::iterator i = KeyCombo.begin(); i != KeyCombo.end(); ++i)
		{
		KeyComboItem &rKeyComboItem = *i;
		if (rKeyComboItem.Key == KEY_Default && rKeyComboItem.sKeyName.getLength())
			{
			// this is a key reference - find it
			C4PlayerControlAssignment *pRefAssignment = pParentSet->GetAssignmentByControlName(rKeyComboItem.sKeyName.getData());
			if (pRefAssignment)
				{
				// resolve itself if necessary
				if (!pRefAssignment->IsRefsResolved()) if (!pRefAssignment->ResolveRefs(pParentSet, pControlDefs)) return false;
				// insert all keys of that combo into own combo
				NewCombo.insert(NewCombo.end(), pRefAssignment->KeyCombo.begin(), pRefAssignment->KeyCombo.end());
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
			NewCombo.push_back(rKeyComboItem);
			}
		}
	KeyCombo = NewCombo;
	// the trigger key is always last of the chain
	if (KeyCombo.size()) TriggerKey = KeyCombo.back().Key; else TriggerKey = C4KeyCodeEx();
	// done
	fRefsResolved = true;
	if (pCircularDetect == this) pCircularDetect = NULL;
	return true;
	}

bool C4PlayerControlAssignment::operator ==(const C4PlayerControlAssignment &cmp) const
	{
	// doesn't compare resolved TriggerKey/iControl
	return KeyCombo == cmp.KeyCombo
	    && sControlName == cmp.sControlName
	    && iTriggerMode == cmp.iTriggerMode
	    && iPriority == cmp.iPriority;
	}


/* C4PlayerControlAssignmentSet */

void C4PlayerControlAssignmentSet::CompileFunc(StdCompiler *pComp)
	{
	if (!pComp->Name("ControlSet")) { pComp->NameEnd(); pComp->excNotFound("ControlSet"); }
	pComp->Value(mkNamingAdapt(mkParAdapt(sName, StdCompiler::RCT_Idtf), "Name", "None"));
	pComp->Value(mkSTLContainerAdapt(Assignments, StdCompiler::SEP_NONE));
	pComp->NameEnd();
	}

void C4PlayerControlAssignmentSet::MergeFrom(const C4PlayerControlAssignmentSet &Src, bool fLowPrio)
	{
	// take over all assignments defined in Src
	for (C4PlayerControlAssignmentVec::const_iterator i = Src.Assignments.begin(); i != Src.Assignments.end(); ++i)
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

bool C4PlayerControlAssignmentSet::ResolveRefs(C4PlayerControlDefs *pDefs)
	{
	// resolve in order; ignore already resolved because they might have been resolved by cross reference
	for (C4PlayerControlAssignmentVec::iterator i = Assignments.begin(); i != Assignments.end(); ++i)
		if (!(*i).IsRefsResolved())
			if (!(*i).ResolveRefs(this, pDefs))
				return false;
	// now sort assignments by priority
	std::sort(Assignments.begin(), Assignments.end());
	return true;
	}

C4PlayerControlAssignment *C4PlayerControlAssignmentSet::GetAssignmentByControlName(const char *szControlName)
	{
	for (C4PlayerControlAssignmentVec::iterator i = Assignments.begin(); i != Assignments.end(); ++i)
		if (SEqual((*i).GetControlName(), szControlName))
			return &*i;
	return NULL;
	}

bool C4PlayerControlAssignmentSet::operator ==(const C4PlayerControlAssignmentSet &cmp) const
	{
	return Assignments == cmp.Assignments
		&& sName == cmp.sName;
	}


/* C4PlayerControlAssignmentSets */

void C4PlayerControlAssignmentSets::Clear()
	{
	Sets.clear();
	}

void C4PlayerControlAssignmentSets::CompileFunc(StdCompiler *pComp)
	{
	pComp->Value(mkNamingAdapt(mkSTLContainerAdapt(Sets, StdCompiler::SEP_NONE), "ControlSets", AssignmentSetList()));
	}

void C4PlayerControlAssignmentSets::MergeFrom(const C4PlayerControlAssignmentSets &Src, bool fLowPrio)
	{
	// take over all assignments in known sets and new sets defined in Src
	for (AssignmentSetList::const_iterator i = Src.Sets.begin(); i != Src.Sets.end(); ++i)
		{
		const C4PlayerControlAssignmentSet &SrcSet = *i;
		// overwrite if def of same name existed if it's not low priority anyway
		C4PlayerControlAssignmentSet *pPrevSet = GetSetByName(SrcSet.GetName());
		if (pPrevSet)
			{
			pPrevSet->MergeFrom(SrcSet, fLowPrio);
			}
		else
			{
			// new def: Append a copy
			Sets.push_back(SrcSet);
			}
		}
	}

bool C4PlayerControlAssignmentSets::ResolveRefs(C4PlayerControlDefs *pDefs)
	{
	for (AssignmentSetList::iterator i = Sets.begin(); i != Sets.end(); ++i)
		if (!(*i).ResolveRefs(pDefs)) return false;
	return true;
	}

C4PlayerControlAssignmentSet *C4PlayerControlAssignmentSets::GetSetByName(const char *szName)
	{
	for (AssignmentSetList::iterator i = Sets.begin(); i != Sets.end(); ++i)
		if (SEqual((*i).GetName(), szName))
			return &*i;
	return NULL;
	}


/* C4PlayerControlFile */

void C4PlayerControlFile::CompileFunc(StdCompiler *pComp)
	{
	pComp->Value(ControlDefs);
	pComp->Value(AssignmentSets);
	}

bool C4PlayerControlFile::Load(C4Group &hGroup, const char *szFilename, C4LangStringTable *pLang)
	{
	// clear previous
	Clear();
	// load and prepare file contents
	StdStrBuf Buf;
	if (!hGroup.LoadEntryString(szFilename, Buf)) return false;
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

void C4PlayerControl::CompileFunc(StdCompiler *pComp)
	{
	// compile sync values only
	pComp->Value(mkNamingAdapt(Sync, "PlayerControl", CSync()));
	}

bool C4PlayerControl::ProcessKeyEvent(const C4KeyCodeEx &key, bool fUp, const C4KeyEventData &rKeyExtraData)
	{
	// collect all matching keys
	C4PlayerControlAssignmentVec Matches;
	pControlSet->GetAssignmentsByKey(key, fUp, &Matches, DownKeys, RecentKeys);
	// process async controls
	C4ControlPlayerControl2 *pControlPacket = NULL;
	for (C4PlayerControlAssignmentVec::const_iterator i = Matches.begin(); i != Matches.end(); ++i)
		{
		const C4PlayerControlAssignment &rAssignment = *i;
		int32_t iControlIndex = rAssignment.GetControl();
		C4PlayerControlDef *pControlDef = ControlDefs.GetControlByIndex(iControlIndex);
		if (pControlDef && pControlDef->IsValid() && (!fUp || pControlDef->IsHoldKey()))
			{
			if (pControlDef->IsAsync() && !pControlPacket)
				{
				if (ExecuteControl(iControlIndex, fUp, rKeyExtraData, rAssignment.GetTriggerMode(), key.IsRepeated()))
					return true;
				}
			else
				{
				// sync control
				// ignore key repeats, because we do our own key repeat for sync controls
				if (key.IsRepeated()) return false;
				// sync control has higher priority - no more async execution then
				// build a control packet and add control data instead. even for async controls later in chain, as they may be blocked by a sync handler
				if (!pControlPacket) pControlPacket = new C4ControlPlayerControl2(iPlr, fUp, rKeyExtraData);
				pControlPacket->AddControl(iControlIndex, rAssignment.GetTriggerMode());
				break;
				}
			}
		}
	// push sync control to input
	if (pControlPacket) Game.Input.Add(CID_PlrControl2, pControlPacket);
	}

bool C4PlayerControl::ProcessKeyDown(const C4KeyCodeEx &key)
	{
	// add key to local "down" list if it's not already in there
	if (std::find(DownKeys.begin(), DownKeys.end(), C4PlayerControlRecentKey(key,0)) == DownKeys.end()) DownKeys.push_back(C4PlayerControlRecentKey(key,Game.FrameCounter));
	// process!
	bool fResult = ProcessKeyEvent(key, false, Game.KeyboardInput.GetLastKeyExtraData());
	// add to recent list unless repeated
	if (!key.IsRepeated()) RecentKeys.push_back(C4PlayerControlRecentKey(key,Game.FrameCounter));
	return fResult;
	}

bool C4PlayerControl::ProcessKeyUp(const C4KeyCodeEx &key)
	{
	// remove key from "down" list
	C4PlayerControlRecentKeyList::iterator i = find(DownKeys.begin(), DownKeys.end(), C4PlayerControlRecentKey(key,0));
	if (i != DownKeys.end()) DownKeys.erase(i);
	// process!
	return ProcessKeyEvent(key, true, Game.KeyboardInput.GetLastKeyExtraData());
	}

void C4PlayerControl::ExecuteControlPacket(const class C4ControlPlayerControl2 *pCtrl)
	{
	// callback from control queue. Execute controls in packet until one of them gets processed
	// assume async packets always as not processed to ensure sync safety (usually, sync commands should better not ovberride async commands anyway)
	for (C4ControlPlayerControl2::ControlItemVec::const_iterator i = pCtrl->GetControlItems().begin(); i != pCtrl->GetControlItems().end(); ++i)
		{
		const C4ControlPlayerControl2::ControlItem &rItem = *i;
		C4PlayerControlDef *pCtrlDef = ControlDefs.GetControlByIndex(rItem.iControl);
		if (pCtrlDef)
			{
			if (ExecuteControl(rItem.iControl, pCtrl->IsReleaseControl(), pCtrl->GetExtraData(), rItem.iTriggerMode, false))
				if (pCtrlDef->IsSync())
					break;
			}
		}
	}

bool C4PlayerControl::ExecuteControl(int32_t iControl, bool fUp, const C4KeyEventData &rKeyExtraData, int32_t iTriggerMode, bool fRepeated)
	{
	// execute single control. return if handled
	C4PlayerControlDef *pControlDef = ControlDefs.GetControlByIndex(iControl);
	if (!pControlDef || Sync.IsControlDisabled(iControl)) return false;
	C4PlayerControlDef::Actions eAction = pControlDef->GetAction();
	C4KeyEventData KeyExtraData(rKeyExtraData);
	// global controls only in global context
	if (IsGlobal() != pControlDef->IsGlobal()) return false;
	// hold-actions only work on script controls with the hold flag
	if (iTriggerMode & (C4PlayerControlAssignment::CTM_Hold | C4PlayerControlAssignment::CTM_Release))
		{
		if (eAction != C4PlayerControlDef::CDA_Script) return false;
		if (!pControlDef->IsHoldKey()) return false;
		if (fUp) return false; // hold triggers have no "up"-event
		// perform hold/release
		const CSync::ControlDownState *pCtrlDownState = Sync.GetControlDownState(iControl);
		if (!pCtrlDownState) return false;
		bool fWasDown = (pCtrlDownState->DownState.iStrength > 0);
		if (fWasDown)
			{
			// control is currently down: release?
			if (iTriggerMode & C4PlayerControlAssignment::CTM_Release)
				{
				KeyExtraData.iStrength = 0;
				Sync.SetControlDownState(iControl, KeyExtraData, Game.FrameCounter, false);
				// now process as a regular "Up" event
				fUp = true;
				fRepeated = false;
				}
			else //if (iTriggerMode & C4PlayerControlAssignment::CTM_Hold) - must be true
				{
				// control is down but trigger key is pressed again: Refresh down state
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
	else if (fUp)
		{
		// regular ControlUp: Only valid if that control was down
		const CSync::ControlDownState *pCtrlDownState = Sync.GetControlDownState(iControl);
		if (!pCtrlDownState) return false;
		bool fWasDown = (pCtrlDownState->DownState.iStrength > 0);
		if (!fWasDown) return false;
		}
	// perform action for this control
	bool fHandled = ExecuteControlAction(iControl, eAction, pControlDef->GetExtraData(), fUp, KeyExtraData, fRepeated);
	// return if handled, unless control is defined as always unhandled
	return fHandled && !(iTriggerMode & C4PlayerControlAssignment::CTM_AlwaysUnhandled);
	}

bool C4PlayerControl::ExecuteControlAction(int32_t iControl, C4PlayerControlDef::Actions eAction, C4ID idControlExtraData, bool fUp, const C4KeyEventData &rKeyExtraData, bool fRepeated)
	{
	// get affected player
	C4Player *pPlr = NULL;
	if (iPlr > -1)
		{
		pPlr = Game.Players.Get(iPlr);
		if (!pPlr) return false;
		}
	// exec action (on player)
	switch (eAction)
		{
		// scripted player control
		case C4PlayerControlDef::CDA_Script:
			return ExecuteControlScript(iControl, idControlExtraData, fUp, rKeyExtraData, fRepeated);

		// menu controls
		case C4PlayerControlDef::CDA_Menu: if (!pPlr || fUp) return false; if (pPlr->Menu.IsActive()) pPlr->Menu.Close(false); else pPlr->ActivateMenuMain(); return true; // toggle
		case C4PlayerControlDef::CDA_MenuOK:     if (!pPlr || !pPlr->Menu.IsActive() || fUp) return false; pPlr->Menu.Control(COM_MenuEnter,0); return true; // ok on item
		case C4PlayerControlDef::CDA_MenuCancel: if (!pPlr || !pPlr->Menu.IsActive() || fUp) return false; pPlr->Menu.Control(COM_MenuClose,0); return true; // close menu
		case C4PlayerControlDef::CDA_MenuLeft:   if (!pPlr || !pPlr->Menu.IsActive() || fUp) return false; pPlr->Menu.Control(COM_MenuLeft ,0); return true; // navigate
		case C4PlayerControlDef::CDA_MenuUp:     if (!pPlr || !pPlr->Menu.IsActive() || fUp) return false; pPlr->Menu.Control(COM_MenuUp   ,0); return true; // navigate
		case C4PlayerControlDef::CDA_MenuRight:  if (!pPlr || !pPlr->Menu.IsActive() || fUp) return false; pPlr->Menu.Control(COM_MenuRight,0); return true; // navigate
		case C4PlayerControlDef::CDA_MenuDown:   if (!pPlr || !pPlr->Menu.IsActive() || fUp) return false; pPlr->Menu.Control(COM_MenuDown ,0); return true; // navigate

		//unknown action
		default: return false;
		}
	}

bool C4PlayerControl::ExecuteControlScript(int32_t iControl, C4ID idControlExtraData, bool fUp, const C4KeyEventData &rKeyExtraData, bool fRepeated)
	{
	if (!fUp)
		{
		// control down
		C4AulFunc *pFunc = Game.ScriptEngine.GetFirstFunc(PSF_PlayerControl);
		if (!pFunc) return false;
		C4AulParSet Pars(C4VInt(iControl), C4VID(idControlExtraData), C4VInt(rKeyExtraData.x), C4VInt(rKeyExtraData.y), C4VInt(rKeyExtraData.iStrength), C4VBool(fRepeated));
		return !!pFunc->Exec(NULL, &Pars);
		}
	else
		{
		// control up
		C4AulFunc *pFunc = Game.ScriptEngine.GetFirstFunc(PSF_PlayerControlRelease);
		if (!pFunc) return false;
		C4AulParSet Pars(C4VInt(iControl), C4VID(idControlExtraData), C4VInt(rKeyExtraData.x), C4VInt(rKeyExtraData.y));
		return !!pFunc->Exec(NULL, &Pars);
		}
	}


void C4PlayerControl::Execute()
	{
	// sync execution: Do keyrepeat, etc.
	}

C4PlayerControl::C4PlayerControl() : ControlDefs(Game.PlayerControlDefs), iPlr(-1), pControlSet(NULL)
	{
	}

void C4PlayerControl::Clear()
	{
	}

void C4PlayerControl::RegisterKeyset(int32_t iPlr, C4PlayerControlAssignmentSet *pKeyset)
	{
	// register all keys into Game.KeyboardInput creating KeyBindings
	}

