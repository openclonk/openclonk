/**
	Legacy.c
	Contains legacy code that will be removed after some time.
	
	@author Marky
*/

static const VERSION_10_0_OC = "10.0";

/* -- Scenario stuff -- */

global func GainMissionAccess(string password)
{
	LogLegacyWarning("GainMissionAccess", "GainScenarioAccess", VERSION_10_0_OC);
	return GainScenarioAccess(password);
}

global func GetMissionAccess(string password)
{
	LogLegacyWarning("GetMissionAccess", "GetScenarioAccess", VERSION_10_0_OC);
	return GetScenarioAccess(password);
}

/* -- Player stuff -- */

global func DoPlayerScore(int player_nr, int change)
{
	LogLegacyWarning("DoPlayerScore", "GetPlayer(player).Score += change", VERSION_10_0_OC);
	var player = GetPlayer(player_nr);
	if (player)
	{
		return player.Score += change;
	}
	else
	{
		return nil;
	}
}

global func EliminatePlayer(int player_nr, bool remove_direct)
{
	LogLegacyWarning("EliminatePlayer", "GetPlayer(player)->Eliminate(remove_direct)", VERSION_10_0_OC);
	var player = GetPlayer(player_nr);
	if (player)
	{
		return player->Eliminate();
	}
	else
	{
		return false;
	}
}

global func GetCrew(int player_nr, int index)
{
	LogLegacyWarning("GetCrew", "GetPlayer(player)->GetCrew(index)", VERSION_10_0_OC);
	var player = GetPlayer(player_nr);
	if (player)
	{
		return player->GetCrew(index);
	}
	else
	{
		return nil;
	}
}

global func GetCrewCount(int player_nr, int index)
{
	LogLegacyWarning("GetCrewCount", "GetPlayer(player)->GetCrewCount()", VERSION_10_0_OC);
	var player = GetPlayer(player_nr);
	if (player)
	{
		return player->GetCrewCount();
	}
	else
	{
		return nil;
	}
}

global func GetCursor(int player_nr)
{
	LogLegacyWarning("GetCursor", "GetPlayer(player)->GetCursor()", VERSION_10_0_OC);
	var player = GetPlayer(player_nr);
	if (player)
	{
		return player->GetCursor();
	}
	else
	{
		return nil;
	}
}

global func GetHiRank(int player_nr)
{
	LogLegacyWarning("GetHiRank", "GetPlayer(player)->GetHiRank()", VERSION_10_0_OC);
	var player = GetPlayer(player_nr);
	if (player)
	{
		return player->GetHiRank();
	}
	else
	{
		return nil;
	}
}

global func GetPlayerColor(int player_nr)
{
	LogLegacyWarning("GetPlayerColor", "GetPlayer(player)->GetColor()", VERSION_10_0_OC);
	var player = GetPlayer(player_nr);
	if (player)
	{
		return player->GetColor();
	}
	else
	{
		return 0;
	}
}

global func GetPlayerControlAssignment(int player_nr, int ctrl, bool human_readable, bool short_name)
{
	LogLegacyWarning("GetPlayerControlAssignment", "GetPlayer(player)->GetControlAssignment(ctrl, human_readable, short_name)", VERSION_10_0_OC);
	var player = GetPlayer(player_nr);
	if (player)
	{
		return player->GetControlAssignment(ctrl, human_readable, short_name);
	}
	else
	{
		return "";
	}
}

global func GetPlayerControlEnabled(int player_nr, int ctrl)
{
	LogLegacyWarning("GetPlayerControlEnabled", "GetPlayer(player)->GetControlEnabled(ctrl)", VERSION_10_0_OC);
	var player = GetPlayer(player_nr);
	if (player)
	{
		return player->GetControlEnabled(ctrl);
	}
	else
	{
		return false;
	}
}

global func GetPlayerControlState(int player_nr, int ctrl, bool analog_strength)
{
	LogLegacyWarning("GetPlayerControlAssignment", "GetPlayer(player)->GetControlState(ctrl, analog_strength)", VERSION_10_0_OC);
	var player = GetPlayer(player_nr);
	if (player)
	{
		return player->GetControlState(ctrl, analog_strength);
	}
	else
	{
		return 0;
	}
}

global func GetPlayerID(int plr_nr)
{
	LogLegacyWarningCustom("GetPlayerID", "The player ID and player number are the same since 9.0 OC", VERSION_10_0_OC);
	return plr_nr;
}

global func GetPlayerByID(int plr_id)
{
	LogLegacyWarningCustom("GetPlayerByID", "The player ID and player number are the same since 9.0 OC", VERSION_10_0_OC);
	return plr_id;
}

global func GetPlayerName(int player_nr)
{
	LogLegacyWarning("GetPlayerName", "GetPlayer(player)->GetName()", VERSION_10_0_OC);
	var player = GetPlayer(player_nr);
	if (player)
	{
		return player->GetName();
	}
	else
	{
		return nil;
	}
}

