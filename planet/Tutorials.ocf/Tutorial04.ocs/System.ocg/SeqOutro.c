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
	this.airplane1->PlaneMount(this.pilot1);
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
	this.airplane2->PlaneMount(this.pilot2);
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
	this.henchman1->CreateContents(Blunderbuss)->CreateContents(LeadBullet);
	this.henchman2 = CreateObject(Clonk);
	this.henchman2->SetAlternativeSkin("Leather");
	this.henchman2->Enter(this.airplane2);
	this.henchman2->SetAction("Walk");
	this.henchman2->SetColor(0xff000000);
	this.henchman2->CreateContents(Blunderbuss)->CreateContents(LeadBullet);
	
	// Another henchman which will control the lookout.
	this.henchman3 = CreateObject(Clonk);
	this.henchman3->SetAlternativeSkin("Leather");	
	this.henchman3->Enter(this.airplane2);
	this.henchman3->SetAction("Walk");
	this.henchman3->SetColor(0xff000000);
	this.henchman3->CreateContents(Blunderbuss)->CreateContents(LeadBullet);
	
	// The faction leader which will do the talking.
	this.leader = CreateObject(Clonk);
	this.leader->SetAlternativeSkin("Doctor");
	this.leader->SetName("Gotham");
	this.leader->Enter(this.airplane1);
	this.leader->SetAction("Walk");
	this.leader->SetColor(0xff000000);
	this.leader->CreateContents(Blunderbuss)->CreateContents(LeadBullet);
	
	// There is also a kidnapper on an airship with a lorry to collect the wipfs.
	// The third henchman shoots down the balloons.
	this.airship = CreateObject(Airship, AbsX(20), AbsY(280));
	this.lorry = CreateObject(Lorry, AbsX(32), AbsY(280));
	this.kidnapper = CreateObject(Clonk, AbsX(30), AbsY(280));
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
	AddEffect("AimBlunderbussAt", this.henchman1, 100, 1, this, nil, this.farmer, 356);
	AddEffect("AimBlunderbussAt", this.henchman2, 100, 1, this, nil, this.farmer, 356);
	// Let leader aim at the village head.
	AddEffect("AimBlunderbussAt", this.leader, 100, 1, this, nil, this.village_head);
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
	AddEffect("AimBlunderbussAt", this.henchman3, 100, 1, this, nil, this.lookout, 280);
	return ScheduleNext(100);
}

public func Outro_7()
{
	return ScheduleNext(36);
}

public func Outro_8()
{
	MessageBox("$MsgVillageHeadWhy$", GetCrew(this.plr, 0), this.village_head, this.plr, true);
	return ScheduleNext(36);
}

public func Outro_9()
{
	// Third henchman tells lookout to drop the weapon.
	this.henchman3->Message("$MsgHenchman3DropBlunderbuss$");
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
	return ScheduleNext(100);
}

public func Outro_13()
{
	MessageBox("$MsgEvilLeaderWipfs$", GetCrew(this.plr, 0), this.leader, this.plr, true);
	this.airship->ControlStop(this.kidnapper);
	AddEffect("TieWipfToBalloon", this.henchman1, 100, 5, this);
	AddEffect("TieWipfToBalloon", this.henchman2, 100, 5, this);
	return ScheduleNext(108);
}

public func Outro_14()
{
	MessageBox("$MsgVillageHeadManiac$", GetCrew(this.plr, 0), this.village_head, this.plr, true);
	ScheduleCall(this, "MessageBox", 108, 0, "$MsgEvilLeader$", GetCrew(this.plr, 0), this.leader, this.plr, true);
	ScheduleCall(this, "MessageBox", 216, 0, "$MsgPlayerDontTakeWipf$", GetCrew(this.plr, 0), GetCrew(this.plr, 0), this.plr, true);
	ScheduleCall(this, "MessageBox", 356, 0, "$MsgEvilLeaderBegging$", GetCrew(this.plr, 0), this.leader, this.plr, true);
	return ScheduleNext(4);
}

public func Outro_15()
{
	// Wait until all wipfs are up then move airship.
	if (!FindObject(Find_ID(Wipf), Find_InRect(AbsX(632), AbsY(280), 216, 112), Find_AnyLayer()))
	{
		AddEffect("MoveAirshipToWipf", this.kidnapper, 100, 5, this);
		return ScheduleNext(10);
	}
	return ScheduleSame(10);
}

