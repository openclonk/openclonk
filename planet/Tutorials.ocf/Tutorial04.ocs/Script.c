/** 
	Tutorial 04: Mining Tools
	Author: Maikel
	
	Produce loam, transport lorry, produce pickaxe and mine ore.

	Following controls and concepts are explained:
	 * Elevator and lorry interaction.
	 * Production menu: foundry, workshop.
	 * Using items: shovel + bucket, barrel, loam, pickaxe.
*/


static guide; // guide object.

protected func Initialize()
{
	// Tutorial goal.
	var goal = CreateObject(Goal_Tutorial);
	goal.Name = "$MsgGoalName$";
	goal.Description = "$MsgGoalDescription$";
	
	// Place objects in different sections.
	InitVillageEntrance();
	InitVillageMain();
	InitOreMine();
	InitVillageUnderground();
	InitVegetation();
	InitAnimals();
	InitAI();
	
	// Dialogue options -> repeat round.
	SetNextScenario("Tutorials.ocf\\Tutorial04.ocs", "$MsgRepeatRound$", "$MsgRepeatRoundDesc$");
	return;
}

// Gamecall from goals, set next mission.
protected func OnGoalsFulfilled()
{
	// Achievement: Tutorial completed.
	GainScenarioAchievement("TutorialCompleted", 3);	
	// Dialogue options -> next round.
	SetNextScenario("Tutorials.ocf\\Tutorial05.ocs", "$MsgNextTutorial$", "$MsgNextTutorialDesc$");
	// Normal scenario ending by goal library.
	return false;
}

private func InitVillageEntrance()
{
	// Create a signpost.
	CreateObjectAbove(EnvPack_Guidepost, 56, 384);	
	
	// Wind generator and sawmill for wood production.
	CreateObjectAbove(WindGenerator, 224, 384)->MakeInvincible();
	CreateObjectAbove(Sawmill, 264, 386)->MakeInvincible();

	// A wooden cabin ruin on fire.
	CreateObjectAbove(WoodenCabin, 370, 352)->MakeInvincible();
	var chest = CreateObjectAbove(Chest, 410, 352);
	chest->MakeInvincible();
	return;
}

private func InitOreMine()
{
	// A foundry halfway down.
	var foundry = CreateObjectAbove(Foundry, 456, 488);
	foundry->MakeInvincible();
	var lantern = foundry->CreateContents(Lantern);
	lantern->TurnOn();
	foundry->CreateContents(Bucket);
	foundry->CreateContents(Barrel);
	foundry->CreateContents(Coal, 10);
	return;
}

private func InitVillageMain()
{
	// Flagpole for power distribution: constructed in tutorial 3.
	CreateObjectAbove(Flagpole, 496, 376);
		
	// Elevator, and grain production on the right.
	var elevator = CreateObjectAbove(Elevator, 601, 392);
	elevator->CreateShaft(262);
	elevator->MakeInvincible();
	CreateObjectAbove(Windmill, 816, 392)->MakeInvincible();
	CreateObjectAbove(Kitchen, 904, 376)->MakeInvincible();
	CreateObjectAbove(Loom, 560, 392)->MakeInvincible();
	Wheat->Place(40 + Random(10), Rectangle(704, 352, 80, 64));
	Cotton->Place(3, Rectangle(654, 352, 40, 64));
	CreateObjectAbove(StrawMan, 714, 391);
	CreateObjectAbove(StrawMan, 762, 391);
	
	// Stone door to protect the village.
	var door = CreateObjectAbove(StoneDoor, 1004, 376);
	var wheel = CreateObjectAbove(SpinWheel, 972, 376);
	wheel->SetSwitchTarget(door);
	
	// Tools and armory down below.	
	CreateObjectAbove(ToolsWorkshop, 698, 504)->MakeInvincible();
	CreateObjectAbove(Armory, 772, 504)->MakeInvincible();
	CreateObjectAbove(Flagpole, 828, 496)->MakeInvincible();
	return;
}

private func InitVillageUnderground()
{
	// A wooden cabin ruin.
	CreateObjectAbove(WoodenCabin, 562, 568)->MakeInvincible();
	
	// Chemical lab ruin and flagpole on the left of the elevator.
	CreateObjectAbove(ChemicalLab, 484, 664)->MakeInvincible();
	CreateObjectAbove(Flagpole, 528, 664)->MakeInvincible();
	
	// Steam engine for power supply below.
	var engine = CreateObjectAbove(SteamEngine, 780, 752);
	engine->CreateContents(Coal, 5);
	engine->MakeInvincible();
	
	// Lorry with materials down below.
	var lorry = CreateObjectAbove(Lorry, 954, 670);
	lorry->CreateContents(Metal, 16);
	lorry->CreateContents(Wood, 16);
	return;
}

