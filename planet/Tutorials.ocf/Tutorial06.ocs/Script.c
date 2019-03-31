/** 
	Tutorial 06: Power Problems
	Author: Maikel
	
	The player has reached the friendly village high up in the skies, they don't have power as there is no wind.
	Help them setting up a back-up power supply and construct an airplane so they will fight against the evil faction.
	
	Following controls and concepts are explained:
	 * Power system and overview
	 * Buying at the flagpole
	 * Construction of steam engine and compensator
	 * Catapult (shooting objects and oneself)
	 * Production of airplane
	 
	TODO:
	 * Outro with airplane who takes the player to the next round.
	 * Intro with airship from bottom left (also add to tutorial 5).
	 * NPC clonks.
	 * Children NPCs playing with the catapults.
	 * Make a script player who is owner of the village.
*/

static guide; // guide object.

protected func Initialize()
{
	// Tutorial goal.
	var goal = CreateObject(Goal_Tutorial);
	goal.Name = "$MsgGoalName$";
	goal.Description = "$MsgGoalDescription$";

	// Enable buying at the flagpole
	CreateObject(Rule_BuyAtFlagpole);
	
	// Place objects in different sections.
	InitBottomIsland();
	InitMiddleIsland();
	InitTopIsland();
	InitVegetation();
	InitAnimals();
	InitAI();	
	
	// Dialogue options -> repeat round.
	SetNextScenario("Tutorials.ocf\\Tutorial06.ocs", "$MsgRepeatRound$", "$MsgRepeatRoundDesc$");
	return;
}

// Gamecall from goals, set next mission.
protected func OnGoalsFulfilled()
{
	// Achievement: Tutorial completed.
	GainScenarioAchievement("TutorialCompleted", 3);	
	// Dialogue options -> next round.
	SetNextScenario("Tutorials.ocf\\Tutorial07.ocs", "$MsgNextTutorial$", "$MsgNextTutorialDesc$");
	// Normal scenario ending by goal library.
	return false;
}

private func InitBottomIsland()
{
	var shipyard = CreateObjectAbove(Shipyard, 250, 568);
	shipyard->MakeInvincible();
	shipyard->CreateContents(Wood, 4);
	shipyard->CreateContents(Metal, 6);
	
	CreateObjectAbove(Flagpole, 440, 544)->MakeInvincible();
	CreateObjectAbove(WindGenerator, 400, 544)->MakeInvincible();
	return;
}

private func InitMiddleIsland()
{
	// A catapult to get to the bottom island.
	var catapult = CreateObjectAbove(Catapult, 760, 408);
	catapult->MakeInvincible();
	catapult->TurnLeft();
	
	var chemical_lab = CreateObjectAbove(ChemicalLab, 850, 368);
	chemical_lab->MakeInvincible();
	chemical_lab->CreateContents(Dynamite, 5);
	
	CreateObjectAbove(WindGenerator, 890, 368)->MakeInvincible();	
	
	var sawmill = CreateObjectAbove(Sawmill, 996, 360);
	sawmill->MakeInvincible();
	sawmill->SetDir(DIR_Right);
	
	// A chest with a windbag on the small island (this is the easter egg).
	var chest = CreateObjectAbove(Chest, 1004, 176);
	chest->MakeInvincible();
	chest->CreateContents(WindBag);
	return;
}

private func InitTopIsland()
{
	// A catapult to get to the middle island.
	var catapult = CreateObjectAbove(Catapult, 432, 248);
	catapult->MakeInvincible();
	
	var elevator = CreateObjectAbove(Elevator, 312, 240);
	elevator->SetDir(DIR_Right);
	elevator->MakeInvincible();
	elevator->CreateShaft(200);
	var case = elevator->GetCase();
	case->SetPosition(case->GetX(), 496);
	case->MakeInvincible();
	
	var workshop = CreateObjectAbove(ToolsWorkshop, 220, 240);
	workshop->MakeInvincible();
	workshop->CreateContents(Hammer, 2);
	workshop->CreateContents(Shovel, 2);
	workshop->CreateContents(Pickaxe, 2);
	
	CreateObjectAbove(Flagpole, 356, 240)->MakeInvincible();
	CreateObjectAbove(WindGenerator, 270, 240)->MakeInvincible();	
	return;
}

private func InitVegetation()
{
	Grass->Place(85);
	PlaceObjects(Rock, 20, "Earth");
	PlaceObjects(Firestone, 8, "Earth");
	Branch->Place(10);
	Mushroom->Place(12);
	Flower->Place(10);
	Trunk->Place(3);
	Tree_Deciduous->Place(12);
	Tree_Coniferous2->Place(2);
	Tree_Coniferous3->Place(2);
	return;
}

