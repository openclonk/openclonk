/*--
		Tutorial 02
		Author: Maikel

		In this tutorial the player will be familiarized with crew selection, backpack control and some tools.
--*/


static guide; // guide object.

protected func Initialize()
{
	// Create goal, all crew members should reach the flag on the far right side.
	var goal = CreateObject(Goal_ReachFlag, 0, 0, NO_OWNER);
	goal->CreateGoalFlag(2950, 280);
		
	// Create all objects, vehicles, chests used by the player.
	var effect, firestone, chest, grapple, dynamite;
	
	// Dynamite box to blast through mine.
	var dyn1 = CreateObjectAbove(Dynamite, 242, 665, NO_OWNER);
	var dyn2 = CreateObjectAbove(Dynamite, 272, 665, NO_OWNER);
	var dyn3 = CreateObjectAbove(Dynamite, 272, 685, NO_OWNER);
	var dyn4 = CreateObjectAbove(Dynamite, 242, 685, NO_OWNER);
	var dyn5 = CreateObjectAbove(Dynamite, 257, 675, NO_OWNER);
	CreateObjectAbove(Fuse, 255, 675, NO_OWNER)->Connect(dyn1, dyn2);
	CreateObjectAbove(Fuse, 255, 675, NO_OWNER)->Connect(dyn2, dyn3);
	CreateObjectAbove(Fuse, 255, 675, NO_OWNER)->Connect(dyn3, dyn4);
	CreateObjectAbove(Fuse, 255, 675, NO_OWNER)->Connect(dyn4, dyn5);
	var igniter = CreateObjectAbove(Igniter, 110, 710, NO_OWNER);
	CreateObjectAbove(Fuse, 240, 685, NO_OWNER)->Connect(dyn5, igniter);
	igniter->SetGraphics("Picture", Igniter, 1, GFXOV_MODE_Picture);
	
	// Miner's hut and chest with catapult stuff.
	//var hut = CreateObjectAbove(WoodenCabin, 570, 740, NO_OWNER);
	//hut->SetObjectLayer(hut);
	chest = CreateObjectAbove(Chest, 510, 740, NO_OWNER);
	for (var i = 0; i < 3; i++)
	{
		firestone = CreateObjectAbove(Firestone, 0, 0, NO_OWNER);
		firestone->Enter(chest);
		firestone->AddRestoreMode(chest);
	}

	// Cannon to blast through rock & chest with powderkeg and firestones.
/*	var cannon = CreateObjectAbove(Cannon, 700, 420, NO_OWNER);
	effect = AddEffect("CannonRestore", cannon, 100, 10);
	effect.to_x = 700;
	effect.to_y = 420;*/

	// Catapult to blast through rock & chest with firestones.
	var catapult = CreateObjectAbove(Catapult, 700, 420, NO_OWNER);
	effect = AddEffect("CatapultRestore", catapult, 100, 10);
	effect.to_x = 700;
	effect.to_y = 420;

	// Chest with flints and dynamite to blast underwater rocks.
	chest = CreateObjectAbove(Chest, 870, 680, NO_OWNER);
	for (var i = 0; i < 2; i++)
	{
		firestone = CreateObjectAbove(Firestone, 0, 0, NO_OWNER);
		firestone->Enter(chest);
		firestone->AddRestoreMode(chest);
	}
	dynamite = CreateObjectAbove(DynamiteBox, 0, 0, NO_OWNER);
	dynamite->Enter(chest);
	effect = AddEffect("DynamiteRestore", dynamite, 100, 10);
	effect.to_container = chest;
	
	// Another chest with flints and dynamite to blast underwater rocks.
	chest = CreateObjectAbove(Chest, 950, 600, NO_OWNER);
	for (var i = 0; i < 2; i++)
	{
		dynamite = CreateObjectAbove(DynamiteBox, 0, 0, NO_OWNER);
		dynamite->Enter(chest);
		effect = AddEffect("DynamiteRestore", dynamite, 100, 10);
		effect.to_container = chest;
	}
	firestone = CreateObjectAbove(Firestone, 0, 0, NO_OWNER);
	firestone->Enter(chest);
	firestone->AddRestoreMode(chest);
	
	// Chest with Grapplebows for the final leap.
	chest = CreateObjectAbove(Chest, 1520, 700, NO_OWNER);
	for (var i = 0; i < 3; i++)
	{
		grapple = CreateObjectAbove(GrappleBow, 0, 0, NO_OWNER);
		grapple->Enter(chest);
		effect = AddEffect("ClonkContentRestore", grapple, 100, 10);
		effect.to_container = chest;
	}
	var shovel = CreateObjectAbove(Shovel, 0, 0, NO_OWNER);
	shovel->Enter(chest);
	effect = AddEffect("ClonkContentRestore", shovel, 100, 10);
	effect.to_container = chest;
	
	// Chest with boompack for fast players.
	chest = CreateObjectAbove(Chest, 1800, 660, NO_OWNER);
	chest->CreateContents(Boompack, 2);
	
	// Set the mood.
	SetSkyParallax(0, 20, 20);
	PlaceGrass(85);
	
	// Dialogue options -> repeat round.
	SetNextMission("Tutorial.ocf\\Tutorial02.ocs", "$MsgRepeatRound$", "$MsgRepeatRoundDesc$");
	return;
}

