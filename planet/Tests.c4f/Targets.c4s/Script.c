/* Targets */

func Initialize()
{
	CreateObject(Environment_Grass, 10, 10);
	CreateObject(Butterfly, 10, 10);
	//CreateObject(PracticeTarget);
	CreateObject(PracticeTarget,629,165,NO_OWNER);
	CreateObject(PracticeTarget,332,295,NO_OWNER);
	CreateObject(PracticeTarget,126,359,NO_OWNER);
	CreateObject(PracticeTarget,788,531,NO_OWNER);
	CreateObject(PracticeTarget,54,573,NO_OWNER);
	CreateObject(PracticeTarget,853,396,NO_OWNER);

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
