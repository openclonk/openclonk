/**
	Iron Peak
	A chilly mountain peak filled with iron ore and coal.
	
	@authors Maikel
*/


// Scenario properties which can be set later by the lobby options.
static const SCENOPT_Material = 1; // Amount of material available from start.
static const SCENOPT_MapSize = 1; // Size of the map.
static const SCENOPT_Difficulty = 1; // Difficulty settings.

// Spawn location for all players.
static mountain_location;

protected func Initialize()
{
	// Goal: Resource extraction, set to ore mining.
	var goal = CreateObject(Goal_ResourceExtraction);
	goal->SetResource("Ore");
	
	// Goal: Construct flagpole at the top.
	// TODO: implement this goal.
	
	// Find A good location for the players to start.
	FindMountainLocation();
	
	// Cover the mountain in some snow already.
	GiveMountainSnowCover();
	
	// Bottom of the map should only be open at the sides.
	CloseMapBottom();
	
	// Add a snow storm effect, strong winds and lot's of snow.
	AddEffect("SnowStorm", nil, 100, 5, nil);
	
	// Create a lorry with necessary equipment to start a settlement.
	//var lorry = CreateObject(Lorry, mountain_location[0], mountain_location[1], NO_OWNER);
	//lorry->CreateContents(Wood, 16);
	//lorry->CreateContents(Metal, 4);
	//lorry->CreateContents(Dynamite, 3);
	// TODO: Make sure lorry stays on mountains.
	
	// Place some coniferous trees, but only up to 2/3 of the mountain.
	Tree_Coniferous->Place(16+Random(5), Rectangle(0,LandscapeHeight()/3, LandscapeWidth(), 2*LandscapeHeight()/3));
		
	// Some mushrooms as source of food.
	Mushroom->Place(30+Random(10));
		
	// Set time of day to evening and create some clouds and celestials.
	Cloud->Place(20);
	CreateObject(Environment_Celestial);
	var time = CreateObject(Environment_Time);
	time->SetTime(60*22);
	time->SetCycleSpeed(0);
	
	// A light blue hue, to indicate the cold climate.
	var blue = 12;
	SetGamma(RGB(0,0,blue), RGB(128-blue,128-blue,128+blue), RGB(255-blue,255-blue,255));
	
	// Some natural disasters. 
	// Earthquake->SetChance(30);
	// TODO: Rockfall.
	
	//LogMatCounts();
	return;
}

private func FindMountainLocation()
{
	// Default to top middle of the map.
	mountain_location = [LandscapeWidth() / 2, LandscapeHeight() / 2];
	var x = 0, y = 0;
	for (var i = 0; i < 1000; i++)
	{
		// Random x coordinate.
		var x = Random(LandscapeWidth()), y = 0;
		// Find corresponding y coordinate.
		while (!GBackSolid(x, y) && y < 9 * LandscapeHeight() / 10)
			y += 2;
		// Check if surface is relatively flat (check for flatter surfaces first).
		var d = i / 250 + 1;		
		if (!GBackSolid(x + 10, y - 20) && !GBackSolid(x - 10, y - 20) && !GBackSolid(x + 10, y - d) && !GBackSolid(x - 10, y - d) && GBackSolid(x + 10, y + d) && GBackSolid(x - 10, y + d))
			if (y > LandscapeHeight() / 2)
			{
				mountain_location = [x, y - 10];
				break;
			}
	} 
	return;
}

private func LogMatCounts()
{
	for (var i = 0; i < 128; i++)
	{
		if (GetMaterialCount(i) > 0)
			Log("Material %s has %d count.", MaterialName(i), GetMaterialCount(i) / 100);
	}
	return;
}

private func GiveMountainSnowCover()
{
	// Loop over the map horizontally.
	for (var x = 0; x < LandscapeWidth(); x += 20)
	{
		// Find height of mountain at this x.
		var y = 0;
		while (!GBackSolid(x, y) && y < LandscapeHeight())
			y += 10;
		if (y < 9 * LandscapeHeight() / 10)
			CastPXS("Snow", 10 + Random(50), 40, x, y - 40, 180, 40);
	}
	return;
}

private func CloseMapBottom()
{
	var y = LandscapeHeight() - 1;
	for (var x = 0; x < LandscapeWidth(); x++)
		if (!GBackSky(x, y))
			DrawMaterialQuad("Brick", x, y-1, x, y+1, x-1, y+1, x-1, y-1, true);
	return;
}

// This effect should be called every 5 frames.
global func FxSnowStormStart(object target, proplist effect)
{
	// Always a strong wind, either to the left or the right.
	effect.wind = (2 * Random(2) - 1) * (90 + Random(10));
	// Accordingly a stormy sound.
	Sound("WindLoop.ogg", true, 80, nil, 1);
	return 1;
}

global func FxSnowStormTimer(object target, proplist effect)
{
	// Change wind every now and then.
	if (!Random(200))
		effect.wind = (2 * Random(2) - 1) * (90 + Random(10));

	// Adapt current wind to target wind and add a little random.
	var wind = GetWind(0, 0, true);
	if (effect.wind > wind)
		SetWind(wind + Random(3));
	else if (effect.wind < wind)
		SetWind(wind - Random(3));
	else 
		SetWind(wind + 1 - Random(3));
	return 1;
}

protected func InitializePlayer(int plr)
{ 
	// Move clonks to location and give them a shovel.
	var index = 0, crew;
	while (crew = GetCrew(plr, index))
	{
		crew->SetPosition(mountain_location[0], mountain_location[1]);
		// First clonk can construct, others can mine.
		if (index == 0)
		{
			crew->CreateContents(Hammer);
			crew->CreateContents(Axe);
			crew->CreateContents(Dynamite, 2);
		}
		else
		{
			crew->CreateContents(Shovel);
			crew->CreateContents(Pickaxe);
			crew->CreateContents(Dynamite, 2);
		}
		index++;
	}
	SetPlayerZoomByViewRange(plr, 5000, 3500, PLRZOOM_LimitMax);
	return;
}

