/**
	Tutorial 08: Waterworks
	Author: Maikel
	
	Rescue the lorry filled with metal from the flooded mine.
	
	Following controls and concepts are explained:
	 * Pump and liquid system
	 * Elevator
	 * Wallkits
*/

static guide; // guide object

protected func Initialize()
{
	// Tutorial goal.
	var goal = CreateObject(Goal_Tutorial);
	goal.Name = "$MsgGoalName$";
	goal.Description = "$MsgGoalDescription$";
	
	// Rules: make the pump faster.
	var rule = CreateObject(Rule_FastPump);
	rule->SetPumpSpeed(4 * Pump.PumpSpeed);
	
	// Place objects in different sections.
	InitVillage();
	InitMines();
	InitVegetation();
	InitAnimals();
	InitAI();
	
	// Dialogue options -> repeat round.
	SetNextScenario("Tutorials.ocf\\Tutorial08.ocs", "$MsgRepeatRound$", "$MsgRepeatRoundDesc$");
	return;
}

// Gamecall from goals, set next mission.
protected func OnGoalsFulfilled()
{
	// Achievement: Tutorial completed.
	GainScenarioAchievement("TutorialCompleted", 3);
	// Dialogue options -> next round.
	//SetNextScenario("Tutorials.ocf\\Tutorial09.ocs", "$MsgNextTutorial$", "$MsgNextTutorialDesc$");
	// Normal scenario ending by goal library.
	return false;
}

private func InitVillage()
{
	// A small forest for decoration.
	Grass->Place(85, Rectangle(0, 160, 200, 100));
	Tree_Deciduous->Place(12, Rectangle(0, 160, 200, 100));
	Flower->Place(5, Rectangle(0, 160, 200, 100));
	
	// Windgenerators, a flag, a pump and an elevator.
	CreateObjectAbove(WindGenerator, 156, 192)->MakeInvincible();
	CreateObjectAbove(WindGenerator, 180, 192)->MakeInvincible();
	CreateObjectAbove(Flagpole, 206, 192)->MakeInvincible();
	CreateObjectAbove(Pump, 228, 192)->MakeInvincible();
	var elevator = CreateObjectAbove(Elevator, 274, 192);
	elevator->CreateShaft(15);
	elevator->MakeInvincible();
	elevator->GetCase()->MakeInvincible();
	
	// A chest with two pipes.
	var chest = CreateObjectAbove(Chest, 206, 192);
	chest->CreateContents(Pipe, 2);
	chest.Plane = chest.Plane + 1;
	chest->MakeInvincible();
	return;
}

private func InitMines()
{
	// First platform with elevator, workshop and chemical lab for explosives.
	var elevator = CreateObjectAbove(Elevator, 538, 816);
	elevator->CreateShaft(159);
	elevator->MakeInvincible();
	elevator->GetCase()->MakeInvincible();
	var chemlab  = CreateObjectAbove(ChemicalLab, 596, 816);
	chemlab->CreateContents(Dynamite, 8);
	chemlab->CreateContents(DynamiteBox, 4);
	chemlab->MakeInvincible();
	var lantern = chemlab->CreateContents(Lantern);
	lantern->TurnOn();
	var workshop = CreateObjectAbove(ToolsWorkshop, 652, 816);
	workshop->CreateContents(Metal, 8);
	workshop->CreateContents(Wood, 6);
	workshop->MakeInvincible();
	
	// Second platform with steam engine and foundry with metal lorry.
	var engine = CreateObjectAbove(SteamEngine, 676, 888);
	engine->CreateContents(Coal, 10);
	engine->MakeInvincible();
	var foundry = CreateObjectAbove(Foundry, 616, 888);
	foundry->CreateContents(Ore, 12);
	foundry->CreateContents(Coal, 12);
	foundry->MakeInvincible();
	var lantern = foundry->CreateContents(Lantern);
	lantern->TurnOn();
	var lorry = CreateObjectAbove(Lorry, 596, 886);
	lorry->CreateContents(Metal, 32);
	lorry.is_metal_lorry = true;
	
	// Another lorry with coal and ore in the lower part of the mines.
	var lorry = CreateObjectAbove(Lorry, 517, 974);
	lorry->CreateContents(Ore, 8);
	lorry->CreateContents(Coal, 16);
	
	// Place some scattered coal and ore in the bottom mine shaft.
	for (var cnt = 0; cnt < 6; cnt++)
	{
		CreateObjectAbove(Ore, RandomX(66, 92), 972);
		CreateObjectAbove(Coal, RandomX(120, 198), 972);
		CreateObjectAbove(Ore, RandomX(338, 474), 972);	
	}
	return;
}

private func InitVegetation()
{
	// Vegetation throughout the scenario.
	PlaceObjects(Firestone, 25 + Random(5), "Earth");
	PlaceObjects(Loam, 15 + Random(5), "Earth");
	Mushroom->Place(10);
	Branch->Place(14);
	Trunk->Place(4);
	// Some vegetation in the lake and flooded mine shaft.
	Seaweed->Place(20);
	Coral->Place(24);
	return;
}

