/* MOOSSSSSS */

func Initialize()
{


	CreateObject(Moss_Lichen,280,380);
	CreateObject(Moss_Lichen,166,166);
	CreateObject(Moss,470,220);

	Log("Try /fast 10 for faster growing!");
	
}

 func InitializePlayer(int iPlr, int iX, int iY, object pBase, int iTeam)
{
	SetFoW(0,iPlr);
	JoinPlayer(iPlr);
	return;
}

 func RelaunchPlayer(int iPlr)
{
	var clonk = CreateObject(Clonk, 0, 0, iPlr);
	clonk->MakeCrewMember(iPlr);
	SetCursor(iPlr,clonk);
	JoinPlayer(iPlr);
	return;
}

 func JoinPlayer(int iPlr)
{
	var clonk = GetCrew(iPlr);
	clonk->DoEnergy(100000);
	clonk->SetPosition(150, 100);
//	clonk->CreateContents(Javelin);
//		clonk->CreateContents(DynamiteBox);
	clonk->CreateContents(Sword);
	clonk->CreateContents(Firestone);
	clonk->CreateContents(Shovel);
	clonk->CreateContents(GrappleBow);
	return;
	
}