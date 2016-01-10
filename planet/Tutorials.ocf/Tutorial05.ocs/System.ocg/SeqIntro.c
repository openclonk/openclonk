// Intro sequence for this tutorial.

#appendto Sequence

public func Intro_Init(int for_plr)
{
	this.plr = for_plr;
	this.head = Dialogue->FindByName("VillageHead")->GetDialogueTarget();
	this.homeless = Dialogue->FindByName("Homeless")->GetDialogueTarget();
	this.plr_clonk = GetCrew(for_plr);
	
	this.head->PushActionSpeed("Walk", 38);
	this.plr_clonk->PushActionSpeed("Walk", 38);
	if (this.head.ActMap == this.head.Prototype.ActMap)
		this.head.ActMap = new this.head.ActMap {};
	if (this.head.ActMap.Walk == this.head.Prototype.ActMap.Walk)	
		this.head.ActMap.Walk = new this.head.ActMap.Walk {};
	this.head.ActMap.Walk.Speed = 38;
	if (this.plr_clonk.ActMap == this.plr_clonk.Prototype.ActMap)
		this.plr_clonk.ActMap = new this.plr_clonk.ActMap {};
	if (this.plr_clonk.ActMap.Walk == this.plr_clonk.Prototype.ActMap.Walk)	
		this.plr_clonk.ActMap.Walk = new this.plr_clonk.ActMap.Walk {};
	this.plr_clonk.ActMap.Walk.Speed = 38;
	return true;
}

public func Intro_JoinPlayer(int plr)
{
	SetPlayerZoomByViewRange(plr, 240, nil, PLRZOOM_Direct | PLRZOOM_Set);
	return;
}

public func Intro_Start()
{
	this.head->SetCommand("MoveTo", nil, this.head->GetX() + 180, this.head->GetY());
	this.plr_clonk->SetCommand("MoveTo", nil, this.plr_clonk->GetX() + 180, this.plr_clonk->GetY());	
	return ScheduleNext(4);
}

public func Intro_1()
{
	MessageBox("$MsgVillageHeadAnnoyed$", this.plr_clonk, this.head, this.plr, true);
	return ScheduleNext(160);
}

public func Intro_2()
{
	MessageBox("$MsgClonkHowToStrike$", this.plr_clonk, this.plr_clonk, this.plr, true);
	return ScheduleNext(108);
}

public func Intro_3()
{
	MessageBox("$MsgVillageHeadOrganize$", this.plr_clonk, this.head, this.plr, true);
	return ScheduleNext(172);
}

public func Intro_4()
{
	MessageBox("$MsgClonkPoorGuy$", this.plr_clonk, this.plr_clonk, this.plr, true);
	return ScheduleNext(140);
}

public func Intro_5()
{
	MessageBox("$MsgHomelessRobbers$", this.plr_clonk, this.homeless, this.plr, true);
	return ScheduleNext(140);
}

public func Intro_6()
{
	MessageBox("$MsgClonkWhere$", this.plr_clonk, this.plr_clonk, this.plr, true);
	return ScheduleNext(140);
}

public func Intro_7()
{
	MessageBox("$MsgHomelessLocation$", this.plr_clonk, this.homeless, this.plr, true);
	return ScheduleNext(140);
}

public func Intro_8()
{
	MessageBox(Format("$MsgVillageHeadTeachThem$", this.plr_clonk->GetName()), this.plr_clonk, this.head, this.plr, true);
	return ScheduleNext(140);
}

public func Intro_9()
{
	MessageBox("$MsgHomelessHappy$", this.plr_clonk, this.homeless, this.plr, true);
	return ScheduleNext(80);
}

public func Intro_10()
{
	return Stop();
}

public func Intro_Stop()
{
	GameCall("OnIntroSequenceFinished", this.plr);
	this.plr_clonk.ActMap.Walk.Speed = Clonk.ActMap.Walk.Speed;
	this.head.ActMap.Walk.Speed = Clonk.ActMap.Walk.Speed;
	SetPlayerZoomByViewRange(this.for_plr, 400, nil, PLRZOOM_Direct | PLRZOOM_Set);
	return true;
}