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
	var effect, firestone, chest, powderkeg, ropeladder, grapple, cata, dynamite;
	
	// Dynamite box to blast through mine.
	var dyn1 = CreateObject(Dynamite, 242, 665, NO_OWNER);
	var dyn2 = CreateObject(Dynamite, 272, 665, NO_OWNER);
	var dyn3 = CreateObject(Dynamite, 272, 685, NO_OWNER);
	var dyn4 = CreateObject(Dynamite, 242, 685, NO_OWNER);
	CreateObject(Fuse, 255, 675, NO_OWNER)->Connect(dyn1, dyn2);
	CreateObject(Fuse, 255, 675, NO_OWNER)->Connect(dyn2, dyn3);
	CreateObject(Fuse, 255, 675, NO_OWNER)->Connect(dyn3, dyn4);
	var igniter = CreateObject(Igniter, 110, 710, NO_OWNER);
	CreateObject(Fuse, 240, 685, NO_OWNER)->Connect(dyn4, igniter);
	igniter->SetGraphics("0", Fuse, 1, GFXOV_MODE_Picture);
	
	// Miner's hut and chest with cannon stuff.
	var hut = CreateObject(WoodenCabin, 570, 740, NO_OWNER);
	hut->SetObjectLayer(hut);
	chest = CreateObject(Chest, 510, 740, NO_OWNER);
	for (var i = 0; i < 3; i++)
	{
		firestone = CreateObject(Firestone, 0, 0, NO_OWNER);
		firestone->Enter(chest);
		firestone->AddRestoreMode(chest);
	}
	powderkeg = CreateObject(PowderKeg, 0, 0, NO_OWNER);
	powderkeg->Enter(chest);
	powderkeg->AddRestoreMode(chest);
	// Decoration for the mine.
	var pickaxe;
	pickaxe = CreateObject(Pickaxe, 185, 680, NO_OWNER);
	pickaxe->SetObjectLayer(pickaxe);
	pickaxe->SetR(-60);
	pickaxe = CreateObject(Pickaxe, 316, 678, NO_OWNER);
	pickaxe->SetObjectLayer(pickaxe);
	pickaxe->SetR(70);
	pickaxe = CreateObject(Pickaxe, 156, 666, NO_OWNER);
	pickaxe->SetObjectLayer(pickaxe);
	pickaxe->SetR(-10);
	var lorry = CreateObject(Lorry, 320, 680, NO_OWNER);
	lorry->SetObjectLayer(lorry);
	
	// Cannon to blast through rock & chest with powderkeg and firestones.
	var cannon = CreateObject(Cannon, 700, 420, NO_OWNER);
	//effect = AddEffect("CannonRestore", cannon, 100, 10);
	//EffectVar(1, cannon, effect) = 180;
	//EffectVar(2, cannon, effect) = 450;

	// Chest with flints and dynamite to blast underwater rocks.
	chest = CreateObject(Chest, 870, 680, NO_OWNER);
	for (var i = 0; i < 2; i++)
	{
		firestone = CreateObject(Firestone, 0, 0, NO_OWNER);
		firestone->Enter(chest);
		firestone->AddRestoreMode(chest);
	}
	dynamite = CreateObject(DynamiteBox, 0, 0, NO_OWNER);
	dynamite->Enter(chest);
	effect = AddEffect("DynamiteRestore", dynamite, 100, 10);
	EffectVar(0, dynamite, effect) = chest;
	
	// Another chest with flints and dynamite to blast underwater rocks.
	chest = CreateObject(Chest, 950, 600, NO_OWNER);
	for (var i = 0; i < 2; i++)
	{
		dynamite = CreateObject(DynamiteBox, 0, 0, NO_OWNER);
		dynamite->Enter(chest);
		effect = AddEffect("DynamiteRestore", dynamite, 100, 10);
		EffectVar(0, dynamite, effect) = chest;
	}
	firestone = CreateObject(Firestone, 0, 0, NO_OWNER);
	firestone->Enter(chest);
	firestone->AddRestoreMode(chest);
	
	// Chest with Grapplebows for the final leap.
	chest = CreateObject(Chest, 1520, 700, NO_OWNER);
	for (var i = 0; i < 3; i++)
	{
		grapple = CreateObject(GrappleBow, 0, 0, NO_OWNER);
		grapple->Enter(chest);
		effect = AddEffect("ClonkContentRestore", grapple, 100, 10);
		EffectVar(0, grapple, effect) = chest;
	}
	var shovel = CreateObject(Shovel, 0, 0, NO_OWNER);
	shovel->Enter(chest);
	effect = AddEffect("ClonkContentRestore", shovel, 100, 10);
	EffectVar(0, shovel, effect) = chest;
	
	// Chest with boompack for fast players.
	chest = CreateObject(Chest, 1800, 660, NO_OWNER);
	chest->CreateContents(Boompack, 2);
	
	// Set the mood.
	SetGamma(RGB(30, 25, 20), RGB(135, 130, 125), RGB(255, 250, 245));
	SetSkyParallax(0, 20, 20);
	PlaceGrass(85);
	
	// Dialogue options -> repeat round.
	SetNextMission("Tutorial.c4f\\Tutorial02.c4s", "$MsgRepeatRound$", "$MsgRepeatRoundDesc$");
	return;
}

