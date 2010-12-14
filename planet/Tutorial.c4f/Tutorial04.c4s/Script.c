/*-- 
	Tutorial 04
	Author: Maikel

	In this tutorial the player will be familiarized with some melee weapons.
--*/


static guide; // guide object.

protected func Initialize()
{
	// Environment 
	PlaceGrass(85);
	CreateObject(Environment_Clouds);
	CreateObject(Environment_Celestial);
	var time = CreateObject(Environment_Time);
	time->SetTime(1125);
	time->SetCycleSpeed(0);
	Sound("WindLoop.ogg", true, 40, nil, 1);
	
	// Goal: Melee, all opponents must be killed.
	CreateObject(Goal_Melee, 0, 0, NO_OWNER);
	
	// First section: Some straw targets to be struck with the sword.
	CreateObject(SwordTarget, 190, 671, NO_OWNER)->SetR(RandomX(-10, 10));
	CreateObject(SwordTarget, 280, 620, NO_OWNER)->SetR(RandomX(-10, 10) + 180);
	CreateObject(SwordTarget, 340, 648, NO_OWNER)->SetR(RandomX(-10, 10));
	CreateObject(SwordTarget, 430, 600, NO_OWNER)->SetR(RandomX(-10, 10) + 180);
	// Gate that opens if all targets have been destroyed.
	var gate = CreateObject(StoneDoor, 556, 640, NO_OWNER);
	AddEffect("IntOpenGate", gate, 100, 5);
	
	// Script player as opponent.
	SetMaxPlayer(2);
	CreateScriptPlayer("Opponent", 0x00ff00, 0, CSPF_FixedAttributes | CSPF_NoScenarioInit, AI_Opponent);
	
	// Second section: Weak opponent with javelins.
	var spearman1 = CreateObject(Clonk, 1050, 560, NO_OWNER);
	spearman1->SetMaxEnergy(40);
	spearman1->CreateContents(Javelin);
	spearman1->AI_GuardArea(800, 400, 400, 250);
	AddEffect("IntContentRemoval", spearman1, 100, 0);
	CreateObject(EnergyBar)->SetTarget(spearman1);
	// Gate that can be opened with a spin wheel.
	var gate = CreateObject(StoneDoor, 1216, 550, NO_OWNER);
	var wheel = CreateObject(SpinWheel, 1140, 560, NO_OWNER);
	wheel->SetStoneDoor(gate);
	
	// Third section: Two opponents in a tower.
	// Lower part: a weak spearman.
	var spearman2 = CreateObject(Clonk, 1753, 410, NO_OWNER);
	spearman2->SetMaxEnergy(40);
	spearman2->CreateContents(Javelin);
	spearman2->AI_GuardArea(1350, 200, 500, 400);
	AddEffect("IntContentRemoval", spearman2, 100, 0);
	CreateObject(EnergyBar)->SetTarget(spearman2);
	// Upper part: a normal bowman.
	var bowman = CreateObject(Clonk, 1732, 330, NO_OWNER);
	bowman->SetMaxEnergy(45);
	bowman->CreateContents(Bow)->CreateContents(Arrow);
	bowman->AI_GuardArea(1350, 200, 500, 400);
	AddEffect("IntContentRemoval", bowman, 100, 0);
	CreateObject(EnergyBar)->SetTarget(bowman);
	// Gate that can be opened with a spin wheel.
	var gate = CreateObject(StoneDoor, 1856, 500, NO_OWNER);
	var wheel = CreateObject(SpinWheel, 1782, 341, NO_OWNER);
	wheel->SetStoneDoor(gate);
	
	// Fourth section: Opponent with sword and shield.
	var swordman = CreateObject(Clonk, 2250, 330, NO_OWNER);
	swordman->SetMaxEnergy(60);
	swordman->CreateContents(Shield);
	swordman->CreateContents(Sword);
	swordman->AI_GuardArea(2050, 300, 300, 100);
	AddEffect("IntContentRemoval", swordman, 100, 0);
	CreateObject(EnergyBar)->SetTarget(swordman);
	// Chest with some extra weapons.
	var chest = CreateObject(Chest, 2260, 620, NO_OWNER);
	chest->CreateContents(Club);
	
	// Brick edges.
	var edges = [[620,640],[630,630],[540,560],[530,550],[530,480],[520,470],[1160,570],[1170,560],[1830,450],[1840,440],[1850,430],[1830,370],[1800,360]];
	for(var i = 0; i < GetLength(edges); i++)
		CreateObject(BrickEdge, edges[i][0], edges[i][1], NO_OWNER)->PermaEdge();

	
	// Dialogue options -> repeat round.
	SetNextMission("Tutorial.c4f\\Tutorial04.c4s", "$MsgRepeatRound$", "$MsgRepeatRoundDesc$");
	return;
}

