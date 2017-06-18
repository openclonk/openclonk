/** 
	Tutorial 03: Carry & Construct
	Author: Maikel
	
	Construct sawmill and flagpole: explains contents menu, dynamite, chopping, foundry and construction site.

	Following controls and concepts are explained:
	 * Contents menu: foundry and chest
	 * Production menu: foundry
	 * Using items: dynamite, hammer, axe
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
	SetNextMission("Tutorials.ocf\\Tutorial03.ocs", "$MsgRepeatRound$", "$MsgRepeatRoundDesc$");
	return;
}

// Gamecall from goals, set next mission.
protected func OnGoalsFulfilled()
{
	// Achievement: Tutorial completed.
	GainScenarioAchievement("TutorialCompleted", 3);	
	// Dialogue options -> next round.
	SetNextMission("Tutorials.ocf\\Tutorial04.ocs", "$MsgNextTutorial$", "$MsgNextTutorialDesc$");
	// Normal scenario ending by goal library.
	return false;
}

private func InitVillageEntrance()
{
	// Create a signpost.
	var post = CreateObjectAbove(EnvPack_Guidepost2, 56, 384);
	post->SetInscription("$MsgWelcomeWipfville$");
	
	// Create a small forest.
	CreateObjectAbove(Tree_Coniferous, 128, 384);
	CreateObjectAbove(Tree_Coniferous, 148, 384);
	CreateObjectAbove(Tree_Coniferous2, 156, 390);
	CreateObjectAbove(Tree_Coniferous3, 176, 390);
	CreateObjectAbove(Tree_Coniferous, 188, 384);
	
	// Wind generator and sawmill construction site for wood production.
	CreateObjectAbove(WindGenerator, 224, 384)->MakeInvincible();
	var site = CreateObjectAbove(ConstructionSite, 264, 386);
	site.MeshTransformation = Trans_Mul(Trans_Rotate(RandomX(-30, 30), 0, 1, 0), Trans_Rotate(RandomX(-10, 10), 1, 0, 0));
	site->Set(Sawmill);
	site->MakeUncancellable();
	site->CreateContents(Wood, 1);
	site->CreateContents(Rock, 1);	
	
	// A hole where the axe is hidden.
	CreateObjectAbove(Axe, 159, 474);
	
	// A wooden cabin ruin on fire.
	var cabin = CreateObjectAbove(Ruin_WoodenCabin, 370, 352);
	cabin.Plane = -400;
	for (var x = -20; x <= 20; x += 10)
		CreateObjectAbove(Flame, cabin->GetX() + x, cabin->GetY());
	cabin->AddScorch(-30, 15, 45, 32, 1200);
	cabin->AddScorch(15, 0, -45, 50, 1500);
	var chest = CreateObjectAbove(Chest, 410, 352);
	chest->CreateContents(Hammer);
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
	
	// A lorry with mining equipment.
	var lorry = CreateObjectAbove(Lorry, 212, 640);
	lorry->CreateContents(Dynamite, 3);
	return;
}

private func InitVillageMain()
{
	// Elevator, and grain production on the right.
	var elevator = CreateObjectAbove(Elevator, 601, 392);
	elevator->CreateShaft(264);
	elevator->MakeInvincible();
	var mill = CreateObjectAbove(Ruin_Windmill, 816, 392);
	mill->AddScorch(-15, 42, 90, 50, 1200);
	CreateObjectAbove(Kitchen, 904, 376);
	CreateObjectAbove(Loom, 560, 392);
	Wheat->Place(40 + Random(10), Rectangle(704, 352, 80, 64));
	Cotton->Place(3, Rectangle(654, 352, 40, 64));
	CreateObjectAbove(StrawMan, 714, 391);
	CreateObjectAbove(StrawMan, 762, 391);
	
	// Stone door to protect the village.
	var door = CreateObjectAbove(StoneDoor, 1004, 376);
	var wheel = CreateObjectAbove(SpinWheel, 972, 376);
	wheel->SetStoneDoor(door);
	
	// A tree is blocking the path down in this round.
	CreateObject(MovingBrick, 870, 396).Plane = 200;
	var tree = CreateObjectAbove(Tree_Coniferous, 890, 422);
	tree->SetR(90);
	tree.Plane = 450;
	
	// Tools and armory down below.	
	CreateObjectAbove(ToolsWorkshop, 698, 504)->MakeInvincible();
	CreateObjectAbove(Armory, 772, 504)->MakeInvincible();
	CreateObjectAbove(Flagpole, 828, 496)->MakeInvincible();
	return;
}

private func InitVillageUnderground()
{
	// A wooden cabin ruin.
	var cabin = CreateObjectAbove(Ruin_WoodenCabin, 562, 568);
	cabin->AddScorch(-20, 12, 45, 50, 1500);
	cabin->AddScorch(15, 10, -45, 50, 1500);
	
	// Chemical lab ruin and flagpole on the left of the elevator.
	var lab = CreateObjectAbove(Ruin_ChemicalLab, 484, 664);
	lab->AddScorch(-12, 18, 130, 80, 1300);
	CreateObjectAbove(Flagpole, 528, 664);
	
	// Steam engine for power supply below.
	var engine = CreateObjectAbove(SteamEngine, 780, 752);
	engine->CreateContents(Coal, 5);
	engine->MakeInvincible();
	
	// Lorry with materials down below, the materials are only added in tutorial 4.
	CreateObjectAbove(Lorry, 954, 670);	
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
	var wipf = CreateObjectAbove(Wipf, 64, 384);
	wipf->SetMeshMaterial("WipfSkin");
	wipf->SetObjectLayer(wipf);
	wipf->EnableTutorialControl(true);
	wipf.Name = "$WipfName$";
	wipf.Description = "$WipfDescription$";
		
	// Some wipfs near the grain field.
	var wipfs = Wipf->Place(4, Rectangle(640, 352, 180, 40));
	for (wipf in wipfs)
	{
		wipf->SetObjectLayer(wipf);
		wipf->EnableTutorialControl(false);
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
	npc_lumberjack->SetDir(DIR_Left);
	npc_lumberjack->SetDialogue("Lumberjack", true);

	// A fireman NPC who extinguishes a burning cabin.	
	var npc_fireman = CreateObjectAbove(Clonk, 200, 384);
	npc_fireman->SetName("Hubert");
	var barrel = npc_fireman->CreateContents(Barrel);
	barrel->PutLiquid("Water", 300);
	npc_fireman->SetObjectLayer(npc_fireman);
	npc_fireman->SetDir(DIR_Left);
	npc_fireman->SetDialogue("Fireman", false);
	npc_fireman->SetAlternativeSkin("MaleDarkHair");
	
	// A builder which tells you where to place the flagpole.
	var npc_builder = CreateObjectAbove(Clonk, 504, 376);
	npc_builder->SetName("Kummerog");
	npc_builder->CreateContents(Hammer);
	npc_builder->SetObjectLayer(npc_builder);
	npc_builder->SetDir(DIR_Left);
	npc_builder->SetDialogue("Builder", false);
	npc_builder->SetAlternativeSkin("Carpenter");
	
	// A farmer near the grain field.
	var npc_farmer = CreateObjectAbove(Clonk, 720, 392);
	npc_farmer->SetColor(0x00ffff);
	npc_farmer->SetName("Genhered");
	npc_farmer->SetObjectLayer(npc_farmer);
	npc_farmer->SetSkin(1);
	npc_farmer->SetDir(DIR_Left);
	npc_farmer->SetDialogue("Farmer", false);
	
	// Lookout.
	var npc_lookout = CreateObjectAbove(Clonk, 992, 296);
	npc_lookout->SetName("Larry");
	npc_lookout->SetObjectLayer(npc_lookout);
	npc_lookout->SetDir(DIR_Left);
	npc_lookout->SetDialogue("Lookout", false);
	npc_lookout->SetAlternativeSkin("Guard");

	// Village head.
	var npc_head = CreateObjectAbove(Clonk, 840, 736);
	npc_head->SetName("Archibald");
	npc_head->SetObjectLayer(npc_head);
	npc_head->SetDir(DIR_Right);
	npc_head->SetDialogue("VillageHead", false);
	var lantern = npc_head->CreateContents(Lantern);
	lantern->TurnOn();
	npc_head->SetAlternativeSkin("Sage");
	return;
}

/*-- Player Handling --*/