// Gamecall from goals, set next mission.
protected func OnGoalsFulfilled()
{
	// Achievement star
	GainScenarioAchievement("Done");
	// Dialogue options -> next round.
	SetNextMission("Tutorial.ocf\\Tutorial03.ocs", "$MsgNextTutorial$", "$MsgNextTutorialDesc$");
	// Normal scenario ending by goal library.
	return false;
}

protected func InitializePlayer(int plr)
{
	var clonk, effect, grapple, ropeladder;
	
	// Standard player zoom for tutorials.
	SetPlayerViewLock(plr, true);
	SetPlayerZoomByViewRange(plr, 400, nil, PLRZOOM_Direct);
	
	// First clonk.
	clonk = GetCrew(plr, 0);
	clonk->SetPosition(200, 440);
	effect = AddEffect("ClonkOneRestore", clonk, 100, 10);
	effect.to_x = 200;
	effect.to_y = 440;
	grapple = CreateObjectAbove(GrappleBow, 0, 0, NO_OWNER);
	grapple->Enter(clonk);
	effect = AddEffect("ClonkContentRestore", grapple, 100, 10);
	effect.to_container = clonk;
	effect = AddEffect("EquipmentRestore", grapple, 100, 10);
	effect.to_container = clonk;
	ropeladder = CreateObjectAbove(Ropeladder, 0, 0, NO_OWNER);
	ropeladder->Enter(clonk);
	effect = AddEffect("ClonkContentRestore", ropeladder, 100, 10);
	effect.to_container = clonk;
	effect = AddEffect("EquipmentRestore", ropeladder, 100, 10);
	effect.to_container = clonk;
	
	// Second clonk.
	clonk = GetCrew(plr, 1);
	clonk->SetPosition(30, 680);
	effect = AddEffect("ClonkTwoRestore", clonk, 100, 10);
	effect.to_x = 30;
	effect.to_y = 680;
	
	// Select first clonk
	SetCursor(plr, GetCrew(plr, 0));
	
	// Create tutorial guide, add messages, show first.
	guide = CreateTutorialGuide(plr);
	guide->AddGuideMessage("$MsgTutWelcome$");
	guide->ShowGuideMessage(0);
	AddEffect("TutorialGrappleUp", nil, 100, 36 * 5);
	return;
}

/*-- Guide Messages --*/

global func FxTutorialGrappleUpStop()
{
	guide->AddGuideMessage("$MsgTutGrappleUp$");
	AddEffect("TutorialReachedEdge", nil, 100, 5);
	return 1;
}

global func FxTutorialReachedEdgeTimer()
{
	if (FindObject(Find_OCF(OCF_CrewMember), Find_InRect(160, 220, 100, 160)))
		return -1;
	return 1;
}

global func FxTutorialReachedEdgeStop()
{
	guide->AddGuideMessage("$MsgTutCrewSelection$");
	return 1;
}

