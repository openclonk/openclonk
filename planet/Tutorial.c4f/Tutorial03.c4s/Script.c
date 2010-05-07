/* Tutorial 3 - Targets */

func Initialize()
{
	//Environment
	CreateObject(Environment_Grass, 10, 10);
	PlaceGrass(85);
	CreateObject(Butterfly, 10, 10);

	//Goals & Events
	CreateObject(ShootTheTargets);
	ScriptGo(true);
	var TutGoal = CreateObject(Goal_Tutorial);
	TutGoal->SetStartpoint(40,446);

	//Chest with javelins
	var chest = CreateObject(Chest,333,486);
	chest->CreateContents(Javelin);
	chest->AddEffect("CheckChest",chest,1,36,chest);

	//Chest for arrows
	CreateObject(Chest,1110,417)->CreateContents(Arrow);
	
	// Dialogue options -> repeat round.
	SetNextMission("Tutorial.c4f\\Tutorial03.c4s", "$MsgRepeatRound$", "$MsgRepeatRoundDesc$");
}

// -- Javelin chest functionalities --
global func FxCheckChestTimer(object target, int num, int time)
{
	//When the chest with the javelins at the beginning is emptied
	if(!FindObject(Find_ID(Javelin),Find_Container(target)))
	{
		TutMsg("$MsgJavelin0$");

		var balloon = CreateObject(TargetBalloon,AbsX(189),AbsY(430),NO_OWNER);
		CreateObject(PracticeTarget,AbsX(189),AbsY(430),NO_OWNER)->SetAction("Attach",balloon);

		TutArrowShowPos(189,430,-45,30);

		target->AddEffect("JavelinSpawn",target,1,108,target);
		return -1;
	}
	return 1;
}

global func FxJavelinSpawnTimer(object target, int num, int time)
{
	if(!FindObject(Find_ID(Javelin))) target->CreateContents(Javelin);
}


//Scripted events
func Script1()
{
	TutMsg("$MsgIntro0$");
}

func Script15()
{
	TutMsg("$MsgIntro1$");
	FindObject(Find_ID(Goal_Tutorial))->TutArrowShowPos(333,480,135,25);
}

func Checkpoint_Bow1()
{
	TutMsg("$MsgBow0$");
}


func Checkpoint_Bow2()
{
	TutMsg("$MsgBow1$");
	var clonk = FindObject(Find_ID(Clonk),Find_OCF(OCF_Alive));
	var javelin = clonk->FindContents(Javelin);
	if(javelin) javelin->RemoveObject();
	clonk->CreateContents(Bow);
	clonk->CreateContents(Arrow);
	TutArrowShowPos(1253,251,55,100);

	clonk->AddEffect("CheckArrow",clonk,1,108,clonk);
	var arrowchest = FindObject(Find_ID(Chest),Find_InRect(1060,367,1160,467));
	arrowchest->AddEffect("ArrowSpawn",arrowchest,1,18,arrowchest);
}

//Checks if player has run out of arrows, and displays message if true.
global func FxCheckArrowTimer(object target, int num, int time)
{
	if(!FindObject(Find_ID(Arrow),Find_Container(target->FindContents(Bow))) && !FindObject(Find_ID(Arrow),Find_Container(target)))
	{
		TutMsg("$MsgBow2$");
		return -1;
	}
}

//respawn arrows in chest
global func FxArrowSpawnTimer(object target, int num, int time)
{
	if(!FindObject(Find_ID(Arrow))) target->CreateContents(Arrow);
}

global func OnTargetDeath(int target)
{
	if(target == 1)
	{
		TutMsg("$MsgJavelin1$");
		TutArrowShowPos(350,300,45,50);
	}

	if(target == 2)
	{
		TutArrowShowPos(538,319,60,50);
	}

	if(target == 3)
	{
		var ladder = CreateObject(Ropeladder,AbsX(530),AbsY(394));
		ladder->Unroll(-1);
		AddEffect("RemoveJavelin",0,1,5,0);

		var goal = FindObject(Find_ID(Goal_Tutorial));
		goal->AddCheckpoint(696,305,"Bow1");
		goal->AddCheckpoint(1034,456,"Bow2")->SetBaseGraphics(Bow);
		TutArrowClear();
	}

	if(target == 4)
	{
		TutArrowShowPos(980,190,-45,100);
	}

	if(target == 5)
	{
		TutArrowShowPos(834,535,-100,40);
	}

	if(target == 6)
	{
		TutArrowShowPos(1380,185,60,150);
	}

	if(target == 7)
	{
		TutArrowShowPos(1412,472,170,130);
	}

	if(target == 8)
	{
		TutArrowClear();
	}
	//creates finish checkpoint on clonk
	var clonk = FindObject(Find_ID(Clonk),Find_OCF(OCF_Alive));
	if(target == 8) FindObject(Find_ID(Goal_Tutorial))->SetFinishpoint(clonk->GetX(), clonk->GetY());
}

func InitializePlayer(int iPlr, int iX, int iY, object pBase, int iTeam)
{
	JoinPlayer(iPlr);
	return;
}

/*
 func RelaunchPlayer(int iPlr)
{
	var clonk = CreateObject(Clonk, 0, 0, iPlr);
	clonk->MakeCrewMember(iPlr);
	SetCursor(iPlr,clonk);
	SelectCrew(iPlr, clonk, true);
	JoinPlayer(iPlr);
	return;
}
*/

 func JoinPlayer(int iPlr)
{
	var clonk = GetCrew(iPlr);
	clonk->DoEnergy(100000);
	clonk->SetPosition(40, 446);
	return;
}
