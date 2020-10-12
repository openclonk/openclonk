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
#include "game/C4Viewport.h"
#include "object/C4Object.h"
#include "script/C4Aul.h"
#include "script/C4AulDefFunc.h"
#include "player/C4Player.h"
#include "player/C4PlayerList.h"
#include "player/C4PlayerScript.h"
#include "platform/C4GamePadCon.h"


// flags for SetPlayerZoom* calls
static const int PLRZOOM_Direct     = 0x01,
                 PLRZOOM_NoIncrease = 0x04,
                 PLRZOOM_NoDecrease = 0x08,
                 PLRZOOM_LimitMin   = 0x10,
                 PLRZOOM_LimitMax   = 0x20,
                 PLRZOOM_Set        = 0x40;


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

static bool FnGainScenarioAchievement(C4Player *player, C4String *achievement_name, Nillable<long> avalue, C4String *for_scenario)
{
	// safety
	if (!achievement_name || !achievement_name->GetData().getLength()) return false;
	// default parameter
	long value = avalue.IsNil() ? 1 : (long)avalue;
	// gain achievement
	return player->GainScenarioAchievement(achievement_name->GetCStr(), value, for_scenario ? for_scenario->GetCStr() : nullptr);
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

static bool FnHostile(C4Player *player, C4Player *opponent, bool check_one_way_only)
{
	if (check_one_way_only)
	{
		return ::Players.HostilityDeclared(player, opponent);
	}
	else
	{
		return !!::Players.Hostile(player, opponent);
	}
}

// strength: 0-1000, length: milliseconds
static bool FnPlayRumble(C4Player *player, long strength, long length)
{
	// Check parameters.
	if (!player || strength <= 0 || strength > 1000 || length <= 0)
	{
		return false;
	}
	// We can't return whether the rumble was actually played.
	if (player->pGamepad)
	{
		player->pGamepad->PlayRumble(strength / 1000.f, length);
	}
	return true;
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

static bool FnSetFilmView(C4Player *player)
{
	// Real switch in replays only
	if (!::Control.isReplay()) return true;
	// Set new target plr
	if (C4Viewport *vp = ::Viewports.GetFirstViewport())
	{
		vp->Init(player->Number, true); // NO_OWNER is not supported anymore
	}
	// Done, always success (sync)
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

static bool FnSetHostility(C4Player *player, C4Player *opponent, bool hostile, bool silent, bool no_calls)
{
	if (!player || !opponent)
	{
		return false;
	}
	// do rejection test first
	if (!no_calls)
	{
		if (!!::Game.GRBroadcast(PSF_RejectHostilityChange, &C4AulParSet(player->Number, opponent->Number, hostile), true, true))
		{
			return false;
		}
	}
	// OK; set hostility
	bool old_hostility = ::Players.HostilityDeclared(player, opponent);
	if (!player->SetHostility(opponent, hostile, silent))
	{
		return false;
	}
	// calls afterwards
	::Game.GRBroadcast(PSF_OnHostilityChange, &C4AulParSet(C4VInt(player->Number), C4VInt(opponent->Number), C4VBool(hostile), C4VBool(old_hostility)), true);
	return true;
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

static bool FnSetViewOffset(C4Player *player, long iX, long iY)
{
	// get player viewport
	C4Viewport *pView = ::Viewports.GetViewport(player->Number);
	if (!pView) return true; // sync safety
	pView->SetViewOffset(iX, iY);
	return true;
}

static bool FnSetViewTarget(C4Player *player, C4Object *target, bool immediate_position)
{
	player->SetViewMode(C4PVM_Target, target, immediate_position);
	return true;
}

static bool FnSetZoom(C4Player *player, long zoom, long precision, long flags)
{
	// Parameter safety. 0/0 means "reset to default".
	if (zoom < 0 || precision < 0)
	{
		return false;
	}
	// Zoom factor calculation
	if (!precision)
	{
		precision = 1;
	}
	C4Real fZoom = itofix(zoom, precision);
	// Adjust values in player
	if (flags & PLRZOOM_LimitMin) player->SetMinZoom(fZoom, !!(flags & PLRZOOM_NoIncrease), !!(flags & PLRZOOM_NoDecrease));
	if (flags & PLRZOOM_LimitMax) player->SetMaxZoom(fZoom, !!(flags & PLRZOOM_NoIncrease), !!(flags & PLRZOOM_NoDecrease));
	if ((flags & PLRZOOM_Set) || !(flags & (PLRZOOM_LimitMin | PLRZOOM_LimitMax)))
	{
		player->SetZoom(fZoom, !!(flags & PLRZOOM_Direct), !!(flags & PLRZOOM_NoIncrease), !!(flags & PLRZOOM_NoDecrease));
	}
	return true;
}

static bool FnSetZoomByViewRange(C4Player *player, long range_wdt, long range_hgt, long flags)
{
	// Zoom size safety - both ranges 0 is fine, it causes a zoom reset to default
	if (range_wdt < 0 || range_hgt < 0)
	{
		return false;
	}
	// Adjust values in player
	if (flags & PLRZOOM_LimitMin) player->SetMinZoomByViewRange(range_wdt, range_hgt, !!(flags & PLRZOOM_NoIncrease), !!(flags & PLRZOOM_NoDecrease));
	if (flags & PLRZOOM_LimitMax) player->SetMaxZoomByViewRange(range_wdt, range_hgt, !!(flags & PLRZOOM_NoIncrease), !!(flags & PLRZOOM_NoDecrease));
	// Set values after setting min/max to ensure old limits don't block new value
	if ((flags & PLRZOOM_Set) || !(flags & (PLRZOOM_LimitMin | PLRZOOM_LimitMax)))
	{
		player->SetZoomByViewRange(range_wdt, range_hgt, !!(flags & PLRZOOM_Direct), !!(flags & PLRZOOM_NoIncrease), !!(flags & PLRZOOM_NoDecrease));
	}
	return true;
}

static bool FnStopRumble(C4Player *player)
{
	if (!player) return false;
	if (player->pGamepad)
	{
		player->pGamepad->StopRumble();
	}
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

C4ScriptConstDef C4ScriptPlayerConstMap[]=
{
	{ "PLRZOOM_Direct"            ,C4V_Int,      PLRZOOM_Direct },
	{ "PLRZOOM_NoIncrease"        ,C4V_Int,      PLRZOOM_NoIncrease },
	{ "PLRZOOM_NoDecrease"        ,C4V_Int,      PLRZOOM_NoDecrease },
	{ "PLRZOOM_LimitMin"          ,C4V_Int,      PLRZOOM_LimitMin },
	{ "PLRZOOM_LimitMax"          ,C4V_Int,      PLRZOOM_LimitMax },
	{ "PLRZOOM_Set"               ,C4V_Int,      PLRZOOM_Set }
};

void C4PlayerScript::RegisterWithEngine(C4AulScriptEngine *engine)
{
    C4PropListStatic* prototype = new C4PropListStatic(nullptr, nullptr, ::Strings.RegString(PROTOTYPE_NAME_ENGINE));
	engine->RegisterGlobalConstant(PROTOTYPE_NAME_ENGINE, C4VPropList(prototype));

	// Add all def constants (all Int)
	for (C4ScriptConstDef *constant = &C4ScriptPlayerConstMap[0]; constant->Identifier; constant++)
	{
		assert(constant->ValType == C4V_Int); // only int supported currently
		engine->RegisterGlobalConstant(constant->Identifier, C4VInt(constant->Data));
	}

#define F(f) ::AddFunc(prototype, #f, Fn##f)
	F(Eliminate);
	F(GainScenarioAchievement);
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
	F(Hostile);
	F(PlayRumble);
	F(ResetCursorView);
	F(SetControlEnabled);
	F(SetCursor);
	F(SetExtraData);
	F(SetFilmView);
	F(SetFoW);
	F(SetHostility);
	F(SetTeam);
	F(SetViewCursor);
	F(SetViewLocked);
	F(SetViewOffset);
	F(SetViewTarget);
	F(SetZoom);
	F(SetZoomByViewRange);
	F(StopRumble);
	F(Surrender);
#undef F
	prototype->Freeze();
}
