// Sequence for Chine: eliminate all players when the cannon is lost.

#appendto Sequence

public func Failure_Start()
{
	return ScheduleNext(4);
}

public func Failure_JoinPlayer(int plr)
{
	SetPlayerZoomByViewRange(plr, 200, nil, PLRZOOM_Set | PLRZOOM_LimitMax);
	return;
}

public func Failure_1()
{
	for (var i = 0; i < GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		var crew0 = GetCrew(plr, 0);
		if (!crew0)
			continue;
		var crew1 = GetCrew(plr, 1) ?? GetCrew(plr, 0);
		MessageBox(Format("$MsgWhoDroppedCannon$", crew1->GetName()), crew0, crew0, plr, true);
	}
	return ScheduleNext(4 * 36);
}

public func Failure_2()
{
	for (var i = 0; i < GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		var crew0 = GetCrew(plr, 0);
		if (!crew0)
			continue;
		var crew1 = GetCrew(plr, 1) ?? GetCrew(plr, 0);
		MessageBox(Format("$MsgItWasMe$", crew0->GetName()), crew0, crew1, plr, true);
	}
	return ScheduleNext(3 * 36);
}

public func Failure_3()
{
	for (var i = 0; i < GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		var crew0 = GetCrew(plr, 0);
		if (!crew0)
			continue;
		var crew1 = GetCrew(plr, 1) ?? GetCrew(plr, 0);
		MessageBox(Format("$MsgShame$", crew1->GetName()), crew0, crew0, plr, true);
	}
	return ScheduleNext(4 * 36);
}

public func Failure_4()
{
	return Stop();
}

public func Failure_Stop()
{
	// Eliminate all players.
	for (var i = 0; i < GetPlayerCount(C4PT_User); ++i)
		EliminatePlayer(GetPlayerByIndex(i, C4PT_User));
	return true;
}