protected func InitializePlayer(int plr)
{
	// Position player's clonk.
	var clonk = GetCrew(plr);
	clonk->SetPosition(48, 374);
	clonk->CreateContents(Shovel);
	var effect = AddEffect("ClonkRestore", clonk, 100, 10);
	effect.to_x = 48;
	effect.to_y = 374;
	
	// Claim ownership of structures.
	for (var structure in FindObjects(Find_Or(Find_Category(C4D_Structure), Find_Func("IsFlagpole"))))
		structure->SetOwner(plr);
	
	// Add an effect to the clonk to track the goal.
	AddEffect("TrackGoal", nil, 100, 2);

	// Standard player zoom for tutorials.
	SetPlayerViewLock(plr, true);
	SetPlayerZoomByViewRange(plr, 400, nil, PLRZOOM_Direct | PLRZOOM_Set);
	
	// Take ownership of the sawmill construction site.
	var site = FindObject(Find_ID(ConstructionSite));
	site->SetOwner(plr);
	
	// Knowledge for the flagpole construction.
	SetPlrKnowledge(plr, Flagpole);

	// Create tutorial guide, add messages, show first.
	guide = CreateObject(TutorialGuide, 0, 0, plr);
	var interact = GetPlayerControlAssignment(plr, CON_Interact, true, true);
	guide->AddGuideMessage(Format("$MsgTutorialWipfville$", interact));
	guide->ShowGuideMessage();
	var effect = AddEffect("TutorialTalkedToLumberjack", nil, 100, 5);
	effect.plr = plr;
	return;
}

