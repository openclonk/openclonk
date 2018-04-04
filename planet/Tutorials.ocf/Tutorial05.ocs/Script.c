/**
	Tutorial 05: Robber's Cave
	Author: Maikel
	
	Fight your way through a cave with robbers on the way to a fiendly village.
*/

static guide; // guide object.

protected func Initialize()
{
	// Tutorial goal.
	var goal = CreateObject(Goal_Tutorial);
	goal.Name = "$MsgGoalName$";
	goal.Description = "$MsgGoalDescription$";
	
	// No power need for the elevator.
	CreateObject(Rule_NoPowerNeed);
	
	// Place objects in different sections.
	InitBeforeCave();
	InitCave();
	InitAfterCave();
	InitVegetation();
	InitAnimals();
	InitAI();
	
	// Dialogue options -> repeat round.
	SetNextScenario("Tutorials.ocf\\Tutorial05.ocs", "$MsgRepeatRound$", "$MsgRepeatRoundDesc$");
	return;
}

// Gamecall from goals, set next mission.
protected func OnGoalsFulfilled()
{
	// Achievement: Tutorial completed.
	GainScenarioAchievement("TutorialCompleted", 3);	
	// Dialogue options -> next round.
	SetNextScenario("Tutorials.ocf\\Tutorial06.ocs", "$MsgNextTutorial$", "$MsgNextTutorialDesc$");
	// Normal scenario ending by goal library.
	return false;
}

private func InitBeforeCave()
{
	// Wooden cabin ruin for the old man.
	CreateObjectAbove(Ruin_WoodenCabin, 252, 384)->MakeInvincible();
	// A small forest at the start.
	Tree_Deciduous->Place(7, Rectangle(0, 260, 100, 160));
	Tree_Coniferous2->Place(2, Rectangle(0, 260, 100, 160));
	Tree_Coniferous2->Place(2, Rectangle(0, 260, 100, 160));
	// A grain field near the wooden cabin.
	Wheat->Place(24, Rectangle(100, 260, 100, 160));
	CreateObjectAbove(StrawMan, 140, 384);
	CreateObjectAbove(StrawMan, 170, 384);
	// Create door to block cave entrance until destroyed straw men.
	var door = CreateObject(StoneDoor, 396, 362);
	door->MakeInvincible();
	door.Visibility = VIS_None;
	return;
}

private func InitCave()
{
	// A chest with javelins to kill the first robber.
	var chest = CreateObjectAbove(Chest, 544, 480);
	chest->CreateContents(Javelin, 2);
	// Some torches to see the enemies.
	CreateObjectAbove(Torch, 500, 474)->AttachToWall(true);
	CreateObjectAbove(Torch, 506, 628)->AttachToWall(true);
	CreateObjectAbove(Torch, 728, 696)->AttachToWall(true);
	// A hidden chest with a blunderbuss.
	var chest = CreateObjectAbove(Chest, 10, 526);
	var blunderbuss = chest->CreateContents(Blunderbuss);
	blunderbuss->CreateContents(LeadBullet);
	return;
}

private func InitAfterCave()
{
	// A small forest at the start.
	Tree_Deciduous->Place(10, Rectangle(640, 200, 140, 160));
	Tree_Coniferous2->Place(3, Rectangle(640, 200, 140, 160));
	Tree_Coniferous3->Place(5, Rectangle(640, 200, 140, 160));
	// Elevator with a shaft down and the case already down.
	var elevator = CreateObjectAbove(Elevator, 836, 264);
	elevator->SetDir(DIR_Right);
	elevator->CreateShaft(440);
	var case = elevator->GetCase();
	case->SetPosition(case->GetX(), 640);
	elevator->MakeInvincible();
	case->MakeInvincible();
	// Create a column at map end.
	CreateObjectAbove(Column, 960, 264);
	return;
}

private func InitVegetation()
{
	Grass->Place(85);
	PlaceObjects(Loam, 10 + Random(5), "Earth");
	PlaceObjects(Rock, 25 + Random(5), "Earth");
	Branch->Place(28);
	Mushroom->Place(24);
	Flower->Place(10);
	Fern->Place(10);
	Seaweed->Place(8);
	Coral->Place(4);
	return;
}

private func InitAnimals()
{
	// Some butterflies as atmosphere.
	Butterfly->Place(25);
	// Fish in the body of water.
	Fish->Place(12);
	return;
}

