/**
	Tutorial 07: Airborne Mining
	Author: Maikel
	
	Mine valuable gems from a ruby stalactite.
	
	Following controls and concepts are explained:
	 * Airship controls
	 * Rope ladders
	 * Gem materials
	 * Selling at the flagpole
*/

static guide; // guide object

protected func Initialize()
{
	// Tutorial goal.
	var goal = CreateObject(Goal_Tutorial);
	goal.Name = "$MsgGoalName$";
	goal.Description = "$MsgGoalDescription$";
	CreateObject(Rule_NoPowerNeed);
	
	// Place objects in different sections.
	InitMountain();
	InitRubyIsland();
	InitIslands();
	InitVegetation();
	InitAnimals();
	InitAI();
	
	// Environment.
	Time->Init();
	Time->SetTime(22 * 60 + 30);
	Time->SetCycleSpeed(0);
	
	// Wealth shown at all time
	GUI_Controller->ShowWealth();

	// Dialogue options -> repeat round.
	SetNextScenario("Tutorials.ocf\\Tutorial07.ocs", "$MsgRepeatRound$", "$MsgRepeatRoundDesc$");
	return;
}

// Gamecall from goals, set next mission.
protected func OnGoalsFulfilled()
{
	// Achievement: Tutorial completed.
	GainScenarioAchievement("TutorialCompleted", 3);
	// Dialogue options -> next round.
	SetNextScenario("Tutorials.ocf\\Tutorial08.ocs", "$MsgNextTutorial$", "$MsgNextTutorialDesc$");
	// Normal scenario ending by goal library.
	return false;
}

private func InitMountain()
{
	// Shipyard with lantern and flagpole.
	CreateObjectAbove(Flagpole, 55, 504)->MakeInvincible();
	var shipyard = CreateObjectAbove(Shipyard, 90, 504);
	shipyard->MakeInvincible();
	var lamp = shipyard->CreateContents(Lantern);
	lamp->TurnOn();
	// Indestructible airship.
	var airship = CreateObjectAbove(Airship, 120, 504);
	airship->MakeInvincible();
	// Lorry with dynamite and tools.
	var lorry = CreateObjectAbove(Lorry, 120, 498);
	for (var cnt = 0; cnt < 4; cnt ++)
	{
		var dynamite_box = lorry->CreateContents(DynamiteBox);
		dynamite_box->SetDynamiteCount(3);
	}
	lorry->CreateContents(Pickaxe);
	lorry->CreateContents(TeleGlove);
	lorry->MakeInvincible();
	return;
}

private func InitRubyIsland()
{
	// Two rope ladders give access to the ruby stalactite.
	CreateObjectAbove(Ropeladder, 740, 160)->Unroll(-1, COMD_Up);
	CreateObjectAbove(Ropeladder, 780, 160)->Unroll(-1, COMD_Up);
	return;
}

private func InitIslands()
{
	// Armory on the lower island with weaponry to kill bats.
	CreateObjectAbove(WindGenerator, 700, 648)->MakeInvincible();
	var armory = CreateObjectAbove(Armory, 654, 648);
	armory->CreateContents(Wood, 10);
	armory->CreateContents(Metal, 5);
	armory->MakeInvincible();
	// Flowers on two of the islands.
	Flower->Place(12, Rectangle(200, 100, 300, 200));
	Flower->Place(8, Rectangle(300, 300, 200, 200));
	return;
}

// Vegetation throughout the scenario.
private func InitVegetation()
{
	PlaceGrass(85);
	PlaceObjects(Firestone, 25 + Random(5), "Earth");
	PlaceObjects(Loam, 15 + Random(5), "Earth");
	Mushroom->Place(10);
	Branch->Place(40);
	Trunk->Place(10);
	Tree_Deciduous->Place(40);
	return;
}

private func InitAnimals()
{
	// The sky islands are a natural place for bats to hide. 		
	var bats = Bat->Place(6);
	// Make the bats a bit weaker so that they are killed with a single arrow.
	for (var bat in bats)
	{
		bat.MaxEnergy = 7000;
		bat->DoEnergy(bat.MaxEnergy - bat->GetEnergy());
	}
	// Some fireflies attracted to trees on two islands.
	Firefly->Place(2, nil, Rectangle(200, 100, 300, 200));
	Firefly->Place(2, nil, Rectangle(300, 300, 200, 200));
	return;
}

