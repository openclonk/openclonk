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