// Player mastered crew selection
public func OnClonkSelection()
{
	if (FrameCounter() == 0)
		return;
	if (GetEffect("TutorialCrewSelected"))
		return;
	
	AddEffect("TutorialCrewSelected", nil, 100, 1000);
	if (GetEffect("TutorialGrappleUp", nil))
		RemoveEffect("TutorialGrappleUp", nil);
	if (GetEffect("TutorialReachedEdge", nil))
		RemoveEffect("TutorialReachedEdge", nil);
		
	guide->AddGuideMessage("$MsgTutBlowUpGold$");
	AddEffect("TutorialBlastedThrough", nil, 100, 5);
	return;
}

global func FxTutorialCrewSelectedTimer()
{
	return 1;
}

global func FxTutorialBlastedThroughTimer()
{
	if (GetPathLength(150, 670, 350, 655))
	{
		guide->AddGuideMessage("$MsgTutFreeOtherClonk$");
		AddEffect("TutorialFoundInteractable", nil, 100, 5);
		return -1;
	}
	return 1;
}

// TODO move this around a little
global func FxTutorialFoundInteractableTimer(object target, effect)
{
	var clonk = GetCursor(GetPlayerByIndex(0));
	var catapult = FindObject(Find_ID(Catapult));
	//var chest = FindObject(Find_ID(Chest), Find_Distance(40, 510, 740));
	if (clonk->GetAction() == "Push")
	{
		var act_trg = clonk->GetActionTarget(0);
		if (act_trg == catapult)
		{
			if (clonk->FindContents(Firestone))
			{
				if (!effect.toldabout_catapult)
					guide->AddGuideMessage("$MsgTutCatapult$");
				if (!effect.toldabout_chest)
					guide->AddGuideMessage("$MsgTutExplosivesChest$");
				guide->AddGuideMessage("$MsgTutFireCatapult$");
				AddEffect("TutorialRockBlasted", nil, 100, 5);
				return -1;
			}
			else if (!effect.toldabout_catapult)
			{
				guide->AddGuideMessage("$MsgTutCatapult$");
				effect.toldabout_catapult = true;
			}		
		}
	}
	if (!effect.toldabout_chest && FindObject(Find_OCF(OCF_CrewMember), Find_Distance(40, 510, 740)))
	{
		guide->AddGuideMessage("$MsgTutExplosivesChest$");
		effect.toldabout_chest = true;
	}
	return 1;
}

global func FxTutorialRockBlastedTimer()
{
	if (GetPathLength(280, 230, 420, 230))
	{
		guide->AddGuideMessage("$MsgTutGrappleSwing$");
		AddEffect("TutorialReachedPlatform", nil, 100, 5);
		return -1;
	}
	return 1;
}

global func FxTutorialReachedPlatformTimer()
{
	if (FindObject(Find_OCF(OCF_CrewMember), Find_Distance(40, 680, 260)))
	{
		guide->AddGuideMessage("$MsgTutRopeladder$");
		AddEffect("TutorialReachedLake", nil, 100, 5);
		return -1;
	}
	return 1;
}

global func FxTutorialReachedLakeTimer()
{
	if (FindObject(Find_OCF(OCF_CrewMember), Find_Distance(40, 960, 360)))
	{
		guide->AddGuideMessage("$MsgTutDive$");
		AddEffect("TutorialReachedGranite", nil, 100, 5);
		return -1;
	}
	return 1;
}

global func FxTutorialReachedGraniteTimer()
{
	if (FindObject(Find_OCF(OCF_CrewMember), Find_Distance(40, 1560, 350)))
	{
		guide->AddGuideMessage("$MsgTutBlastGranite$");
		AddEffect("TutorialPassedGranite", nil, 100, 5);
		return -1;
	}
	return 1;
}

global func FxTutorialPassedGraniteTimer()
{
	if (FindObject(Find_OCF(OCF_CrewMember), Find_InRect(1800, 0, 1200, 750)))
	{
		guide->AddGuideMessage("$MsgTutBlastedGranite$");
		AddEffect("TutorialReachedAcid", nil, 100, 5);
		return -1;
	}
	return 1;
}