private func InitAnimals()
{
	// Some animals as atmosphere.
	Butterfly->Place(20);
	Zaphive->Place(2);
	Mosquito->Place(2);
	return;
}

// Initializes the AI: which is all defined in System.ocg
private func InitAI()
{
	// Village head.
	var npc_head = CreateObjectAbove(Clonk, 250, 558);
	npc_head->SetName("Archibald");
	npc_head->SetObjectLayer(npc_head);
	npc_head->SetDir(DIR_Left);
	npc_head->SetDialogue("VillageHead");
	npc_head->SetAlternativeSkin("Sage");
	
	// Skyville head.
	var npc_head = CreateObjectAbove(Clonk, 260, 558);
	npc_head->SetName("Ludwig");
	npc_head->SetObjectLayer(npc_head);
	npc_head->SetDir(DIR_Left);
	npc_head->SetDialogue("SkyvilleHead");
	npc_head->SetAlternativeSkin("MaleBlackHair");
	return;
}

/*-- Player Handling --*/

protected func InitializePlayer(int plr)
{
	// Position player's clonk.
	var clonk = GetCrew(plr);
	clonk->SetPosition(220, 558);
	clonk->CreateContents(Shovel);
	var effect = AddEffect("ClonkRestore", clonk, 100, 10);
	effect.to_x = 220;
	effect.to_y = 558;

	// Standard player zoom for tutorials.
	SetPlayerViewLock(plr, true);
	SetPlayerZoomByViewRange(plr, 300, nil, PLRZOOM_Direct | PLRZOOM_Set | PLRZOOM_LimitMax);
	SetPlayerZoomByViewRange(plr, LandscapeWidth(), nil, PLRZOOM_LimitMax);
	
	// Claim ownership of structures.
	for (var structure in FindObjects(Find_Or(Find_Category(C4D_Structure), Find_Func("IsFlagpole"))))
		structure->SetOwner(plr);
		
	// Knowledge and base material for this round.
	SetPlrKnowledge(plr, Compensator);
	SetPlrKnowledge(plr, SteamEngine);
	SetBaseMaterial(plr, Metal, 20);
	
	// Set wealth to buy items.
	SetWealth(plr, 400);
	
	// Add an effect to the clonk to track the goal.
	var track_goal = AddEffect("TrackGoal", nil, 100, 2);
	track_goal.plr = plr;
	
	// Start the intro sequence.
	StartSequence("Intro", 0, plr);
	
	// Create tutorial guide and control effect.
	guide = CreateObject(TutorialGuide, 0, 0, plr);
	guide->HideGuide();
	var effect = AddEffect("TutorialReachedSkyIsland", nil, 100, 5);
	effect.plr = plr;
	return;
}


/*-- Intro, Tutorial Goal & Outro --*/

