// Outro sequence for this tutorial.

#appendto Sequence

public func Outro_Init(int for_plr)
{
	return true;
}

public func Outro_JoinPlayer(int plr)
{
	SetPlayerZoomByViewRange(plr, 300, nil, PLRZOOM_Direct | PLRZOOM_Set);
	return;
}

public func Outro_Start()
{
	return ScheduleNext(4);
}

public func Outro_1()
{
	// Show last guide message and then stop the sequence and fulfill the goal.
	GameCall("ShowLastGuideMessage");
	return ScheduleNext(108);
}

public func Outro_2()
{
	return Stop();
}

public func Outro_Stop()
{
	// Fulfill the tutorial goal.
	var goal = FindObject(Find_ID(Goal_Tutorial));
	if (goal)
		goal->Fulfill();
	return true;
}