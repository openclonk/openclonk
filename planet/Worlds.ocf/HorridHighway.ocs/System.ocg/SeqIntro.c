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

public func Intro_JoinPlayer(proplist plr)
{
	plr->SetZoomByViewRange(300, nil, PLRZOOM_Set | PLRZOOM_Direct);
	var crew, index = 0;
	while (crew = plr->GetCrew(index++))
		crew->Enter(this.locomotive);
	return;
}

public func Intro_1()
{
	for (var i = 0; i < GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		MessageBox(Format("$MsgWelcomeHorridHighway$", plr->GetCrew(0)->GetName()), plr->GetCrew(0), this.npc, plr, true);
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
		MessageBox("$MsgHighwayBroken$", plr->GetCrew(0), this.npc, plr, true);
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
		while (crew = plr->GetCrew(index++))
			crew->Exit();
	}
	return ScheduleNext(1 * 36);
}

public func Intro_5()
{
	for (var i = 0; i < GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		MessageBox("$MsgFixHighway$", plr->GetCrew(0), this.npc, plr, true);
	}
	return ScheduleNext(6 * 36);
}

public func Intro_6()
{
	for (var i = 0; i < GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		MessageBox("$MsgRunLocomotives$", plr->GetCrew(0), this.npc, plr, true);
	}
	return Stop();
}

public func Intro_Stop()
{
	// Reset player zoom.
	for (var player in GetPlayers(C4PT_User)) player->SetZoomByViewRange(500, nil, PLRZOOM_Set | PLRZOOM_Direct);
	// Remove objects.
	this.npc->RemoveObject();
	this.locomotive->RemoveObject();
	return true;
}