global func FxTutorialReachedAcidTimer()
{
	if (FindObject(Find_OCF(OCF_CrewMember), Find_Distance(40, 2130, 330)))
	{
		guide->AddGuideMessage("$MsgTutLastGrapple$");
		return -1;
	}
	return 1;
}

protected func OnGuideMessageShown(int plr, int index)
{
	// Show the guide location again.
	if (index == 0)
	{
		var guide = FindObject(Find_ID(TutorialGuide));
		if (guide)
			TutArrowShowGUITarget(guide, 0);
	}
	// Show grapple hook position.
	if (index == 1)
		TutArrowShowPos(60, 280, 0);
	// Show crew selection in the HUD.
	if (index == 2)
		for (var crew_sel in FindObjects(Find_ID(GUI_CrewSelector)))
			TutArrowShowGUITarget(crew_sel, 0);
	
	// Show dynamite detonator.
	if (index == 3)
	{
		var detonator = FindObject(Find_ID(Igniter), Find_InRect(0, 600, 300, 150));
		if (detonator)
			TutArrowShowTarget(detonator, 225, 16);
	}
	// Show where to shoot with catapult.
	if (index == 7)
		TutArrowShowPos(380, 240, 270);
	// Show grapple jump & hook position.
	if (index == 8)
	{
		TutArrowShowPos(440, 310, 225);
		TutArrowShowPos(580, 170, 0);
	}
	// Show ropeladder position.
	if (index == 9)
		TutArrowShowPos(630, 310, 135);
	// Show resurface locations.
	if (index == 10)
	{
		TutArrowShowPos(1160, 590, 0);
		TutArrowShowPos(1285, 520, 0);		
		TutArrowShowPos(1230, 510, 0);		
	}
	// Show granite blast location.
	if (index == 11)
		TutArrowShowPos(1705, 345, 90);
	// Show grapple tunnel
	if (index == 12)
		TutArrowShowPos(1500, 480, 135);
	// Show grapple aim positions.
	if (index == 13)
	{
		TutArrowShowPos(2270, 225, 0);
		TutArrowShowPos(2340, 225, 0);
		TutArrowShowPos(2435, 260, 0);
		TutArrowShowPos(2515, 230, 0);
		TutArrowShowPos(2680, 260, 0);
	}
	return;
}

protected func OnGuideMessageRemoved(int plr, int index)
{
	TutArrowClear();
	return;
}

/*-- Clonk restoring --*/

global func FxClonkOneRestoreTimer(object target, effect, int time)
{
	// Restore clonk to its original location if there is no hanging rope ladder and clonk has fallen down in first sector.
	if (target->GetY() > 360 && Inside(target->GetX(), 360, 720) && !FindObject(Find_InRect(340, 310, 30, 80), Find_Func("IsLadder")))
	{
		var restorer = CreateObjectAbove(ObjectRestorer, 0, 0, NO_OWNER);
		var x = BoundBy(target->GetX(), 0, LandscapeWidth());
		var y = BoundBy(target->GetY(), 0, LandscapeHeight());
		restorer->SetPosition(x, y);
		var to_x = effect.to_x;
		var to_y = effect.to_y;
		restorer->SetRestoreObject(target, nil, to_x, to_y, 0, "ClonkOneRestore");
		return -1;
	}
	// Respawn to new location if reached cliff to grapple from.
	if (Distance(target->GetX(), target->GetY(), 400, 250) < 40)
	{
		effect.to_x = 400;
		effect.to_y = 250;		
	}
	// Respawn to new location if reached ledge.
	if (Distance(target->GetX(), target->GetY(), 680, 260) < 40)
	{
		effect.to_x = 680;
		effect.to_y = 260;		
	}
	// Respawn to new location if reached lake.
	if (Distance(target->GetX(), target->GetY(), 960, 360) < 40)
	{
		effect.to_x = 960;
		effect.to_y = 360;		
	}
	// Respawn to new location if reached granite blasting.
	if (Distance(target->GetX(), target->GetY(), 1560, 350) < 40)
	{
		effect.to_x = 1560;
		effect.to_y = 350;		
	}
	// Respawn to new location if reached acid lake.
	if (Distance(target->GetX(), target->GetY(), 2130, 330) < 40)
	{
		effect.to_x = 2130;
		effect.to_y = 330;		
	}
	return 1;
}

