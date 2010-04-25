/* Tutorial 3 - Targets */

func Initialize()
{
	CreateObject(Goal_ShootTheTargets,10,10);
	ScriptGo(true);
	CreateObject(Environment_Grass, 10, 10);
	PlaceGrass(85);
	CreateObject(Butterfly, 10, 10);
	var balloon = CreateObject(TargetBalloon,332,295,NO_OWNER);
	CreateObject(PracticeTarget,332,295,NO_OWNER)->SetAction("Attach",balloon);
}

func Script1()
{
	TutMsg("$MsgIntro0$");
}

func Script20()
{
	TutMsg("$MsgIntro1$");
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
	clonk->SetPosition(384, 469);
	clonk->CreateContents(Bow);
	clonk->Collect(CreateObject(Arrow));
	return;
}
