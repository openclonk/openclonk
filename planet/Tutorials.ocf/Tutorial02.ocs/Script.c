/**
	Tutorial 02: Bombing Barriers
	Author: Maikel
	
	Again your wipf is lost: explains items, collection, inventory management, using them and throwing them.
	
	Following controls and concepts are explained:
	 * Collection of objects
	 * Inventory control
	 * Throwing items: firestones
	 * Dropping items: mushroom/berries
	 * Using items: shovel, loam
	 * Liquids: acid
*/


static guide; // guide object.

protected func Initialize()
{
	// Tutorial goal.
	var goal = CreateObject(Goal_Tutorial);
	goal.Name = "$MsgGoalName$";
	goal.Description = "$MsgGoalDescription$";
	
	// Set the mood.
	SetSkyParallax(0, 20, 20);
	
	// Place objects in different sections.
	InitCaveEntrance();
	InitCaveMiddle();
	InitCaveBottom();
	InitCliffTop();
	InitAcidLake();
	InitVegetation();
	InitAnimals();
	
	// Dialogue options -> repeat round.
	SetNextScenario("Tutorials.ocf\\Tutorial02.ocs", "$MsgRepeatRound$", "$MsgRepeatRoundDesc$");
	return;
}

// Gamecall from goals, set next mission.
protected func OnGoalsFulfilled()
{
	// Achievement: Tutorial completed.
	GainScenarioAchievement("TutorialCompleted", 3);
	// Dialogue options -> next round.
	SetNextScenario("Tutorials.ocf\\Tutorial03.ocs", "$MsgNextTutorial$", "$MsgNextTutorialDesc$");
	// Normal scenario ending by goal library.
	return false;
}

private func InitCaveEntrance()
{
	var windgenerator = CreateObjectAbove(WindGenerator, 40, 328);
	windgenerator->SetObjectLayer(windgenerator);
	var armory =CreateObjectAbove(Armory, 112, 328);
	armory->SetObjectLayer(armory);
	var workshop = CreateObjectAbove(ToolsWorkshop, 192, 328);
	workshop->SetObjectLayer(workshop);
	
	// Place a shovel for digging.
	var shovel = CreateObjectAbove(Shovel, 200, 328);
	shovel->SetR(Random(360));
	return;
}

private func InitCaveMiddle()
{
	// Make sure the brick overlaps the rock to blast.
	DrawMaterialQuad("Brick", 432, 536, 472, 536, 472, 542, 432, 542, true);
	
	// Widen and cover the exit for the wipf.
	DrawMaterialQuad("Tunnel", 550, 526, 570, 526, 570, 536, 550, 536, DMQ_Sub);
	var trunk = CreateObjectAbove(Trunk, 570, 558);
	trunk.MeshTransformation = [821, 0, 795, 0, 0, 1145, 0, 0, -795, 0, 821, 0];
	trunk.Plane = 510; trunk->SetR(90);
	
	// A source of light drawing attention to the wipf.
	var torch = CreateObjectAbove(Torch, 484, 528);
	torch->AttachToWall(true);
		
	// Some mushrooms and ferns in the middle: left cave.
	Fern->Place(2 + Random(2), Rectangle(0, 480, 56, 40));
	Mushroom->Place(4 + Random(2), Rectangle(0, 480, 56, 40));

	// Some mushrooms and ferns in the middle: middle cave.
	Fern->Place(3 + Random(2), Rectangle(128, 464, 104, 80));
	Mushroom->Place(6 + Random(2), Rectangle(128, 464, 104, 80));
	
	// Some mushrooms and ferns in the middle: bottom cave.
	Fern->Place(3 + Random(2), Rectangle(400, 580, 100, 64));
	Mushroom->Place(6 + Random(2), Rectangle(400, 580, 100, 64));
	
	// Seaweed in the two lakes.
	Seaweed->Place(5, Rectangle(48, 400, 96, 32));
	return;
}

