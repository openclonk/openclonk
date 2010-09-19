/* Tutorial 1 */

static guide; // guide object.
static loam_chest; //chest containing loam
static flint_chest; //chest containing flints
static tutstage; //how far has the player gotten? used for repsawn location

protected func Initialize()
{
	//Tutorial
	var goal = CreateObject(Goal_ReachFlag, 0, 0, NO_OWNER);
	goal->CreateGoalFlag(2330, 1040);
	AddEffect("TutorialScale",0,1,18);

	//Environment
	PlaceGrass(85);
	CreateObject(Tree_Coniferous,900,629);
	CreateObject(Plane,950,605);

	//Shovel in water
	var shovel = CreateObject(Shovel,1368,1160,NO_OWNER);
	shovel->SetR(180);
	AddEffect("ShovelGet",shovel,1,36,shovel);

	//Chest with loam.
	var chest = CreateObject(Chest,1800,1100,NO_OWNER);
	var loam = chest->CreateContents(Loam);
	AddEffect("LoamGet",loam,1,36,loam);
	loam->AddRestoreMode(chest);

	//Chest with firestones
	chest = CreateObject(Chest,2026,1089,NO_OWNER);
	chest->CreateContents(Firestone)->AddRestoreMode(chest); //I figure a 'for' statement is a little overkill here...
	chest->CreateContents(Firestone)->AddRestoreMode(chest);
	
	// Dialogue options -> repeat round.
	SetNextMission("Tutorial.c4f\\Tutorial01.c4s", "$MsgRepeatRound$", "$MsgRepeatRoundDesc$");
	return true;
}

// Gamecall from goals, set next mission.
protected func OnGoalsFulfilled()
{
	// Dialogue options -> next round.
	SetNextMission("Tutorial.c4f\\Tutorial02.c4s", "$MsgNextTutorial$", "$MsgNextTutorialDesc$");
	// Normal scenario ending by goal library.
	return false;
}


func InitializePlayer(int plr)
{
	var clonk = GetCrew(plr, 0);
	clonk->SetPosition(230, 955);
	var effect = AddEffect("ClonkRestore", clonk, 100, 10);
	EffectVar(1, clonk, effect) = 230;
	EffectVar(2, clonk, effect) = 955;

	SetPlayerViewLock(plr,true);
	SetPlayerZoomByViewRange(plr,300,nil,PLRZOOM_Direct | PLRZOOM_LimitMin | PLRZOOM_LimitMax);
	
	// Create tutorial guide, add messages, show first.
	guide = CreateTutorialGuide(plr);
	ScriptGo(1);
	return true;
}

/* Script progress */

// Part 1: Welcome
func Script1()
{
	guide->AddGuideMessage("@$MsgIntro0$");
	guide->ShowGuideMessage(0);
}

func Script11()
{
	guide->AddGuideMessage("@$MsgIntro1$");
	guide->ShowGuideMessage(1);
	TutArrowShowTarget(GetCrew(GetPlayerByIndex()), 225, 35);
}

func Script20()
{
	guide->AddGuideMessage("@$MsgIntro2$");
	guide->ShowGuideMessage(2);
	TutArrowClear();
}

func Script50()
{
	guide->AddGuideMessage("$MsgIntro3$");
	guide->ShowGuideMessage(3);
	guide->AddGuideMessage("$GuideMsgMovement$");
	TutArrowClear();
	ScriptGo();
}

/* Tutorial Guide Messages */
//Finds when the Clonk has done 'X', and changes the message.

global func FxTutorialScaleTimer(object target, int num, int timer)
{
	if(FindObject(Find_ID(Clonk),Find_InRect(650,990,140,90)))
	{
		guide->AddGuideMessage("$GuideMsgScale$");
		AddEffect("TutorialHangle", 0, 1, 18);
		return -1;
	}
}

global func FxTutorialHangleTimer(object target, int num, int timer)
{
	if(FindObject(Find_ID(Clonk),Find_InRect(820,940,190,140)))
	{
		guide->AddGuideMessage("$GuideMsgHangle$");
		AddEffect("TutorialSwim", 0, 1, 18);
		return -1;
	}
}

global func FxTutorialSwimTimer(object target, int num, int timer)
{
	if(FindObject(Find_ID(Clonk),Find_InRect(1120,1030,140,60)))
	{
		tutstage = 1;
		guide->AddGuideMessage("$GuideMsgSwim$");
		AddEffect("TutorialDig", 0, 1, 18);
		return -1;
	}
}

global func FxTutorialDigTimer(object target, int num, int timer)
{
	if(FindObject(Find_ID(Clonk),Find_InRect(1550,1040,130,60)))
	{
		guide->AddGuideMessage("$GuideMsgDig$");
		return -1;
	}
}

global func FxShovelGetTimer(object target, int num, int timer)
{
	if(target->Contained() != nil)
	{
		guide->AddGuideMessage("$GuideMsgTools$");
		RemoveEffect("TutorialDig");
		AddEffect("TutorialChest", 0, 1, 18);
		return -1;
	}
}

global func FxTutorialChestTimer(object target, int num, int timer)
{
	if(FindObject(Find_ID(Clonk),Find_InRect(1750,1030,130,80)))
	{
		guide->AddGuideMessage("$GuideMsgChest$");
		return -1;
	}
}

global func FxLoamGetTimer(object target, int num, int timer)
{
	if(target->Contained()->GetID() != Chest)
	{
		guide->AddGuideMessage("$GuideMsgLoam$");
		RemoveEffect("TutorialChest");
		AddEffect("TutorialFlint", 0, 1, 18);
		return -1;
	}
}

global func FxTutorialFlintTimer(object target, int num, int timer)
{
	if(FindObject(Find_ID(Clonk),Find_InRect(1990,1020,130,90)))
	{
		tutstage = 2;
		guide->AddGuideMessage("$GuideMsgFlint$");
		return -1;
	}
}



/*-- Clonk restoring --*/

global func FxClonkRestoreTimer(object target, int num, int time)
{
	// Respawn to new location if reached bow & arrow chest.
	if(tutstage == 1)
	{
		EffectVar(1, target, num) = 1240;
		EffectVar(2, target, num) = 1070;		
	}
	// Respawn to new location if reached brick climb.
	if(tutstage == 2)
	{
		EffectVar(1, target, num) = 2010;
		EffectVar(2, target, num) = 1020;		
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
		restorer->SetRestoreObject(clonk, nil, to_x, to_y, "ClonkRestore");
	}
	return 1;
}