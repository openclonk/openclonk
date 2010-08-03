/* Tutorial 3 - Targets */

func Initialize()
{
	//Environment
	CreateObject(Environment_Grass, 10, 10);
	PlaceGrass(85);
	CreateObject(Butterfly, 10, 10);

	//Goals & Events
	ScriptGo(true);
	var TutGoal = CreateObject(Goal_Tutorial);
	TutGoal->SetStartpoint(40,609);
	CreateObject(Goal_Flag,2230,310);

	//General Objects
	var chest = CreateObject(Chest,240,650);
	chest->CreateContents(Javelin);
	chest->AddEffect("CheckChest",chest,1,36,chest);

	var chest = CreateObject(Chest,785,550);
	chest->CreateContents(Bow);
	chest->CreateContents(Arrow);

	//Targets
	//1
	MakeTarget(280,580,true);
	//2
	MakeTarget(180,560,true);
	//3
	var target3 = MakeTarget(410,580,false);
	AddEffect("Target3",target3,1,0,target3);
	//4
	var target4 = MakeTarget(380,300,true)->GetActionTarget();
	AddEffect("HorizMoving",target4,1,1,target4);
	//5
	var target5 = MakeTarget(690,421,true);
	AddEffect("FlintDrop",target5,1,0,target5);
	//6
	var target6 = MakeTarget(880,520,true)->GetActionTarget();
	AddEffect("HorizMoving",target6,1,1,target6);
	//7
	MakeTarget(1250,450,true);
	//8
	var target8 = MakeTarget(1364,300,true);
	AddEffect("FlintDrop",target8,1,0,target8);
	//9
	MakeTarget(1660,450,true);
	//10
	var target10 = MakeTarget(1560,320,true)->GetActionTarget();
	AddEffect("HorizMoving",target10,1,1,target10);
	//11
	MakeTarget(1710,230,true);
	//12
	MakeTarget(1800,260,true);
	//13
	var target13 = MakeTarget(2140,250,true);
	AddEffect("Target13",target13,1,0,target13);

	// Dialogue options -> repeat round.
	SetNextMission("Tutorial.c4f\\Tutorial03.c4s", "$MsgRepeatRound$", "$MsgRepeatRoundDesc$");
}

// -- Javelin chest functionalities --
global func FxCheckChestTimer(object target, int num, int time)
{
	//When the chest with the javelins at the beginning is emptied
	if(!FindObject(Find_ID(Javelin),Find_Container(target)))
	{
		FindObject(Find_ID(Goal_Tutorial))->TutArrowClear();
		
		//Apply target pointer to player
		var clonk = FindObject(Find_ID(Clonk),Find_OCF(OCF_Alive));
		var arrow = CreateObject(GUI_TutArrow);
		arrow->SetAction("Show",clonk);
		arrow->SetClrModulation(RGBa(255,0,0,125));
		arrow->SetOwner(clonk->GetOwner());
		AddEffect("ArrowPoint",arrow,1,1,arrow);
		return -1;
	}
	return 1;
}

//Scripted events
func Script1()
{
	TutMsg("$MsgIntro0$");
}

func Script15()
{
	TutMsg("$MsgIntro1$");
	FindObject(Find_ID(Goal_Tutorial))->TutArrowShowPos(240,650,135,25);
}

global func FxTarget3Stop(object target, int num, int reason, bool temporary)
{
	CreateObject(Rock,AbsX(430),AbsY(620))->Explode(25);
}

global func FxTarget13Stop(object target, int num, int reason, bool temporary)
{
	CreateObject(Ropeladder,AbsX(2140),AbsY(320))->Unroll(1);
	var flag = FindObject(Find_ID(Goal_Flag));
	AddEffect("FlagWin",flag,1,1,flag);
}

global func FxHorizMovingTimer(object target, int num, int time)
{
	target->SetXDir(Sin(time,20));
}

global func FxFlintDropStop(object target, int num, int reason, bool temporary)
{
	CreateObject(Firestone);
}

global func FxFlagWinTimer(object target, int num, int timer)
{
	var clonk = target->FindObject(Find_ID(Clonk),Find_Distance(30),Find_OCF(OCF_Alive));
	if(clonk)
	{
		var ballooncount = ObjectCount(Find_ID(PracticeTarget));
		if(ballooncount > 0)
		{
		target->Message("$TargetsRemain$");
		return;
		}
		FindObject(Find_ID(Goal_Tutorial))->SetFinishpoint(clonk->GetX(), clonk->GetY());
		return -1;
	}
}

protected func MakeTarget(int ix, int iy, bool flying)
{
	if(flying == nil) balloon = false;

	var target = CreateObject(PracticeTarget,ix,iy);
	if(flying == true)
	{
		var balloon = CreateObject(TargetBalloon,ix,iy-30);
		target->SetAction("Attach",balloon);
		CreateParticle("Flash",ix,iy-50,0,0,500,RGB(255,255,255));
	}

	if(flying == false)
	{
		CreateParticle("Flash",ix,iy,0,0,500,RGB(255,255,255));
		target->SetAction("Float");
	}
	return target;
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

global func FxArrowPointTimer(object arrow, int num, int timer)
{
	var practicetarget = FindObject(Find_ID(PracticeTarget),Sort_Distance());
	if(practicetarget)
	{
		arrow->SetR(Angle(AbsX(arrow->GetX()),AbsY(arrow->GetY()),AbsX(practicetarget->GetX()),AbsY(practicetarget->GetY())));
	}
	else
		arrow->RemoveObject();
}