// Gamecall from goals, set next mission.
protected func OnGoalsFulfilled()
{
	// Dialogue options -> next round.
	SetNextMission("Tutorial.c4f\\Tutorial03.c4s", "$MsgNextTutorial$", "$MsgNextTutorialDesc$");
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
	clonk = GetCrew(plr, 1);
	clonk->SetPosition(200, 440);
	effect = AddEffect("ClonkOneRestore", clonk, 100, 10);
	EffectVar(1, clonk, effect) = 200;
	EffectVar(2, clonk, effect) = 440;
	grapple = CreateObject(GrappleBow, 0, 0, NO_OWNER);
	grapple->Enter(clonk);
	effect = AddEffect("ClonkContentRestore", grapple, 100, 10);
	EffectVar(0, grapple, effect) = clonk;
	ropeladder = CreateObject(Ropeladder, 0, 0, NO_OWNER);
	ropeladder->Enter(clonk);
	effect = AddEffect("ClonkContentRestore", ropeladder, 100, 10);
	EffectVar(0, ropeladder, effect) = clonk;
	
	// Second clonk.
	clonk = GetCrew(plr, 0);
	clonk->SetPosition(30, 680);
	effect = AddEffect("ClonkTwoRestore", clonk, 100, 10);
	EffectVar(1, clonk, effect) = 30;
	EffectVar(2, clonk, effect) = 680;
	
	// Select first clonk
	SetCursor(plr, GetCrew(plr, 1));
	
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
		AddEffect("TutorialFoundCannon", nil, 100, 5);
		return -1;
	}
	return 1;
}

// TODO move this around a little
global func FxTutorialFoundCannonTimer()
{
	if (FindObject(Find_OCF(OCF_CrewMember), Find_Distance(40, 700, 420)))
	{
		guide->AddGuideMessage("$MsgTutCannon$");
		AddEffect("TutorialFreeClonk", nil, 100, 36 * 8);
		return -1;
	}
	return 1;
}

global func FxTutorialFreeClonkStop()
{
	guide->AddGuideMessage("$MsgTutFindExplosives$");
	AddEffect("TutorialFoundExplosives", nil, 100, 5);
}

