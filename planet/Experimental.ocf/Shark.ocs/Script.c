func Initialize()
{
	Shark->Place(3);
	Squid->Place(1);
	Seaweed->Place(71);
	Coral->Place(4);
	Fern->Place(2);
	Tree_Deciduous->Place(6);
	Tree_Deciduous_Burned->Place(1);
	SproutBerryBush->Place(3);
	Butterfly->Place(1);
}

private func InitializePlayer(iPlr)
{
	SetFoW(0, iPlr);
	var crew = GetCrew(iPlr);
	crew->SetPosition(10, 300);
	crew->CreateContents(Shovel);
	crew->CreateContents(Hammer);
	crew->CreateContents(Rock, 3);
}

func RelaunchPlayer(int iPlr)
{
	var pclonk = CreateObject(Clonk, 10, 300, iPlr);
	pclonk->MakeCrewMember(iPlr);
	SetCursor(iPlr, pclonk);
	pclonk->CreateContents(Shovel);
}
