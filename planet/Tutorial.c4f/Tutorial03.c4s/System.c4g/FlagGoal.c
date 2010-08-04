// Flag goal also checks if all targets are destroyed.

#appendto Goal_ReachFlag

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
			return true;
		else
			flag->Message("$MsgTargetsLeft$", balloon_count);
	}
	// Otherwise unfulfilled.
	return false;
}