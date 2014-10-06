/*--
	Tutorial 03
	Author: Ringwall

	In this tutorial the player will be familiarized with some ranged weapons.
--*/


static guide; // guide object.

protected func Initialize()
{
	// Environment
	PlaceGrass(85);
	
	// Goal: flag goal also checks if all targets are destroyed.
	var goal = CreateObject(Goal_ReachFlag, 0, 0, NO_OWNER);
	goal->CreateGoalFlag(2230, 292);

	// A chest with javelins.
	var chest = CreateObject(Chest, 240, 656, NO_OWNER);
	var javelin = CreateObject(Javelin, 0, 0, NO_OWNER);
	javelin->Enter(chest);
	//javelin->AddRestoreMode(chest);
	
	// A chest with bow & arrows.
	var chest = CreateObject(Chest, 785, 560, NO_OWNER);
	var bow = CreateObject(Bow, 0, 0, NO_OWNER);
	bow->CreateContents(Arrow);
	bow->Enter(chest);
	//bow->AddRestoreMode(chest);

	// Create practice targets.
	var target;
	// Two flying targets above the chest.
	MakeTarget(280, 580, true);
	MakeTarget(180, 560, true);
	// A static target which opens the sand barrier.
	var target = MakeTarget(410, 580, false);
	AddEffect("Blast", target, 1, 0, target);
	// A moving target.
	var target = MakeTarget(380, 300, true)->GetActionTarget();
	AddEffect("HorizontalMoving", target, 1, 1, target);
	// A flying target which drops a flint.
	var target = MakeTarget(690, 400, true);
	AddEffect("FlintDrop", target, 1, 0, target);
	// A moving and a static target.
	var target = MakeTarget(880, 520, true)->GetActionTarget();
	AddEffect("HorizontalMoving", target, 1, 1, target);
	MakeTarget(1250, 450, true);
	// A flying target dropping a flint.
	var target = MakeTarget(1367, 300, true);
	AddEffect("FlintDrop", target, 1, 0, target);
	// Three flying targets and a moving target.
	MakeTarget(1660, 450, true);
	var target = MakeTarget(1560, 320, true)->GetActionTarget();
	AddEffect("HorizontalMoving", target, 1, 1, target);
	MakeTarget(1710, 230, true);
	MakeTarget(1800, 260, true);
	// The final target, creates a ropeladder.
	var target = MakeTarget(2140, 250, true);
	AddEffect("Ropeladder", target, 1, 0, target);

	// Dialogue options -> repeat round.
	SetNextMission("Tutorial.ocf\\Tutorial03.ocs", "$MsgRepeatRound$", "$MsgRepeatRoundDesc$");
	return;
}

// Gamecall from goals, set next mission.
protected func OnGoalsFulfilled()
{
	// Achievement star
	GainScenarioAchievement("Done");
	// Dialogue options -> next round.
	 SetNextMission("Tutorial.ocf\\Tutorial04.ocs", "$MsgNextTutorial$", "$MsgNextTutorialDesc$");
	// Normal scenario ending by goal library.
	return false;
}

protected func InitializePlayer(int plr)
{
	// Two times standard player zoom for tutorials, since targets can be distant.
	SetPlayerViewLock(plr, true);
	SetPlayerZoomByViewRange(plr, 800, nil, PLRZOOM_Direct);
	
	// Clonk to position and add restore effect.
	var clonk = GetCrew(plr, 0);
	clonk->SetPosition(30, 620);
	var effect = AddEffect("ClonkRestore", clonk, 100, 10);
	effect.var1 = 30;
	effect.var2 = 620;

	// Create tutorial guide, add messages, show first.
	guide = CreateTutorialGuide(plr);
	guide->AddGuideMessage("$MsgTutWelcome$");
	guide->ShowGuideMessage(0);
	AddEffect("TutorialJavelin", nil, 100, 36 * 5);
	return;
}

/*-- Guide control --*/

global func FxTutorialJavelinStop()
{
	guide->AddGuideMessage("$MsgTutJavelin$");
	AddEffect("TutorialHasJavelin", nil, 100, 5);
	return 1;
}

