/* Sky race */

func Initialize()
{
//	CreateObject(Ropeladder, 174, 445)->Unroll(-1, 0, 25);
	CreateObject(Ropeladder, 328, 564);
	CreateObject(Ropeladder, 226, 330);

	var Rock1 = CreateObject(Rock, 159, 363);
	var Rock2 = CreateObject(Rock, 232, 388);
//	CreateObject(Ropeladder, 197, 432)->MakeBridge(Rock1, Rock2);
}

 func InitializePlayer(int iPlr, int iX, int iY, object pBase, int iTeam)
{
	JoinPlayer(iPlr);
	return;
}

 func RelaunchPlayer(int iPlr)
{
	var clonk = CreateObject(Clonk, 0, 0, iPlr);
	clonk->SetCrewStatus(iPlr,true);
	SetCursor(iPlr,clonk);
	JoinPlayer(iPlr);
	return;
}

 func JoinPlayer(int iPlr)
{
	var clonk = GetCrew(iPlr);
	clonk->DoEnergy(100000);
	clonk->SetPosition(50, 490);
//	clonk->CreateContents(Musket);
//	clonk->CreateContents(LeadShot);
//	clonk->CreateContents(Javelin);
//		clonk->CreateContents(DynamiteBox);
	clonk->CreateContents(GrappleBow);
//	clonk->CreateContents(Bow);
//	clonk->Collect(CreateObject(Arrow));
	return;
}