// Initializes the AI: which is all defined in System.ocg
private func InitAI()
{
	// An old man without home npc to point to robbers.
	var npc_homeless = CreateObjectAbove(Clonk, 220, 384);
	npc_homeless->SetName("Dirk");
	npc_homeless->SetObjectLayer(npc_homeless);
	npc_homeless->SetDir(DIR_Left);
	npc_homeless->SetDialogue("Homeless");
	npc_homeless->SetAlternativeSkin("Beggar");

	// Village head.
	var npc_head = CreateObjectAbove(Clonk, 20, 384);
	npc_head->SetName("Archibald");
	npc_head->SetObjectLayer(npc_head);
	npc_head->SetDir(DIR_Left);
	npc_head->SetDialogue("VillageHead");
	npc_head->SetAlternativeSkin("Sage");
	
	// Robbers.
	var robber1 = CreateObjectAbove(Clonk, 490, 524);
	robber1->CreateContents(Bow)->CreateContents(Arrow);
	robber1->CreateContents(Arrow);
	robber1->CreateContents(Shield);
	AI->AddAI(robber1);
	AI->SetHome(robber1, 490, 514, DIR_Left);
	AI->SetGuardRange(robber1, 300, 400, 400, 200);
	AI->SetAllyAlertRange(robber1, 60);
	robber1->SetDir(DIR_Left);
	robber1->SetAlternativeSkin("Youngster");
	robber1.first_robber = true;

	var robber2 = CreateObjectAbove(Clonk, 470, 644);
	robber2->CreateContents(Sword);
	robber2->CreateContents(Shield);
	robber2->CreateContents(Bread);
	AI->AddAI(robber2);
	AI->SetHome(robber2, 470, 634, DIR_Right);
	AI->SetGuardRange(robber2, 300, 560, 400, 200);
	AI->SetAllyAlertRange(robber2, 60);
	robber2->SetDir(DIR_Right);
	robber2->SetAlternativeSkin("YoungsterBlond");
	robber2.second_robber = true;

	var robber3 = CreateObjectAbove(Clonk, 800, 706);
	robber3->CreateContents(Javelin, 3);
	robber3->CreateContents(Shield);
	AI->AddAI(robber3);
	AI->SetHome(robber3, 800, 696, DIR_Left);
	AI->SetGuardRange(robber3, 600, 600, 400, 200);
	AI->SetAllyAlertRange(robber3, 60);
	robber3->SetDir(DIR_Left);
	robber3.last_robber = true;
	
	var robber4 = CreateObjectAbove(Clonk, 830, 706);
	robber4->CreateContents(Sword);
	robber4->CreateContents(Shield);
	AI->AddAI(robber4);
	AI->SetHome(robber4, 830, 696, DIR_Left);
	AI->SetGuardRange(robber4, 600, 600, 400, 200);
	AI->SetAllyAlertRange(robber4, 60);
	robber4->SetDir(DIR_Left);
	robber4.last_robber = true;
	return;
}


/*-- Player Handling --*/

protected func InitializePlayer(int plr)
{
	// Position player's clonk.
	var clonk = GetCrew(plr);
	clonk->SetPosition(10, 374);
	clonk->CreateContents(Shovel);
	var effect = AddEffect("ClonkRestore", clonk, 100, 10);
	effect.to_x = 300;
	effect.to_y = 374;

	// Standard player zoom for tutorials.
	SetPlayerViewLock(plr, true);
	SetPlayerZoomByViewRange(plr, 400, nil, PLRZOOM_Direct | PLRZOOM_Set | PLRZOOM_LimitMax);
	
	// Start the intro sequence.
	StartSequence("Intro", 0, plr);
	
	// Create tutorial guide and control effect.
	guide = CreateObject(TutorialGuide, 0, 0, plr);
	guide->HideGuide();
	var effect = AddEffect("TutorialTalkedToVillageHead", nil, 100, 5);
	effect.plr = plr;
	return;
}


/*-- Guide Messages --*/

public func OnIntroSequenceFinished()
{
	guide->AddGuideMessage("$MsgTutorialTalkToHead$");
	guide->ShowGuide();
	guide->ShowGuideMessage();
	return;
}

public func OnHasTalkedToVillageHead()
{
	RemoveEffect("TutorialTalkedToVillageHead");
	return;
}

global func FxTutorialTalkedToVillageHeadTimer()
{
	return FX_OK;
}

global func FxTutorialTalkedToVillageHeadStop(object target, proplist effect, int reason, bool temp)
{
	if (temp)
		return FX_OK;
	var use = GetPlayerControlAssignment(effect.plr, CON_Use, true, true);
	guide->AddGuideMessage(Format("$MsgTutorialNotHelpful$", use));
	guide->ShowGuideMessage();
	var new_effect = AddEffect("TutorialDestroyedStrawMen", nil, 100, 5);
	new_effect.plr = effect.plr;
	return FX_OK;
}

