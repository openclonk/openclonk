/**
	Legacy.c
	Contains legacy code that will be removed after some time.
	
	@author Marky
*/

static const VERSION_10_0_OC = "10.0";

/* -- Scenario stuff -- */

/* -- Player stuff -- */

global func GetPlayerControlAssignment(any player, int ctrl, bool human_readable, bool short_name)
{
	LogLegacyWarning("GetPlayerControlAssignment", "GetPlayer(player)->GetControlAssignment(ctrl, human_readable, short_name)", VERSION_10_0_OC);
	player = GetPlayerLegacy(player);
	if (player)
	{
		return player->GetControlAssignment(ctrl, human_readable, short_name);
	}
	else
	{
		return nil;
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

global func GetPlayerName(any player)
{
	LogLegacyWarning("GetPlayerName", "GetPlayer(player)->GetName()", VERSION_10_0_OC);
	player = GetPlayerLegacy(player);
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

// Returns the name of a player, including color markup using the player color.
// documented in /docs/sdk/script/fn
global func GetTaggedPlayerName(any player)
{
	LogLegacyWarning("GetTaggedPlayerName", "GetPlayer(player)->GetTaggedName()", VERSION_10_0_OC);
	player = GetPlayerLegacy(player);
	if (player)
	{
		return player->GetTaggedName();
	}
}

global func Hostile(any player, any opponent, bool check_one_way_only)
{
	LogLegacyWarning("Hostile", "GetPlayer(player)->Hostile(opponent, check_one_way_only)", VERSION_10_0_OC);
	if (player)
	{
		return GetPlayerLegacy(player)->Hostile(GetPlayerLegacy(opponent), check_one_way_only, ...);
	}
	else
	{
		return false;
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

global func SetHostility(any player, any opponent, bool hostile, bool silent, bool no_calls)
{
	LogLegacyWarning("SetHostility", "GetPlayer(player)->SetHostility(opponent, hostile, silent, no_calls)", VERSION_10_0_OC);
	if (player)
	{
		return GetPlayerLegacy(player)->SetHostility(GetPlayerLegacy(opponent), hostile, silent, no_calls, ...);
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

global func SetPlayerViewLock(any player, bool is_locked)
{
	LogLegacyWarning("SetPlayerViewLock", "GetPlayer(player)->SetViewLocked(is_locked)", VERSION_10_0_OC);
	// special player NO_OWNER: apply to all players
	if (player == NO_OWNER)
	{
		for (var index = 0; index < GetPlayerCount(); index++)
		{
			GetPlayerByIndex(index)->SetViewLocked(is_locked);
		}
		return true;
	}
	player = GetPlayerLegacy(player);
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

global func SetPlayerZoom(int player_nr, int zoom, int precision, int flags)
{
	LogLegacyWarning("SetPlayerZoom", "GetPlayer(player)->SetZoom(zoom, precision, flags)", VERSION_10_0_OC);
	// special player NO_OWNER: apply to all players
	if (player_nr == NO_OWNER)
	{
		for (var index = 0; index < GetPlayerCount(); index++)
		{
			GetPlayerByIndex(index)->SetZoom(zoom, precision, flags, ...);
		}
		return true;
	}
	var player = GetPlayer(player_nr);
	if (player)
	{
		player->SetZoom(zoom, precision, flags, ...);
		return true;
	}
	else
	{
		return false;
	}
}

global func SetPlayerZoomByViewRange(int player_nr, int range_wdt, int range_hgt, int flags)
{
	LogLegacyWarning("SetPlayerZoomByViewRange", "GetPlayer(player)->SetZoomByViewRange(range_wdt, range_hgt, flags)", VERSION_10_0_OC);
	// special player NO_OWNER: apply to all players
	if (player_nr == NO_OWNER)
	{
		for (var index = 0; index < GetPlayerCount(); index++)
		{
			GetPlayerByIndex(index)->SetZoomByViewRange(range_wdt, range_hgt, flags, ...);
		}
		return true;
	}
	var player = GetPlayer(player_nr);
	if (player)
	{
		player->SetZoomByViewRange(range_wdt, range_hgt, flags, ...);
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

global func SetViewOffset(any player, int x, int y)
{
	LogLegacyWarning("SetViewOffset(player, x, y)", "GetPlayer(player)->SetViewOffset(x, y)", VERSION_10_0_OC);
	return GetPlayerLegacy(player)->SetViewOffset(x, y, ...);
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

/* -- Player number conversion -- */

global func GetPlayerLegacy(any player)
{
	if (GetType(player) == C4V_Int)
	{
		LogLegacyWarning("player number", "player proplist version", VERSION_10_0_OC);
		return GetPlayer(player);
	}
	return player;
}

global func CheckVisibility(any player)
{
	return inherited(GetPlayerLegacy(player), ...);
}

global func CreateConstruction(type, int x, int y, any owner, int completion, bool adjust_terrain, bool check_site)
{
	return inherited(type, x, y, GetPlayerLegacy(owner), completion, adjust_terrain, check_site, ...);
}
/*
global func CreateObjectAbove(type, int x, int y, any owner)
{
	return inherited(type, x, y, GetPlayerLegacy(owner), ...);
}
*/
global func CreateObject(type, int x, int y, any owner)
{
	return inherited(type, x, y, GetPlayerLegacy(owner), ...);
}

global func CustomMessage(string message, object obj, any player, int offset_x, int offset_y, int color, id deco, proplist portrait, int flags, int width)
{
	return inherited(message, obj, GetPlayerLegacy(player), offset_x, offset_y, color, deco, portrait, flags, width, ...);
}

global func DoScoreboardShow(int change,  any player)
{
	if (GetType(player) == C4V_Int)
	{
		player -= 1;
	}
	return inherited(change, GetPlayerLegacy(player), ...);
}

global func GetPlayerID(any player)
{
	LogLegacyWarning("GetPlayerID()", "GetPlayer(player).ID", VERSION_10_0_OC);
	return GetPlayerLegacy(player).ID;
}

global func GetPlayerInfoCoreVal(string entry, string section, any player, int index)
{
	return inherited(entry, section, GetPlayerLegacy(player), index, ...);
}

global func GetPlayerVal(string entry, string section, any player, int index)
{
	return inherited(entry, section, GetPlayerLegacy(player), index, ...);
}

global func MakeCrewMember(any player)
{
	return inherited(GetPlayerLegacy(player), ...);
}

global func PlayerMessage(any player, string message)
{
	return inherited(GetPlayerLegacy(player), message, ...);
}

global func PlayRumble(any player, int strength, int length)
{
	if (GetType(player) == C4V_Int && player == NO_OWNER)
	{
		// NO_OWNER: play rumble for all players (e.g. earthquakes)
		LogLegacyWarning("PlayRumble(-1)", "for-loop", VERSION_10_0_OC);
		for (var index = 0; index < GetPlayerCount(); index++)
		{
			GetPlayerByIndex(index)->PlayRumble(strength, length, ...);
		}
		return true;
	}
	return GetPlayerLegacy(player)->PlayRumble(strength, length, ...);	
}

global func SetCrewStatus(any player, bool in_crew)
{
	return inherited(GetPlayerLegacy(player), in_crew, ...);
}

global func SetGlobalSoundModifier(proplist modifier_props, any player)
{
	return inherited(modifier_props, GetPlayerLegacy(player), ...);
}

global func SetPlayList(any playlist, any player, bool force_switch, int fade_time_ms, int max_resume_time_ms)
{
	return inherited(playlist, GetPlayerLegacy(player), force_switch, fade_time_ms, max_resume_time_ms, ...);
}

global func Sound(string sound, bool global, int level, any player, int loop_count, int custom_falloff_distance, int pitch, proplist modifier_props)
{
	return inherited(sound, global, level, GetPlayerLegacy(player), loop_count, custom_falloff_distance, pitch, modifier_props, ...);
}

global func SoundAt(string sound, int x, int y, int level, any player, int custom_falloff_distance, int pitch, proplist modifier_props)
{
	return inherited(sound, x, y, level, GetPlayerLegacy(player), custom_falloff_distance, pitch, modifier_props, ...);
}

global func StopRumble(any player)
{
	if (GetType(player) == C4V_Int && player == NO_OWNER)
	{
		// NO_OWNER: stop rumble for all players
		// Not sure whether this makes sense to do - mainly provided for symmetry with PlayRumble().
		LogLegacyWarning("StopRumble(-1)", "for-loop", VERSION_10_0_OC);
		for (var index = 0; index < GetPlayerCount(); index++)
		{
			GetPlayerByIndex(index)->StopRumble();
		}
		return true;
	}
	return GetPlayerLegacy(player)->StopRumble();	
}

/* -- Other stuff -- */


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
	DebugLog("WARNING: Do not use the legacy function \"%s\" anymore; you can use \"%s\" instead (complete removal is planned for \"%s\").", function_name, replacement_name, version);
}

global func LogLegacyWarningRemoved(string function_name)
{
	DebugLog("WARNING: Do not use the legacy function \"%s\" anymore; it was removed and has no effect.", function_name);
}

global func LogLegacyWarningCustom(string function_name, string custom, string version)
{
	DebugLog("WARNING: Do not use the legacy function \"%s\" anymore; %s (complete removal is planned for \"%s\").", function_name, custom, version);
}