global func FxTutorialFoundExplosivesTimer()
{
	if (FindObject(Find_OCF(OCF_CrewMember), Find_Distance(40, 510, 740)))
	{
		guide->AddGuideMessage("$MsgTutExplosivesChest$");
		AddEffect("TutorialRockBlasted", nil, 100, 5);
		return -1;
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
	// Show grapple hook position.
	if (index == 1)
		TutArrowShowPos(60, 280, 0);
	// Show dynamite detonator.
	if (index == 3)
	{
		var detonator = FindObject(Find_ID(Igniter), Find_InRect(0, 600, 300, 150));
		if (detonator)
			TutArrowShowTarget(detonator, 225, 16);
	}
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

global func FxClonkOneRestoreTimer(object target, int num, int time)
{
	// Restore clonk to its original location if there is no hanging rope ladder and clonk has fallen down in first sector.
	if (target->GetY() > 360 && Inside(target->GetX(), 360, 720) && !FindObject(Find_InRect(340, 310, 30, 80), Find_Func("IsLadder")))
	{
		var restorer = CreateObject(ObjectRestorer, 0, 0, NO_OWNER);
		var x = BoundBy(target->GetX(), 0, LandscapeWidth());
		var y = BoundBy(target->GetY(), 0, LandscapeHeight());
		restorer->SetPosition(x, y);
		var to_x = EffectVar(1, target, num);
		var to_y = EffectVar(2, target, num);
		restorer->SetRestoreObject(target, nil, to_x, to_y, "ClonkOneRestore");
		return -1;
	}
	// Respawn to new location if reached cliff to grapple from.
	if (Distance(target->GetX(), target->GetY(), 400, 250) < 40)
	{
		EffectVar(1, target, num) = 400;
		EffectVar(2, target, num) = 250;		
	}
	// Respawn to new location if reached ledge.
	if (Distance(target->GetX(), target->GetY(), 680, 260) < 40)
	{
		EffectVar(1, target, num) = 680;
		EffectVar(2, target, num) = 260;		
	}
	// Respawn to new location if reached lake.
	if (Distance(target->GetX(), target->GetY(), 960, 360) < 40)
	{
		EffectVar(1, target, num) = 960;
		EffectVar(2, target, num) = 360;		
	}
	// Respawn to new location if reached granite blasting.
	if (Distance(target->GetX(), target->GetY(), 1560, 350) < 40)
	{
		EffectVar(1, target, num) = 1560;
		EffectVar(2, target, num) = 350;		
	}
	// Respawn to new location if reached acid lake.
	if (Distance(target->GetX(), target->GetY(), 2130, 330) < 40)
	{
		EffectVar(1, target, num) = 2130;
		EffectVar(2, target, num) = 330;		
	}
	return 1;
}

// Relaunches the clonk, from death or removal.
global func FxClonkOneRestoreStop(object target, int num, int reason, bool  temporary)
{
	if (reason == 3 || reason == 4)
	{
		var restorer = CreateObject(ObjectRestorer, 0, 0, NO_OWNER);
		var x = BoundBy(target->GetX(), 0, LandscapeWidth());
		var y = BoundBy(target->GetY(), 0, LandscapeHeight());
		restorer->SetPosition(x, y);
		var to_x = EffectVar(1, target, num);
		var to_y = EffectVar(2, target, num);
		// Respawn new clonk.
		var plr = target->GetOwner();
		var clonk = CreateObject(Clonk, 0, 0, plr);
		clonk->GrabObjectInfo(target);
		if (GetCursor(plr) == target)
			SetCursor(plr, clonk);
		clonk->DoEnergy(100000);
		// Transfer contents(grapple bow and shovel).
		for (var transfer in FindObjects(Find_Container(target), Find_Or(Find_ID(Shovel), Find_ID(GrappleBow))))
		{
			var obj = CreateObject(transfer->GetID(), 0, 0, NO_OWNER);
			obj->Enter(clonk);
			var effect = AddEffect("ClonkContentRestore", obj, 100, 10);
			EffectVar(0, obj, effect) = clonk;
		}
		restorer->SetRestoreObject(clonk, nil, to_x, to_y, "ClonkOneRestore");
	}
	return 1;
}

global func FxClonkTwoRestoreTimer(object target, int num, int time)
{
	// Respawn to new location if reached ledge.
	if (Distance(target->GetX(), target->GetY(), 680, 260) < 40)
	{
		EffectVar(1, target, num) = 680;
		EffectVar(2, target, num) = 260;		
	}
	// Respawn to new location if reached lake.
	if (Distance(target->GetX(), target->GetY(), 960, 360) < 40)
	{
		EffectVar(1, target, num) = 960;
		EffectVar(2, target, num) = 360;		
	}
	// Respawn to new location if reached granite blasting.
	if (Distance(target->GetX(), target->GetY(), 1560, 350) < 40)
	{
		EffectVar(1, target, num) = 1560;
		EffectVar(2, target, num) = 350;		
	}
	// Respawn to new location if reached acid lake.
	if (Distance(target->GetX(), target->GetY(), 2130, 330) < 40)
	{
		EffectVar(1, target, num) = 2130;
		EffectVar(2, target, num) = 330;		
	}
	return 1;
}

// Relaunches the clonk, from death or removal.
global func FxClonkTwoRestoreStop(object target, int num, int reason, bool  temporary)
{
	if (reason == 3 || reason == 4)
	{
		var restorer = CreateObject(ObjectRestorer, 0, 0, NO_OWNER);
		var x = BoundBy(target->GetX(), 0, LandscapeWidth());
		var y = BoundBy(target->GetY(), 0, LandscapeHeight());
		restorer->SetPosition(x, y);
		var to_x = EffectVar(1, target, num);
		var to_y = EffectVar(2, target, num);
		// Respawn new clonk.
		var plr = target->GetOwner();
		var clonk = CreateObject(Clonk, 0, 0, plr);
		clonk->GrabObjectInfo(target);
		if (GetCursor(plr) == target)
			SetCursor(plr, clonk);
		clonk->DoEnergy(100000);
		// Transfer contents(grapple bow and shovel).
		for (var transfer in FindObjects(Find_Container(target), Find_Or(Find_ID(Shovel), Find_ID(GrappleBow))))
		{
			var obj = CreateObject(transfer->GetID(), 0, 0, NO_OWNER);
			obj->Enter(clonk);
			var effect = AddEffect("ClonkContentRestore", obj, 100, 10);
			EffectVar(0, obj, effect) = clonk;
		}
		restorer->SetRestoreObject(clonk, nil, to_x, to_y, "ClonkTwoRestore");
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
global func FxDynamiteRestoreStop(object target, int num, int reason, bool  temporary)
{
	if (reason == 3)
	{
		var restorer = CreateObject(ObjectRestorer, 0, 0, NO_OWNER);
		var x = BoundBy(target->GetX(), 0, LandscapeWidth());
		var y = BoundBy(target->GetY(), 0, LandscapeHeight());
		restorer->SetPosition(x, y);
		var to_container = EffectVar(0, target, num);
		var restored = CreateObject(DynamiteBox, 0, 0, target->GetOwner());
		restorer->SetRestoreObject(restored, to_container, nil, nil, "DynamiteRestore");
	}
	return 1;
}

// Dynamite box, effect timer is always needed.
global func FxDynamiteRestoreTimer(object target, int num, int time)
{
	return 1;
}

// Cannon, restore position if pushed to far to the right.
global func FxCannonRestoreTimer(object target, int num, int time)
{
	if ((target->GetX() > 300 || target->GetY() > 500) && !target->Contained())
	{
		var restorer = CreateObject(ObjectRestorer, 0, 0, NO_OWNER);
		var x = BoundBy(target->GetX(), 0, LandscapeWidth());
		var y = BoundBy(target->GetY(), 0, LandscapeHeight());
		restorer->SetPosition(x, y);
		var to_x = EffectVar(1, target, num);
		var to_y = EffectVar(2, target, num);
		restorer->SetRestoreObject(target, nil, to_x, to_y, "CannonRestore");
		return -1;
	}
	return 1;
}

// Catapult, restore position if pushed to far to the left or right.
global func FxCataRestoreTimer(object target, int num, int time)
{
	if ((target->GetX() < 1110 || target->GetX() > 1650 || target->GetY() > 460) && !target->Contained())
	{
		var restorer = CreateObject(ObjectRestorer, 0, 0, NO_OWNER);
		var x = BoundBy(target->GetX(), 0, LandscapeWidth());
		var y = BoundBy(target->GetY(), 0, LandscapeHeight());
		restorer->SetPosition(x, y);
		var to_x = EffectVar(1, target, num);
		var to_y = EffectVar(2, target, num);
		restorer->SetRestoreObject(target, nil, to_x, to_y, "CataRestore");
		return -1;
	}
	return 1;
}

// Catapult, might be dropped within 1100 X-coordinate.
global func FxCataRestoreStop(object target, int num, int reason, bool  temporary)
{
	if (reason == 3)
	{
		var restorer = CreateObject(ObjectRestorer, 0, 0, NO_OWNER);
		var x = BoundBy(target->GetX(), 0, LandscapeWidth());
		var y = BoundBy(target->GetY(), 0, LandscapeHeight());
		restorer->SetPosition(x, y);
		var to_x = EffectVar(1, target, num);
		var to_y = EffectVar(2, target, num);
		restorer->SetRestoreObject(target, nil, to_x, to_y, "CataRestore");
	}
	return 1;
}

// Ropeladder, restore if thrown away to unreachable location.
global func FxRopeladderRestoreTimer(object target, int num, int time)
{
	if (target->GetX() < 680 && target->GetY() > 340 && !target->Contained())
	{
		var restorer = CreateObject(ObjectRestorer, 0, 0, NO_OWNER);
		var x = BoundBy(target->GetX(), 0, LandscapeWidth());
		var y = BoundBy(target->GetY(), 0, LandscapeHeight());
		restorer->SetPosition(x, y);
		var to_container = EffectVar(0, target, num);
		restorer->SetRestoreObject(target, to_container, nil, nil, "RopeladderRestore");
		return -1;
	}
	return 1;
}

// Ropeladder, restore if destroyed.
global func FxRopeladderRestoreStop(object target, int num, int reason, bool  temporary)
{
	if (reason == 3)
	{
		var restorer = CreateObject(ObjectRestorer, 0, 0, NO_OWNER);
		var x = BoundBy(target->GetX(), 0, LandscapeWidth());
		var y = BoundBy(target->GetY(), 0, LandscapeHeight());
		restorer->SetPosition(x, y);
		var to_container = EffectVar(0, target, num);
		var restored = CreateObject(Ropeladder, 0, 0, target->GetOwner());
		restorer->SetRestoreObject(restored, to_container, nil, nil, "RopeladderRestore");
	}
	return 1;
}

// Clonk content, set new restore location to last clonk.
global func FxClonkContentRestoreTimer(object target, int num, int time)
{

	// Content objects to the container containing them last.
	if (target->Contained())
		EffectVar(0, target, num) = target->Contained();
	return 1;
}

// Clonk content, restore if destroyed.
global func FxClonkContentRestoreStop(object target, int num, int reason, bool  temporary)
{
	if (reason == 3)
	{
		var restorer = CreateObject(ObjectRestorer, 0, 0, NO_OWNER);
		var x = BoundBy(target->GetX(), 0, LandscapeWidth());
		var y = BoundBy(target->GetY(), 0, LandscapeHeight());
		restorer->SetPosition(x, y);
		var to_container = EffectVar(0, target, num);
		var restored = CreateObject(target->GetID(), 0, 0, target->GetOwner());
		restorer->SetRestoreObject(restored, to_container, nil, nil, "ClonkContentRestore");
	}
	return 1;
}
