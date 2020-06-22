/**
	Tutorial 01: Wandering Wipf
	Author: Maikel
	
	First introduction to the world of OpenClonk: explains basic movement controls.
	
	Following controls and concepts are explained:
	 * Clonk HUD 
	 * Walking and jumping with WASD
	 * Scaling, wall jump and hangling
	 * Swimming, diving and breath 
	 * Liquids: water
*/

static guide; // guide object

protected func Initialize()
{
	// Tutorial goal.
	var goal = CreateObject(Goal_Tutorial);
	goal.Name = "$MsgGoalName$";
	goal.Description = "$MsgGoalDescription$";
	CreateObject(Rule_NoPowerNeed);	
	GUI_Controller->ShowWealth();
	
	// Place objects in different sections.
	InitStartLake();
	InitCaveEntrance();
	InitHangleSection();
	InitCaveCenter();
	InitGoldMine();
	InitCaveExit();
	InitVegetation();
	InitAnimals();
	
	// Show wealth in HUD.
	GUI_Controller->ShowWealth();
	
	// Environment.
	var time = CreateObject(Time);
	time->SetTime(18 * 60 + 30);
	time->SetCycleSpeed(0);

	// Dialogue options -> repeat round.
	SetNextScenario("Tutorials.ocf\\Tutorial01.ocs", "$MsgRepeatRound$", "$MsgRepeatRoundDesc$");
	return;
}

// Gamecall from goals, set next mission.
protected func OnGoalsFulfilled()
{
	// Achievement: Tutorial completed.
	GainScenarioAchievement("TutorialCompleted", 3);
	// Dialogue options -> next round.
	SetNextScenario("Tutorials.ocf\\Tutorial02.ocs", "$MsgNextTutorial$", "$MsgNextTutorialDesc$");
	// Normal scenario ending by goal library.
	return false;
}

// Lake and start position.
private func InitStartLake()
{
	// Seaweed, fishes and shells in the lakes.
	Seaweed->Place(5, Rectangle(0, 600, 50, 100));
	Seaweed->Place(5, Rectangle(120, 600, 120, 120));
	Seaweed->Place(5, Rectangle(220, 400, 140, 50));
	
	// A shipwreck on the beach.
	
	// A few coconut trees.
	CreateObjectAbove(Tree_Coconut, 52, 620);
	CreateObjectAbove(Tree_Coconut, 79, 620);
	CreateObjectAbove(Tree_Coconut, 105, 620);
	
	// Ropebridge over the lake.
	Ropebridge->Create(144, 616, 232, 616);
	
	// Small sink hole for the water to flow through, cover by a trunk.
	DrawMaterialQuad("Tunnel", 231, 631, 231, 633, 251, 615, 249, 615, DMQ_Sub);
	var trunk = CreateObjectAbove(Trunk, 241, 644);
	trunk->SetR(45); trunk.Plane = 510; trunk->DoCon(-42);
	trunk.MeshTransformation = [-1000, 0, -17, 0, 0, 1000, 0, 0, 17, 0, -1000, 0];
	return;
}

// Cave entrance with waterfall.
private func InitCaveEntrance()
{
	// Waterfall from a leaking trunk.
	var trunk = CreateObjectAbove(Trunk, 324, 465);
	trunk->SetR(150); trunk.Plane = 510;
	trunk.MeshTransformation = [-70, 0, 998, 0, 0, 1000, 0, 0, -998, 0, -70, 0];
	var waterfall;
	waterfall = CreateWaterfall(325, 448, 2, "Water");
	waterfall->SetDirection(3, 3, 1, 3);
	waterfall = CreateWaterfall(338, 450, 8, "Water");
	waterfall->SetDirection(3, 4, 2, 4);
	CreateLiquidDrain(160, 648, 10);
	CreateLiquidDrain(184, 648, 10);
	CreateLiquidDrain(208, 648, 10);
	
	// Some vegetation below the waterfall.
	Fern->Place(2 + Random(2), Rectangle(300, 600, 60, 40));
	Mushroom->Place(4 + Random(2), Rectangle(300, 600, 60, 40));
	
	// Make the wipf hole a bit wider.
	DrawMaterialQuad("Tunnel", 360, 600, 410, 600, 410, 589, 360, 589, DMQ_Sub);
	
	// Vegetation in wall jump hole.
	var rock = CreateObjectAbove(BigRock, 313, 525);
	rock.MeshTransformation = [-314, -543, 671, 0, 0, 1085, 878, 0, -974, 222, -275, 0];
	rock.Plane = 510;
	
	// Torches to show the path.
	var torch = CreateObjectAbove(Torch, 288, 608);
	torch->AttachToWall(true);
	var torch = CreateObjectAbove(Torch, 388, 488);
	torch->AttachToWall(true);
	return;
}

