#appendto Goal_Melee

public func GetDescription(int plr)
{
	// Count active enemy clonks.
	var hostile_count = ObjectCount(Find_OCF(OCF_CrewMember), Find_NoContainer(), Find_Hostile(plr));
	var message;
	if (!hostile_count)
		message = "$MsgGoalFulfilled$";
	else
		message = Format("$MsgGoalUnfulfilled$", hostile_count);

	// Also report the remaining rounds.
	message = Format("%s|%s", message, CurrentRoundStr());

	return message;
}