// Relaunches the clonk, from death or removal.
global func FxClonkOneRestoreStop(object target, effect, int reason, bool  temporary)
{
	if (reason == 3 || reason == 4)
	{
		var restorer = CreateObjectAbove(ObjectRestorer, 0, 0, NO_OWNER);
		var x = BoundBy(target->GetX(), 0, LandscapeWidth());
		var y = BoundBy(target->GetY(), 0, LandscapeHeight());
		restorer->SetPosition(x, y);
		var to_x = effect.to_x;
		var to_y = effect.to_y;
		// Respawn new clonk.
		var plr = target->GetOwner();
		var clonk = CreateObjectAbove(Clonk, 0, 0, plr);
		clonk->GrabObjectInfo(target);
		if (GetCursor(plr) == target)
			SetCursor(plr, clonk);
		clonk->DoEnergy(100000);
		// Transfer contents(grapple bow and shovel).
		for (var transfer in FindObjects(Find_Container(target), Find_Or(Find_ID(Shovel), Find_ID(GrappleBow))))
		{
			var obj = CreateObjectAbove(transfer->GetID(), 0, 0, NO_OWNER);
			obj->Enter(clonk);
			var new_effect = AddEffect("ClonkContentRestore", obj, 100, 10);
			new_effect.to_container = clonk;
		}
		restorer->SetRestoreObject(clonk, nil, to_x, to_y, 0, "ClonkOneRestore");
	}
	return 1;
}

global func FxClonkTwoRestoreTimer(object target, effect, int time)
{
	// Respawn to new location if reached ledge.
	if (Distance(target->GetX(), target->GetY(), 680, 260) < 40)
	{
		effect.to_x = 680;
		effect.to_y = 260;		
	}
	// Respawn to new location if reached lake.
	if (Distance(target->GetX(), target->GetY(), 960, 360) < 40)
	{
		effect.to_x = 960;
		effect.to_y = 360;		
	}
	// Respawn to new location if reached granite blasting.
	if (Distance(target->GetX(), target->GetY(), 1560, 350) < 40)
	{
		effect.to_x = 1560;
		effect.to_y = 350;		
	}
	// Respawn to new location if reached acid lake.
	if (Distance(target->GetX(), target->GetY(), 2130, 330) < 40)
	{
		effect.to_x = 2130;
		effect.to_y = 330;		
	}
	return 1;
}

// Relaunches the clonk, from death or removal.
global func FxClonkTwoRestoreStop(object target, effect, int reason, bool  temporary)
{
	if (reason == 3 || reason == 4)
	{
		var restorer = CreateObjectAbove(ObjectRestorer, 0, 0, NO_OWNER);
		var x = BoundBy(target->GetX(), 0, LandscapeWidth());
		var y = BoundBy(target->GetY(), 0, LandscapeHeight());
		restorer->SetPosition(x, y);
		var to_x = effect.to_x;
		var to_y = effect.to_y;
		// Respawn new clonk.
		var plr = target->GetOwner();
		var clonk = CreateObjectAbove(Clonk, 0, 0, plr);
		clonk->GrabObjectInfo(target);
		if (GetCursor(plr) == target)
			SetCursor(plr, clonk);
		clonk->DoEnergy(100000);
		// Transfer contents(grapple bow and shovel).
		for (var transfer in FindObjects(Find_Container(target), Find_Or(Find_ID(Shovel), Find_ID(GrappleBow))))
		{
			var obj = CreateObjectAbove(transfer->GetID(), 0, 0, NO_OWNER);
			obj->Enter(clonk);
			var new_effect = AddEffect("ClonkContentRestore", obj, 100, 10);
			new_effect.to_container = clonk;
		}
		restorer->SetRestoreObject(clonk, nil, to_x, to_y, 0, "ClonkTwoRestore");
	}
	return 1;
}