// Hangle section with large mushrooms and gap to cross.
private func InitHangleSection()
{
	// Some large mushrooms.
	var mushroom = CreateConstruction(LargeCaveMushroom, 489, 557, NO_OWNER, 100, false, false);
	mushroom.MeshTransformation = Trans_Scale(500, 500, 500); mushroom->SetR(240);
	mushroom = CreateConstruction(LargeCaveMushroom, 517, 606, NO_OWNER, 100, false, false);
	mushroom.MeshTransformation = Trans_Scale(550, 550, 550); 	
	mushroom = CreateConstruction(LargeCaveMushroom, 597, 670, NO_OWNER, 100, false, false);
	mushroom.MeshTransformation = Trans_Scale(600, 600, 600); 
	mushroom = CreateConstruction(LargeCaveMushroom, 642, 686, NO_OWNER, 100, false, false);
	mushroom.MeshTransformation = Trans_Scale(700, 700, 700); 
	
	// Some ferns and mushrooms.	
	Fern->Place(5 + Random(3), Rectangle(480, 560, 220, 140));
	Mushroom->Place(9 + Random(4), Rectangle(480, 560, 220, 140));
	
	// Make the diagonal wipf hole a bit wider.
	DrawMaterialQuad("Tunnel", 690, 671, 754, 607, 769, 607, 690, 686, DMQ_Sub);
	
	// Wipf hole covered by trunk.	
	var trunk = CreateObjectAbove(Trunk, 822, 621);
	trunk.MeshTransformation = [821, 0, 795, 0, 0, 1145, 0, 0, -795, 0, 821, 0];
	trunk.Plane = 510; trunk->SetR(90);
	DrawMaterialQuad("Tunnel", 796, 591, 836, 591, 836, 602, 796, 602, DMQ_Sub);
	return;
}

// The cave center has two lakes with seaweed.
private func InitCaveCenter()
{
	// Seaweed in the two lakes.
	Seaweed->Place(5, Rectangle(540, 480, 140, 100));
	Seaweed->Place(12, Rectangle(520, 330, 380, 100));
	
	// Some vegetation between the two bodies of water.
	var ferns = Fern->Place(5 + Random(2), Rectangle(420, 370, 110, 90));
	for (var fern in ferns)
		fern->SetR(40 + Random(10));
	var mushrooms = Mushroom->Place(7 + Random(3), Rectangle(420, 370, 110, 90));
	for (var mushroom in mushrooms)
		mushroom->SetR(40 + Random(10));
	return;
}