global func FxTutorialDestroyedStrawMenTimer(object target, proplist effect)
{
	if (!FindObject(Find_ID(StrawMan)))
	{
		// Remove door blocking cave entrance.
		RemoveAll(Find_ID(StoneDoor));
		guide->AddGuideMessage("$MsgTutorialFirstEnemy$");
		guide->ShowGuideMessage();
		var new_effect = AddEffect("TutorialKilledFirstRobber", nil, 100, 5);
		new_effect.plr = effect.plr;
		return FX_Execute_Kill;
	}
	return FX_OK;
}

public func OnClonkRestore()
{
	if (FindObject(Find_OCF(OCF_CrewMember), Find_Property("first_robber")))
	{
		guide->AddGuideMessage("$MsgTutorialOnRespawn$");
		guide->ShowGuideMessage();
	}
	return;
}

global func FxTutorialKilledFirstRobberTimer(object target, proplist effect)
{
	if (!FindObject(Find_OCF(OCF_CrewMember), Find_Property("first_robber")))
	{
		// Show previous guide message about alternative way to kill the first robber.
		if (guide->GetMessageCount() <= 3)
			guide->AddGuideMessage("$MsgTutorialOnRespawn$");
		guide->AddGuideMessage("$MsgTutorialKilledFirst$");
		guide->ShowGuideMessage();
		var new_effect = AddEffect("TutorialKilledSecondRobber", nil, 100, 5);
		new_effect.plr = effect.plr;
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialKilledSecondRobberTimer(object target, proplist effect)
{
	if (!FindObject(Find_OCF(OCF_CrewMember), Find_Property("second_robber")))
	{
		var use = GetPlayerControlAssignment(effect.plr, CON_Use, true, true);
		guide->AddGuideMessage(Format("$MsgTutorialKilledSecond$", use));
		guide->ShowGuideMessage();
		var new_effect = AddEffect("TutorialKilledLastRobbers", nil, 100, 5);
		new_effect.plr = effect.plr;
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialKilledLastRobbersTimer(object target, proplist effect)
{
	if (!FindObject(Find_OCF(OCF_CrewMember), Find_Property("last_robber")))
	{
		// Start the intro sequence.
		guide->HideGuide();
		StartSequence("Outro", 0, effect.plr);
		return FX_Execute_Kill;
	}
	return FX_OK;
}

public func ShowLastGuideMessage()
{
	guide->AddGuideMessage("$MsgTutorialCompleted$");
	guide->ShowGuideMessage();
	guide->ShowGuide();
	return;
}

protected func OnGuideMessageShown(int plr, int index)
{
	// Show the village head.	
	if (index == 0)
	{
		TutArrowShowTarget(Dialogue->FindByName("VillageHead")->GetDialogueTarget());
	}
	// Show the strawmen.
	if (index == 1)
	{
		for (var strawman in FindObjects(Find_ID(StrawMan)))
			TutArrowShowTarget(strawman);
	}
	// Show the first robber.
	if (index == 2)
		TutArrowShowTarget(FindObject(Find_OCF(OCF_CrewMember), Find_Property("first_robber")));
	// Show path to dig and chest.
	if (index == 3)
	{
		TutArrowShowTarget(FindObject(Find_ID(Chest), Sort_Distance(540, 480)));
		TutArrowShowPos(490, 420, 135);
	}
	return;
}

protected func OnGuideMessageRemoved(int plr, int index)
{
	TutArrowClear();
	return;
}


/*-- Clonk restoring --*/

global func FxClonkRestoreTimer(object target, proplist effect, int time)
{
	// Respawn clonk to new location if reached certain position.
	return FX_OK;
}

// Relaunches the clonk, from death or removal.
global func FxClonkRestoreStop(object target, effect, int reason, bool  temporary)
{
	if (reason == 3 || reason == 4)
	{
		var restorer = CreateObject(ObjectRestorer, 0, 0, NO_OWNER);
		var x = BoundBy(target->GetX(), 0, LandscapeWidth());
		var y = BoundBy(target->GetY(), 0, LandscapeHeight());
		restorer->SetPosition(x, y);
		var to_x = effect.to_x;
		var to_y = effect.to_y;
		// Respawn new clonk.
		var plr = target->GetOwner();
		var clonk = CreateObject(Clonk, 0, 0, plr);
		clonk->GrabObjectInfo(target);
		Rule_Relaunch->TransferInventory(target, clonk);
		SetCursor(plr, clonk);
		clonk->DoEnergy(100000);
		restorer->SetRestoreObject(clonk, nil, to_x, to_y, 0, "ClonkRestore");
		GameCall("OnClonkRestore", clonk);
	}
	return FX_OK;
}