// Intro sequence for Gold Rush: crew runs to middle of screen talking about settlement.

#appendto Sequence

public func Intro_Start()
{
	return ScheduleNext(4);
}

public func Intro_JoinPlayer(proplist plr)
{
	plr->SetZoomByViewRange(300, nil, PLRZOOM_Set | PLRZOOM_LimitMax);
	return;
}

public func Intro_1()
{
	for (var i = 0; i < GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		MessageBox(Format("$MsgPeacefulPlace$", plr->GetCrew(1)->GetName()), plr->GetCrew(0), plr->GetCrew(0), plr, true);
	}
	return ScheduleNext(6 * 36);
}

public func Intro_2()
{
	for (var i = 0; i < GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		MessageBox(Format("$MsgSettlement$", plr->GetCrew(0)->GetName()), plr->GetCrew(0), plr->GetCrew(1), plr, true);
	}
	return ScheduleNext(6 * 36);
}

public func Intro_3()
{
	for (var i = 0; i < GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		MessageBox("$MsgToBeRich$", plr->GetCrew(0), plr->GetCrew(0), plr, true);
	}
	return Stop();
}

public func Intro_Stop()
{
	// Reset player zoom.
	for (var player in GetPlayers(C4PT_User)) player->SetZoomByViewRange(500, nil, PLRZOOM_Set | PLRZOOM_LimitMax);
	return true;
}