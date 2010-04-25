/* Tutorial 3 - Targets */

func Initialize()
{
	CreateObject(Goal_ShootTheTargets);
	ScriptGo(true);
	CreateObject(Environment_Grass, 10, 10);
	PlaceGrass(85);
	CreateObject(Butterfly, 10, 10);

	var TutGoal = CreateObject(Goal_Tutorial);
	TutGoal->SetStartpoint(40,446);
	TutGoal->AddCheckpoint(300,463,"Javelin");
}

func Script1()
{
	TutMsg("$MsgIntro0$");
}

func Script15()
{
	TutMsg("$MsgIntro1$");
}

func Checkpoint_Javelin()
{
	TutMsg("$MsgJavelin0$");
	FindObject(Find_ID(Clonk),Find_OCF(OCF_Alive))->CreateContents(Javelin);
	var balloon = CreateObject(TargetBalloon,189,430,NO_OWNER);
	var target = CreateObject(PracticeTarget,189,430,NO_OWNER);
	target->SetAction("Attach",balloon);
	target->AddEffect("Check",target,1,1,target);
}

func Checkpoint_Bow1()
{
	TutMsg("$MsgBow0$");
}


func Checkpoint_Bow2()
{
	TutMsg("$MsgBow1$");
	var clonk = FindObject(Find_ID(Clonk),Find_OCF(OCF_Alive));
	clonk->FindContents(Javelin)->RemoveObject();
	clonk->CreateContents(Bow);
	clonk->CreateContents(Arrow);
}


global func OnTargetDeath(int target)
{
	if(target == 1) TutMsg("$MsgJavelin1$");
	if(target == 2) TutMsg("$MsgJavelin2$");
	if(target == 3)
	{
		var ladder = CreateObject(Ropeladder,AbsX(530),AbsY(388));
		ladder->Unroll(-1);
		AddEffect("RemoveJavelin",0,1,5,0);

		var goal = FindObject(Find_ID(Goal_Tutorial));
		goal->AddCheckpoint(696,305,"Bow1");
		goal->AddCheckpoint(1034,456,"Bow2");
	}

	var clonk = FindObject(Find_ID(Clonk),Find_OCF(OCF_Alive));
	if(target == 8) FindObject(Find_ID(Goal_Tutorial))->SetFinishpoint(clonk->GetX(), clonk->GetY());
}

func FxRemoveJavelinTimer(object target, int num, int time)
{
	var javelin = FindObject(Find_ID(Clonk), Find_InRect(650,0,740,LandscapeHeight()))->FindContents(Javelin);
	if(javelin)
	{
		javelin->Exit();
		javelin->RemoveObject();
		return -1;
	}
}

func InitializePlayer(int iPlr, int iX, int iY, object pBase, int iTeam)
{
	JoinPlayer(iPlr);
	return;
}

 func RelaunchPlayer(int iPlr)
{
	var clonk = CreateObject(Clonk, 0, 0, iPlr);
	clonk->MakeCrewMember(iPlr);
	SetCursor(iPlr,clonk);
	SelectCrew(iPlr, clonk, true);
	JoinPlayer(iPlr);
	return;
}

 func JoinPlayer(int iPlr)
{
	var clonk = GetCrew(iPlr);
	clonk->DoEnergy(100000);
	clonk->SetPosition(40, 446);
//	clonk->CreateContents(Bow);
//	clonk->Collect(CreateObject(Arrow));
	return;
}
