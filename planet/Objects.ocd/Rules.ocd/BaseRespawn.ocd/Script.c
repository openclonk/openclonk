/* Converts itself to the Rule_Relaunch rule. */

protected func Construction()
{
	DebugLog("WARNING: Rule_BaseRespawn is deprecated and will be removed in version 9.0, use Rule_Relaunch instead.");
	var relaunch_rule = GetRelaunchRule();
	relaunch_rule->SetBaseRespawn(true);
	relaunch_rule->SetFreeCrew(false);
	relaunch_rule->SetLastClonkRespawn(true);
	return RemoveObject();
}