private func InitVegetation()
{
	Grass->Place(85);
	PlaceObjects(Firestone, 10 + Random(5), "Earth");
	PlaceObjects(Rock, 25 + Random(5), "Earth");
	Branch->Place(20, nil, {underground = true});
	Flower->Place(10);
	return;
}

private func InitAnimals()
{
	// The wipf as your friend, controlled by AI.
	var wipf = CreateObjectAbove(Wipf, 732, 392);
	wipf->SetMeshMaterial("WipfSkin");
	wipf->SetObjectLayer(wipf);
	wipf->EnableTutorialControl();
	wipf.Name = "$WipfName$";
	wipf.Description = "$WipfDescription$";
		
	// Some wipfs near the grain field.
	var wipfs = Wipf->Place(4, Rectangle(640, 352, 180, 40));
	for (wipf in wipfs)
	{
		wipf->SetObjectLayer(wipf);
		wipf->EnableTutorialControl();
	}
	
	// Some butterflies as atmosphere.
	Butterfly->Place(25);
	return;
}

// Initializes the AI: which is all defined in System.ocg
private func InitAI()
{
	// A lumberjack npc for explaining how to cut trees.
	var npc_lumberjack = CreateObjectAbove(Clonk, 192, 384);
	npc_lumberjack->SetColor(0xffff00);
	npc_lumberjack->SetName("Sato");
	npc_lumberjack->SetObjectLayer(npc_lumberjack);
	npc_lumberjack->SetSkin(3);
	npc_lumberjack->SetDir(DIR_Right);
	npc_lumberjack->SetDialogue("Lumberjack");

	// A fireman NPC who extinguishes a burning cabin.	
	var npc_fireman = CreateObjectAbove(Clonk, 340, 352);
	npc_fireman->SetName("Hubert");
	npc_fireman->SetObjectLayer(npc_fireman);
	npc_fireman->SetDir(DIR_Right);
	npc_fireman->SetDialogue("Fireman");
	npc_fireman->SetAlternativeSkin("MaleDarkHair");
	
	// A builder which tells you where to place the flagpole.
	var npc_builder = CreateObjectAbove(Clonk, 504, 376);
	npc_builder->SetName("Kummerog");
	npc_builder->CreateContents(Hammer);
	npc_builder->SetObjectLayer(npc_builder);
	npc_builder->SetDir(DIR_Left);
	npc_builder->SetDialogue("Builder");
	npc_builder->SetAlternativeSkin("Carpenter");	
	
	// A farmer near the grain field.
	var npc_farmer = CreateObjectAbove(Clonk, 720, 392);
	npc_farmer->SetColor(0x00ffff);
	npc_farmer->SetName("Genhered");
	npc_farmer->SetObjectLayer(npc_farmer);
	npc_farmer->SetSkin(1);
	npc_farmer->SetDir(DIR_Left);
	npc_farmer->SetDialogue("Farmer");
	
	// Lookout.
	var npc_lookout = CreateObjectAbove(Clonk, 992, 296);
	npc_lookout->SetName("Larry");
	npc_lookout->SetObjectLayer(npc_lookout);
	npc_lookout->SetDir(DIR_Left);
	npc_lookout->SetDialogue("Lookout");
	npc_lookout->SetAlternativeSkin("Guard");

	// Village head.
	var npc_head = CreateObjectAbove(Clonk, 924, 672);
	npc_head->SetName("Archibald");
	npc_head->SetObjectLayer(npc_head);
	npc_head->SetDir(DIR_Left);
	npc_head->SetDialogue("VillageHead", true);
	npc_head->SetAlternativeSkin("Sage");
	return;
}


/*-- Player Handling --*/

