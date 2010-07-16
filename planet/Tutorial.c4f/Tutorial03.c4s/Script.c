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
	TutGoal->SetStartpoint(40,609);

	//General Objects
	var chest = CreateObject(Chest,336,650);
	chest->CreateContents(Javelin);
	chest->AddEffect("CheckChest",chest,1,36,chest);
	CreateObject(Ropeladder,751,465)->Unroll(1);
	
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

		var balloon = CreateObject(TargetBalloon,AbsX(189),AbsY(580),NO_OWNER);
		CreateObject(PracticeTarget,AbsX(189),AbsY(580),NO_OWNER)->SetAction("Attach",balloon);

		TutArrowShowPos(189,580,-45,30);

		target->AddEffect("JavelinSpawn",target,1,36,target);
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
	FindObject(Find_ID(Goal_Tutorial))->TutArrowShowPos(333,650,135,25);
}

func Checkpoint_Javelin1()
{
	TutMsg("$MsgJavelin3$");
}

func Checkpoint_Bow1()
{
	TutMsg("$MsgBow0$");
}


func Checkpoint_Bow2()
{
	TutMsg("$MsgBow1$");
	var clonk = FindObject(Find_ID(Clonk),Find_OCF(OCF_Alive));
	clonk->CreateContents(Bow);
	clonk->CreateContents(Arrow);

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
		TutArrowShowPos(300,520,45,50);
	}

	if(target == 2)
	{
		TutArrowShowPos(510,500,95,70);
		TutMsg("$MsgJavelin2$");
	}

	if(target == 3)
	{
		TutArrowShowPos(360,340,-90,130);
		var goal = FindObject(Find_ID(Goal_Tutorial));
		goal->AddCheckpoint(650,370,"Javelin1");
	}

	if(target == 4)
	{
		TutArrowShowTarget(FindObject(Find_ID(PracticeTarget)),45,35);
	}

	if(target == 5)
	{
		TutArrowShowPos(1525,150,30,50);
		var goal = FindObject(Find_ID(Goal_Tutorial));
		goal->AddCheckpoint(1412,273,"Bow1");
		goal->AddCheckpoint(1795,457,"Bow2")->SetBaseGraphics(Bow);
	}

	if(target == 6)
	{
		TutArrowShowPos(1908,300,75,50);
		//Creation of ammochest
		var ammochest = CreateObject(Chest,AbsX(2480),AbsY(417));
		ammochest->CreateContents(Arrow);
		ammochest->CreateContents(Javelin);
	}

	if(target == 7)
	{
		var ladder = CreateObject(Ropeladder,AbsX(1900),AbsY(394));
		ladder->Unroll(-1);
		TutArrowShowPos(2352,247,-45,40);
	}

	if(target == 8)
	{
		TutArrowShowPos(2632,285,45,100);
	}

	if(target == 9)
	{
		TutArrowShowPos(2204,535,-100,40);
	}

	if(target == 10)
	{
		TutArrowShowPos(2165,310,-50,70);
	}

	if(target == 11)
	{
		TutArrowShowPos(2750,185,60,150);
	}

	if(target == 12)
	{
		TutArrowShowPos(2782,472,170,130);
	}

	//creates finish checkpoint on clonk
	var clonk = FindObject(Find_ID(Clonk),Find_OCF(OCF_Alive));
	if(target == 13)
	{
		FindObject(Find_ID(Goal_Tutorial))->SetFinishpoint(clonk->GetX(), clonk->GetY());
		TutArrowClear();
	}
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
	clonk->SetPosition(40, 609);
	return;
}