// Gamecall from goals, set next mission.
protected func OnGoalsFulfilled()
{
	// Dialogue options -> next round.
	// Uncomment if there is a 5th tutorial.
	// SetNextMission("Tutorial.c4f\\Tutorial05.c4s", "$MsgNextTutorial$", "$MsgNextTutorialDesc$"); 
	// Normal scenario ending by goal library.
	return false;
}

protected func InitializePlayer(int plr)
{
	// Standard player zoom for tutorials.
	SetPlayerViewLock(plr, true);
	SetPlayerZoomByViewRange(plr, 600, nil, PLRZOOM_Direct);
	
	// Clonk to position and add restore effect.
	var clonk = GetCrew(plr, 0);
	clonk->SetPosition(30, 620);
	var effect = AddEffect("ClonkRestore", clonk, 100, 10);
	EffectVar(1, clonk, effect) = 30;
	EffectVar(2, clonk, effect) = 620;
	// Clonk starts with sword and shield.
	clonk->CreateContents(Sword);
	clonk->CreateContents(Shield);

	// Create tutorial guide, add messages, show first.
	guide = CreateTutorialGuide(plr);
	guide->AddGuideMessage("$MsgTutWelcome$");
	guide->ShowGuideMessage(0);
	AddEffect("TutorialStrawTargets", nil, 100, 5);
	return;
}

// Opens the gate if all sword targets are destroyed.
global func FxIntOpenGateTimer(object target)
{
	if (ObjectCount(Find_ID(SwordTarget)) == 0)
	{
		// Open gate.
		target->OpenGateDoor();
		return -1;
	}
	return 1;
}

/*-- Guide control --*/

global func FxTutorialStrawTargetsTimer()
{
	if (ObjectCount(Find_ID(SwordTarget)) == 0)
	{
		guide->AddGuideMessage("$MsgTutGateOpened$");
		AddEffect("TutorialReachedSecondSection", nil, 100, 5);
		return -1;
	}
	return 1;
}

global func FxTutorialReachedSecondSectionTimer()
{
	if (FindObject(Find_OCF(OCF_CrewMember), Find_Distance(40, 635, 450)))
	{
		guide->AddGuideMessage("$MsgTutFirstEnemy$");
		AddEffect("TutorialCompletedSecondSection", nil, 100, 5);
		return -1;
	}
	return 1;
}

global func FxTutorialCompletedSecondSectionTimer()
{
	var opponent = FindObject(Find_OCF(OCF_CrewMember), Find_InRect(800, 400, 400, 250), Find_Not(Find_Owner(0)));
	var crew = FindObject(Find_OCF(OCF_CrewMember), Find_Owner(0));
	if (!opponent || (crew && crew->GetX() > 1220))
	{
		guide->AddGuideMessage("$MsgTutOpenGate$");
		AddEffect("TutorialReachedThirdSection", nil, 100, 5);
		return -1;
	}
	return 1;
}

global func FxTutorialReachedThirdSectionTimer()
{
	if (FindObject(Find_OCF(OCF_CrewMember), Find_Distance(40, 1370, 545)))
	{
		guide->AddGuideMessage("$MsgTutTwoEnemies$");
		AddEffect("TutorialCompletedThirdSection", nil, 100, 5);
		return -1;
	}
	return 1;
}

