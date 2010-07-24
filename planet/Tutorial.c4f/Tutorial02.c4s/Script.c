/*-- 
		Tutorial 2
		Author: Maikel

		In this tutorial the player will be familiarized with crew selection, backpack control and some tools.
--*/

static pos_1, pos_2; // Respawn positions of either clonk.
static clonk_1, clonk_2; // Pointer to either clonk.
static grapple_1, grapple_2; // Progress of either clonk.
static guide; // guide object.

protected func Initialize()
{
	// Create goal, all crew members should reach the flag on the far right side.
	var goal = CreateObject(Goal_ReachFlag, 0, 0, NO_OWNER);
	goal->CreateGoalFlag(2970, 260);
	AddEffect("HasCompletedGoal", 0, 100, 10);
		
	// Respawn positions.
	pos_1 = [190, 220];
	pos_2 = [100, 430];
	
	// Create all objects, vehicles, chests used by the player.
	var cannon, effect, firestone, chest, powderkeg, ropeladder, grapple, cata, dynamite;
	
	// Cannon to blast through rock & chest with powderkeg and firestones.
	cannon = CreateObject(Cannon, 180, 450, NO_OWNER); 
	effect = AddEffect("CannonRestore", cannon, 100, 35);
	EffectVar(1, cannon, effect) = 180;
	EffectVar(2, cannon, effect) = 450;
	chest = CreateObject(Chest, 80, 430, NO_OWNER);
	for (var i = 0; i < 4; i++)
	{
		firestone = CreateObject(Firestone, 0, 0, NO_OWNER);
		firestone->Enter(chest);
		firestone->AddRestoreMode(chest);
	}
	powderkeg = CreateObject(PowderKeg, 0, 0, NO_OWNER);
	powderkeg->Enter(chest);
	powderkeg->AddRestoreMode(chest);
	
	// Chest with ropeladder.
	chest = CreateObject(Chest, 790, 220, NO_OWNER);
	for (var i = 0; i < 2; i++) 
	{
		ropeladder = CreateObject(Ropeladder, 0, 0, NO_OWNER);
		ropeladder->Enter(chest);
		effect = AddEffect("RopeladderRestore", ropeladder, 100, 35);
		EffectVar(0, ropeladder, effect) = chest;
	}
	
	// Chest with GrappleBow.
	chest = CreateObject(Chest, 580, 440, NO_OWNER);
	grapple = CreateObject(GrappleBow, 0, 0, NO_OWNER); //TODO better restore.
	grapple->Enter(chest);
	grapple->AddRestoreMode(chest);
	
	// Catapult to return grappling bow. TODO: implement catapult here.
	cata = CreateObject(Cannon, 1320, 440, NO_OWNER); 
	effect = AddEffect("CataRestore", cata, 100, 35);
	EffectVar(1, cata, effect) = 1320;
	EffectVar(2, cata, effect) = 440;
	powderkeg = CreateObject(PowderKeg, 0, 0, NO_OWNER); 
	powderkeg->Enter(cata);
	powderkeg->AddRestoreMode(cata);
	
	// Chest with dynamite supplies.
	chest = CreateObject(Chest, 1400, 540, NO_OWNER);
	for (var i = 0; i < 2; i++)
	{
		dynamite = CreateObject(DynamiteBox, 0, 0, NO_OWNER);
		dynamite->Enter(chest);
		effect = AddEffect("DynamiteRestore", dynamite, 100, 35);
		EffectVar(0, dynamite, effect) = chest;
	}
	
	// Chest with flints to blast underwater rocks.
	chest = CreateObject(Chest, 1590, 100, NO_OWNER);
	for (var i = 0; i < 3; i++)
	{
		firestone = CreateObject(Firestone, 0, 0, NO_OWNER);
		firestone->Enter(chest);
		firestone->AddRestoreMode(chest);
	}
	
	// Chest with Grapplebows for the final leap.
	chest = CreateObject(Chest, 1800, 540, NO_OWNER);
	for (var i = 0; i < 2; i++)
	{
		grapple = CreateObject(GrappleBow, 0, 0, NO_OWNER); //TODO better restore.
		grapple->Enter(chest);
		grapple->AddRestoreMode(chest);
	}
	
	// Create brick edges.
	var x = [30,40,50,60,140,150,220,270,30,40,50,240,260,310,380,390,400,410,420,430,980,990,1000,1010,1020,1030,1070,1080,1090,1100,1110,1120,1260,1280,1300,1500,1520,2260,2270,2280,2320,2330,2340,2480,2490,2500,2540,2550,2560,2680,2690,2700,2740,2750,2760];
	var y = [420,430,440,450,460,470,470,470,230,240,250,250,240,240,330,320,310,300,290,280,50,60,70,80,90,100,100,90,80,70,60,50,430,440,450,380,380,80,90,100,100,90,80,80,90,100,100,90,80,80,90,100,100,90,80];
	for(var i = 0; i < GetLength(x); i++)
		CreateObject(BrickEdge, x[i], y[i], NO_OWNER);
	
	// Scriptcounter
	ScriptGo(true);
	
	// Set the mood.
	SetGamma(RGB(30, 25, 20), RGB(135, 130, 125), RGB(255, 250, 245));
	SetSkyParallax(0, 20, 20);
	
	// Dialogue options -> repeat round.
	SetNextMission("Tutorial.c4f\\Tutorial02.c4s", "$MsgRepeatRound$", "$MsgRepeatRoundDesc$");
	return;
}