// Gold mine with dynamite and lorry.
private func InitGoldMine()
{
	// Foundry with a lorry and some metal.
	CreateObjectAbove(Foundry, 980, 486)->MakeInvincible();
	CreateObjectAbove(Metal, 950, 474);
	
	// Pickaxe + ore near ore field.
	CreateObjectAbove(Lorry, 824, 460);
	CreateObjectAbove(Pickaxe, 808, 446)->SetR(RandomX(300, 330));
	CreateObjectAbove(Ore, 818, 460);
	CreateObjectAbove(Ore, 839, 460);
	
	// Shovel + coal chunks near coal field.
	CreateObjectAbove(Shovel, 966, 534)->SetR(RandomX(90, 135));
	CreateObjectAbove(Coal, 959, 532);
	CreateObjectAbove(Coal, 936, 532);
	
	// Dynamite box to blast through mine.
	var dyn1 = CreateObjectAbove(Dynamite, 959, 585, NO_OWNER); dyn1->SetR(-135);
	var dyn2 = CreateObjectAbove(Dynamite, 974, 597, NO_OWNER); dyn2->SetR(-105);
	var dyn3 = CreateObjectAbove(Dynamite, 970, 618, NO_OWNER); dyn3->SetR(-60);
	var dyn4 = CreateObjectAbove(Dynamite, 955, 633, NO_OWNER); dyn4->SetR(-30);
	var dyn5 = CreateObjectAbove(Dynamite, 941, 637, NO_OWNER); dyn5->SetR(-15);
	CreateObjectAbove(Fuse, 959, 585, NO_OWNER)->Connect(dyn1, dyn2);
	CreateObjectAbove(Fuse, 974, 597, NO_OWNER)->Connect(dyn2, dyn3);
	CreateObjectAbove(Fuse, 970, 618, NO_OWNER)->Connect(dyn3, dyn4);
	CreateObjectAbove(Fuse, 955, 633, NO_OWNER)->Connect(dyn4, dyn5);
	var igniter = CreateObjectAbove(DynamiteBox, 909, 637, NO_OWNER);
	igniter->ChangeToIgniter();
	CreateObjectAbove(Fuse, 941, 637, NO_OWNER)->Connect(dyn5, igniter);
	return;
}

// Cave exit: Elevator, cabin and flag.
private func InitCaveExit()
{
	// A small settlement.
	var elevator = CreateObjectAbove(Elevator, 872, 328);
	elevator->CreateShaft(248);
	elevator->MakeInvincible();
	CreateObjectAbove(WoodenCabin, 968, 328)->MakeInvincible();
	
	// Some vegetation / rocks on top of the cave.
	SproutBerryBush->Place(1 + Random(2), Rectangle(420, 200, 160, 70));
	SproutBerryBush->Place(1, Rectangle(560, 200, 100, 70));
	Cotton->Place(2, Rectangle(560, 200, 100, 70));
	Flower->Place(10, Rectangle(420, 300, 160, 70));
	var bigrock = CreateObjectAbove(BigRock, 218, 395);
	bigrock.MeshTransformation = [-103, 561, -337, 0, 0, 730, 1216, 0, 555, 74, -44, 0];
	return;
}

// Vegetation throughout the scenario.
private func InitVegetation()
{
	PlaceGrass(85);
	PlaceObjects(Firestone, 25 + Random(5), "Earth");
	PlaceObjects(Loam, 15 + Random(5), "Earth");
	
	Branch->Place(32, nil, {underground = true});
	Branch->Place(10, nil, {underground = false});
	return;
}

private func InitAnimals()
{
	// The wipf as your friend, controlled by AI.
	var wipf = CreateObjectAbove(Wipf, 76, 616);
	wipf->EnableTutorialControl();
	wipf->SetMeshMaterial("WipfSkin");
	wipf.Name = "$WipfName$";
	wipf.Description = "$WipfDescription$";
	
	// Some butterflies as atmosphere.
	for (var i = 0; i < 25; i++)
		PlaceAnimal(Butterfly);
		
	// Some fish in the big lake.
	Fish->Place(5, Rectangle(500, 300, 300, 150));
	return;
}	

/*-- Player Handling --*/