public func Outro_16()
{
	if (FindObject(Find_ID(Wipf), Find_NoContainer(), Find_InRect(AbsX(632), AbsY(0), 216, 352), Find_Property("tied_up")))
		return ScheduleSame(10);
	return ScheduleNext(10);
}

public func Outro_17()
{
	this.airship->ControlRight(this.kidnapper);
	this.airship->ControlUp(this.kidnapper);
	this.kidnapper->Message("$MsgKidnapperGotThem$");
	ScheduleCall(this.kidnapper, "RemoveObject", 300, 0);	
	ScheduleCall(this.airship, "RemoveObject", 300, 0);	
	ScheduleCall(this.lorry, "RemoveObject", 300, 0);	
		
	var boompack = this.henchman3->CreateContents(Boompack);
	boompack->SetFuel(10000);
	boompack->ControlUse(this.henchman3, -8, -40);
	boompack->SetDirectionDeviation();
	this.henchman3->Message("$MsgHenchman3SeeYa$");
	RemoveAll(Find_Container(this.henchman3));
	ScheduleCall(this.henchman3, "RemoveObject", 120, 0);
	ScheduleCall(boompack, "RemoveObject", 120, 0);
	
	this.henchman1->SetCommand("MoveTo", nil, 632, 382);
	this.henchman2->SetCommand("MoveTo", nil, 632, 382);
	return ScheduleNext(70);
}

public func Outro_18()
{
	var boompack = this.henchman2->CreateContents(Boompack);
	boompack->SetFuel(10000);
	boompack->ControlUse(this.henchman2, -10, -40);
	boompack->SetDirectionDeviation();
	this.henchman2->Message("$MsgHenchman3SeeYa$");
	RemoveAll(Find_Container(this.henchman2));
	ScheduleCall(this.henchman2, "RemoveObject", 100, 0);
	ScheduleCall(boompack, "RemoveObject", 100, 0);	
	return ScheduleNext(10);
}

public func Outro_19()
{
	MessageBox("$MsgEvilLeaderBye$", GetCrew(this.plr, 0), this.leader, this.plr, true);
		
	var boompack = this.henchman1->CreateContents(Boompack);
	boompack->SetFuel(10000);
	boompack->ControlUse(this.henchman1, -10, -40);
	boompack->SetDirectionDeviation();
	this.henchman1->Message("$MsgHenchman3SeeYa$");
	RemoveAll(Find_Container(this.henchman1));
	ScheduleCall(this.henchman1, "RemoveObject", 120, 0);
	ScheduleCall(boompack, "RemoveObject", 120, 0);
	
	RemoveEffect("AimBlunderbussAt", this.leader);
	return ScheduleNext(36);
}

public func Outro_20()
{
	var boompack = this.leader->CreateContents(Boompack);
	boompack->SetFuel(10000);
	boompack->ControlUse(this.leader, -8, -40);
	boompack->SetDirectionDeviation();
	RemoveAll(Find_Container(this.leader));
	ScheduleCall(this.leader, "RemoveObject", 120, 0);
	ScheduleCall(boompack, "RemoveObject", 120, 0);
	return ScheduleNext(188);
}

public func Outro_21()
{	
	// Show last guide message and then stop the sequence and fulfill the goal.
	GameCall("ShowLastGuideMessage");
	return ScheduleNext(108);
}

