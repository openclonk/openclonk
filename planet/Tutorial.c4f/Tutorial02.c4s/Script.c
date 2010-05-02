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
	// Goal
	var goal = FindObject(Find_ID(Goal_Script));
	if (!goal) 
		goal = CreateObject(Goal_Script);
	AddEffect("HasCompletedGoal", 0, 100, 10);
		
	// Respawn positions.
	pos_1 = [150, 358];
	pos_2 = [565, 110];
	
	// Abandoned mine: Lorry with firestones and dynamite.
	var lorry = CreateObject(Lorry, 200, 560, NO_OWNER);
	lorry->CreateContents(Firestone, 3);
	lorry->CreateContents(DynamiteBox);
	AddEffect("IntLorryFill", lorry, 100, 70);
	// Abandoned mine: Scenery.
	var pickaxe;
	var pickaxe = CreateObject(Pickaxe, 133, 546, NO_OWNER);
	pickaxe->SetObjectLayer(pickaxe);
	pickaxe->SetR(-60);
	var pickaxe = CreateObject(Pickaxe, 177, 530, NO_OWNER);
	pickaxe->SetObjectLayer(pickaxe);
	pickaxe->SetR(70);
	var pickaxe = CreateObject(Pickaxe, 254, 538, NO_OWNER);
	pickaxe->SetObjectLayer(pickaxe);
	pickaxe->SetR(-10);
	for (var i = 0; i < 6; i++)
	{
		var gold = CreateObject(Gold, 140 + Random(120), 550, NO_OWNER);
		gold->SetObjectLayer(gold);
		gold->SetR(Random(360));	
	}
	
	// Dynamite(FIXME) or catapult to blast through rock.
	var dyn1 = CreateObject(Dynamite, 480, 115, NO_OWNER);
	var dyn2 = CreateObject(Dynamite, 485, 125, NO_OWNER);
	var dyn3 = CreateObject(Dynamite, 480, 135, NO_OWNER);
	var dyn4 = CreateObject(Dynamite, 485, 145, NO_OWNER);
	CreateObject(Fuse, 480, 115, NO_OWNER)->Connect(dyn1, dyn2);
	CreateObject(Fuse, 485, 125, NO_OWNER)->Connect(dyn2, dyn3);
	CreateObject(Fuse, 480, 135, NO_OWNER)->Connect(dyn3, dyn4);
	var igniter = CreateObject(Igniter, 410, 235, NO_OWNER);
	CreateObject(Fuse, 485, 145, NO_OWNER)->Connect(dyn4, igniter);
	igniter->SetGraphics("0", Fuse, 1, GFXOV_MODE_Picture);
	
	// Scriptcounter
	ScriptGo(true);
	
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
	// Shovel to backpack.
	clonk_1->Switch2Items(0, 2);
	
	// Second clonk.
	clonk_2 = GetCrew(plr, 1);
	clonk_2->SetPosition(pos_2[0], pos_2[1]);
	AddEffect("IntClonkTwo", clonk_2, 100, 10);
	// Remove shovel, create ropeladders.
	if (clonk_2->Contents())
		clonk_2->Contents()->RemoveObject();
	clonk_2->CreateContents(Ropeladder, 2);	
	
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
	SelectCrew(plr, new_clonk, true);
	new_clonk->DoEnergy(100000);
	// Remove shovel.
	if (new_clonk->Contents())
		new_clonk->Contents()->RemoveObject();
	// Set position.
	if (clonk == clonk_1)
	{
		clonk_1 = new_clonk;
		clonk_1->SetPosition(pos_1[0], pos_1[1]);
		AddEffect("IntClonkOne", clonk_1, 100, 10);
		if (grapple_1)
			clonk_1->CreateContents(GrappleBow, 2);	
		else 
			clonk_1->CreateContents(Shovel);	
	}
	else if (clonk == clonk_2)
	{
		clonk_2 = new_clonk;
		clonk_2->SetPosition(pos_2[0], pos_2[1]);
		AddEffect("IntClonkTwo", clonk_2, 100, 10);
		if (grapple_2)
			clonk_2->CreateContents(GrappleBow, 2);
		else
			clonk_2->CreateContents(Ropeladder, 2);
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
	if (Distance(target->GetX(), target->GetY(), 820, 390) < 20)
	{
		grapple_1 = true;
		pos_1 = [820, 390];
		for (var content in FindObjects(Find_Container(target)))
			content->RemoveObject();
		target->CreateContents(GrappleBow, 2);	
	}
	return;
}

global func FxIntClonkTwoTimer(object target)
{
	if (Distance(target->GetX(), target->GetY(), 820, 390) < 20)
	{
		grapple_2 = true;
		pos_2 = [820, 390];
		for (var content in FindObjects(Find_Container(target)))
			content->RemoveObject();
		target->CreateContents(GrappleBow, 2);	
	}
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
	if (FindObject(Find_OCF(OCF_CrewMember), Find_Distance(40, 1450, 380)))
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
