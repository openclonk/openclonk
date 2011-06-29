/*-- 
	Tutorial 01
	Author: Ringwall
	
	First introduction to the world of OpenClonk: explains movement, shovel, loam and firestones.
--*/

static guide; // guide object
static loam_chest; // chest containing loam
static flint_chest; // chest containing flints

protected func Initialize()
{
	// Tutorial goal.
	var goal = CreateObject(Goal_ReachFlag, 0, 0, NO_OWNER);
	goal->CreateGoalFlag(2330, 1040);
	
	// Environment.
	PlaceGrass(85);
	CreateObject(Tree_Coniferous, 900, 629);
	CreateObject(Plane, 950, 605);

	// Shovel in water.
	var shovel = CreateObject(Shovel, 1368, 1160, NO_OWNER);
	shovel->SetR(150);
	AddEffect("ShovelGet", shovel, 100, 36, shovel);

	// Chest with loam.
	var chest = CreateObject(Chest, 1815, 1100, NO_OWNER);
	var loam = chest->CreateContents(Loam);
	AddEffect("LoamGet", loam, 1, 36, loam);
	loam->AddRestoreMode(chest);

	// Chest with firestones.
	chest = CreateObject(Chest, 2026, 1089, NO_OWNER);
	chest->CreateContents(Firestone)->AddRestoreMode(chest);
	chest->CreateContents(Firestone)->AddRestoreMode(chest);
	
	// Dialogue options -> repeat round.
	SetNextMission("Tutorial.ocf\\Tutorial01.ocs", "$MsgRepeatRound$", "$MsgRepeatRoundDesc$");
	return;
}

// Gamecall from goals, set next mission.
protected func OnGoalsFulfilled()
{
	// Dialogue options -> next round.
	SetNextMission("Tutorial.ocf\\Tutorial02.ocs", "$MsgNextTutorial$", "$MsgNextTutorialDesc$");
	// Normal scenario ending by goal library.
	return false;
}


protected func InitializePlayer(int plr)
{
	var clonk = GetCrew(plr, 0);
	clonk->SetPosition(230, 955);
	var effect = AddEffect("ClonkRestore", clonk, 100, 10);
	effect.var1 = 230;
	effect.var2 = 955;

	// Standard player zoom for tutorials, player is not allowed to zoom in/out.
	SetPlayerViewLock(plr, true);
	SetPlayerZoomByViewRange(plr, 400, nil, PLRZOOM_Direct | PLRZOOM_LimitMin | PLRZOOM_LimitMax);
	
	// Create tutorial guide, add messages, show first.
	guide = CreateTutorialGuide(plr);
	guide->AddGuideMessage("@$MsgTutIntro0$");
	guide->ShowGuideMessage(0);
	AddEffect("TutorialIntro1", nil, 100, 36 * 3);
	AddEffect("TutorialScale", nil, 100, 5);
	return;
}

/*-- Guide Messages --*/
// Finds when the Clonk has done 'X', and changes the message.

global func FxTutorialIntro1Stop()
{
	guide->AddGuideMessage("@$MsgTutIntro1$");
	guide->ShowGuideMessage(1);
	AddEffect("TutorialIntro2", nil, 100, 36 * 3);
	return 1;
}

global func FxTutorialIntro2Stop()
{
	guide->AddGuideMessage("@$MsgTutIntro2$");
	guide->ShowGuideMessage(2);
	AddEffect("TutorialIntro3", nil, 100, 36 * 10);
	return 1;
}

global func FxTutorialIntro3Stop()
{
	guide->AddGuideMessage("$MsgTutIntro3$");
	guide->ShowGuideMessage(3);
	guide->AddGuideMessage("$MsgTutMovement$");
	return 1;
}

global func FxTutorialScaleTimer(object target, effect, int timer)
{
	if(FindObject(Find_ID(Clonk), Find_InRect(650, 990, 140, 90)))
	{
		while (GetEffect("TutorialIntro*"))
			RemoveEffect("TutorialIntro*");
		guide->ClearGuideMessage();
		guide->AddGuideMessage("$MsgTutScale$");
		AddEffect("TutorialHangle", 0, 100, 18);
		return -1;
	}
}

global func FxTutorialHangleTimer(object target, effect, int timer)
{
	if(FindObject(Find_ID(Clonk), Find_InRect(820, 940, 190, 140)))
	{
		guide->AddGuideMessage("$MsgTutHangle$");
		AddEffect("TutorialSwim", 0, 100, 18);
		return -1;
	}
}

