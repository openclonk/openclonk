/*--
	Settlement
	Author: Maikel
	
	A nice round to test the settlement objects.
--*/

protected func Initialize()
{
	// Goal: Resource extraction, set to gold mining.
	var goal = CreateObject(Goal_ResourceExtraction);
	goal->SetResource("Gold");
	goal->SetResource("Ore");
	DoEnvironment();
}

protected func InitializePlayer(int plr)
{ 
	// No FoW needed for now.
	SetFoW(false, plr);
	// Give clonk shovel and a lorry with some equipment.
	var clonk = GetCrew(plr);
	clonk->CreateContents(Shovel);
	var lorry = CreateObject(Lorry, clonk->GetX(), clonk->GetY(), plr);
	lorry->CreateContents(Pipe, 2);
	return;
}

private func DoEnvironment()
{
	Cloud->Place(30);
	CreateObject(Environment_Celestial);
	CreateObject(Environment_Time);
	Sound("BirdsLoop",true,100,nil,+1);
}