// Initializes the AI: which is all defined in System.ocg
private func InitAI()
{
	// A pilot npc for explaining the sky islands.
	var npc_pilot = CreateObjectAbove(Clonk, 108, 496);
	npc_pilot->SetColor(0xffff00);
	npc_pilot->SetName("Chris");
	npc_pilot->SetObjectLayer(npc_pilot);
	npc_pilot->SetSkin(2);
	npc_pilot->SetDir(DIR_Right);
	npc_pilot->SetDialogue("Pilot", true);
	return;
}

/*-- Player Handling --*/

protected func InitializePlayer(int plr)
{
	// Position player's clonk.
	var clonk = GetCrew(plr, 0);
	clonk->SetPosition(60, 494);
	var effect = AddEffect("ClonkRestore", clonk, 100, 10);
	effect.to_x = 60;
	effect.to_y = 494;
	
	// Items for the clonk.
	clonk->CreateContents(Shovel);
	var dynamite_box = clonk->CreateContents(DynamiteBox);
	dynamite_box->SetDynamiteCount(3);
	
	// Take ownership of the flags.
	for (var flag in FindObjects(Find_Or(Find_Func("IsFlagpole"), Find_ID(WindGenerator))))
		flag->SetOwner(plr);
	
	// Knowledge to construct bow and arrow.
	SetPlrKnowledge(plr, Bow);
	SetPlrKnowledge(plr, Arrow);
	
	// Add an interaction to call the airship.
	Helper_CallAirship->Create(clonk, Dialogue->FindByName("Pilot")->GetDialogueTarget(), FindObject(Find_ID(Airship)));
	
	// Add an effect to the clonk to track the goal.
	var track_goal = AddEffect("TrackGoal", nil, 100, 2);
	track_goal.plr = plr;

	// Standard player zoom for tutorials, player is not allowed to zoom in/out.
	SetPlayerViewLock(plr, true);
	SetPlayerZoomByViewRange(plr, 400, nil, PLRZOOM_Direct | PLRZOOM_LimitMax);
	
	// Determine player movement keys.
	var interact_prev = GetPlayerControlAssignment(plr, CON_InteractNext_Right, true, true);
	var interact_next = GetPlayerControlAssignment(plr, CON_InteractNext_Left, true, true);
	var interact_cycle = GetPlayerControlAssignment(plr, CON_InteractNext_CycleObject, true, true);
	var interact_cancel = GetPlayerControlAssignment(plr, CON_InteractNext_Stop, true, true);
	
	// Create tutorial guide, add messages, show first.
	guide = CreateObjectAbove(TutorialGuide, 0, 0, plr);
	guide->AddGuideMessage(Format("$MsgTutorialTalkToPilot$", interact_prev, interact_next, interact_cycle, interact_cancel));
	guide->ShowGuideMessage();
	var effect = AddEffect("TutorialTalkedToPilot", nil, 100, 2);
	effect.plr = plr;
	return;
}


/*-- Intro, Tutorial Goal & Outro --*/

