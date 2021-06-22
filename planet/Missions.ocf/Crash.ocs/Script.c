/**
	Crash
	First attempt at a nontrivial settlement scenario
	
	@authors Sven2, Maikel, ck
*/

static g_is_initialized, g_has_bought_plans, npc_pyrit;

public func Initialize()
{
	// Show wealth in HUD.
	GUI_Controller->ShowWealth();
	return true;
}

func DoInit(proplist first_player)
{

	CreateObjectAbove(Windmill, 152, 825 + 48, 0);

	// Set time of day to evening and create some clouds and celestials.
	Cloud->Place(20);
	var time = CreateObject(Time);
	time->SetTime(600);
	time->SetCycleSpeed(20);
	// Waterfall
	AddEffect("IntWaterfall", nil, 1, 1);
	// Windmill owner
	var windmill = FindObject(Find_ID(Windmill));
	if (windmill) windmill->SetOwner(first_player);
	
	// Goal
	CreateObject(Goal_Airplane);
	
	// Rules
	CreateObject(Rule_TeamAccount, 50, 50);
	
	// NPC: Merchant.
	var merchant = CreateObjectAbove(Clonk, 76, 870);
	merchant->MakeInvincible();
	merchant->SetColor(RGB(55, 65, 75)); // currently overridden by skin
	merchant->SetAlternativeSkin("Leather");
	merchant->SetName("$NameMerchant$");
	merchant->SetDir(DIR_Left);
	merchant->SetObjectLayer(merchant);
	merchant->SetDialogue("Merchant", true);
	
	// Newbies like to kill off all trees
	var tree_area = Shape->Rectangle(166, 774, 300, 170);
	ScheduleCall(nil, Scenario.EnsureTrees, 100, 99999999, tree_area);
	
	// Start intro if not yet started
	StartSequence("Intro", 0, first_player->GetCrew());
	
	GetRelaunchRule()
		->SetInventoryTransfer(true)
		->SetLastClonkRespawn(true)
		->SetFreeCrew(true)
		->SetAllowPlayerRestart(true)
		->SetBaseRespawn(true)
		->SetRespawnDelay(0);
	
	return true;
}

func EnsureTrees(proplist area)
{
	var nr_trees = ObjectCount(Find_Func("IsTree"), area->Find_In());
	if (nr_trees < 2)
		PlaceVegetation(Tree_Coniferous, 0, 0, LandscapeWidth(), LandscapeHeight(), 10, area);
	return true;
}

global func FxIntWaterfallTimer(object obj, proplist eff)
{
	InsertMaterial(Material("Water"), 1560, 840);
	ExtractLiquid(1314, 901);
}

func InitializePlayer(proplist plr)
{
	var crew;
	// Scenario init
	if (!g_is_initialized) g_is_initialized = DoInit(plr);
	// Late joining players just start in the village
	var index;
	for (index = 0; crew = plr->GetCrew(index); ++index)
	{
		if (!crew->Contained()) // if not put into plane by intro
		{
			var x = 50 + Random(50);
			var y = 850;
			crew->SetPosition(x , y);
		}
	}
	
	// Extra plans from merchant to newly joined players
	if (g_has_bought_plans) GiveExtraPlans(plr);

	// Give clonks initial tools
	for (var index = 0; crew = plr->GetCrew(index); ++index)
	{
		crew->CreateContents(Shovel);
		// First Clonk can construct and mine.
		if (!index)
		{
			crew->CreateContents(Hammer);
			crew->CreateContents(Axe);
		}
	}
	
	plr->GiveKnowledge([Chest, Idol, Foundry, SteamEngine, ToolsWorkshop, WindGenerator, Flagpole, Sawmill, Elevator, Armory, ChemicalLab, Basement, Lorry, Pickaxe, Axe, Hammer, Shovel, Barrel, Dynamite, DynamiteBox, Loam, Bucket]);
	
	return true;
}

func OnGoalsFulfilled()
{
	SetNextScenario("Missions.ocf/DeepSeaMining.ocs");
	for (var player in GetPlayers(C4PT_User)) player->GainScenarioAchievement("Done");
	GainScenarioAccess("S2Crash");
	return false;
}

func GiveExtraPlans(proplist plr)
{
	plr->GiveKnowledge(Pump);
	plr->GiveKnowledge(Pipe);
	plr->GiveKnowledge(Catapult);
	plr->GiveKnowledge(Cannon);
	return true;
}
