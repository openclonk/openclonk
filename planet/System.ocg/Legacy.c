/**
	Legacy.c
	Contains legacy code that will be removed after some time.
	
	@author Marky
*/

static const VERSION_10_0_OC = "10.0";

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

