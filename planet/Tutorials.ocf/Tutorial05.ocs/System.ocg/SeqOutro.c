// Outro sequence for this tutorial.

#appendto Sequence

public func Outro_Init(int for_plr)
{
	this.plr = for_plr;
	this.head = Dialogue->FindByName("VillageHead")->GetDialogueTarget();
	this.plr_clonk = GetCrew(for_plr);
	return true;
}

public func Outro_JoinPlayer(int plr)
{
	SetPlayerZoomByViewRange(plr, 300, nil, PLRZOOM_Direct | PLRZOOM_Set);
	return;
}

public func Outro_Start()
{
	this.plr_clonk->SetCommand("MoveTo", nil,  820, 702);	
	return ScheduleNext(4);
}

public func Outro_1()
{
	MessageBox(Format("$MsgClonkSafe$", this.head->GetName()), this.plr_clonk, this.plr_clonk, this.plr, true);
	return ScheduleNext(160);
}

public func Outro_2()
{
	this.head.ActMap.Walk.Speed = 38;
	this.head->SetPosition(458, 732);
	this.head->SetCommand("MoveTo", nil, 816, 702);
	MessageBox("$MsgVillageHeadComing$", this.plr_clonk, this.head, this.plr, true);
	return ScheduleNext(10);
}

public func Outro_3()
{
	if (!Inside(this.head->GetX(), 812, 816))
		return ScheduleSame(10);
	var case = FindObject(Find_ID(ElevatorCase));
	case->MoveTo(720);
	return ScheduleNext(10);
}

public func Outro_4()
{
	var case = FindObject(Find_ID(ElevatorCase));
	if (case->GetY() <= 700)
		return ScheduleSame(10);
	this.head->SetCommand("Grab", case);
	case->MoveTo(240);
	MessageBox("$MsgVillageHeadTakeElevator$", this.plr_clonk, this.head, this.plr, true);
	return ScheduleNext(140);
}

public func Outro_5()
{
	var case = FindObject(Find_ID(ElevatorCase));
	if (case->GetY() >= 254)
		return ScheduleSame(10);
	this.head->SetCommand("UnGrab");
	this.head->SetCommand("MoveTo", nil, this.head->GetX() + 100, this.head->GetY());
	MessageBox("$MsgVillageHeadPassedCave$", this.plr_clonk, this.head, this.plr, true);
	return ScheduleNext(30);
}

public func Outro_6()
{
	if (this.plr_clonk.ActMap == this.plr_clonk.Prototype.ActMap)
		this.plr_clonk.ActMap = new this.plr_clonk.ActMap {};
	if (this.plr_clonk.ActMap.Walk == this.plr_clonk.Prototype.ActMap.Walk)	
		this.plr_clonk.ActMap.Walk = new this.plr_clonk.ActMap.Walk {};
	this.plr_clonk.ActMap.Walk.Speed = 38;
	this.plr_clonk->SetCommand("MoveTo", nil, this.plr_clonk->GetX() + 90, this.plr_clonk->GetY());	
	return ScheduleNext(180);
}

public func Outro_7()
{
	// Show last guide message and then stop the sequence and fulfill the goal.
	GameCall("ShowLastGuideMessage");
	return ScheduleNext(108);
}

public func Outro_8()
{
	this.plr_clonk.ActMap.Walk.Speed = Clonk.ActMap.Walk.Speed;
	this.head.ActMap.Walk.Speed = Clonk.ActMap.Walk.Speed;
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