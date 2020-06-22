// Intro sequence for Horrid Highway.

#appendto Sequence

public func Intro_Init()
{
	this.bridge_left = FindObject(Find_ID(WoodenBridge), Sort_Distance(0, LandscapeHeight() / 2));
	var start_x = this.bridge_left->GetX() - 130;
	this.locomotive = CreateObjectAbove(Locomotive, start_x, LandscapeHeight() / 2 - 3);
	this.locomotive->SetDir(DIR_Right);
	this.locomotive->CreateContents(Coal, 100);
	this.locomotive.LiquidCapacity = 100000;
	this.locomotive->PutLiquid(Water, 100000);
	this.npc = CreateObjectAbove(Clonk, start_x, LandscapeHeight() / 2 - 3);
	this.npc->SetName("Kummerog");
	this.npc->Enter(this.locomotive);
	this.npc->SetAlternativeSkin("Carpenter");	
	this.locomotive->ContainedRight(this.npc);
	return true;
}

public func Intro_Start()
{
	return ScheduleNext(4);
}

public func Intro_JoinPlayer(int plr)
{
	SetPlayerZoomByViewRange(plr, 300, nil, PLRZOOM_Set | PLRZOOM_Direct);
	var crew, index = 0;
	while (crew = GetCrew(plr, index++))
		crew->Enter(this.locomotive);
	return;
}

public func Intro_1()
{
	for (var i = 0; i < GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		MessageBox(Format("$MsgWelcomeHorridHighway$", GetCrew(plr, 0)->GetName()), GetCrew(plr, 0), this.npc, plr, true);
	}
	return ScheduleNext(3 * 36);
}

public func Intro_2()
{
	this.locomotive->ContainedDown(this.npc);
	LaunchMeteor(this.bridge_left->GetX() + 132, 0, RandomX(40, 60), -15, RandomX(40, 50));
	return ScheduleNext(1 * 36);
}

public func Intro_3()
{
	for (var i = 0; i < GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		MessageBox("$MsgHighwayBroken$", GetCrew(plr, 0), this.npc, plr, true);
	}
	return ScheduleNext(5 * 36);
}

public func Intro_4()
{
	this.locomotive->ContainedLeft(this.npc);
	for (var i = 0; i < GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		var crew, index = 0;
		while (crew = GetCrew(plr, index++))
			crew->Exit();
	}
	return ScheduleNext(1 * 36);
}

public func Intro_5()
{
	for (var i = 0; i < GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		MessageBox("$MsgFixHighway$", GetCrew(plr, 0), this.npc, plr, true);
	}
	return ScheduleNext(6 * 36);
}

public func Intro_6()
{
	for (var i = 0; i < GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		MessageBox("$MsgRunLocomotives$", GetCrew(plr, 0), this.npc, plr, true);
	}
	return Stop();
}

public func Intro_Stop()
{
	// Reset player zoom.
	SetPlayerZoomByViewRange(NO_OWNER, 500, nil, PLRZOOM_Set | PLRZOOM_Direct);
	// Remove objects.
	this.npc->RemoveObject();
	this.locomotive->RemoveObject();
	return true;
}