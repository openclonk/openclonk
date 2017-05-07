/* Converts itself to the Rule_Relaunch rule. */

protected func Construction()
{
	var relaunch_rule = GetRelaunchRule();
	relaunch_rule->SetBaseRespawn(true);
	relaunch_rule->SetFreeCrew(false);
	relaunch_rule->SetLastClonkRespawn(true);
	return RemoveObject();
}