global func GetPlayerTeam(int player_nr)
{
	LogLegacyWarning("GetPlayerTeam", "GetPlayer(player)->GetTeam()", VERSION_10_0_OC);
	var player = GetPlayer(player_nr);
	if (player)
	{
		return player->GetTeam();
	}
	else
	{
		return 0;
	}
}

global func GetPlayerType(int player_nr)
{
	LogLegacyWarning("GetPlayerType", "GetPlayer(player).Type", VERSION_10_0_OC);
	var player = GetPlayer(player_nr);
	if (player)
	{
		return player.Type;
	}
	else
	{
		return 0;
	}
}

global func GetPlayerZoomLimits(int player_nr)
{
	LogLegacyWarning("GetPlayerZoomLimits", "GetPlayer(player) and the respective zoom limit", VERSION_10_0_OC);
	var player = GetPlayer(player_nr);
	if (player)
	{
		return
		{
			MaxWidth  = player.ZoomLimit_MaxWidth,
			MaxHeight = player.ZoomLimit_MaxHeight,
			MaxValue  = player.ZoomLimit_MaxValue,
			MinWidth  = player.ZoomLimit_MinWidth,
			MinHeight = player.ZoomLimit_MinHeight,
			MinValue  = player.ZoomLimit_MinValue,
		};
	}
	else
	{
		return nil;
	}
}

global func GetPlrClonkSkin(int player_nr)
{
	LogLegacyWarning("GetPlrClonkSkin", "GetPlayer(player).CrewSkin", VERSION_10_0_OC);
	var player = GetPlayer(player_nr);
	if (player)
	{
		return player.CrewSkin;
	}
	else
	{
		return nil;
	}
}

global func GetPlrExtraData(int player_nr, string data_name)
{
	LogLegacyWarning("GetPlrExtraData", "GetPlayer(player)->GetExtraData(data_name)", VERSION_10_0_OC);
	var player = GetPlayer(player_nr);
	if (player)
	{
		return player->GetExtraData(data_name);
	}
	else
	{
		return nil;
	}
}

global func GetPlrView(int player_nr)
{
	LogLegacyWarning("GetPlrView", "GetPlayer(player)->GetViewTarget()", VERSION_10_0_OC);
	var player = GetPlayer(player_nr);
	if (player)
	{
		return player->GetViewTarget();
	}
	else
	{
		return -1;
	}
}

global func GetPlrViewMode(int player_nr)
{
	LogLegacyWarning("GetPlrViewMode", "GetPlayer(player)->GetViewMode()", VERSION_10_0_OC);
	var player = GetPlayer(player_nr);
	if (player)
	{
		return player->GetViewMode();
	}
	else
	{
		return -1;
	}
}

global func GetPlayerScore(int player_nr)
{
	LogLegacyWarning("GetPlayerScore", "GetPlayer(player).Score", VERSION_10_0_OC);
	var player = GetPlayer(player_nr);
	if (player)
	{
		return player.Score;
	}
	else
	{
		return 0;
	}
}

global func GetPlayerScoreGain(player_nr)
{
	LogLegacyWarning("GetPlayerScoreGain", "GetPlayer(player).Score - GetPlayer(player).InitialScore", VERSION_10_0_OC);
	var player = GetPlayer(player_nr);
	if (player)
	{
		return player.Score - player.InitialScore;
	}
	else
	{
		return 0;
	}
}

global func GetScriptPlayerExtraID(player_nr)
{
	LogLegacyWarning("GetScriptPlayerExtraID", "GetPlayer(player).ExtraID", VERSION_10_0_OC);
	var player = GetPlayer(player_nr);
	if (player)
	{
		return player.ExtraID;
	}
	else
	{
		return nil;
	}
}

global func GetViewCursor(int player_nr)
{
	LogLegacyWarning("GetViewCursor", "GetPlayer(player)->GetViewCursor()", VERSION_10_0_OC);
	var player = GetPlayer(player_nr);
	if (player)
	{
		return player->GetViewCursor();
	}
	else
	{
		return nil;
	}
}

global func ResetCursorView(int player_nr, bool immediate)
{
	LogLegacyWarning("ResetCursorView", "GetPlayer(player)->ResetCursorView(immediate)", VERSION_10_0_OC);
	var player = GetPlayer(player_nr);
	if (player)
	{
		player->ResetCursorView(immediate);
	}
}

global func SetCursor(int player_nr, object target, bool no_select_arrow)
{
	LogLegacyWarning("SetCursor", "GetPlayer(player)->SetCursor(target, no_select_arrow)", VERSION_10_0_OC);
	var player = GetPlayer(player_nr);
	if (player)
	{
		return player->SetCursor(target, no_select_arrow);
	}
	else
	{
		return false;
	}
}