/*-- Item restoring --*/
// All done through global effects, which use ObjectRestorer.
// In all cases the effects have:
// Timer interval: 10 frames.
// Effectvar 0: Container to which must be restored.
// Effectvar 1: x-coordinate to which must be restored.
// Effectvar 2: y-coordinate to which must be restored.

// Dynamite box, needs seperate effect since changedef call.
global func FxDynamiteRestoreStop(object target, effect, int reason, bool  temporary)
{
	if (reason == 3)
	{
		var restorer = CreateObjectAbove(ObjectRestorer, 0, 0, NO_OWNER);
		var x = BoundBy(target->GetX(), 0, LandscapeWidth());
		var y = BoundBy(target->GetY(), 0, LandscapeHeight());
		restorer->SetPosition(x, y);
		var to_container = effect.to_container;
		var restored = CreateObjectAbove(DynamiteBox, 0, 0, target->GetOwner());
		restorer->SetRestoreObject(restored, to_container, nil, nil, nil, "DynamiteRestore");
	}
	return 1;
}

// Dynamite box, effect timer is always needed.
global func FxDynamiteRestoreTimer(object target, effect, int time)
{
	return 1;
}

// Catapult, restore position if pushed to far to the right.
global func FxCatapultRestoreTimer(object target, effect, int time)
{
	if ((target->GetX() < 595 && target->GetY() > 415) && !target->Contained())
	{
		var restorer = CreateObjectAbove(ObjectRestorer, 0, 0, NO_OWNER);
		var x = BoundBy(target->GetX(), 0, LandscapeWidth());
		var y = BoundBy(target->GetY(), 0, LandscapeHeight());
		restorer->SetPosition(x, y);
		var to_x = effect.to_x;
		var to_y = effect.to_y;
		restorer->SetRestoreObject(target, nil, to_x, to_y, 0, "CatapultRestore");
		return -1;
	}
	return 1;
}

// Ropeladder, restore if thrown away to unreachable location.
global func FxEquipmentRestoreTimer(object target, effect, int time)
{
	if (target->GetX() < 680 && target->GetY() > 340 && !target->Contained())
	{
		var restorer = CreateObjectAbove(ObjectRestorer, 0, 0, NO_OWNER);
		var x = BoundBy(target->GetX(), 0, LandscapeWidth());
		var y = BoundBy(target->GetY(), 0, LandscapeHeight());
		restorer->SetPosition(x, y);
		var to_container = effect.to_container;
		restorer->SetRestoreObject(target, to_container, nil, nil, 0, "RopeladderRestore");
		return -1;
	}
	return 1;
}

// Ropeladder, restore if destroyed.
global func FxRopeladderRestoreStop(object target, effect, int reason, bool  temporary)
{
	if (reason == 3)
	{
		var restorer = CreateObjectAbove(ObjectRestorer, 0, 0, NO_OWNER);
		var x = BoundBy(target->GetX(), 0, LandscapeWidth());
		var y = BoundBy(target->GetY(), 0, LandscapeHeight());
		restorer->SetPosition(x, y);
		var to_container = effect.to_container;
		var restored = CreateObjectAbove(Ropeladder, 0, 0, target->GetOwner());
		restorer->SetRestoreObject(restored, to_container, nil, nil, 0, "RopeladderRestore");
	}
	return 1;
}

// Clonk content, set new restore location to last clonk.
global func FxClonkContentRestoreTimer(object target, effect, int time)
{

	// Content objects to the container containing them last.
	if (target->Contained())
		effect.to_container = target->Contained();
	return 1;
}

// Clonk content, restore if destroyed.
global func FxClonkContentRestoreStop(object target, effect, int reason, bool  temporary)
{
	if (reason == 3)
	{
		var restorer = CreateObjectAbove(ObjectRestorer, 0, 0, NO_OWNER);
		var x = BoundBy(target->GetX(), 0, LandscapeWidth());
		var y = BoundBy(target->GetY(), 0, LandscapeHeight());
		restorer->SetPosition(x, y);
		var to_container = effect.to_container;
		var restored = CreateObjectAbove(target->GetID(), 0, 0, target->GetOwner());
		restorer->SetRestoreObject(restored, to_container, nil, nil, 0, "ClonkContentRestore");
	}
	return 1;
}