global func FxTrackGoalTimer(object target, proplist effect, int time)
{
	if (GetWealth(effect.plr) >= 250)
	{
		var outro = AddEffect("GoalOutro", target, 100, 5);
		outro.plr = effect.plr;
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxGoalOutroStart(object target, proplist effect, int temp)
{
	if (temp)
		return FX_OK;	
	// Show guide message congratulating.
	guide->AddGuideMessage("$MsgTutorialCompleted$");
	guide->ShowGuideMessage();	
	return FX_OK;
}

global func FxGoalOutroTimer(object target, proplist effect, int time)
{
	if (time >= 60)
	{
		var goal = FindObject(Find_ID(Goal_Tutorial));
		goal->Fulfill();
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxGoalOutroStop(object target, proplist effect, int reason, bool temp)
{
	if (temp) 
		return FX_OK;
	return FX_OK;
}


/*-- Guide Messages --*/

public func OnHasTalkedToPilot()
{
	RemoveEffect("TutorialTalkedToPilot");
	return;
}

global func FxTutorialTalkedToPilotTimer()
{
	return FX_OK;
}

global func FxTutorialTalkedToPilotStop(object target, proplist effect, int reason, bool temp)
{
	if (temp)
		return FX_OK;

	// Determine player movement keys.
	var left = GetPlayerControlAssignment(effect.plr, CON_Left, true, true);
	var right = GetPlayerControlAssignment(effect.plr, CON_Right, true, true);
	var up = GetPlayerControlAssignment(effect.plr, CON_Up, true, true);
	var down = GetPlayerControlAssignment(effect.plr, CON_Down, true, true);
	var interact = GetPlayerControlAssignment(effect.plr, CON_Interact, true, true);
	var control_keys = Format("[%s] [%s] [%s] [%s]", up, left, down, right);
	
	guide->AddGuideMessage(Format("$MsgTutorialFindRubies$", interact, control_keys));
	guide->ShowGuideMessage();
	var new_effect = AddEffect("TutorialFoundStalactite", nil, 100, 2);
	new_effect.plr = effect.plr;
	return FX_OK;
}

global func FxTutorialFoundStalactiteTimer(object target, proplist effect, int timer)
{
	var clonk = FindObject(Find_ID(Clonk), Find_Distance(150, 780, 200));
	if (clonk)
	{
		guide->AddGuideMessage(Format("$MsgTutorialParkAirship$"));
		guide->ShowGuideMessage();
		AddEffect("TutorialAirshipParked", nil, 100, 2);
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialAirshipParkedTimer(object target, proplist effect, int timer)
{
	var clonk = FindObject(Find_ID(Clonk), Find_Distance(30, 688, 200));
	var airship = FindObject(Find_ID(Airship), Find_Distance(30, 688, 200));
	if (clonk && airship)
	{
		var plr = clonk->GetOwner();
		var left = GetPlayerControlAssignment(plr, CON_Left, true, true);
		var right = GetPlayerControlAssignment(plr, CON_Right, true, true);
		guide->AddGuideMessage(Format("$MsgTutorialLadderJump$", left, right));
		guide->ShowGuideMessage();
		AddEffect("TutorialOnStalactite", nil, 100, 2);
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialOnStalactiteTimer(object target, proplist effect, int timer)
{
	if (FindObject(Find_ID(Clonk), Find_InRect(810, 150, 24, 72)))
	{
		guide->AddGuideMessage("$MsgTutorialBlastGems$");
		guide->ShowGuideMessage();
		AddEffect("TutorialCollectGems", nil, 100, 2);
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialCollectGemsTimer(object target, proplist effect, int timer)
{
	if (FindObject(Find_ID(Ruby)))
	{
		guide->AddGuideMessage("$MsgTutorialCollectGems$");
		guide->ShowGuideMessage();
		return FX_Execute_Kill;
	}
	return FX_OK;
}

protected func OnGuideMessageShown(int plr, int index)
{
	// Show airship parking space.
	if (index == 2)
		TutArrowShowPos(688, 220, 135);
	// Show dynamite placement location.
	if (index == 4)
		TutArrowShowPos(800, 200, 60);	
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
		var airship = FindObject(Find_ID(Airship));
		if (airship)
		{
			to_x = airship->GetX();
			to_y = airship->GetY();
		}
		// Respawn new clonk.
		var plr = target->GetOwner();
		var clonk = CreateObject(Clonk, 0, 0, plr);
		clonk->GrabObjectInfo(target);
		Rule_Relaunch->TransferInventory(target, clonk);
		SetCursor(plr, clonk);
		clonk->DoEnergy(100000);
		// Add an interaction to call the airship.
		Helper_CallAirship->Create(clonk, Dialogue->FindByName("Pilot")->GetDialogueTarget(), FindObject(Find_ID(Airship)));
		restorer->SetRestoreObject(clonk, nil, to_x, to_y, 0, "ClonkRestore");
	}
	return FX_OK;
}