private func InitCaveBottom()
{
	PlaceObjects(Loam, 2 + Random(2), "Earth", 496, 592, 40, 24);
	
	// Seaweed in the two lakes.
	Seaweed->Place(5, Rectangle(128, 688, 88, 48));
	Seaweed->Place(5, Rectangle(568, 688, 80, 48));
	return;
}

private func InitCliffTop()
{
	// Some trees and bushes on top of the cliff.
	SproutBerryBush->Place(3 + Random(3), Rectangle(240, 160, 400, 120));
	Flower->Place(10, Rectangle(240, 160, 400, 120));
		
	// Berry bush on the right side of the cliff.
	SproutBerryBush->Place(1, Rectangle(580, 310, 60, 60));
	var bush = FindObject(Find_ID(SproutBerryBush), Sort_Distance(610, 340));
	if (bush)
		bush->CreateObject(Sproutberry);
	return;
}

private func InitAcidLake()
{
	// Ropebridge over the acid lake.
	Ropebridge->Create(816, 528, 928, 528);
	
	// Make the acid lake bubbling a bit.
	BoilingAcid->Place();
	
	// An acid power plant on the right with some dead trees.
	CreateObjectAbove(Tree_Coniferous_Burned, 754, 532)->DoCon(-30);
	CreateObjectAbove(Tree_Coniferous_Burned, 792, 532)->DoCon(-10);
	CreateObjectAbove(Tree_Coniferous_Burned, 942, 532)->DoCon(-35);	
	var engine = CreateObjectAbove(SteamEngine, 980, 528); // TODO: replace with the acid machine
	engine->SetObjectLayer(engine);
	return;
}

// Vegetation throughout the scenario.
private func InitVegetation()
{
	PlaceGrass(85);
	PlaceObjects(Firestone, 25 + Random(5), "Earth");
	PlaceObjects(Loam, 15 + Random(5), "Earth");
	return;
}

private func InitAnimals()
{
	// The wipf as your friend, controlled by AI.
	var wipf = CreateObjectAbove(Wipf, 500, 536);
	wipf->EnableTutorialControl();
	wipf->SetMeshMaterial("WipfSkin");
	wipf.Name = "$WipfName$";
	wipf.Description = "$WipfDescription$";
	
	// Some butterflies as atmosphere.
	for (var i = 0; i < 25; i++)
		PlaceAnimal(Butterfly);
	return;
}	

/*-- Player Handling --*/

protected func InitializePlayer(int plr)
{
	// Position player's clonk.
	var clonk = GetCrew(plr, 0);
	clonk->SetPosition(40, 318);
	var effect = AddEffect("ClonkRestore", clonk, 100, 10);
	effect.to_x = 40;
	effect.to_y = 318;
	
	// Add an effect to the clonk to track the goal.
	var track_goal = AddEffect("TrackGoal", nil, 100, 2);
	track_goal.plr = plr;

	// Standard player zoom for tutorials.
	SetPlayerViewLock(plr, true);
	SetPlayerZoomByViewRange(plr, 400, nil, PLRZOOM_Direct | PLRZOOM_Set);

	// Create tutorial guide, add messages, show first.
	guide = CreateObject(TutorialGuide, 0, 0, plr);
	guide->AddGuideMessage("$MsgTutorialIntro$");
	guide->ShowGuideMessage();
	AddEffect("TutorialShovel", nil, 100, 5);
	return;
}

/*-- Intro, Tutorial Goal & Outro --*/

