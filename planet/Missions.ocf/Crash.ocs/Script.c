/**
	Crash
	First attempt at a nontrivial settlement scenario
	
	@authors Sven2, Maikel, ck
*/

static g_is_initialized;

func DoInit(int first_player)
{
	CreateObject(Windmill, 152, 825+48, 0);

	// Set time of day to evening and create some clouds and celestials.
	Cloud->Place(20);
	CreateObject(Environment_Celestial);
	var time = CreateObject(Environment_Time);
	time->SetTime(600);
	time->SetCycleSpeed(20);
	// Waterfall
	AddEffect("IntWaterfall", nil, 1, 1);
	// Windmill owner
	var windmill = FindObject(Find_ID(Windmill));
	if (windmill) windmill->SetOwner(first_player);
	
	// Goal
	CreateObject(Goal_Plane);
	
	// Rules
	CreateObject(Rule_TeamAccount, 50, 50);
	
	// NPC: Merchant.
	var merchant = CreateObject(Clonk, 170, 870);
	merchant->MakeInvincible();
	merchant->MakeNonFlammable();
	merchant->SetSkin(1);
	merchant->SetName("$NameMerchant$");
	merchant->SetColor(RGB(55, 65, 75));
	merchant->SetDir(DIR_Left);
	merchant->SetObjectLayer(merchant);
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
	var crew;
	// Scenario init
	if (!g_is_initialized) g_is_initialized = DoInit(plr);
	// Start intro if not yet started
	IntroStart();
	// Add player to intro if recently started
	if(!IntroAddPlayer(plr))
	{
		// Too late for entry? Just start in the village
		var index = 0;
		for(var index = 0; crew = GetCrew(plr, index); ++index)
		{
			var x = 50 + Random(50);
			var y = 850;
			crew->SetPosition(x , y);
		}
	}

	// Give clonks initial tools
	for(var index = 0; crew = GetCrew(plr, index); ++index)
	{
		crew->CreateContents(Shovel);
		// First Clonk can construct and mine.
		if (!index)
		{
			crew->CreateContents(Hammer);
			crew->CreateContents(Axe);
		}
	}
	return;
}

