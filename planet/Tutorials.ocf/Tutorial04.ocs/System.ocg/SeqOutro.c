// Outro sequence for this tutorial.

#appendto Sequence

public func Outro_Init(int for_plr)
{
	this.plr = for_plr;
	// Store villagers.
	this.village_head = Dialogue->FindByName("VillageHead")->GetDialogueTarget();
	this.farmer = Dialogue->FindByName("Farmer")->GetDialogueTarget();
	this.lookout = Dialogue->FindByName("Lookout")->GetDialogueTarget();
	this.lumberjack = Dialogue->FindByName("Lumberjack")->GetDialogueTarget();
	this.fireman = Dialogue->FindByName("Fireman")->GetDialogueTarget();
	this.builder = Dialogue->FindByName("Builder")->GetDialogueTarget();
	// Create two airplanes with pilots and let them fly from left to right.
	this.airplane1 = CreateObject(Airplane, AbsX(40), AbsY(160));
	this.pilot1 = CreateObject(Clonk);
	this.pilot1->SetSkin(2);
	this.pilot1->Enter(this.airplane1);
	this.pilot1->SetAction("Walk");
	this.pilot1->SetColor(0xff000000);
	this.airplane1->FaceRight();
	this.airplane1->StartInstantFlight(90, 15);
	this.airplane1->SetXDir(12);
	this.airplane1->SetYDir(-1);
	this.airplane1->MakeInvincible();
	this.airplane1->SetColor(0xff000000);
	this.airplane1.BorderBound = 0;
	this.airplane2 = CreateObject(Airplane, AbsX(0), AbsY(190));
	this.pilot2 = CreateObject(Clonk);
	this.pilot2->SetSkin(2);
	this.pilot2->Enter(this.airplane2);
	this.pilot2->SetAction("Walk");
	this.pilot2->SetColor(0xff000000);
	this.airplane2->FaceRight();
	this.airplane2->StartInstantFlight(90, 15);
	this.airplane2->SetXDir(12);
	this.airplane2->SetYDir(-1);
	this.airplane2->MakeInvincible();
	this.airplane2->SetColor(0xff000000);
	this.airplane2.BorderBound = 0;
	// Each plane has a henchman which will kidnap the wipfs.
	this.henchman1 = CreateObject(Clonk);
	this.henchman1->SetAlternativeSkin("Leather");
	this.henchman1->Enter(this.airplane1);
	this.henchman1->SetAction("Walk");
	this.henchman1->SetColor(0xff000000);
	this.henchman1->CreateContents(Musket)->CreateContents(LeadShot);
	this.henchman2 = CreateObject(Clonk);
	this.henchman2->SetAlternativeSkin("Leather");
	this.henchman2->Enter(this.airplane2);
	this.henchman2->SetAction("Walk");
	this.henchman2->SetColor(0xff000000);
	this.henchman2->CreateContents(Musket)->CreateContents(LeadShot);
	
	// Another henchman which will control the lookout.
	this.henchman3 = CreateObject(Clonk);
	this.henchman3->SetAlternativeSkin("Leather");	
	this.henchman3->Enter(this.airplane2);
	this.henchman3->SetAction("Walk");
	this.henchman3->SetColor(0xff000000);
	this.henchman3->CreateContents(Musket)->CreateContents(LeadShot);
	
	// The faction leader which will do the talking.
	this.leader = CreateObject(Clonk);
	this.leader->SetAlternativeSkin("Doctor");
	this.leader->Enter(this.airplane1);
	this.leader->SetAction("Walk");
	this.leader->SetColor(0xff000000);
	this.leader->CreateContents(Musket)->CreateContents(LeadShot);
	
	// There is also a kidnapper on an airship with a lorry to collect the wipfs.
	// The third henchman shoots down the balloons.
	this.airship = CreateObject(Airship, AbsX(20), AbsY(240));
	this.lorry = CreateObject(Lorry, AbsX(30), AbsY(240));
	this.kidnapper = CreateObject(Clonk, AbsX(30), AbsY(240));
	this.kidnapper->SetSkin(3);
	this.kidnapper->SetAction("Walk");
	this.kidnapper->SetCommand("Grab", this.airship);
	this.kidnapper->SetColor(0xff000000);
	this.airship->ControlRight(this.kidnapper);
	return true;
}

public func Outro_JoinPlayer(int plr)
{
	SetPlayerZoomByViewRange(plr, LandscapeWidth(), nil, PLRZOOM_Set | PLRZOOM_LimitMax);
	return;
}

public func Outro_Start()
{
	return ScheduleNext(4);
}

public func Outro_1()
{
	MessageBox("$MsgVillageHeadNoise$", GetCrew(this.plr, 0), this.village_head, this.plr, true);
	return ScheduleNext(72);
}

public func Outro_2()
{
	MessageBox("$MsgFarmerAirplanes$", GetCrew(this.plr, 0), this.farmer, this.plr, true);
	// Fireman and lumberjack flee into wooden cabin.
	this.fireman->SetCommand("Enter", this.fireman->FindObject(Find_ID(WoodenCabin), Find_AnyLayer(), Sort_Distance()));
	this.lumberjack->SetCommand("Enter", this.lumberjack->FindObject(Find_ID(WoodenCabin), Find_AnyLayer(), Sort_Distance()));
	// Builder flees into the mine.
	this.builder->SetCommand("MoveTo", this.builder->FindObject(Find_ID(Foundry), Find_AnyLayer(), Sort_Distance()));
	return ScheduleNext(36);
}