global func FxTrackGoalTimer(object target, proplist effect, int time)
{
	var crew = GetCrew(effect.plr);
	if (Inside(crew->GetX(), 982, 1002) && Inside(crew->GetY(), 504, 528))
	{
		if (FindObject(Find_ID(Wipf), Find_Distance(15, crew->GetX(), crew->GetY())))
		{
			AddEffect("GoalOutro", crew, 100, 5);
			return FX_Execute_Kill;	
		}		
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

	// Enable wipf activity.
	var wipf = FindObject(Find_ID(Wipf));
	if (wipf)
		wipf->DisableTutorialControl();		
	return FX_OK;
}

/*-- Guide Messages --*/

global func FxTutorialShovelTimer()
{
	var clonk = FindObject(Find_ID(Clonk), Find_InRect(120, 264, 64, 64));
	if (clonk)
	{
		var plr = clonk->GetOwner();
		var pick_up = GetPlayerControlAssignment(plr, CON_PickUp, true, true);
		guide->AddGuideMessage(Format("$MsgTutorialShovel$", pick_up));
		guide->ShowGuideMessage();
		AddEffect("TutorialInventory", nil, 100, 5);
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialInventoryTimer()
{
	var clonk = FindObject(Find_ID(Clonk), Find_InRect(120, 264, 192, 64));
	if (clonk)
	{
		var plr = clonk->GetOwner();
		var shovel = FindObject(Find_ID(Shovel), Find_Container(clonk));
		if (shovel)
		{
			var inv_hotkey_ids = [CON_Hotkey1, CON_Hotkey2, CON_Hotkey3, CON_Hotkey4, CON_Hotkey5];
			var inv_hotkeys = "";
			for (var inv_hotkey in inv_hotkey_ids)
				inv_hotkeys = Format("%s[%s]", inv_hotkeys,  GetPlayerControlAssignment(plr, inv_hotkey, true, true));
			var inv_inv_scroll_up = GetPlayerControlAssignment(plr, CON_InventoryShiftForward, true, true);
			var inv_inv_scroll_down = GetPlayerControlAssignment(plr, CON_InventoryShiftBackward, true, true);
			var inv_scroll = Format("[%s][%s]", inv_inv_scroll_up, inv_inv_scroll_down);
			guide->AddGuideMessage(Format("$MsgTutorialInventory$", inv_hotkeys, inv_scroll));
			guide->ShowGuideMessage();
			AddEffect("TutorialDigging", nil, 100, 5);
			return FX_Execute_Kill;
		}
	}
	return FX_OK;
}

global func FxTutorialDiggingTimer()
{
	var clonk = FindObject(Find_OCF(OCF_CrewMember), Find_InRect(328, 304, 96, 64));
	if (clonk)
	{
		var plr = clonk->GetOwner();
		var shovel = FindObject(Find_ID(Shovel), Find_Container(clonk));
		if (shovel)
		{
			var use = GetPlayerControlAssignment(plr, CON_Use, true, true);
			guide->AddGuideMessage(Format("$MsgTutorialDigging$", use));
			guide->ShowGuideMessage();
			AddEffect("TutorialFirestones", nil, 100, 5);
			return FX_Execute_Kill;
		}
	}
	return FX_OK;
}

global func FxTutorialFirestonesTimer()
{
	if (FindObject(Find_OCF(OCF_CrewMember), Find_InRect(288, 384, 24, 96)))
	{
		guide->AddGuideMessage("$MsgTutorialFirestones$");
		guide->ShowGuideMessage();
		AddEffect("TutorialWipfHole", nil, 100, 5);
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialWipfHoleTimer()
{
	var clonk = FindObject(Find_OCF(OCF_CrewMember), Find_InRect(325, 525, 72, 56));
	if (clonk)
	{
		var plr = clonk->GetOwner();
		var throw = GetPlayerControlAssignment(plr, CON_Throw, true, true);
		guide->AddGuideMessage(Format("$MsgTutorialFoundWipf$", throw));
		guide->ShowGuideMessage();
		AddEffect("TutorialBlastedRock", nil, 100, 5);
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialBlastedRockTimer()
{
	var clonk = FindObject(Find_OCF(OCF_CrewMember), Find_InRect(468, 496, 72, 48));
	if (clonk)
	{
		var plr = clonk->GetOwner();
		var drop = GetPlayerControlAssignment(plr, CON_DropHotkey1, true, true);
		guide->AddGuideMessage(Format("$MsgTutorialBlastedRock$", drop));
		guide->ShowGuideMessage();
		AddEffect("TutorialFedWipf", nil, 100, 5);
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialFedWipfTimer()
{
	var wipf = FindObject(Find_ID(Wipf));
	if (wipf->HadFood())
	{
		guide->AddGuideMessage("$MsgTutorialFedWipf$");
		guide->ShowGuideMessage();
		AddEffect("TutorialDigOutLoam", nil, 100, 5);
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialDigOutLoamTimer()
{
	var clonk = FindObject(Find_OCF(OCF_CrewMember), Find_InRect(424, 600, 32, 64));
	if (clonk)
	{
		var plr = clonk->GetOwner();
		var use = GetPlayerControlAssignment(plr, CON_Use, true, true);
		guide->AddGuideMessage(Format("$MsgTutorialDigOutLoam$", use));
		guide->ShowGuideMessage();
		AddEffect("TutorialFragileBridge", nil, 100, 5);
		var bridge = FindObject(Find_ID(Ropebridge));
		bridge->SetFragile();
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialFragileBridgeTimer()
{
	var clonk = FindObject(Find_OCF(OCF_CrewMember), Find_InRect(744, 480, 80, 48));
	if (clonk)
	{
		guide->AddGuideMessage("$MsgTutorialFragileBridge$");
		guide->ShowGuideMessage();
		// Stop the controls of the clonk for a few seconds.
		DisablePlrControls(clonk->GetOwner());
		clonk->SetComDir(COMD_Stop);
		var effect = AddEffect("TutorialWaitForBridge", nil, 100, 5);
		effect.plr = clonk->GetOwner();
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialWaitForBridgeTimer(object target, proplist effect, int time)
{
	if (time > 2 * 36)
	{
		var use = GetPlayerControlAssignment(effect.plr, CON_Use, true, true);
		guide->AddGuideMessage(Format("$MsgTutorialMakeLoamBridge$", use));
		guide->ShowGuideMessage();
		// Start the controls of the clonk again.
		EnablePlrControls(effect.plr);
		return FX_Execute_Kill;
	}
	return FX_OK;
}

protected func OnGuideMessageShown(int plr, int index)
{
	// Show the direction to move.
	if (index == 0)
		TutArrowShowPos(104, 312, 90);
	// Show the shovel.
	if (index == 1)
	{
		var shovel = FindObject(Find_ID(Shovel), Find_NoContainer());
		if (shovel)
			TutArrowShowTarget(shovel); 
	}
	// Show the direction to dig.
	if (index == 3)
		TutArrowShowPos(366, 396, 225);	
	// Show the firestone material.
	if (index == 4)
		TutArrowShowPos(268, 456);
	// Show the rock to blow up.
	if (index == 5)
		TutArrowShowPos(436, 520, 90);
	// Show the clonks friend: the wipf.	
	if (index == 6)
		TutArrowShowTarget(FindObject(Find_ID(Wipf))); 
	// Show to the way down to the settlement.
	if (index == 7)
		TutArrowShowPos(368, 592);
	// Show the loam pieces to collect.
	if (index == 8)
		TutArrowShowPos(504, 614, 90);
	// Show the fragile bridge.
	if (index == 9)
		TutArrowShowPos(848, 520);
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
	if (FindObject(Find_OCF(OCF_CrewMember), Find_InRect(266, 382, 80, 100)))
	{
		effect.to_x = 304;
		effect.to_y = 444;             
	}
	if (FindObject(Find_OCF(OCF_CrewMember), Find_InRect(408, 574, 60, 58)))
	{
		effect.to_x = 428;
		effect.to_y = 624;             
	}
	if (FindObject(Find_OCF(OCF_CrewMember), Find_InRect(744, 464, 72, 64)))
	{
		effect.to_x = 776;
		effect.to_y = 518;             
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