protected func InitializePlayer(int plr)
{
	// First clonk.
	clonk_1 = GetCrew(plr, 0);
	clonk_1->SetPosition(pos_1[0], pos_1[1]);
	AddEffect("IntClonkOne", clonk_1, 100, 10);
	
	// Second clonk.
	clonk_2 = GetCrew(plr, 1);
	clonk_2->SetPosition(pos_2[0], pos_2[1]);
	AddEffect("IntClonkTwo", clonk_2, 100, 10);
	
	// Professor
	guide = CreateTutorialGuide(plr);
	return;
}

protected func OnClonkDeath(object clonk)
{
	// Respawn new clonk.
	var plr = clonk->GetOwner();
	var new_clonk = CreateObject(Clonk, 0, 0, plr);
	new_clonk->GrabObjectInfo(clonk);
	SetCursor(plr, new_clonk);
	UnselectCrew(plr);
	SelectCrew(plr, new_clonk, true);
	new_clonk->DoEnergy(100000);
	// Set position.
	if (clonk == clonk_1)
	{
		clonk_1 = new_clonk;
		clonk_1->SetPosition(pos_1[0], pos_1[1]);
		AddEffect("IntClonkOne", clonk_1, 100, 10);
	}
	else if (clonk == clonk_2)
	{
		clonk_2 = new_clonk;
		clonk_2->SetPosition(pos_2[0], pos_2[1]);
		AddEffect("IntClonkTwo", clonk_2, 100, 10);
	}
	return;
}


/*-- Effects --*/

global func FxIntLorryFillTimer(object target)
{
	// Refill when objects are taken (3 firestones and 1 dynamite box).
	if (ObjectCount(Find_ID(Firestone), Find_Container(target)) < 3)
		target->CreateContents(Firestone);	
	if (ObjectCount(Find_ID(DynamiteBox), Find_Container(target)) < 1)
		target->CreateContents(DynamiteBox);	
	return;
}

global func FxIntClonkOneTimer(object target)
{
	return;
}

global func FxIntClonkTwoTimer(object target)
{
	return;
}

/*-- Messages --*/

// Welcome player, indicate goal.
func Script1()
{
	guide->SetGuideMessage("$MsgTutWelcome$");
	guide->SetGuideGraphics(GUIDE_Graphics_Normal);
	guide->ShowGuideMessage();
	AddEffect("CrewSelection", 0, 100, 35);
}

// Player has wandered around enough, explain crew selection.
global func FxCrewSelectionTimer(object trg, int num, int time)
{
	if (time >= 35 * 8)
	{	
		guide->SetGuideMessage("$MsgTutCrewSelection$");
		guide->SetGuideGraphics(GUIDE_Graphics_Balloon, Clonk);
		return -1;
	}
}

// Player mastered crew selection, explain backpack control.
func OnClonkSelection()
{
	if (FrameCounter() == 0)
		return;
	guide->SetGuideMessage("$MsgTutBackpack$");
	guide->SetGuideGraphics(GUIDE_Graphics_Balloon);
	AddEffect("ReachedMine", 0, 100, 10);
}

// Player has reached abandoned gold mine.
global func FxReachedMineTimer(object trg, int num, int time)
{
	if (FindObject(Find_OCF(OCF_CrewMember), Find_Distance(30, 200, 560)))
	{
		guide->SetGuideMessage("$MsgTutLorry$");
		guide->SetGuideGraphics(GUIDE_Graphics_Balloon, Lorry);
		AddEffect("BlastedRock", 0, 100, 10);
		return -1;	
	}
}

// Player has blasted first rock, explain rope ladder.
global func FxBlastedRockTimer(object trg, int num, int time)
{
	if (GetPathLength(440, 110, 520, 110) > 0)
	{
		guide->SetGuideMessage("$MsgTutRopeladder$");
		guide->SetGuideGraphics(GUIDE_Graphics_Balloon, Ropeladder);
		AddEffect("PlacedLadder", 0, 100, 10);
		return -1;
	}
}

// Player has placed rope ladder, explain dynamite box.
global func FxPlacedLadderTimer(object trg, int num, int time)
{
	if (FindObject(Find_Func("IsLadder"), Find_Distance(40, 425, 185)))
	{
		guide->SetGuideMessage("$MsgTutDynamiteBox$");
		guide->SetGuideGraphics(GUIDE_Graphics_Balloon, DynamiteBox);
		AddEffect("BlastedRock2", 0, 100, 10);
		return -1;
	}
}

// Player has blasted second rock, explain grapple bow.
global func FxBlastedRock2Timer(object trg, int num, int time)
{
	if (GetPathLength(620, 290, 680, 350) > 0)
	{
		guide->SetGuideMessage("$MsgTutGrappleBow$");
		guide->SetGuideGraphics(GUIDE_Graphics_Balloon, GrappleBow);
		return -1;		
	}
}