/*-- Intro, Tutorial Goal & Outro --*/

global func FxTrackGoalTimer(object target, proplist effect, int time)
{
	if (FindObject(Find_ID(Sawmill)) && ObjectCount(Find_ID(Flagpole)) >= 3)
	{
		AddEffect("GoalOutro", target, 100, 5);
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
	if (time >= 54)
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
}

/*-- Guide Messages --*/

public func OnHasTalkedToLumberjack()
{
	RemoveEffect("TutorialTalkedToLumberjack");
	RemoveEffect("TutorialTalkedToLumberjack2");
	return;
}

global func FxTutorialTalkedToLumberjackTimer()
{
	return FX_OK;
}

global func FxTutorialTalkedToLumberjackStop(object target, proplist effect, int reason, bool temp)
{
	if (temp)
		return FX_OK;
	guide->AddGuideMessage("$MsgTutorialFindRock$");
	guide->ShowGuideMessage();
	var new_effect = AddEffect("TutorialFoundLorry", nil, 100, 5);
	new_effect.plr = effect.plr;
	return FX_OK;
}

global func FxTutorialFoundLorryTimer(object target, proplist effect)
{
	if (FindObject(Find_OCF(OCF_CrewMember), Find_Owner(effect.plr), Find_InRect(168, 576, 104, 72)))
	{
		var interaction_menu = GetPlayerControlAssignment(effect.plr, CON_Contents, true, true);
		guide->AddGuideMessage(Format("$MsgTutorialDynamiteLorry$", interaction_menu));
		guide->ShowGuideMessage();
		var new_effect = AddEffect("TutorialObtainedDynamite", nil, 100, 5);
		new_effect.plr = effect.plr;
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialObtainedDynamiteTimer(object target, proplist effect)
{
	var clonk = FindObject(Find_OCF(OCF_CrewMember), Find_Owner(effect.plr));
	if (clonk && FindObject(Find_ID(Dynamite), Find_Container(clonk)))
	{
		var use = GetPlayerControlAssignment(effect.plr, CON_Use, true, true);
		guide->AddGuideMessage(Format("$MsgTutorialBlastRock$", use));
		guide->ShowGuideMessage();
		var new_effect = AddEffect("TutorialBlastedRock", nil, 100, 5);
		new_effect.plr = effect.plr;
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialBlastedRockTimer(object target, proplist effect)
{
	if (FindObject(Find_ID(Rock), Find_InRect(180, 592, 100, 64)))
	{
		guide->AddGuideMessage("$MsgTutorialPickUpRock$");
		guide->ShowGuideMessage();
		var new_effect = AddEffect("TutorialObtainedRock", nil, 100, 5);
		new_effect.plr = effect.plr;
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialObtainedRockTimer(object target, proplist effect)
{
	var clonk = FindObject(Find_OCF(OCF_CrewMember), Find_Owner(effect.plr));
	var has_three_rocks = clonk && ObjectCount(Find_ID(Rock), Find_Container(clonk)) >= 3; 
	var finished_sawmill = FindObject(Find_ID(Sawmill));
	// Also progress if the sawmill has been finished without the three rocks at the same time.
	if (has_three_rocks || finished_sawmill)
	{
		var interaction_menu = GetPlayerControlAssignment(effect.plr, CON_Contents, true, true);
		guide->AddGuideMessage(Format("$MsgTutorialSawmill$", interaction_menu));
		guide->ShowGuideMessage();
		var new_effect = AddEffect("TutorialSawmillFinished", nil, 100, 5);
		new_effect.plr = effect.plr;
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialSawmillFinishedTimer(object target, proplist effect)
{
	if (FindObject(Find_ID(Sawmill)))
		return FX_Execute_Kill;
	return FX_OK;
}

global func FxTutorialSawmillFinishedStop(object target, proplist effect, int reason, bool temp)
{
	if (temp)
		return FX_OK;
	guide->AddGuideMessage("$MsgTutorialTalkToFireman$");
	guide->ShowGuideMessage();
	var new_effect = AddEffect("TutorialTalkedForFlagpole", nil, 100, 5);
	new_effect.plr = effect.plr;
	// Notify lumberjack the sawmill is done.
	var dialogue_lumberjack = Dialogue->FindByName("Lumberjack");
	if (dialogue_lumberjack)
		dialogue_lumberjack->SetDialogueProgress(5, nil, false);
	Dialogue->FindByName("Fireman")->AddAttention();
	Dialogue->FindByName("Builder")->AddAttention();
	return FX_OK;
}


public func OnHasTalkedToFireman()
{
	var effect = GetEffect("TutorialTalkedForFlagpole");
	if (effect)
		effect.talked_to_fireman = true;
	return;
}

public func OnHasTalkedToBuilder()
{
	var effect = GetEffect("TutorialTalkedForFlagpole");
	if (effect)
		effect.talked_to_builder = true;
	return;
}

global func FxTutorialTalkedForFlagpoleTimer(object target, proplist effect)
{
	if (effect.talked_to_fireman && effect.talked_to_builder)
	{
		var use = GetPlayerControlAssignment(effect.plr, CON_Use, true, true);
		guide->AddGuideMessage(Format("$MsgTutorialConstructFlagpole$", use));
		guide->ShowGuideMessage();
		var new_effect = AddEffect("TutorialPlacedFlagpole", nil, 100, 5);
		new_effect.plr = effect.plr;
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialPlacedFlagpoleTimer(object target, proplist effect)
{
	if (FindObject(Find_ID(ConstructionSite), Find_AtRect(480, 356, 32, 20)))
	{
		guide->AddGuideMessage("$MsgTutorialFlagpoleMaterials$");
		guide->ShowGuideMessage();
		var new_effect = AddEffect("TutorialTalkedToLumberjack2", nil, 100, 5);
		new_effect.plr = effect.plr;
		// Notify lumberjack that player talked to other npc's.
		var dialogue_lumberjack = Dialogue->FindByName("Lumberjack");
		if (dialogue_lumberjack)
			dialogue_lumberjack->SetDialogueProgress(6, nil, true);
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialTalkedToLumberjack2Timer()
{
	return FX_OK;
}

global func FxTutorialTalkedToLumberjack2Stop(object target, proplist effect, int reason, bool temp)
{
	if (temp)
		return FX_OK;
	guide->AddGuideMessage("$MsgTutorialAxeChopping$");
	guide->ShowGuideMessage();
	var new_effect = AddEffect("TutorialWoodInSite", nil, 100, 5);
	new_effect.plr = effect.plr;
	return FX_OK;
}

global func FxTutorialWoodInSiteTimer(object target, proplist effect)
{
	var site = FindObject(Find_ID(ConstructionSite), Find_AtRect(480, 356, 32, 20));
	if (site && ObjectCount(Find_ID(Wood), Find_Container(site)) >= 3)
	{
		guide->AddGuideMessage("$MsgTutorialOreMining$");
		guide->ShowGuideMessage();
		var new_effect = AddEffect("TutorialHasOreCoal", nil, 100, 5);
		new_effect.plr = effect.plr;
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialHasOreCoalTimer(object target, proplist effect)
{
	var clonk = FindObject(Find_OCF(OCF_CrewMember), Find_Owner(effect.plr));
	if (FindObject(Find_ID(Ore), Find_Container(clonk)) && FindObject(Find_ID(Coal), Find_Container(clonk)))
	{
		var interaction_menu = GetPlayerControlAssignment(effect.plr, CON_Contents, true, true);
		guide->AddGuideMessage(Format("$MsgTutorialProduceMetal$", interaction_menu));
		guide->ShowGuideMessage();
		var new_effect = AddEffect("TutorialMetalFinished", nil, 100, 5);
		new_effect.plr = effect.plr;
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialMetalFinishedTimer(object target, proplist effect)
{
	if (FindObject(Find_ID(Metal)))
	{
		guide->AddGuideMessage("$MsgTutorialMetalToSite$");
		guide->ShowGuideMessage();
		return FX_Execute_Kill;
	}
	return FX_OK;
}

protected func OnGuideMessageShown(int plr, int index)
{
	// Show the lumberjack.	
	if (index == 0)
		TutArrowShowTarget(FindObject(Find_ID(Clonk), Find_Not(Find_Owner(plr)), Sort_Distance(192, 384))); 
	// Show the statue and the mine entrance.
	if (index == 1)
	{
		TutArrowShowTarget(FindObject(Find_ID(ConstructionSite))); 
		TutArrowShowPos(428, 392, 225);
		TutArrowShowPos(368, 516, 225);
	}
	// Show the lorry with dynamite.
	if (index == 2)
		TutArrowShowTarget(FindObject(Find_ID(Lorry), Sort_Distance(212, 632))); 
	// Show the rock to blast.
	if (index == 3)
		TutArrowShowPos(192, 596, 315);
	// Show the closest chunks of rock.
	if (index == 4)
	{
		var clonk = FindObject(Find_OCF(OCF_CrewMember), Find_Owner(plr));
		var rock = FindObject(Find_ID(Rock), Find_NoContainer(), Find_OCF(OCF_InFree), Find_Distance(60, clonk->GetX(), clonk->GetY()), Sort_Distance(clonk->GetX(), clonk->GetY()));
		TutArrowShowTarget(rock);
	}
	// Show the sawmill construction site.
	if (index == 5)
		TutArrowShowTarget(FindObject(Find_ID(ConstructionSite))); 
	// Show the fireman and builder.
	if (index == 6)
	{
		TutArrowShowTarget(FindObject(Find_ID(Clonk), Find_Not(Find_Owner(plr)), Sort_Distance(304, 366))); 	
		TutArrowShowTarget(FindObject(Find_ID(Clonk), Find_Not(Find_Owner(plr)), Sort_Distance(504, 376))); 	
	}
	// Show the chest for the hammer and the construction location.
	if (index == 7)
	{
		TutArrowShowTarget(FindObject(Find_ID(Chest)));
		TutArrowShowPos(496, 376, 135);
	}
	// Show the lumberjack.	
	if (index == 8)
		TutArrowShowTarget(FindObject(Find_ID(Clonk), Find_Not(Find_Owner(plr)), Sort_Distance(192, 384))); 
	// Show way to the axe and show the axe and a tree.
	if (index == 9)
	{
		TutArrowShowPos(292, 512, 315);
		TutArrowShowPos(184, 320, 225);
		TutArrowShowTarget(FindObject(Find_ID(Axe))); 
	}		
	// Show the ore and coal.
	if (index == 10)
	{
		TutArrowShowPos(136, 636, 270);
		TutArrowShowPos(306, 628, 90);
	}
	// Show the foundry.
	if (index == 11)
	{
		TutArrowShowTarget(FindObject(Find_ID(Foundry)));
	}
	// Show the metal and the flagpole site.
	if (index == 12)
	{
		TutArrowShowTarget(FindObject(Find_ID(Metal)));
		TutArrowShowTarget(FindObject(Find_ID(ConstructionSite), Find_AtRect(480, 356, 32, 20)));	
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