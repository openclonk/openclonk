/* Sky race */

func Initialize()
{
	CreateObject(Ropeladder, 174, 445)->Unroll(-1);
	CreateObject(Ropeladder, 328, 564);
	CreateObject(Ropeladder, 226, 330);
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
	clonk->SetPosition(50, 490);
	clonk->CreateContents(Bow);
	clonk->Collect(CreateObject(Arrow));
	return;
}