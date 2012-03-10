/**
	Crash
	First attempt at a nontrivial settlement scenario
	
	@authors Sven2 Maikel ck
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
	AddEffect("IntWaterfall", 0, 1, 1);
	// Windmill owner
	var windmill = FindObject(Find_ID(Windmill));
	if (windmill) windmill->SetOwner(first_player);
	
	// Goal
	CreateObject(Goal_Plane);
	
	// NPC: Pilot.
	var man = CreateObject(Clonk, 100, 870);
	man->SetSkin(2);
	man->SetName("$NamePilot$");
	man->SetColor(RGB(55, 65, 75));
	man->SetDir(DIR_Left);
	man->SetObjectLayer(man);
	man->SetDialogue("Pilot");
	
	// NPC: Merchant.
	var merchant = CreateObject(Clonk, 170, 870);
	merchant->SetSkin(1);
	merchant->SetName("$NameMerchant$");
	merchant->SetColor(RGB(55, 65, 75));
	merchant->SetDir(DIR_Left);
	merchant->SetObjectLayer(man);
	merchant->SetDialogue("Merchant");
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
		// First Clonk can construct and mine.
		if (!index)
		{
			crew->CreateContents(Hammer);
			crew->CreateContents(Axe);
		}
		index++;
	}
	return;
}