global func FxTutorialSwimTimer(object target, effect, int timer)
{
	if(FindObject(Find_ID(Clonk), Find_InRect(1120, 1030, 140, 60)))
	{
		guide->AddGuideMessage("$MsgTutSwim$");
		AddEffect("TutorialDig", 0, 100, 18);
		return -1;
	}
}

global func FxTutorialDigTimer(object target, effect, int timer)
{
	if(FindObject(Find_ID(Clonk), Find_InRect(1530, 1040, 130, 60)))
	{
		return -1;
	}
	return 1;
}

global func FxTutorialDigStop()
{
	guide->AddGuideMessage("$MsgTutDig$");
	return 1;
}

global func FxShovelGetTimer(object target, effect, int timer)
{
	if(target->Contained() != nil)
	{
		if (GetEffect("TutorialDig"))
			RemoveEffect("TutorialDig");
		guide->AddGuideMessage("$MsgTutTools$");
		AddEffect("TutorialChest", 0, 100, 18);
		return -1;
	}
}

global func FxTutorialChestTimer(object target, effect, int timer)
{
	if(FindObject(Find_ID(Clonk), Find_InRect(1750, 1030, 130, 80)))
	{
		guide->AddGuideMessage("$MsgTutChest$");
		return -1;
	}
}

global func FxLoamGetTimer(object target, effect, int timer)
{
	if(target->Contained()->GetID() != Chest)
	{
		guide->AddGuideMessage("$MsgTutLoam$");
		RemoveEffect("TutorialChest");
		AddEffect("TutorialFlint", 0, 100, 18);
		return -1;
	}
}

global func FxTutorialFlintTimer(object target, effect, int timer)
{
	if(FindObject(Find_ID(Clonk), Find_InRect(1990, 1020, 130, 90)))
	{
		guide->AddGuideMessage("$MsgTutFlint$");
		return -1;
	}
}

protected func OnGuideMessageShown(int plr, int index)
{
	// Show the player his clonk.
	if (index == 1)
		TutArrowShowTarget(GetCrew(GetPlayerByIndex()), 225, 24);
	// Show where the goal is located in the HUD.
	if (index == 2)
		TutArrowShowGUIPos(- 64 - 16 - GUI_Goal->GetDefHeight() / 2, 8 + GUI_Goal->GetDefHeight() / 2, 0, 40);
	// Show where the guide is located in the HUD.
	if (index == 3)	
		TutArrowShowGUIPos(- 128 - 32 - TutorialGuide->GetDefWidth() / 2, 8 + TutorialGuide->GetDefHeight() / 2, 0, 40);
	// Show wall to climb.
	if (index == 5)
		TutArrowShowPos(800, 1030, 90);		
	// Show ceiling to hangle.
	if (index == 6)
		TutArrowShowPos(1030, 1030, 45);		
	// Show shovel under water.
	if (index == 8)
		TutArrowShowTarget(FindObject(Find_ID(Shovel)), 225);
	// Show inventory slots.
	if (index == 9)
	{
		TutArrowShowGUIPos(248 + GUI_ObjectSelector->GetDefHeight() / 2, -16 - GUI_ObjectSelector->GetDefHeight() / 2, 180, 60);
		TutArrowShowGUIPos(341 + GUI_ObjectSelector->GetDefHeight() / 2, -16 - GUI_ObjectSelector->GetDefHeight() / 2, 180, 60);
	}
	return;
}

protected func OnGuideMessageRemoved(int plr, int index)
{
	TutArrowClear();
	return;
}

/*-- Clonk restoring --*/

global func FxClonkRestoreTimer(object target, effect, int time)
{
	// Respawn to new location if reached bow & arrow chest.
	if(FindObject(Find_ID(Clonk), Find_InRect(1120, 1030, 140, 60)))
	{
		effect.var1 = 1240;
		effect.var2 = 1070;		
	}
	// Respawn to new location if reached brick climb.
	if(FindObject(Find_ID(Clonk), Find_InRect(1990, 1020, 130, 90)))
	{
		effect.var1 = 2010;
		effect.var2 = 1020;		
	}
	return 1;
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
		var to_x = effect.var1;
		var to_y = effect.var2;
		// Respawn new clonk.
		var plr = target->GetOwner();
		var clonk = CreateObject(Clonk, 0, 0, plr);
		clonk->GrabObjectInfo(target);
		SetCursor(plr, clonk);
		clonk->DoEnergy(100000);
		restorer->SetRestoreObject(clonk, nil, to_x, to_y, "ClonkRestore");
	}
	return 1;
}