global func SetFoW(bool enabled, int player_nr)
{
	LogLegacyWarning("SetFoW", "GetPlayer(player)->SetFoW(enabled)", VERSION_10_0_OC);
	var player = GetPlayer(player_nr);
	if (player)
	{
		return player->SetFoW(enabled);
	}
	else
	{
		return nil;
	}
}

global func SetPlrExtraData(int player_nr, string data_name, any data)
{
	LogLegacyWarning("SetPlrExtraData", "GetPlayer(player)->SetExtraData(data_name, data)", VERSION_10_0_OC);
	var player = GetPlayer(player_nr);
	if (player)
	{
		return player->SetExtraData(data_name, data);
	}
	else
	{
		return nil;
	}
}

global func SetPlrView(int player_nr, object target, bool immediate)
{
	LogLegacyWarning("SetPlrView", "GetPlayer(player)->SetViewTarget(target, immediate)", VERSION_10_0_OC);
	var player = GetPlayer(player_nr);
	if (player)
	{
		return player->SetCursor(target, immediate);
	}
	else
	{
		return false;
	}
}

global func SetPlayerControlEnabled(int player_nr, int ctrl, bool is_enabled)
{
	LogLegacyWarning("SetPlayerControlEnabled", "GetPlayer(player)->SetControlEnabled(ctrl, is_enabled)", VERSION_10_0_OC);
	var player = GetPlayer(player_nr);
	if (player)
	{
		return player->SetControlEnabled(ctrl, is_enabled);
	}
	else
	{
		return false;
	}
}

global func SetPlayerTeam(int player_nr, int new_team, bool no_calls)
{
	LogLegacyWarning("SetPlayerTeam", "GetPlayer(player)->SetTeam(new_team, no_calls)", VERSION_10_0_OC);
	var player = GetPlayer(player_nr);
	if (player)
	{
		return player->SetTeam(new_team, no_calls);
	}
	else
	{
		return false;
	}
}

global func SetPlayerViewLock(int player_nr, bool is_locked)
{
	LogLegacyWarning("SetPlayerViewLock", "GetPlayer(player)->SetViewLocked(is_locked)", VERSION_10_0_OC);
	// special player NO_OWNER: apply to all players
	if (player_nr == NO_OWNER)
	{
		for (var index = 0; index < GetPlayerCount(); index++)
		{
			SetPlayerViewLock(GetPlayerByIndex(index), is_locked);
		}
		return true;
	}
	var player = GetPlayer(player_nr);
	if (player)
	{
		player->SetViewLocked(is_locked);
		return true;
	}
	else
	{
		return false;
	}
}

global func SetViewCursor(int player_nr, object target)
{
	LogLegacyWarning("SetViewCursor", "GetPlayer(player)->SetViewCursor(target)", VERSION_10_0_OC);
	var player = GetPlayer(player_nr);
	if (player)
	{
		return player->SetViewCursor(target);
	}
	else
	{
		return false;
	}
}

global func SurrenderPlayer(int player_nr, bool remove_direct)
{
	LogLegacyWarning("SurrenderPlayer", "GetPlayer(player)->Surrender()", VERSION_10_0_OC);
	var player = GetPlayer(player_nr);
	if (player)
	{
		return player->Surrender();
	}
	else
	{
		return false;
	}
}

/* -- Other stuff -- */


global func PlaceAnimal(id animal_id)
{
	LogLegacyWarning("PlaceAnimal", "id->Place()", VERSION_10_0_OC);
	if (animal_id)
	{
		if (animal_id.Place)
		{
			var animals = animal_id->Place(1);
			return animals[0];
		}
		else
		{
			Log("WARNING: Place(int amount, proplist area, proplist settings) not supported by ID: %i", animal_id);
			return nil;
		}
	}
	return nil;
}

global func SetNextMission(string filename, string title, string description)
{
	LogLegacyWarning("SetNextMission", "SetNextScenario", VERSION_10_0_OC);
	return SetNextScenario(filename, title, description);
}

global func SetBridgeActionData()
{
	LogLegacyWarningRemoved("SetBridgeActionData");
}

/* -- Internal helpers -- */

global func LogLegacyWarning(string function_name, string replacement_name, string version)
{
	Log("WARNING: Do not use the legacy function \"%s\" anymore; you can use \"%s\" instead (complete removal is planned for \"%s\").", function_name, replacement_name, version);
}

global func LogLegacyWarningRemoved(string function_name)
{
	Log("WARNING: Do not use the legacy function \"%s\" anymore; it was removed and has no effect.", function_name);
}

global func LogLegacyWarningCustom(string function_name, string custom, string version)
{
	Log("WARNING: Do not use the legacy function \"%s\" anymore; %s (complete removal is planned for \"%s\").", function_name, custom, version);
}

