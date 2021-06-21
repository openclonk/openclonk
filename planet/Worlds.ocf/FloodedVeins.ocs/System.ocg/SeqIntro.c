// Intro sequence for Gold Rush: crew runs to middle of screen talking about settlement.

#appendto Sequence

public func Intro_Start()
{
	return ScheduleNext(4);
}

public func Intro_JoinPlayer(proplist plr)
{
	plr->SetZoomByViewRange(200, nil, PLRZOOM_Set | PLRZOOM_LimitMax);
	return;
}

public func Intro_1()
{
	for (var i = 0; i < GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		MessageBox(Format("$MsgAbandonedSettlement$", plr->GetCrew(1)->GetName()), plr->GetCrew(0), plr->GetCrew(0), plr, true);
		plr->GetCrew(0)->SetCommand("MoveTo", nil, 200 + Random(12), 160);
	}
	return ScheduleNext(4 * 36);
}

public func Intro_2()
{
	for (var i = 0; i < GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		MessageBox("$MsgLegendGems$", plr->GetCrew(0), plr->GetCrew(1), plr, true);
		plr->GetCrew(0)->SetDir(DIR_Left);
		plr->GetCrew(1)->SetCommand("MoveTo", nil, 180 + Random(12), 160);
	}
	return ScheduleNext(4 * 36);
}

public func Intro_3()
{
	for (var i = 0; i < GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		MessageBox(Format("$MsgExplore$", plr->GetCrew(1)->GetName()), plr->GetCrew(0), plr->GetCrew(0), plr, true);
		plr->GetCrew(0)->SetCommand("MoveTo", nil, 320 + Random(12), 160);
	}
	return ScheduleNext(4 * 36);
}

public func Intro_4()
{
	for (var i = 0; i < GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		MessageBox("$MsgAgree$", plr->GetCrew(0), plr->GetCrew(1), plr, true);
		plr->GetCrew(0)->SetDir(DIR_Left);
		plr->GetCrew(1)->SetCommand("MoveTo", nil, 300 + Random(12), 160);
	}
	return ScheduleNext(4 * 36);
}

public func Intro_5()
{
	for (var i = 0; i < GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		plr->GetCrew(0)->SetCommand("None");
		plr->GetCrew(1)->SetCommand("None");
	}
	return Stop();
}

public func Intro_Stop()
{
	// Reset player zoom.
	for (var player in GetPlayers(C4PT_User)) player->SetZoomByViewRange(300, nil, PLRZOOM_Set | PLRZOOM_LimitMax);
	return true;
}