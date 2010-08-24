// Flag goal also checks if all targets are destroyed.

#appendto Goal_ReachFlag

protected func Initialize()
{
	SetProperty("Description", "$NewGoalDescription$", this);
	return _inherited(...);
}

public func IsFulfilled() 
{ 
	// No flag, goal fulfilled.
	if (!flag)
		return true;
	var clonk = FindObject(Find_OCF(OCF_CrewMember), Find_Distance(50, flag->GetX() - GetX(), flag->GetY() - GetY()));
	if (clonk)
	{
		var balloon_count = ObjectCount(Find_ID(PracticeTarget));
		if (balloon_count == 0)
		{
			// Balloon_count zero, hence goal fulfilled.
			return true;
		}
		else
		{
			// Notify the player.
			if (balloon_count == 1)
				flag->Message("$MsgOneTargetLeft$");
			else
				flag->Message("$MsgTargetsLeft$", balloon_count);
		}
	}
	// Otherwise unfulfilled.
	return false;
}