protected func InitializePlayer(int plr)
{
	// Position player's clonk.
	var clonk = GetCrew(plr);
	clonk->SetPosition(480, 366);
	clonk->CreateContents(Shovel);
	var effect = AddEffect("ClonkRestore", clonk, 100, 10);
	effect.to_x = 480;
	effect.to_y = 366;
	
	// Claim ownership of structures.
	for (var structure in FindObjects(Find_Or(Find_Category(C4D_Structure), Find_Func("IsFlagpole"))))
		structure->SetOwner(plr);
	
	// Add an effect to the clonk to track the goal.
	var goal_effect = AddEffect("TrackGoal", nil, 100, 2);
	goal_effect.plr = plr;

	// Standard player zoom for tutorials.
	SetPlayerViewLock(plr, true);
	SetPlayerZoomByViewRange(plr, 400, nil, PLRZOOM_Direct | PLRZOOM_Set);
	
	// Knowledge for the pickaxe construction.
	SetPlrKnowledge(plr, Pickaxe);

	// Create tutorial guide, add messages, show first.
	guide = CreateObject(TutorialGuide, 0, 0, plr);
	var interact = GetPlayerControlAssignment(plr, CON_Interact, true, true);
	var up = GetPlayerControlAssignment(plr, CON_Up, true, true);
	var down = GetPlayerControlAssignment(plr, CON_Down, true, true);
	guide->AddGuideMessage(Format("$MsgTutorialVillageHead$", interact, up, down));
	guide->ShowGuideMessage();
	var effect = AddEffect("TutorialTalkedToVillageHead", nil, 100, 5);
	effect.plr = plr;
	return;
}


/*-- Intro, Tutorial Goal & Outro --*/