private func InitAnimals()
{
	// Some fish in the lake and flooded mine shaft.
	Fish->Place(25);
	// Some small animals in the forest.
	Butterfly->Place(12, Rectangle(0, 160, 200, 100));
	Zaphive->Place(2, Rectangle(0, 160, 200, 100));
	return;
}

// Initializes the AI: which is all defined in System.ocg
private func InitAI()
{
	// A chief miner npc for requesting help.
	var npc_chief = CreateObjectAbove(Clonk, 218, 192);
	npc_chief->SetColor(0x999900);
	npc_chief->SetName("Bill");
	npc_chief->SetObjectLayer(npc_chief);
	npc_chief->SetDir(DIR_Left);
	npc_chief->SetDialogue("Chief", true);
	
	// A miner npc for random conversation.
	var npc_miner = CreateObjectAbove(Clonk, 300, 974);
	npc_miner->SetColor(0x990099);
	npc_miner->SetName("Demi");
	npc_miner->SetObjectLayer(npc_miner);
	npc_miner->SetSkin(1);
	npc_miner->SetDir(DIR_Right);
	npc_miner->SetDialogue("Miner");
	
	// An explosive expert for hints on dynamite.
	var npc_expert = CreateObjectAbove(Clonk, 604, 816);
	npc_expert->SetColor(0x990000);
	npc_expert->SetName("Deto");
	npc_expert->SetObjectLayer(npc_expert);
	npc_expert->SetSkin(2);
	npc_expert->SetDir(DIR_Left);
	npc_expert->SetDialogue("Expert");
	return;
}


/*-- Player Handling --*/

protected func InitializePlayer(int plr)
{
	// Position player's clonk.
	var clonk = GetCrew(plr, 0);
	clonk->SetPosition(40, 182);
	clonk->SetDir(DIR_Right);
	var effect = AddEffect("ClonkRestore", clonk, 100, 10);
	effect.to_x = 40;
	effect.to_y = 182;
	
	// Items for the clonk.
	clonk->CreateContents(Shovel);

	// Take ownership of the flags.
	for (var flag in FindObjects(Find_Or(Find_Func("IsFlagpole"), Find_ID(WindGenerator), Find_ID(SteamEngine))))
		flag->SetOwner(plr);
	
	// Knowledge to construct bow and arrow.
	SetPlrKnowledge(plr, Pipe);
	SetPlrKnowledge(plr, WallKit);
	
	// Standard player zoom for tutorials, player is not allowed to zoom in/out.
	SetPlayerViewLock(plr, true);
	SetPlayerZoomByViewRange(plr, 400, nil, PLRZOOM_Direct | PLRZOOM_LimitMax);
	
	// Create tutorial guide, add messages, show first.
	guide = CreateObject(TutorialGuide, 0, 0, plr);
	guide->AddGuideMessage("$MsgTutorialFloodedMines$");
	guide->ShowGuideMessage();
	var effect = AddEffect("TutorialTalkedToMineChief", nil, 100, 5);
	effect.plr = plr;
	return;
}


/*-- Intro, Tutorial Goal & Outro --*/

