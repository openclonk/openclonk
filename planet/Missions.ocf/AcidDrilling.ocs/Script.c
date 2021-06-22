/* Acid drilling */

static g_was_player_init, g_crystal_player;

func Initialize()
{
	g_crystal_player = NO_OWNER;
	// Environment
	SetSkyParallax(1, 20, 20, 0, 0, nil, nil);

	CreateObject(PlayerStart)->SetStartingCrew([{id = Clonk, count = 2}])
                         ->SetStartingBaseMaterial([])
                         ->SetStartingMaterial([])
                         ->SetStartingKnowledge();
	return true;
}

func InitializePlayer(proplist plr)
{
	var i;
	// Script player owns power crystals
	if (plr.Type == C4PT_Script)
	{
		g_crystal_player = plr;
		for (i = 0; i<GetPlayerCount(C4PT_User); ++i) plr->SetHostility(GetPlayerByIndex(i, C4PT_User), true, true, true);
		while (plr->GetCrew()) plr->GetCrew()->RemoveObject();
		InitPowerCrystals(plr);
		return true;
	}
	if (g_crystal_player != NO_OWNER) g_crystal_player->SetHostility(plr, true, true, true);
	// First player init base
	if (!g_was_player_init)
	{
		CreateScriptPlayer("POMMES", 0, 0, CSPF_FixedAttributes | CSPF_NoEliminationCheck | CSPF_Invisible);
		InitBase(plr);
		g_was_player_init = true;
	}
	// Position and materials
	var crew;
	for (i = 0; crew = plr->GetCrew(i); ++i)
	{
		crew->SetPosition(2100 + Random(40), 233-10);
		crew->CreateContents(Shovel);
	}
	// Base material
	var materials = [
		[Clonk, 50, 50],
		[Bread, 50, 50],
		[Wood , 50, 50],
		[Metal, 50, 50]
	];
	GivePlayerBaseMaterial(plr, materials);
	// Knowledge
	plr->GiveKnowledge([Flagpole, Foundry, WindGenerator, Compensator, Sawmill, ChemicalLab, Elevator, Pump, ToolsWorkshop, Basement, WallKit, GoldBar, Loam, Metal, Axe, Barrel, Bucket, Dynamite, Hammer, WindBag, Pickaxe, Pipe, Shovel, TeleGlove, DynamiteBox, GrappleBow, InventorsLab, Lorry, Ropeladder, WoodenBridge, Chest]);
	return true;
}

private func InitPowerCrystals(proplist owner)
{
	var positions = [[1013, 59], [1030, 320], [1050, 500], [1000, 660]];
	for (var pos in positions) CreateObjectAbove(PowerCrystals, pos[0], pos[1]+16, owner);
	return true;
}

private func InitBase(proplist owner)
{
	// Create standard base owned by player
	var y = 232;
	var lorry = CreateObjectAbove(Lorry, 2040, y-2, owner);
	if (lorry)
	{
		lorry->CreateContents(Loam, 6);
		lorry->CreateContents(Wood, 15);
		lorry->CreateContents(Metal, 4);
		lorry->CreateContents(WallKit, 2);
		lorry->CreateContents(Axe, 1);
		lorry->CreateContents(Pickaxe, 1);
		lorry->CreateContents(Hammer, 1);
		lorry->CreateContents(Dynamite, 2);
		lorry->CreateContents(Pipe, 2);
	}
	CreateObjectAbove(Pump, 2062, y, owner);
	CreateObjectAbove(Flagpole, 2085, y, owner);
	CreateObjectAbove(WindGenerator, 2110, y, owner);
	CreateObjectAbove(ToolsWorkshop, 2150, y, owner);
	CreateObjectAbove(WindGenerator, 2200, y, owner);
	CreateObjectAbove(WoodenCabin, 2250, y, owner);
	
	CreateObjectAbove(Foundry, 1793, y, owner);
	CreateObjectAbove(Flagpole, 1819, y, owner);
	CreateObjectAbove(Sawmill, 1845, y, owner);
	return true;
}

func OnGoalsFulfilled()
{
	for (var player in GetPlayers(C4PT_User)) player->GainScenarioAchievement("Done");
	return false;
}
