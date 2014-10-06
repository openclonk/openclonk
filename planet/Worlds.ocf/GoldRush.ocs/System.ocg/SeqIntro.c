// Intro sequence for Gold Rush: crew runs to middle of screen talking about settlement.

#appendto Sequence

public func Intro_Start()
{
	return ScheduleNext(4);
}

public func Intro_JoinPlayer(int plr)
{
	SetPlayerZoomByViewRange(plr, 300, nil, PLRZOOM_Set | PLRZOOM_LimitMax);
	return;
}

public func Intro_1()
{
	for (var i = 0; i < GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		MessageBox(Format("$MsgPeacefulPlace$", GetCrew(plr, 1)->GetName()), GetCrew(plr, 0), GetCrew(plr, 0), plr, true);
	}
	return ScheduleNext(6 * 36);
}

public func Intro_2()
{
	for (var i = 0; i < GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		MessageBox(Format("$MsgSettlement$", GetCrew(plr, 0)->GetName()), GetCrew(plr, 0), GetCrew(plr, 1), plr, true);
	}
	return ScheduleNext(6 * 36);
}

public func Intro_3()
{
	for (var i = 0; i < GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		MessageBox("$MsgToBeRich$", GetCrew(plr, 0), GetCrew(plr, 0), plr, true);
	}
	return Stop();
}

public func Intro_Stop()
{
	// Reset player zoom.
	SetPlayerZoomByViewRange(NO_OWNER, 500, nil, PLRZOOM_Set | PLRZOOM_LimitMax);
	return true;
}