public func OnGoalCompleted(int plr)
{
	var outro = AddEffect("GoalOutro", nil, 100, 5);
	outro.plr = plr;
	return;
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
// Finds when the Clonk has done 'X', and changes the message.

public func OnHasTalkedMineChief()
{
	RemoveEffect("TutorialTalkedToMineChief");
	return;
}

global func FxTutorialTalkedToMineChiefTimer()
{
	return FX_OK;
}

global func FxTutorialTalkedToMineChiefStop(object target, proplist effect, int reason, bool temp)
{
	if (temp)
		return FX_OK;
	guide->AddGuideMessage("$MsgTutorialConnectPipe$");
	guide->ShowGuideMessage();
	var new_effect = AddEffect("TutorialConnectedSourcePipe", nil, 100, 5);
	new_effect.plr = effect.plr;
	return FX_OK;
}

global func FxTutorialConnectedSourcePipeTimer(object target, proplist effect)
{
	var connected_source = false;
	var pump = FindObject(Find_ID(Pump));
	for (var pipe in FindObjects(Find_ID(Pipe)))
		for (var line in FindObjects(Find_Func("IsConnectedTo", pump)))
			if (line->IsConnectedTo(pipe))
				connected_source = true;
	
	if (connected_source)
	{
		guide->AddGuideMessage("$MsgTutorialFindMetalLorry$");
		guide->ShowGuideMessage();
		var new_effect = AddEffect("TutorialFoundMetalLorry", nil, 100, 5);
		new_effect.plr = effect.plr;
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialFoundMetalLorryTimer(object target, proplist effect)
{
	if (FindObject(Find_OCF(OCF_CrewMember), Find_Owner(effect.plr), Find_AtRect(586, 866, 40, 24)))
	{
		guide->AddGuideMessage("$MsgTutorialMoveLorry$");
		guide->ShowGuideMessage();
		var new_effect = AddEffect("TutorialShovedMetalLorry", nil, 100, 5);
		new_effect.plr = effect.plr;
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialShovedMetalLorryTimer(object target, proplist effect)
{
	if (FindObject(Find_ID(Lorry), Find_AtRect(360, 840, 20, 20)))
	{
		var interact = GetPlayerControlAssignment(effect.plr, CON_Interact, true, true);
		guide->AddGuideMessage(Format("$MsgTutorialCallElevator$", interact));
		guide->ShowGuideMessage();
		var new_effect = AddEffect("TutorialTalkedToExplosiveExpert", nil, 100, 5);
		new_effect.plr = effect.plr;
		return FX_Execute_Kill;
	}
	return FX_OK;
}

public func OnHasTalkedExplosiveExpert()
{
	var effect = GetEffect("TutorialTalkedToExplosiveExpert");
	if (effect)
		effect.has_talked = true;
	return;
}

global func FxTutorialTalkedToExplosiveExpertTimer(object target, proplist effect)
{
	if (effect.has_talked && FindObject(Find_ID(ElevatorCase), Find_AtRect(282, 816, 20, 48)))
	{
		guide->AddGuideMessage("$MsgTutorialProduceWallKit$");
		guide->ShowGuideMessage();
		var new_effect = AddEffect("TutorialProducedWallKit", nil, 100, 5);
		new_effect.plr = effect.plr;
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialProducedWallKitTimer(object target, proplist effect)
{
	var workshop = FindObject(Find_ID(ToolsWorkshop));
	if (FindObject(Find_ID(WallKit), Find_Container(workshop)))
	{
		guide->AddGuideMessage("$MsgTutorialCloseMineShaft$");
		guide->ShowGuideMessage();
		var new_effect = AddEffect("TutorialHasClosedShaft", nil, 100, 5);
		new_effect.plr = effect.plr;
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialHasClosedShaftTimer(object target, proplist effect)
{
	var pathlength1 = GetPathLength(280, 856, 280, 748);
	var pathlength2 = GetPathLength(320, 856, 320, 748);
	if (pathlength1 == nil && pathlength2 == nil)
	{
		guide->AddGuideMessage("$MsgTutorialWaitAndMoveLorry$");
		guide->ShowGuideMessage();
		var new_effect = AddEffect("TutorialHasPlacedLorryInCase", nil, 100, 5);
		new_effect.plr = effect.plr;
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialHasPlacedLorryInCaseTimer(object target, proplist effect)
{
	var elevator_case = FindObject(Find_ID(ElevatorCase), Find_AtRect(282, 800, 20, 64));
	if (!elevator_case)
		return FX_OK;
	if (elevator_case->FindObject(Find_ID(Lorry), Find_AtPoint()))
	{
		guide->AddGuideMessage("$MsgTutorialBlastWall$");
		guide->ShowGuideMessage();
		var new_effect = AddEffect("TutorialHasBlastedWall", nil, 100, 5);
		new_effect.plr = effect.plr;
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialHasBlastedWallTimer(object target, proplist effect)
{
	var blasted_free = true;
	for (var x = 280; x <= 304; x += 2)
	{
		if (!PathFree(x, 848, x, 732))
		{
			blasted_free = false;
			break;
		}
	}
	if (blasted_free)
	{
		guide->AddGuideMessage("$MsgTutorialSwimUp$");
		guide->ShowGuideMessage();
		return FX_Execute_Kill;
	}
	return FX_OK;
}

protected func OnGuideMessageShown(int plr, int index)
{
	if (index == 0)
	{
		TutArrowShowTarget(FindObject(Find_ID(Clonk), Find_AtPoint(218, 182), Find_Not(Find_Owner(plr))));
	}	
	if (index == 2)
	{
		TutArrowShowPos(302, 270, 180);
		TutArrowShowTarget(FindObject(Find_ID(Lorry), Find_Property("is_metal_lorry")));
	}	
	if (index == 3)
	{
		TutArrowShowPos(556, 830, 0);
		TutArrowShowPos(426, 820, 225);
	}
	if (index == 4)
	{
		TutArrowShowPos(292, 850, 180);
		TutArrowShowTarget(FindObject(Find_ID(Clonk), Find_AtPoint(604, 806), Find_Not(Find_Owner(plr))));
	}
	if (index == 6)
	{
		TutArrowShowPos(282, 786, 0);
		TutArrowShowPos(312, 786, 0);
	}
	if (index == 8)
	{
		TutArrowShowPos(292, 786, 0);
	}
	if (index == 9)
	{
		TutArrowShowTarget(FindObject(Find_ID(Clonk), Find_AtPoint(218, 182), Find_Not(Find_Owner(plr))));
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
	if (FindObject(Find_OCF(OCF_CrewMember), Find_InRect(448, 752, 40, 64)))
	{
		effect.to_x = 468;
		effect.to_y = 806;             
	}
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
	}
	return FX_OK;
}