public func Outro_3()
{
	// Exit the leader from the airplane on a balloon.
	this.leader->Exit();
	var balloon = this.leader->CreateContents(Balloon);
	balloon->ControlUseStart(this.leader);
	this.leader->GetActionTarget()->ControlDown(this.leader);
	return ScheduleNext(36);
}

public func Outro_4()
{
	// Exit henchman from the airplanes on balloons.
	this.henchman1->Exit();
	var balloon = this.henchman1->CreateContents(Balloon);
	balloon->ControlUseStart(this.henchman1);
	this.henchman1->GetActionTarget()->ControlDown(this.henchman1);
	this.henchman2->Exit();
	var balloon = this.henchman2->CreateContents(Balloon);
	balloon->ControlUseStart(this.henchman2);
	this.henchman2->GetActionTarget()->ControlDown(this.henchman2);
	// Let henchmen aim at the farmer.
	AddEffect("AimMusketAt", this.henchman1, 100, 1, this, nil, this.farmer, 240);
	AddEffect("AimMusketAt", this.henchman2, 100, 1, this, nil, this.farmer, 240);
	// Let leader aim at the village head.
	AddEffect("AimMusketAt", this.leader, 100, 1, this, nil, this.village_head);
	return ScheduleNext(78);
}

public func Outro_5()
{
	MessageBox("$MsgEvilLeaderItsUs$", GetCrew(this.plr, 0), this.leader, this.plr, true);
	// Exit a third henchman from the airplane on a balloon.
	this.henchman3->Exit();
	var balloon = this.henchman3->CreateContents(Balloon);
	balloon->ControlUseStart(this.henchman3);
	this.henchman3->GetActionTarget()->ControlDown(this.henchman3);
	return ScheduleNext(20);
}

public func Outro_6()
{
	// Third henchman aims at lookout
	AddEffect("AimMusketAt", this.henchman3, 100, 1, this, nil, this.lookout, 280);
	return ScheduleNext(100);
}

public func Outro_7()
{
	return ScheduleNext(36);
}

public func Outro_8()
{

	return ScheduleNext(36);
}

public func Outro_9()
{
	// Third henchman tells lookout to drop the weapon.
	this.henchman3->Message("$MsgHenchman3DropMusket$");
	return ScheduleNext(18);
}

public func Outro_10()
{
	// First and second henchman scare off farmer.
	this.henchman1->Message("$MsgHenchman1RunGirl$");
	this.henchman2->Message("$MsgHenchman2Wipfs$");
	return ScheduleNext(18);
}

public func Outro_11()
{
	// Lookout surrenders.
	this.lookout->Message("$MsgLookoutSurrender$");
	this.lookout->SetCommand("Drop", this.lookout->Contents());
	return ScheduleNext(18);
}

public func Outro_12()
{
	// Farmer runs off into the mines.
	this.farmer->SetCommand("MoveTo", this.farmer->FindObject(Find_ID(ToolsWorkshop), Find_AnyLayer(), Sort_Distance()));
	this.farmer->Message("$MsgFarmerComment$");
	return ScheduleNext(200);
}

public func Outro_13()
{
	this.airship->ControlStop(this.kidnapper);

	for (var wipf in FindObjects(Find_ID(Wipf), Find_AnyLayer()))
	{
		
		wipf->SetObjectLayer();
		var balloon = wipf->CreateContents(Balloon);
		balloon->HangWipfOnBalloon(wipf);
	}
	return ScheduleNext(36);
}

public func Outro_14()
{



	return ScheduleNext(36);
}

public func Outro_15()
{	
	// Show last guide message and then stop the sequence and fulfill the goal.
	GameCall("ShowLastGuideMessage");
	return ScheduleNext(108);
}

public func Outro_16()
{
	return Stop();
}

public func Outro_Stop()
{
	// Fulfill the wormhole destruction goal.
	var goal = FindObject(Find_ID(Goal_Tutorial));
	if (goal)
		goal->Fulfill();
	return true;
}


/*-- Effects --*/

public func FxAimMusketAtStart(object target, proplist effect, int temp, object aim, int length)
{
	if (temp)
		return FX_OK;
	effect.aim_target = aim;
	effect.aim_length = length;
	effect.musket = FindObject(Find_ID(Musket), Find_Container(target));
	effect.musket.loaded = true;
	effect.musket->ControlUseStart(target, effect.aim_target->GetX() - target->GetX(), effect.aim_target->GetY() - target->GetY());
	return FX_OK;
}

public func FxAimMusketAtTimer(object target, proplist effect, int time)
{
	effect.musket->ControlUseHolding(target, effect.aim_target->GetX() - target->GetX(), effect.aim_target->GetY() - target->GetY());
	if (effect.aim_length != nil && time >= effect.aim_length)
		return FX_Execute_Kill;	
	return FX_OK;
}

public func FxAimMusketAtStop(object target, proplist effect, int reason, bool temp)
{
	if (temp)
		return FX_OK;
	effect.musket->ControlUseCancel(target, effect.aim_target->GetX() - target->GetX(), effect.aim_target->GetY() - target->GetY());
	return FX_OK;
}