global func FxTrackGoalTimer(object target, proplist effect, int time)
{
	if (effect.completed)
	{
		AddEffect("GoalOutro", target, 100, 5);
		// Hide the tutorial guide during the outro.
		guide->HideGuide();
		// Start the outro sequence which fulfills the goal.
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


/*-- Guide Messages --*/

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
	guide->AddGuideMessage("$MsgTutorialLoamProduction$");
	guide->ShowGuideMessage();
	var new_effect = AddEffect("TutorialGotBucket", nil, 100, 5);
	new_effect.plr = effect.plr;
	return FX_OK;
}

global func FxTutorialGotBucketTimer(object target, proplist effect)
{
	var clonk = FindObject(Find_OCF(OCF_CrewMember), Find_Owner(effect.plr));
	if (clonk && FindObject(Find_ID(Bucket), Find_Container(clonk)))
	{
		guide->AddGuideMessage("$MsgTutorialFillBucket$");
		guide->ShowGuideMessage();
		var new_effect = AddEffect("TutorialFilledBucket", nil, 100, 5);
		new_effect.plr = effect.plr;
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialFilledBucketTimer(object target, proplist effect)
{
	var clonk = FindObject(Find_OCF(OCF_CrewMember), Find_Owner(effect.plr));
	if (!clonk)
		return FX_OK;
	var bucket = FindObject(Find_ID(Bucket), Find_Container(clonk));
	if (bucket && bucket->Contents() && bucket->Contents()->~GetStackCount() >= 3)
	{
		guide->AddGuideMessage("$MsgTutorialGetBarrel$");
		guide->ShowGuideMessage();
		var new_effect = AddEffect("TutorialGotBarrel", nil, 100, 5);
		new_effect.plr = effect.plr;
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialGotBarrelTimer(object target, proplist effect)
{
	var clonk = FindObject(Find_OCF(OCF_CrewMember), Find_Owner(effect.plr));
	if (clonk && FindObject(Find_ID(Barrel), Find_Container(clonk)))
	{
		guide->AddGuideMessage("$MsgTutorialFillBarrel$");
		guide->ShowGuideMessage();
		var new_effect = AddEffect("TutorialFilledBarrel", nil, 100, 5);
		new_effect.plr = effect.plr;
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialFilledBarrelTimer(object target, proplist effect)
{
	var foundry = FindObject(Find_ID(Foundry));
	if (!foundry)
		return FX_OK;
	if (foundry->GetLiquidAmount("Water") >= 100)
	{
		var contents = GetPlayerControlAssignment(effect.plr, CON_Contents, true, true);
		guide->AddGuideMessage(Format("$MsgTutorialProduceLoam$", contents));
		guide->ShowGuideMessage();
		var new_effect = AddEffect("TutorialGotLoam", nil, 100, 5);
		new_effect.plr = effect.plr;
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialGotLoamTimer(object target, proplist effect)
{
	var clonk = FindObject(Find_OCF(OCF_CrewMember), Find_Owner(effect.plr), Find_InRect(920, 642, 40, 30));
	if (!clonk)
		return FX_OK;
	if (FindObject(Find_ID(Loam), Find_Container(clonk)))
	{
		var use = GetPlayerControlAssignment(effect.plr, CON_Use, true, true);
		var interact = GetPlayerControlAssignment(effect.plr, CON_Interact, true, true);
		guide->AddGuideMessage(Format("$MsgTutorialMakeLoamBridge$", use, interact));
		guide->ShowGuideMessage();
		var new_effect = AddEffect("TutorialMovedLorryToWorkshop", nil, 100, 5);
		new_effect.plr = effect.plr;
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialMovedLorryToWorkshopTimer(object target, proplist effect)
{
	if (FindObject(Find_ID(Lorry), Find_InRect(674, 474, 44, 30)))
	{
		var contents = GetPlayerControlAssignment(effect.plr, CON_Contents, true, true);
		guide->AddGuideMessage(Format("$MsgTutorialProducePickaxe$", contents));
		guide->ShowGuideMessage();
		var new_effect = AddEffect("TutorialProducedPickaxe", nil, 100, 5);
		new_effect.plr = effect.plr;
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialProducedPickaxeTimer(object target, proplist effect)
{
	var workshop = FindObject(Find_ID(ToolsWorkshop));
	if (!workshop)
		return FX_OK;
	if (FindObject(Find_ID(Pickaxe), Find_Container(workshop)))
	{
		var use = GetPlayerControlAssignment(effect.plr, CON_Use, true, true);
		guide->AddGuideMessage(Format("$MsgTutorialMineOre$", use));
		guide->ShowGuideMessage();
		var new_effect = AddEffect("TutorialMinedOre", nil, 100, 5);
		new_effect.plr = effect.plr;
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialMinedOreTimer(object target, proplist effect)
{
	var workshop = FindObject(Find_ID(ToolsWorkshop));
	if (!workshop)
		return FX_OK;
	if (ObjectCount(Find_ID(Ore), Find_InRect(260, 600, 128, 68)) >= 3)
	{
		guide->AddGuideMessage("$MsgTutorialPutOre$");
		guide->ShowGuideMessage();
		var new_effect = AddEffect("TutorialTalkedToVillageHeadFinished", nil, 100, 5);
		new_effect.plr = effect.plr;
		// Notify village head ore has been mined.
		var dialogue_village_head = Dialogue->FindByName("VillageHead");
		if (dialogue_village_head)
			dialogue_village_head->SetDialogueProgress(9, nil, true);
		// Move village head up.
		if (dialogue_village_head)
		{
			var head = dialogue_village_head->GetDialogueTarget();
			var head_ctrl_effect = GetEffect("IntVillageHead", head);
			if (head_ctrl_effect)
				head_ctrl_effect.move_up = true;
		}
		return FX_Execute_Kill;
	}
	return FX_OK;
}

public func OnHasTalkedToVillageHeadFinished()
{
	RemoveEffect("TutorialTalkedToVillageHeadFinished");
	return;
}

global func FxTutorialTalkedToVillageHeadFinishedTimer()
{
	return FX_OK;
}

global func FxTutorialTalkedToVillageHeadFinishedStop(object target, proplist effect, int reason, bool temp)
{
	if (temp)
		return FX_OK;
	var goal_effect = GetEffect("TrackGoal");
	goal_effect.completed = true;
	return FX_OK;
}

protected func OnGuideMessageShown(int plr, int index)
{
	// Show the elevator case and the village head.	
	if (index == 0)
	{
		TutArrowShowTarget(FindObject(Find_ID(ElevatorCase)));
		TutArrowShowTarget(FindObject(Find_ID(Clonk), Find_Not(Find_Owner(plr)), Sort_Distance(924, 672)));
	}
	// Show the gap and the foundry.
	if (index == 1)
	{
		TutArrowShowPos(894, 676, 180);
		TutArrowShowTarget(FindObject(Find_ID(Foundry)));
	}
	// Show where to dig earth.
	if (index == 2)
		TutArrowShowPos(504, 456, 60);
	// Show the foundry.
	if (index == 3)
		TutArrowShowTarget(FindObject(Find_ID(Foundry)));
	// Show where to dive for water.
	if (index == 4)
	{
		TutArrowShowPos(112, 520, 200);
		TutArrowShowPos(292, 512, 315);
	}
	// Show where to stand for loam bridge and show workshop.
	if (index == 6)
	{
		TutArrowShowPos(916, 672, 135);
		TutArrowShowTarget(FindObject(Find_ID(ToolsWorkshop)));
	}
	// Show ore to mine.
	if (index == 8)
	{
		TutArrowShowPos(620, 400, 0);
		TutArrowShowPos(400, 398, 240);
		TutArrowShowPos(306, 628, 90);
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
	}
	return FX_OK;
}