// Checks whether the goal has been reached.
global func FxHasCompletedGoalTimer(object trg, int num, int time)
{
	if (ObjectCount(Find_OCF(OCF_CrewMember), Find_Distance(60, 2970, 250)) == 2)
	{
		// Dialogue options -> next round.
		SetNextMission("Tutorial.c4f\\Tutorial03.c4s", "$MsgNextTutorial$", "$MsgNextTutorialDesc$");
		// Fulfill goal.
		var goal = FindObject(Find_ID(Goal_Script));
		if (goal)
			goal->Fulfill();		
	}	
	return;
}

/*-- Item restoring --*/

// Dynamite box, needs seperate effect since changedef call.
global func FxDynamiteRestoreStop(object target, int num, int reason, bool  temporary)
{
	if (reason == 3)
	{
		var reviver = CreateObject(ItemRestorer, 0, 0, NO_OWNER);
		var x = BoundBy(target->GetX(), 0, LandscapeWidth());
		var y = BoundBy(target->GetY(), 0, LandscapeHeight());
		reviver->SetPosition(x, y);
		var to_container = EffectVar(0, target, num);
		var revived = CreateObject(DynamiteBox, 0, 0, target->GetOwner());
		reviver->SetRestoreObject(revived, to_container, nil, nil, "DynamiteRestore");	
	}
	return 1;
}

// Dynamite box, times is always needed.
global func FxDynamiteRestoreTimer()
{
	return 1;
}

// Cannon, restore position if pushed to far to the right.
global func FxCannonRestoreTimer(object target, int num, int time)
{
	if ((target->GetX() > 300 || target->GetY() > 500) && !target->Contained())
	{
		var reviver = CreateObject(ItemRestorer, 0, 0, NO_OWNER);
		var x = BoundBy(target->GetX(), 0, LandscapeWidth());
		var y = BoundBy(target->GetY(), 0, LandscapeHeight());
		reviver->SetPosition(x, y);
		var to_x = EffectVar(1, target, num);
		var to_y = EffectVar(2, target, num);
		reviver->SetRestoreObject(target, nil, to_x, to_y, "CannonRestore");
	}
	return 1;
}

// Catapult, restore position if pushed to far to the left or right.
global func FxCataRestoreTimer(object target, int num, int time)
{
	if ((target->GetX() < 1110 || target->GetX() > 1650 || target->GetY() > 460) && !target->Contained())
	{
		var reviver = CreateObject(ItemRestorer, 0, 0, NO_OWNER);
		var x = BoundBy(target->GetX(), 0, LandscapeWidth());
		var y = BoundBy(target->GetY(), 0, LandscapeHeight());
		reviver->SetPosition(x, y);
		var to_x = EffectVar(1, target, num);
		var to_y = EffectVar(2, target, num);
		reviver->SetRestoreObject(target, nil, to_x, to_y, "CataRestore");
	}
	return 1;
}

// Catapult, might be dropped within 1100 X-coordinate.
global func FxCataRestoreStop(object target, int num, int reason, bool  temporary)
{
	if (reason == 3)
	{
		var reviver = CreateObject(ItemRestorer, 0, 0, NO_OWNER);
		var x = BoundBy(target->GetX(), 0, LandscapeWidth());
		var y = BoundBy(target->GetY(), 0, LandscapeHeight());
		reviver->SetPosition(x, y);
		var to_x = EffectVar(1, target, num);
		var to_y = EffectVar(2, target, num);
		reviver->SetRestoreObject(target, nil, to_x, to_y, "CataRestore");
	}
	return 1;
}

// Ropeladder, restore if thrown away to unreachable location.
global func FxRopeladderRestoreTimer(object target, int num, int time)
{
	if (target->GetX() < 680 && target->GetY() > 340 && !target->Contained())
	{
		var reviver = CreateObject(ItemRestorer, 0, 0, NO_OWNER);
		var x = BoundBy(target->GetX(), 0, LandscapeWidth());
		var y = BoundBy(target->GetY(), 0, LandscapeHeight());
		reviver->SetPosition(x, y);
		var to_container = EffectVar(0, target, num);
		reviver->SetRestoreObject(target, to_container, nil, nil, "RopeladderRestore");
	}
	return 1;
}

// Ropeladder, restore if destroyed.
global func FxRopeladderRestoreStop(object target, int num, int reason, bool  temporary)
{
	if (reason == 3)
	{
		var reviver = CreateObject(ItemRestorer, 0, 0, NO_OWNER);
		var x = BoundBy(target->GetX(), 0, LandscapeWidth());
		var y = BoundBy(target->GetY(), 0, LandscapeHeight());
		reviver->SetPosition(x, y);
		var to_container = EffectVar(0, target, num);
		var revived = CreateObject(Ropeladder, 0, 0, target->GetOwner());
		reviver->SetRestoreObject(revived, to_container, nil, nil, "RopeladderRestore");	
	}
	return 1;
}