public func Outro_22()
{
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


/*-- Effects --*/

public func FxAimBlunderbussAtStart(object target, proplist effect, int temp, object aim, int length)
{
	if (temp)
		return FX_OK;
	effect.aim_target = aim;
	effect.aim_length = length;
	effect.blunderbuss = FindObject(Find_ID(Blunderbuss), Find_Container(target));
	effect.blunderbuss.loaded = true;
	effect.blunderbuss->ControlUseStart(target, effect.aim_target->GetX() - target->GetX(), effect.aim_target->GetY() - target->GetY());
	return FX_OK;
}

public func FxAimBlunderbussAtTimer(object target, proplist effect, int time)
{
	effect.blunderbuss->ControlUseHolding(target, effect.aim_target->GetX() - target->GetX(), effect.aim_target->GetY() - target->GetY());
	if (effect.aim_length != nil && time >= effect.aim_length)
		return FX_Execute_Kill;	
	return FX_OK;
}

public func FxAimBlunderbussAtStop(object target, proplist effect, int reason, bool temp)
{
	if (temp)
		return FX_OK;
	effect.blunderbuss->ControlUseCancel(target, effect.aim_target->GetX() - target->GetX(), effect.aim_target->GetY() - target->GetY());
	return FX_OK;
}


public func FxTieWipfToBalloonStart(object target, proplist effect, int temp)
{
	if (temp)
		return FX_OK;
	effect.wait_time = 0;
	return FX_OK;
}

public func FxTieWipfToBalloonTimer(object target, proplist effect, int time)
{
	if (effect.wait_time > 0)
	{
		effect.wait_time -= effect.Interval;
		return FX_OK;
	}
	
	if (!effect.wipf)
	{
		effect.wipf = target->FindObject(Find_ID(Wipf), Find_InRect(target->AbsX(632), target->AbsY(260), 216, 132), Find_AnyLayer(), Find_Not(Find_Or(Find_Property("tied_up"), Find_Property("tie_target"))), Sort_Distance());
		if (!effect.wipf)
			return FX_Execute_Kill;
		effect.wipf.tie_target = true;
	}

	if (ObjectDistance(target, effect.wipf) < 10)
	{
		effect.wipf->SetObjectLayer();
		effect.wipf.Collectible = true;
		var balloon = effect.wipf->CreateContents(Balloon);
		balloon->HangWipfOnBalloon(effect.wipf);
		target->Message(Translate(Format("MsgHenchmanGotWipf%d", 1 + Random(3))));
		effect.wipf.tied_up = true;
		effect.wipf = nil;
		effect.wait_time = 36;
		return FX_OK;
	}
	
	target->SetCommand("MoveTo", nil, effect.wipf->GetX(), effect.wipf->GetY());
	return FX_OK;
}

public func FxTieWipfToBalloonStop(object target, proplist effect, int reason, bool temp)
{
	if (temp)
		return FX_OK;
	return FX_OK;
}


public func FxMoveAirshipToWipfStart(object target, proplist effect, int temp)
{
	if (temp)
		return FX_OK;

	return FX_OK;
}

public func FxMoveAirshipToWipfTimer(object target, proplist effect, int time)
{
	var right_wipf = FindObject(Find_ID(Wipf), Find_NoContainer(), Find_InRect(AbsX(616), AbsY(0), 248, 280), Find_Property("tied_up"), Sort_Reverse(Sort_Func("GetX")));
	if (!right_wipf)
		return FX_Execute_Kill;
	if (Abs(this.airship->GetX() - right_wipf->GetX()) < 8)
	{
		this.airship->ControlStop(this.kidnapper);
		// Shoot blunderbuss.
		if (!right_wipf.shot)
		{
			var blunderbuss = FindObject(Find_ID(Blunderbuss), Find_Container(this.henchman3));
			blunderbuss.BulletsPerShot = 1;
			blunderbuss.BulletSpread = 0;
			blunderbuss.loaded = true;
			blunderbuss->ControlUseStart(target, right_wipf->GetX() - this.henchman3->GetX(), right_wipf->GetY() - this.henchman3->GetY() - 24);
			blunderbuss->ControlUseHolding(target, right_wipf->GetX() - this.henchman3->GetX(), right_wipf->GetY() - this.henchman3->GetY() - 24);
			blunderbuss->ControlUseStop(target, right_wipf->GetX() - this.henchman3->GetX(), right_wipf->GetY() - this.henchman3->GetY() - 24);
			right_wipf.shot = true;
		}
		return FX_OK;
	}
	if (this.airship->GetX() > right_wipf->GetX())
		this.airship->ControlLeft(this.kidnapper);
	else
		this.airship->ControlRight(this.kidnapper);
	return FX_OK;
}

public func FxMoveAirshipToWipfStop(object target, proplist effect, int reason, bool temp)
{
	if (temp)
		return FX_OK;
	return FX_OK;
}