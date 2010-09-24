// Melee changed for the moment since CreateScriptPlayer does not work.

#appendto Goal_Melee

public func IsFulfilled()
{
	if (ObjectCount(Find_OCF(OCF_CrewMember), Find_Owner(NO_OWNER)))
		return false;
	return true;
}

public func Activate(int byplr)
{
	// Output
	if (IsFulfilled())
		MessageWindow("$MsgGoalFulfilled$", byplr);
	else
		MessageWindow(Format("$MsgGoalUnfulfilled$", 1), byplr);
	return;
}