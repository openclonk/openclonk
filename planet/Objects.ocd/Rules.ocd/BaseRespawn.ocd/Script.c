/* Converts itself to the Rule_Relaunch rule. */

protected func Construction()
{
	GetRelaunchRule()->SetBaseRespawn(true);
	return RemoveObject();
}