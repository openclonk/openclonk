/* Targets */

func Initialize()
{
	CreateObject(GRSP, 10, 10);
	Message("@Here is a butterfly!|(ck draw it!)", CreateObject(_BTF, 10, 10));
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
	clonk->SetPosition(384, 469);
	clonk->CreateContents(BOW1);
	clonk->Collect(CreateObject(ARRW));
	return;
}