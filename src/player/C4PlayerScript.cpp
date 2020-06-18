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

#include "C4Include.h"

#include "control/C4GameControl.h"
#include "object/C4Object.h"
#include "script/C4Aul.h"
#include "script/C4AulDefFunc.h"
#include "player/C4Player.h"
#include "player/C4PlayerList.h"
#include "player/C4PlayerScript.h"


static long FnEliminate(C4Player *player, bool remove_direct)
{
	// direct removal?
	if (remove_direct)
	{
		// do direct removal (no fate)
		if (::Control.isCtrlHost())
		{
			::Players.CtrlRemove(player->ID, false);
		}
		return true;
	}
	else
	{
		// do regular elimination
		if (player->Eliminated)
		{
			return false;
		}
		player->Eliminate();
	}
	return true;
}

static long FnGetColor(C4Player *player)
{
	return player->ColorDw;
}

static C4Object *FnGetCrew(C4Player *player, long index)
{
	return player->Crew.GetObject(index);
}

static long FnGetCrewCount(C4Player *player)
{
	return player->Crew.ObjectCount();
}

static C4Value FnGetCrewMembers(C4Player *player)
{
	int32_t count = FnGetCrewCount(player);
	C4ValueArray *results = new C4ValueArray(count);
	for (int32_t i = 0; i < count; ++i)
	{
		(*results)[i] = C4VObj(FnGetCrew(player, i));
	}
	return C4VArray(results);
}

// undocumented!
static C4String *FnGetControlAssignment(C4Player *player, long control, bool human_readable, bool short_name)
{
	// WARNING: As many functions returning strings, the result is not sync safe!
	// "" is returned for invalid controls to make the obvious if (GetPlayerControlAssignmentName(...)) not cause a sync loss
	// get desired assignment from parameters
	if (!player->ControlSet) return String(""); // player has no control (remote player)
	C4PlayerControlAssignment *assignment = player->ControlSet->GetAssignmentByControl(control);
	if (!assignment) return String("");
	// get assignment as readable string
	return String(assignment->GetKeysAsString(human_readable, short_name).getData());
}

// undocumented!
static bool FnGetControlEnabled(C4Player *player, long ctrl)
{
	// get control set to check
	C4PlayerControl *plrctrl = &(player->Control);
	if (plrctrl)
	{
		return !plrctrl->IsControlDisabled(ctrl);
	}
	// invalid player or no controls
	return false;
}

static long FnGetControlState(C4Player *player, long iControl, bool fMovedState)
{
	// get control set to check
	C4PlayerControl *plrctrl = &(player->Control);
	if (plrctrl)
	{
		// query control
		const C4PlayerControl::CSync::ControlDownState *pControlState = plrctrl->GetControlDownState(iControl);
		// no state means not down
		if (!pControlState) return 0;
		// otherwise take either down-value or moved-value
		return fMovedState ? pControlState->MovedState.iStrength : pControlState->DownState.iStrength;
	}
	// invalid player or no controls
	return 0;
}

static C4Object *FnGetCursor(C4Player *player)
{
	return player->Cursor;
}

static C4Value FnGetExtraData(C4Player *player, C4String *DataName)
{
	const char *strDataName = FnStringPar(DataName);
	// no name list?
	if (!player->ExtraData.pNames) return C4Value();
	long ival;
	if ((ival = player->ExtraData.pNames->GetItemNr(strDataName)) == -1) return C4Value();
	// return data
	return player->ExtraData[ival];
}

static C4Object *FnGetHiRank(C4Player *player)
{
	return player->GetHiRankActiveCrew();
}

static long FnGetTeam(C4Player *player)
{
	// search team containing this player
	C4Team *pTeam = Game.Teams.GetTeamByPlayerID(player->ID);
	if (pTeam) return pTeam->GetID();
	// special value of -1 indicating that the team is still to be chosen
	if (player->IsChosingTeam()) return -1;
	// No team.
	return 0;
}

static C4Object *FnGetViewCursor(C4Player *player)
{
	return player->ViewCursor ? player->ViewCursor : player->Cursor;
}

static long FnGetViewMode(C4Player *player)
{
	if (::Control.SyncMode())
	{
		return -1;
	}
	return player->ViewMode;
}

static C4Object *FnGetViewTarget(C4Player *player)
{
	if (player->ViewMode != C4PVM_Target)
	{
		return nullptr;
	}
	return player->ViewTarget;
}

static void FnResetCursorView(C4Player *player, bool immediate_position)
{
	player->ResetCursorView(immediate_position);
}

// undocumented!
static bool FnSetControlEnabled(C4Player *player, long ctrl, bool is_enabled)
{
	// get control set to check
	C4PlayerControl *plrctrl = &(player->Control);
	if (plrctrl)
	{
		// invalid control
		if (ctrl >= int32_t(Game.PlayerControlDefs.GetCount())) return false;
		// query
		return plrctrl->SetControlDisabled(ctrl, !is_enabled);
	}
	// invalid player or no controls
	return false;
}