protected func InitializePlayer(int plr)
{
	// Position player's clonk.
	var clonk = GetCrew(plr, 0);
	clonk->SetPosition(60, 606);
	var fx = AddEffect("ClonkRestore", clonk, 100, 10);
	fx.to_x = 60;
	fx.to_y = 606;
	
	// Some wealth for the guide message.
	SetWealth(plr, 25);
	
	// Player controls disabled at the start.
	DisablePlrControls(plr);
	
	// Add an effect to the clonk to track the goal.
	var track_goal = AddEffect("TrackGoal", nil, 100, 2);
	track_goal.plr = plr;

	// Standard player zoom for tutorials, player is not allowed to zoom in/out.
	SetPlayerViewLock(plr, true);
	SetPlayerZoomByViewRange(plr, 400, nil, PLRZOOM_Direct | PLRZOOM_LimitMin | PLRZOOM_LimitMax);
	
	// Determine player movement keys.
	var left = GetPlayerControlAssignment(plr, CON_Left, true, true);
	var right = GetPlayerControlAssignment(plr, CON_Right, true, true);
	var up = GetPlayerControlAssignment(plr, CON_Up, true, true);
	var down = GetPlayerControlAssignment(plr, CON_Down, true, true);
	var jump = GetPlayerControlAssignment(plr, CON_Jump, true, true);
	var control_keys = Format("[%s] [%s] [%s] [%s]", up, left, down, right);
	
	// Create tutorial guide, add messages, show first.
	guide = CreateObjectAbove(TutorialGuide, 0, 0, plr);
	guide->AddGuideMessage(Format("$MsgTutorialWelcome$", control_keys));
	guide->AddGuideMessage("$MsgTutorialHUD$");
	guide->AddGuideMessage(Format("$MsgTutorialFollowFriend$", left, right, jump));
	guide->ShowGuideMessage(0);
	AddEffect("TutorialScale", nil, 100, 2);
	return;
}

/*-- Intro, Tutorial Goal & Outro --*/

private func OnFinishedTutorialIntro(int plr)
{
	// enable crew
	EnablePlrControls(plr);
	
	// start the wipf
	FindObject(Find_ID(Wipf))->StartMoving();
}

global func FxTrackGoalTimer(object target, proplist effect, int time)
{
	var crew = GetCrew(effect.plr);
	if (Inside(crew->GetX(), 980, 996) && Inside(crew->GetY(), 314, 324))
	{
		if (FindObject(Find_ID(Wipf), Find_Distance(15, crew->GetX(), crew->GetY())))
		{
			var outro = AddEffect("GoalOutro", crew, 100, 5);
			outro.plr = effect.plr;
			return FX_Execute_Kill;	
		}		
	}
	return FX_OK;
}

global func FxGoalOutroStart(object target, proplist effect, int temp)
{
	if (temp)
		return;
		
	// Show guide message congratulating.
	guide->AddGuideMessage("$MsgTutorialCompleted$");
	guide->ShowGuideMessage();
		
	// Halt clonk and disable player controls.	
	DisablePlrControls(effect.plr);
	target->SetComDir(COMD_Stop);
	
	// Find wipf and enter it into the clonk.	
	var wipf = FindObject(Find_ID(Wipf), Find_Distance(15, target->GetX(), target->GetY()));
	if (wipf)
		wipf->Enter(target);		
	return FX_OK;
}

global func FxGoalOutroTimer(object target, proplist effect, int time)
{
	if (time >= 60)
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
		
	// Enable player controls again.
	EnablePlrControls(effect.plr);
	// Enable wipf activity.
	var wipf = FindObject(Find_ID(Wipf));
	if (wipf)
		wipf->DisableTutorialControl();
	return FX_OK;
}

/*-- Guide Messages --*/
// Finds when the Clonk has done 'X', and changes the message.