global func FxTutorialHasJavelinTimer()
{
	var clonk = FindObject(Find_OCF(OCF_CrewMember));
	if (!clonk)
		return 1;
	if (FindObject(Find_Container(clonk), Find_ID(Javelin)))
	{
		guide->AddGuideMessage("$MsgTutJumpThrow$");
		AddEffect("TutorialBow", nil, 100, 5);
		return -1;
	}
	return 1;
}

global func FxTutorialBowTimer()
{
	if (FindObject(Find_OCF(OCF_CrewMember), Find_InRect(680, 500, 100, 100)))
	{
		guide->AddGuideMessage("$MsgTutBow$");
		AddEffect("TutorialHighBalloon", nil, 100, 5);
		return -1;
	}
	return 1;
}

global func FxTutorialHighBalloonTimer()
{
	if (FindObject(Find_OCF(OCF_CrewMember), Find_InRect(1200, 400, 150, 150)))
	{
		guide->AddGuideMessage("$MsgTutHighBalloon$");
		return -1;
	}
	return 1;
}

protected func OnGuideMessageShown(int plr, int index)
{
	// Show first three targets with the arrow.
	if (index == 0)
		for (var target in FindObjects(Find_ID(PracticeTarget), Find_InRect(100, 450, 350, 150)))
			TutArrowShowTarget(target, RandomX(-45, 45), 24);
	// Show javelin chest with an arrow.
	if (index == 1)
		TutArrowShowPos(240, 650);
	// Show bow chest with an arrow.
	if (index == 3)
		TutArrowShowPos(785, 550);
	// Show high balloon.
	if (index == 4)
	{
		var target = FindObject(Find_ID(PracticeTarget), Find_Distance(25, 1364, 300));
		if (target)
			TutArrowShowTarget(target, RandomX(-45, 45), 24);
	}
	return;
}

protected func OnGuideMessageRemoved(int plr, int index)
{
	TutArrowClear();
	return;
}

/*-- Target control --*/

private func MakeTarget(int x, int y, bool flying)
{
	var target = CreateObject(PracticeTarget, x, y, NO_OWNER);
	if (flying)
	{
		var balloon = CreateObject(TargetBalloon, x, y-30, NO_OWNER);
		target->SetAction("Attach", balloon);
		CreateParticle("Flash", x, y - 50, 0, 0, 8, Particles_Flash());
	}
	else
	{
		CreateParticle("Flash", x, y, 0, 0, 8, Particles_Flash());
		target->SetAction("Float");
	}
	return target;
}

// Blasts the first sand barrier on destruction.
global func FxBlastStop(object target, effect, int reason, bool temporary)
{
	CreateObject(Rock, AbsX(430), AbsY(618), NO_OWNER)->Explode(25);
	return 1;
}

// Creates a ropeladder on destruction to reach the final edge.
global func FxRopeladderStop(object target, effect, int reason, bool temporary)
{
	CreateObject(Ropeladder, AbsX(2140), AbsY(320), NO_OWNER)->Unroll(-1);
	return 1;
}

// Target moves horizontal.
global func FxHorizontalMovingTimer(object target, effect, int time)
{
	target->SetXDir(Sin(time, 20));
	return 1;
}

// Drops a firestone on destruction.
global func FxFlintDropStop(object target, effect, int reason, bool temporary)
{
	CreateObject(Firestone, 0, 0, NO_OWNER);
	return 1;
}

/*-- Clonk restoring --*/

global func FxClonkRestoreTimer(object target, effect, int time)
{
	// Respawn to new location if reached bow & arrow chest.
	if (Distance(target->GetX(), target->GetY(), 830, 560) < 40)
	{
		effect.var1 = 830;
		effect.var2 = 560;		
	}
	// Respawn to new location if reached brick climb.
	if (Distance(target->GetX(), target->GetY(), 1490, 470) < 40)
	{
		effect.var1 = 1490;
		effect.var2 = 470;		
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
		// Transfer contents.
		var transfer, index = target->ContentsCount();
		while (transfer = target->Contents(--index))
			transfer->Enter(clonk);
		restorer->SetRestoreObject(clonk, nil, to_x, to_y, 0, "ClonkRestore");
	}
	return 1;
}

/*-- Item restoring --*/
// TODO: Is done in WeaponFade.c currently.
