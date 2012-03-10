/**
	WorkingTitle
	First attempt at a nontrivial settlement scenario
	
	@authors Sven2
*/

static g_is_initialized;

func DoInit(int first_player)
{
	// Set time of day to evening and create some clouds and celestials.
	CreateObject(Environment_Clouds);
	CreateObject(Environment_Celestial);
	var time = CreateObject(Environment_Time);
	time->SetTime(600);
	time->SetCycleSpeed(12);
	// Waterfall
	AddEffect("IntWaterfall", 0, 1, 2);
	// Windmill owner
	var windmill = FindObject(Find_ID(Windmill));
	if (windmill) windmill->SetOwner(first_player);
	return true;
}

global func FxIntWaterfallTimer(object obj, int eff)
{
	InsertMaterial(Material("Water"), 1560,840);
	ExtractLiquid(1314,901);
}

func InitializePlayer(int plr)
{ 
	// Scenario init
	if (!g_is_initialized) g_is_initialized = DoInit(plr);
	// Move clonks to location and give them a shovel.
	var index = 0, crew;
	while (crew = GetCrew(plr, index))
	{
		var x = 50 + Random(50);
		var y = 850;
		crew->SetPosition(x , y);
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