static bool FnSetCursor(C4Player *player, C4Object *target, bool no_select_arrow)
{
	if ((target && !target->Status) || (target && target->CrewDisabled))
	{
		return false;
	}
	player->SetCursor(target, !no_select_arrow);
	return true;
}

static C4Value FnSetExtraData(C4Player *player, C4String *DataName, const C4Value & Data)
{
	const char * strDataName = FnStringPar(DataName);
	// do not allow data type C4V_Array or C4V_C4Object
	if (Data.GetType() != C4V_Nil &&
	    Data.GetType() != C4V_Int &&
	    Data.GetType() != C4V_Bool &&
	    Data.GetType() != C4V_String) return C4VNull;
	if (!player) return C4Value();
	// no name list created yet?
	if (!player->ExtraData.pNames)
		// create name list
		player->ExtraData.CreateTempNameList();
	// data name already exists?
	long ival;
	if ((ival = player->ExtraData.pNames->GetItemNr(strDataName)) != -1)
	{
		player->ExtraData[ival] = Data;
	}
	else
	{
		// add name
		player->ExtraData.pNames->AddName(strDataName);
		// get val id & set
		if ((ival = player->ExtraData.pNames->GetItemNr(strDataName)) == -1) return C4Value();
		player->ExtraData[ival] = Data;
	}
	// ok, return the value that has been set
	return Data;
}

static void FnSetFoW(C4Player *player, bool enabled)
{
	player->SetFoW(!!enabled);
}

static bool FnSetTeam(C4Player *player, long idNewTeam, bool no_calls)
{
	// no team changing in league games
	if (Game.Parameters.isLeague()) return false;
	C4PlayerInfo *playerinfo = player->GetInfo();
	if (!playerinfo) return false;
	// already in that team?
	if (player->Team == idNewTeam) return true;
	// ask team setting if it's allowed (also checks for valid team)
	if (!Game.Teams.IsJoin2TeamAllowed(idNewTeam, playerinfo->GetType())) return false;
	// ask script if it's allowed
	if (!no_calls)
	{
		if (!!::Game.GRBroadcast(PSF_RejectTeamSwitch, &C4AulParSet(player->ID, idNewTeam), true, true))
			return false;
	}
	// exit previous team
	C4Team *pOldTeam = Game.Teams.GetTeamByPlayerID(player->ID);
	int32_t idOldTeam = 0;
	if (pOldTeam)
	{
		idOldTeam = pOldTeam->GetID();
		pOldTeam->RemovePlayerByID(player->ID);
	}
	// enter new team
	if (idNewTeam)
	{
		C4Team *pNewTeam = Game.Teams.GetGenerateTeamByID(idNewTeam);
		if (pNewTeam)
		{
			pNewTeam->AddPlayer(*playerinfo, true);
			idNewTeam = pNewTeam->GetID();
		}
		else
		{
			// unknown error
			player->Team = idNewTeam = 0;
		}
	}
	if (!no_calls)
	{
	    // update hositlities if this is not a "silent" change
		player->SetTeamHostility();
	    // do callback to reflect change in scenario
		::Game.GRBroadcast(PSF_OnTeamSwitch, &C4AulParSet(player->ID, idNewTeam, idOldTeam), true);
	}
	return true;
}

static bool FnSetViewCursor(C4Player *player, C4Object *target)
{
	player->ViewCursor = target;
	return true; // For same behaviour as in SetCursor
}

static bool FnSetViewLocked(C4Player *player, bool is_locked)
{
	player->SetViewLocked(is_locked);
	return true;
}

static bool FnSetViewTarget(C4Player *player, C4Object *target, bool immediate_position)
{
	player->SetViewMode(C4PVM_Target, target, immediate_position);
	return true;
}

static bool FnSurrender(C4Player *player)
{
	if (player->Eliminated)
	{
		return false;
	}
	player->Surrender();
	return true;
}

void C4PlayerScript::RegisterWithEngine(C4AulScriptEngine *engine)
{
    C4PropListStatic* prototype = new C4PropListStatic(nullptr, nullptr, ::Strings.RegString(PROTOTYPE_NAME_ENGINE));
	engine->RegisterGlobalConstant(PROTOTYPE_NAME_ENGINE, C4VPropList(prototype));
	#define F(f) ::AddFunc(prototype, #f, Fn##f)
		F(Eliminate);
		F(GetColor);
		F(GetControlAssignment);
		F(GetControlEnabled);
		F(GetControlState);
		F(GetCursor);
	    F(GetCrew);
	    F(GetCrewCount);
	    F(GetCrewMembers);
		F(GetExtraData);
		F(GetHiRank);
        F(GetTeam);
		F(GetViewCursor);
	    F(GetViewMode);
	    F(GetViewTarget);
		F(ResetCursorView);
		F(SetControlEnabled);
		F(SetCursor);
		F(SetExtraData);
	    F(SetFoW);
        F(SetTeam);
		F(SetViewCursor);
		F(SetViewLocked);
	    F(SetViewTarget);
		F(Surrender);
	#undef F
	prototype->Freeze();
}