global func FxTutorialCompletedThirdSectionTimer()
{
	var opponent = FindObject(Find_OCF(OCF_CrewMember), Find_InRect(1350, 200, 500, 400), Find_Not(Find_Owner(0)));
	var crew = FindObject(Find_OCF(OCF_CrewMember), Find_Owner(0));
	if (!opponent || (crew && crew->GetX() > 1860 && crew->GetY() > 460))
	{
		guide->AddGuideMessage("$MsgTutMoveOn$");
		AddEffect("TutorialReachedFourthSection", nil, 100, 5);
		return -1;
	}
	return 1;
}

global func FxTutorialReachedFourthSectionTimer()
{
	if (FindObject(Find_OCF(OCF_CrewMember), Find_Distance(40, 2200, 420)))
	{
		guide->AddGuideMessage("$MsgTutLastEnemy$");
		return -1;
	}
	return 1;
}

protected func OnGuideMessageShown(int plr, int index)
{
	// Show first four targets with the arrow.
	if (index == 0)
		for (target in FindObjects(Find_ID(SwordTarget)))
		{
			var angle = RandomX(-45, 45);
			if (target->GetY() > 620)
				angle = RandomX(135, 225);
			TutArrowShowTarget(target, angle, 24);
		}
	// Show way through gate.
	if (index == 1)
	{
		TutArrowShowPos(570, 625, 90);
		TutArrowShowPos(610, 540, 315);
	}
	// Show opponent in second section.
	if (index == 2)
	{
		var target = FindObject(Find_OCF(OCF_CrewMember), Find_InRect(800, 400, 400, 250), Find_Not(Find_Owner(0)));
		if (target)
			TutArrowShowTarget(target);		
	}
	// Show spin wheel.
	if (index == 3)
	{
		var wheel = FindObject(Find_ID(SpinWheel), Find_Distance(40, 1140, 560));
		if (wheel)
			TutArrowShowTarget(wheel);	
	}
	// Show two opponents in third section.
	if (index == 4)
	{
		for (var target in FindObjects(Find_OCF(OCF_CrewMember), Find_InRect(1350, 200, 500, 400), Find_Not(Find_Owner(0))))
			if (target)
				TutArrowShowTarget(target);		
	}
	// Show spin wheel and way to last opponent.
	if (index == 5)
	{
		var wheel = FindObject(Find_ID(SpinWheel), Find_Distance(40, 1782, 341));
		if (wheel)
			TutArrowShowTarget(wheel);	
		TutArrowShowPos(1970, 490, 90);
	}
	// Show opponent in fourth section.
	if (index == 6)
	{
		var target = FindObject(Find_OCF(OCF_CrewMember), Find_InRect(1900, 250, 400, 200), Find_Not(Find_Owner(0)));
		if (target)
			TutArrowShowTarget(target);		
	}
	return;
}

protected func OnGuideMessageRemoved(int plr, int index)
{
	TutArrowClear();
	return;
}

/*-- Clonk restoring --*/

global func FxClonkRestoreTimer(object target, int num, int time)
{
	// Respawn to new location if reached second section.
	if (Distance(target->GetX(), target->GetY(), 635, 450) < 40)
	{
		EffectVar(1, target, num) = 635;
		EffectVar(2, target, num) = 450;		
	}
	// Respawn to new location if reached third section.
	if (Distance(target->GetX(), target->GetY(), 1370, 545) < 40)
	{
		EffectVar(1, target, num) = 1370;
		EffectVar(2, target, num) = 545;		
	}
	// Respawn to new location if reached fourth section.
	if (Distance(target->GetX(), target->GetY(), 1910, 485) < 40)
	{
		EffectVar(1, target, num) = 1910;
		EffectVar(2, target, num) = 485;		
	}
	return 1;
}

// Relaunches the clonk, from death or removal.
global func FxClonkRestoreStop(object target, int num, int reason, bool  temporary)
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
		SetCursor(plr, clonk);
		clonk->DoEnergy(100000);
		// Transfer contents.
		for (var transfer in FindObjects(Find_Container(target)))
			transfer->Enter(clonk);
		restorer->SetRestoreObject(clonk, nil, to_x, to_y, "ClonkRestore");
	}
	return 1;
}

/*-- Item restoring --*/

// Removes content on death.
global func FxIntContentRemovalStop(object target, int num, int reason)
{
	if (reason != 4)
		return 1;
	for (var obj in FindObjects(Find_Container(target)))
			obj->RemoveObject();
	return 1;
}

