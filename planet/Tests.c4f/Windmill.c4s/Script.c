/* Windmill Island */

func Initialize()
{
	FxBoomAttackTimer();
	AddEffect("BoomAttack", 0, 100, 35*10);
}

global func FxBoomAttackTimer()
{
	var angle = Random(360);
	var radius = Min(LandscapeWidth()/2, LandscapeHeight()/2);
	var x =  Sin(angle, radius)+LandscapeWidth()/2;
	var y = -Cos(angle, radius)+LandscapeHeight()/2;
	CreateObject(BOOM_Attack, x, y);
}

func InitializePlayer(int iPlr, int iX, int iY, object pBase, int iTeam)
{
	JoinPlayer(iPlr);
	return;
}

 func RelaunchPlayer(int iPlr)
{
	var clonk = CreateObject(CLNK, 0, 0, iPlr);
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
	clonk->SetPosition(LandscapeWidth()/2, 0);
	clonk->CreateContents(BOW1);
	clonk->Collect(CreateObject(ARRW));
	return;
}