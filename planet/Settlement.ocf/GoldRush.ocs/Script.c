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
	//Tree_Coniferous->Place(16+Random(4), Rectangle(0,LandscapeHeight()/3, LandscapeWidth(), LandscapeHeight()));
	PlaceForest([Tree_Coniferous, SproutBerryBush],0, LandscapeHeight()/2+50, nil, true);

	PlaceGrass(100);
	
	CreateEnvironmentObjects("Temperate");
	
	// Set time of day to evening and create some clouds and celestials.
	Cloud->Place(10);
	Cloud->SetPrecipitation("Water", 15);
	CreateObject(Environment_Celestial);
	var time = CreateObject(Environment_Time);
	time->SetTime(600);
	time->SetCycleSpeed(12);
	return;
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