global func FxTrackGoalTimer(object target, proplist effect, int time)
{
	if (FindObject(Find_ID(Airplane)))
	{
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

public func OnIntroSequenceFinished()
{
	var interact = GetPlayerControlAssignment(guide->GetOwner(), CON_Interact, true, true);
	guide->AddGuideMessage(Format("$MsgTutorialTakeElevator$", interact));
	guide->ShowGuide();
	guide->ShowGuideMessage();
	return;
}

global func FxTutorialReachedSkyIslandTimer(object target, proplist effect)
{
	var crew = GetCrew(effect.plr);
	if (crew && FindObject(Find_ID(Hammer), Find_Container(crew)))
	{
		guide->AddGuideMessage("$MsgTutorialBuildCompensator$");
		guide->ShowGuideMessage();
		var new_effect = AddEffect("TutorialPlacedCompensatorSite", nil, 100, 5);
		new_effect.plr = effect.plr;
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialPlacedCompensatorSiteTimer(object target, proplist effect)
{
	var site = FindObject(Find_ID(ConstructionSite), Find_AtRect(160, 260, 80, 40));
	var compensator = FindObject(Find_ID(Compensator), Find_AtRect(160, 260, 80, 40));
	if ((site && site.definition == Compensator) || compensator)
	{
		var interaction_menu = GetPlayerControlAssignment(effect.plr, CON_Contents, true, true);
		guide->AddGuideMessage(Format("$MsgTutorialBuyMetal$", interaction_menu));
		guide->ShowGuideMessage();
		var new_effect = AddEffect("TutorialFinishedCompensator", nil, 100, 5);
		new_effect.plr = effect.plr;
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialFinishedCompensatorTimer(object target, proplist effect)
{
	if (FindObject(Find_ID(Compensator), Find_AtRect(160, 260, 80, 40)))
	{
		guide->AddGuideMessage("$MsgTutorialPowerOverview$");
		guide->ShowGuideMessage();
		var new_effect = AddEffect("TutorialLookedAtPowerOverview", nil, 100, 5);
		new_effect.plr = effect.plr;
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialLookedAtPowerOverviewTimer(object target, proplist effect)
{
	// TODO: fix me (should be triggered when the E menu is opened).
	if (effect.looked_at || true)
	{
		var interact = GetPlayerControlAssignment(effect.plr, CON_Interact, true, true);
		var interact_cycle = GetPlayerControlAssignment(effect.plr, CON_InteractNext_CycleObject, true, true);
		guide->AddGuideMessage(Format("$MsgTutorialEnterCatapult$", interact, interact_cycle, interact));
		guide->ShowGuideMessage();
		var new_effect = AddEffect("TutorialEnteredCatapult", nil, 100, 5);
		new_effect.plr = effect.plr;
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialEnteredCatapultTimer(object target, proplist effect)
{
	var catapult = FindObject(Find_ID(Catapult), Find_Distance(80, 432, 248));
	if (catapult && GetCrew(effect.plr) == FindObject(Find_OCF(OCF_CrewMember), Find_Container(catapult)))
	{
		var zoom_in = GetPlayerControlAssignment(effect.plr, CON_WheelZoomIn, true, true);
		var zoom_out = GetPlayerControlAssignment(effect.plr, CON_WheelZoomOut, true, true);
		var use = GetPlayerControlAssignment(effect.plr, CON_Use, true, true);
		guide->AddGuideMessage(Format("$MsgTutorialLaunchCatapult$", zoom_in, zoom_out, use));
		guide->ShowGuideMessage();
		var new_effect = AddEffect("TutorialReachedMiddleIsland", nil, 100, 5);
		new_effect.plr = effect.plr;
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialReachedMiddleIslandTimer(object target, proplist effect)
{
	if (FindObject(Find_OCF(OCF_CrewMember), Find_Owner(effect.plr), Find_InRect(744, 336, 300, 280)))
	{
		guide->AddGuideMessage("$MsgTutorialBuildSteamEngine$");
		guide->ShowGuideMessage();
		var new_effect = AddEffect("TutorialPlacedSteamEngineSite", nil, 100, 5);
		new_effect.plr = effect.plr;
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialPlacedSteamEngineSiteTimer(object target, proplist effect)
{
	var site = FindObject(Find_ID(ConstructionSite), Find_AtRect(832, 400, 72, 56));
	if (site && site.definition == SteamEngine)
	{
		guide->AddGuideMessage("$MsgTutorialTransportMaterials$");
		guide->ShowGuideMessage();
		var new_effect = AddEffect("TutorialReachedTopIsland", nil, 100, 5);
		new_effect.plr = effect.plr;
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialReachedTopIslandTimer(object target, proplist effect)
{
	if (FindObject(Find_OCF(OCF_CrewMember), Find_Owner(effect.plr), Find_InRect(200, 200, 200, 40)))
	{
		var interact = GetPlayerControlAssignment(effect.plr, CON_Interact, true, true);
		guide->AddGuideMessage(Format("$MsgTutorialShootCoalAndMetal$", interact));
		guide->ShowGuideMessage();
		var new_effect = AddEffect("TutorialReachedMiddleIslandSecond", nil, 100, 5);
		new_effect.plr = effect.plr;
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialReachedMiddleIslandSecondTimer(object target, proplist effect)
{
	if (FindObject(Find_OCF(OCF_CrewMember), Find_Owner(effect.plr), Find_InRect(744, 336, 300, 280)))
	{
		guide->AddGuideMessage("$MsgTutorialFinishSteamEngine$");
		guide->ShowGuideMessage();
		var new_effect = AddEffect("TutorialFinishSteamEngine", nil, 100, 5);
		new_effect.plr = effect.plr;
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialFinishSteamEngineTimer(object target, proplist effect)
{
	if (FindObject(Find_OCF(OCF_Fullcon), Find_ID(SteamEngine), Find_Owner(effect.plr)))
	{
		SetPlrKnowledge(effect.plr, Airplane);
		return FX_Execute_Kill;
	}
	return FX_OK;
}

protected func OnGuideMessageShown(int plr, int index)
{
	// Show the location for the compensator.
	if (index == 1)
	{
		TutArrowShowPos(200, 300, 135);
	}
	
	// Show the location for the steam engine.
	if (index == 6)
	{
		TutArrowShowPos(872, 456, 135);
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
