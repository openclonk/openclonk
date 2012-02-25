/**
	Goldmine
	A simple landscape with some gold, the goal is to mine all gold.
	
	@authors Maikel
*/


protected func Initialize()
{
	// Goal: Resource extraction, set to gold mining.
	var goal = CreateObject(Goal_Wealth);
	goal->SetWealthGoal(400);
	
	// Place some trees.
	for (var i = 0; i < 16 + Random(4); i++)
		PlaceVegetation(Tree_Coniferous, 0, LandscapeHeight() / 3, LandscapeWidth(), LandscapeHeight(), 1000 * (61 + Random(40)));
	
	// place some sprout berries
	var bush = PlaceVegetation(SproutBerryBush, 0, LandscapeHeight() / 3, LandscapeWidth(), LandscapeHeight(), 100000);
	if(bush)
		for (var i = 0; i < 2; i++)
			PlaceVegetation(SproutBerryBush, bush->GetX() - 200, bush->GetY() - 200, 400, 400, 100000);
	PlaceGrass(100);
		
	// Set time of day to evening and create some clouds and celestials.
	CreateObject(Environment_Clouds);
	CreateObject(Environment_Celestial);
	var time = CreateObject(Environment_Time);
	time->SetTime(600);
	time->SetCycleSpeed(12);
	return;
}

private func FindHeight(int x)
{
	var y = 0;
	while (!GBackSemiSolid(x, y) && y < LandscapeHeight())
		y += 10;
	return y;
}

protected func InitializePlayer(int plr)
{ 
	// Increase wealth goal per player.
	var goal = FindObject(Find_ID(Goal_Wealth));
	if (goal)
		goal->SetWealthGoal(300 + 100 * Min(GetPlayerCount(), 3));

	// Move clonks to location and give them a shovel.
	var index = 0, crew;
	while (crew = GetCrew(plr, index))
	{
		var x = 275 + Random(50);
		crew->SetPosition(x , FindHeight(x) - 20);
		crew->CreateContents(Shovel);
		// First clonk can construct, others can mine.
		if (index == 0)
			crew->CreateContents(Hammer);
		else
			crew->CreateContents(Axe);
		index++;
	}
	return;
}