global func FxTutorialScaleTimer(object target, proplist effect, int timer)
{
	var clonk = FindObject(Find_ID(Clonk), Find_InRect(264, 568, 80, 48));
	if (clonk)
	{
		var plr = clonk->GetOwner();
		while (GetEffect("TutorialIntro*"))
			RemoveEffect("TutorialIntro*");
		var up = GetPlayerControlAssignment(plr, CON_Up, true, true);
		var down = GetPlayerControlAssignment(plr, CON_Down, true, true);
		guide->AddGuideMessage(Format("$MsgTutorialScaleWall$", up, down));
		guide->ShowGuideMessage();
		AddEffect("TutorialHangle", nil, 100, 2);
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialHangleTimer(object target, proplist effect, int timer)
{
	var clonk = FindObject(Find_ID(Clonk), Find_InRect(480, 560, 80, 48));
	if (clonk)
	{
		var plr = clonk->GetOwner();
		var down = GetPlayerControlAssignment(plr, CON_Down, true, true);
		guide->AddGuideMessage(Format("$MsgTutorialHangle$", down));
		guide->ShowGuideMessage();
		AddEffect("TutorialJump", nil, 100, 2);
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialJumpTimer(object target, proplist effect, int timer)
{
	if (FindObject(Find_ID(Clonk), Find_InRect(744, 568, 56, 40)))
	{
		guide->AddGuideMessage("$MsgTutorialJump$");
		guide->ShowGuideMessage();
		AddEffect("TutorialSwimming", nil, 100, 2);
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialSwimmingTimer(object target, proplist effect, int timer)
{
	var clonk = FindObject(Find_ID(Clonk), Find_InRect(672, 440, 80, 72));
	if (clonk)
	{
		var plr = clonk->GetOwner();
		var left = GetPlayerControlAssignment(plr, CON_Left, true, true);
		var right = GetPlayerControlAssignment(plr, CON_Right, true, true);
		var up = GetPlayerControlAssignment(plr, CON_Up, true, true);
		var down = GetPlayerControlAssignment(plr, CON_Down, true, true);
		var control_keys = Format("[%s] [%s] [%s] [%s]", up, left, down, right);
		guide->AddGuideMessage(Format("$MsgTutorialSwimming$", control_keys));
		guide->ShowGuideMessage();
		AddEffect("TutorialDiving", nil, 100, 2);
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func FxTutorialDivingTimer(object target, proplist effect, int timer)
{
	if (FindObject(Find_ID(Clonk), Find_InRect(472, 280, 48, 64)))
	{
		guide->AddGuideMessage("$MsgTutorialDiving$");
		guide->ShowGuideMessage();
		return FX_Execute_Kill;
	}
	return FX_OK;
}

protected func OnGuideMessageShown(int plr, int index)
{
	// Show the player his clonk and the guide.
	if (index == 0)
	{
		TutArrowShowTarget(GetCrew(GetPlayerByIndex()), 225, 24);
	}
	// Show the player HUD.
	if (index == 1)
	{
		TutArrowShowGUITarget(FindObject(Find_ID(GUI_Controller_CrewBar)), 0); 	
		TutArrowShowGUITarget(FindObject(Find_ID(GUI_Controller_Goal)), 0); 	
	}
	// Show the clonks friend: the wipf.	
	if (index == 2)
	{
		OnFinishedTutorialIntro(plr);
		TutArrowShowTarget(FindObject(Find_ID(Wipf)));
	}
	// Show wall to climb.
	if (index == 3)
		TutArrowShowPos(366, 570, 45);		
	// Show ceiling on which to hangle.
	if (index == 4)
		TutArrowShowPos(592, 564, 45);
	// Show wall to jump to.
	if (index == 5)
	{
		TutArrowShowPos(780, 560, 0);
		TutArrowShowPos(756, 544, 315);
	}
	// Show sea to swim in.
	if (index == 6)
		TutArrowShowPos(652, 478, 225);
	// Show breath indicator.
	if (index == 7)
		TutArrowShowGUITarget(FindObject(Find_ID(GUI_Controller_CrewBar)), 0);
		
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
	if (FindObject(Find_OCF(OCF_CrewMember), Find_InRect(656, 432, 40, 48)))
	{
		effect.to_x = 680;
		effect.to_y = 470;             
	}
	if (FindObject(Find_OCF(OCF_CrewMember), Find_InRect(488, 280, 32, 48)))
	{
		effect.to_x = 500;
		effect.to_y = 318;             
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