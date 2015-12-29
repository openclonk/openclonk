// Intro sequence for Clonkomotive: crew drives train which runs out of fuel.

#appendto Sequence

public func Intro_Start()
{
	var train = FindObject(Find_ID(Locomotive));
	train->CreateContents(Coal, 2)->SetCon(20);
	train->CreateContents(Barrel)->SetFilled("Water", 300);
	train->ContainedRight();
	return ScheduleNext(4);
}

public func Intro_JoinPlayer(int plr)
{
	SetPlayerZoomByViewRange(plr, 300, nil, PLRZOOM_Set | PLRZOOM_LimitMax);
	var train = FindObject(Find_ID(Locomotive));
	var index = 0, crew;
	while (crew = GetCrew(plr, index))
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
		MessageBox("$MsgDriveTrain$", GetCrew(plr, 0), GetCrew(plr, 0), plr, true);
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
		while (crew = GetCrew(plr, index++))
			crew->SetCommand("Exit");
	}	

	for (var i = 0; i < GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		MessageBox("$MsgOutOfFuel$", GetCrew(plr, 0), GetCrew(plr, 1), plr, true);
	}
	return ScheduleNext(3 * 36);
}

public func Intro_3()
{
	for (var i = 0; i < GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		MessageBox("$MsgBridgesGone$", GetCrew(plr, 0), GetCrew(plr, 0), plr, true);
	}
	return Stop();
}

public func Intro_Stop()
{
	// Reset player zoom.
	SetPlayerZoomByViewRange(NO_OWNER, 600, nil, PLRZOOM_Set | PLRZOOM_LimitMax);
	return true;
}