/**
	Legacy.c
	Contains legacy code that will be removed after some time.
	
	@author Marky
*/

/* -- Scenario stuff -- */

global func SetNextMission(string filename, string title, string description)
{
	LogLegacyWarning("SetNextMission", "SetNextScenario");
	return SetNextScenario(filename, title, description);
}

/* -- Internal helpers -- */

global func LogLegacyWarning(string function_name, string replacement_name)
{
	Log("WARNING: Do not use the legacy function \"%s\" anymore; use \"%s\" instead.", function_name, replacement_name);
}
