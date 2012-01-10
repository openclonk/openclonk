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
	for (var i = 0; i < 12 + Random(4); i++)
		PlaceVegetation(Tree_Coniferous, 0, LandscapeHeight() / 3, LandscapeWidth(), LandscapeHeight(), 1000 * (61 + Random(40)));
	PlaceGrass(100);
		
	// Set time of day to evening and create some clouds and celestials.
	CreateObject(Environment_Clouds);
	CreateObject(Environment_Celestial);
	var time = CreateObject(Environment_Time);
	time->SetTime(600);
	time->SetCycleSpeed(12);

	// Create a small settlement to test stuff.
	/*
	var foundry = CreateConstruction(Foundry, 300, FindHeight(300), NO_OWNER, 100, true);
	foundry->CreateContents(Coal,3);
	foundry->CreateContents(Ore,3);	
	var flag = CreateConstruction(Flagpole, 360, FindHeight(360), NO_OWNER, 100, true);
	var workshop = CreateConstruction(ToolsWorkshop, 420, FindHeight(420), NO_OWNER, 100, true);
	workshop->CreateContents(Wood, 10);
	workshop->CreateContents(Metal, 10);
	workshop->CreateContents(Coal, 10);
	workshop->CreateContents(Sulphur, 10);
	var wind = CreateConstruction(WindGenerator, 480, FindHeight(480), NO_OWNER, 100, true);
	var line = CreateObject(PowerLine);
	line->SetActionTargets(wind, workshop);
	var sawmill = CreateConstruction(Sawmill, 520, FindHeight(520), NO_OWNER, 100, true);
	var line = CreateObject(PowerLine);
	line->SetActionTargets(wind, sawmill);
	
	// Create a lorry with necessary equipment to start a settlement.
	var lorry = CreateObject(Lorry, 300, FindHeight(300));
	lorry->CreateContents(Wood, 6);
	lorry->CreateContents(Metal, 4);
	lorry->CreateContents(Dynamite, 3);
	lorry->CreateContents(Loam, 3);
	*/
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
	// first player gets the base to test.
	/*
	var flagpole = FindObject(Find_ID(Flagpole));
	if (flagpole && !GetPlayerName(flagpole->GetOwner()))
		flagpole->SetOwner(plr);
	*/
	
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
		{
			crew->CreateContents(Hammer);
			crew->CreateContents(CableReel);
		}
		else
		{
			crew->CreateContents(Axe);
			crew->CreateContents(CableReel);
		}
		index++;
	}
	return;
}

