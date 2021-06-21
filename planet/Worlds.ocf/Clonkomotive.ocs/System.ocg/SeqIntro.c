// Intro sequence for Clonkomotive: crew drives train which runs out of fuel.

#appendto Sequence

public func Intro_Start()
{
	var train = FindObject(Find_ID(Locomotive));
	train->CreateContents(Coal, 2)->SetCon(20);
	train->PutLiquid(Water, 300);
	train->ContainedRight();
	return ScheduleNext(4);
}

public func Intro_JoinPlayer(proplist plr)
{
	plr->SetZoomByViewRange(300, nil, PLRZOOM_Set | PLRZOOM_LimitMax);
	var train = FindObject(Find_ID(Locomotive));
	var index = 0, crew;
	while (crew = plr->GetCrew(index))
	{
		crew->Enter(train);
		index++;
	}
	return;
}

public func Intro_1()
{
	for (var i = 0; i < GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		MessageBox("$MsgDriveTrain$", plr->GetCrew(0), plr->GetCrew(0), plr, true);
	}
	return ScheduleNext(6 * 36);
}

public func Intro_2()
{
	// Stop the train and remove fuel
	var train = FindObject(Find_ID(Locomotive));
	train->ContainedStop();
	RemoveAll(Find_ID(Barrel), Find_Container(train));
	
	// Exit crew from the train.
	for (var i = 0; i < GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		var index = 0, crew;
		while (crew = plr->GetCrew(index++))
			crew->SetCommand("Exit");
	}	

	for (var i = 0; i < GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		MessageBox("$MsgOutOfFuel$", plr->GetCrew(0), plr->GetCrew(1), plr, true);
	}
	return ScheduleNext(3 * 36);
}

public func Intro_3()
{
	for (var i = 0; i < GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		MessageBox("$MsgBridgesGone$", plr->GetCrew(0), plr->GetCrew(0), plr, true);
	}
	return Stop();
}

public func Intro_Stop()
{
	// Reset player zoom.
	for (var player in GetPlayers(C4PT_User)) player->SetZoomByViewRange(600, nil, PLRZOOM_Set | PLRZOOM_LimitMax);
	return true;
}