/**
	Aerobatics
	Several small sky islands form a chaotic parkour with lots of usable and respawning items.
	
	@author Maikel
*/


protected func Initialize()
{
	// Create the parkour goal.
	var goal = CreateObject(Goal_Parkour);
	goal->TransferContentsOnRelaunch(true);
	
	// Iterate over checkpoint list and create the checkpoints.
	// Also add items spawns close to the checkpoints.
	var map_zoom = GetScenarioVal("MapZoom", "Landscape");
	for (var index = 0; index < GetLength(checkpoint_locations); index++)
	{
		var cp = checkpoint_locations[index];
		var x = map_zoom * cp[0] + map_zoom / 2;
		var y = map_zoom * cp[1] + map_zoom / 2 - 20;
		if (index == 0)
		{
			goal->SetStartpoint(x, y);
			ItemSpawn->Create(Dynamite, x - 30, y);
			ItemSpawn->Create(Loam, x + 30, y);
			continue;
		}
		if (index == GetLength(checkpoint_locations) - 1)
		{
			goal->SetFinishpoint(x, y);
			if (SCENPAR_GameMode == 1)
				ItemSpawn->Create(Balloon, x + 30, y);			
			continue;
		}		
		var mode = PARKOUR_CP_Check | PARKOUR_CP_Ordered | PARKOUR_CP_Respawn | PARKOUR_CP_Bonus;
		goal->AddCheckpoint(x - 10, y, mode);
		var spawn_id = GetItemSpawnType(index, y);
		ItemSpawn->Create(spawn_id, x + 20, y);
	}
	
	// Rules: no power and restart with keeping inventory.
	CreateObject(Rule_NoPowerNeed);
	//GetRelaunchRule()
	//	->AllowPlayerRestart()
	//	->SetInventoryTransfer(true);
	
	// Initialize parts of the scenario.
	var amount = BoundBy(SCENPAR_NrCheckPoints, 6, 20);
	InitMaterials(amount);
	InitEnvironment(amount);
	InitVegetation(amount);
	InitAnimals(amount);
	
	// Start the intro sequence.
	StartSequence("Intro", 0, checkpoint_locations[0], checkpoint_locations[-1]);
	return;
}

private func GetItemSpawnType(int cp_number, int height)
{
	// For normal checkpoints the item type depends on the height and is random.
	var spawn_items = [Dynamite, Loam];
	if (cp_number >= 3)
	{
		if (height < LandscapeHeight() / 2)
			var spawn_items = [Dynamite, Loam, DynamiteBox];
		else
			var spawn_items = [Dynamite, Loam, Loam, Boompack];
	}
	return RandomElement(spawn_items);
}

private func InitMaterials(int amount)
{
	// In material objects.
	PlaceObjects(Dynamite, 4 * amount, "Earth");
	PlaceObjects(Loam, 4 * amount, "Earth");
	PlaceObjects(Metal, 2 * amount, "Earth");
	PlaceObjects(Cloth, amount, "Earth");
	// Additional item spawns.
	if (SCENPAR_GameMode == 2)
	{
		// There is no balloon in the horizontal mode at the finish so place one randomly in the first part of the map.
		var pos = FindLocation(Loc_Sky(), Loc_Space(30), Loc_InRect(LandscapeWidth() / 6, 150, LandscapeWidth() / 6, LandscapeHeight() - 300));
		if (pos)
			ItemSpawn->Create(Balloon, pos.x, pos.y);
		
	}
	// Place chests on several of the sky islands.
	for (var count = 0; count < amount / 2; count++)
	{
		var pos = FindIslandLocation(true);
		if (!pos)
			continue;	
		var chest = CreateObjectAbove(Chest, pos.x, pos.y);
		chest->CreateContents(Dynamite, 4);
		chest->CreateContents(Club, 4);
		chest->CreateContents(Blunderbuss)->CreateContents(LeadBullet);
		chest->CreateContents(Blunderbuss)->CreateContents(LeadBullet);
		chest->CreateContents(Blunderbuss)->CreateContents(LeadBullet);
		chest->CreateContents(IronBomb, 4);
		chest->CreateContents(GrenadeLauncher)->CreateContents(IronBomb);
		chest->CreateContents(GrenadeLauncher)->CreateContents(IronBomb);
		if (!Random(2))
			chest->CreateContents(Boompack);
		if (!Random(2))
			chest->CreateContents(WallKit);
	}
	// Load all weapons in the chests.
	for (var weapon in FindObjects(Find_Or(Find_ID(Blunderbuss), Find_ID(GrenadeLauncher))))
		weapon->SetLoaded();
	// Place some catapults.
	for (var count = 0; count < amount / 4; count++)
	{
		var pos = FindIslandLocation();
		if (pos)
			CreateObjectAbove(Catapult, pos.x, pos.y);
	}	
	// Place some cannons.
	for (var count = 0; count < amount / 4; count++)
	{
		var pos = FindIslandLocation();
		if (pos)
		{
			var cannon = CreateObjectAbove(Cannon, pos.x, pos.y);
			cannon->CreateContents(PowderKeg, 1);
		}
	}
	// An inventor's lab on its island.
	var map_zoom = GetScenarioVal("MapZoom", "Landscape");
	var lab = CreateObjectAbove(InventorsLab, inventorslab_location[0] * map_zoom, inventorslab_location[1] * map_zoom);
	lab->MakeInvincible();
	return;
}

private func FindIslandLocation(bool is_chest)
{
	var map_zoom = GetScenarioVal("MapZoom", "Landscape");	
	var pos = {x = inventorslab_location[0] * map_zoom, y = inventorslab_location[1] * map_zoom};
	for (var tries = 0; tries < 200; tries++)
	{
		pos = FindLocation(Loc_Sky(), Loc_Wall(CNAT_Bottom));
		if (!pos)
			continue;
		if (FindObject(Find_Category(C4D_Vehicle), Find_Distance(30, pos.x, pos.y)))
			continue;
		if (is_chest && FindObject(Find_Or(Find_ID(ParkourCheckpoint), Find_ID(Chest)), Find_Distance(30, pos.x, pos.y)))
			continue;
		break;
	}
	return pos;
}

private func InitEnvironment(int amount)
{
	for (var count = 0; count < amount / 2; count++)
	{
		var pos = FindLocation(Loc_Sky(), Loc_Space(60), Loc_InRect(100, 100, LandscapeWidth() - 200, LandscapeHeight() - 200));
		if (!pos)
			return;
		var pos2 = FindLocation(Loc_Sky(), Loc_Space(40), Loc_InRect(pos.x - 20, pos.y - 60, 40, 40));
		if (!pos2)
			return;
		JetStream->CreateLine([[pos.x, pos.y + 20], [pos.x + RandomX(-8, 8), pos.y - 30 - RandomX(-8, 8)], [pos2.x, pos2.y - 10]], nil, RandomX(32, 48), RandomX(40, 60));
	}
	return;
}

private func InitVegetation(int amount)
{
	Grass->Place(100);
	Branch->Place(3 * amount);
	Mushroom->Place(amount);
	Fern->Place(amount);
	Flower->Place(3 * amount / 2);
	Tree_Deciduous->Place(amount);
	Tree_Coniferous2->Place(amount / 4);
	Tree_Coniferous3->Place(amount / 4);
	Cotton->Place(amount / 2);
	Vine->Place(amount);
	return;
}

private func InitAnimals(int amount)
{
	Butterfly->Place(amount);
	Mosquito->Place(amount / 2);
	Zaphive->Place(amount / 4);
	return;
}


/*-- Player Control --*/

protected func InitializePlayer(int plr)
{
	// Make the player enemy to all other players.
	Goal_Melee->MakeHostileToAll(plr, GetPlayerTeam(plr));
	// Set large zoom ranges for the player.
	SetPlayerZoomByViewRange(plr, 1200, nil, PLRZOOM_LimitMax);
	// Give the player knowledge for items in the inventor's lab.
	SetPlrKnowledge(plr, WindBag);
	SetPlrKnowledge(plr, WallKit);
	SetPlrKnowledge(plr, Balloon);	
	return;
}

public func OnPlayerRespawn(int plr, object cp)
{
	var crew = GetCrew(plr);
	if (!crew)
		return;
	// Ensure at least a shovel, loam and dynamite on respawn when items not present and there is space in inventory.
	if (!FindObject(Find_ID(Shovel), Find_Container(crew)) && crew->ContentsCount() < crew.MaxContentsCount)
		crew->CreateContents(Shovel);
	if (!FindObject(Find_ID(Loam), Find_Container(crew)) && crew->ContentsCount() < crew.MaxContentsCount)
		crew->CreateContents(Loam);
	if (!FindObject(Find_ID(Dynamite), Find_Container(crew)) && crew->ContentsCount() < crew.MaxContentsCount)
		crew->CreateContents(Dynamite);
	// Give the crew more HP, so that it can survive more flint jumps.
	crew.MaxEnergy = 80000;
	crew->DoEnergy(crew.MaxEnergy / 1000);	
	return;
}

// Give the player a bonus when he reaches a new checkpoint for the first time and is behind the leader.
public func GivePlrBonus(int plr, object cp)
{
	var crew = GetCrew(plr);
	if (!crew)
		return;
	var goal = cp->GetCPController();
	if (!goal)
		return;
	var cp_behind = goal->GetLeaderClearedCheckpoints() - goal->GetPlayerClearedCheckpoints(plr);
	if (cp_behind <= 2)
		return;
	var windbag = FindObject(Find_ID(WindBag), Find_Container(crew));
	if (windbag)
	{
		windbag->SetUsageCount(windbag->GetUsageCount() + cp_behind);	
	}
	else
	{
		if (crew->ContentsCount() < crew.MaxContentsCount)
			windbag = crew->CreateContents(WindBag);
		else
			windbag = CreateObjectAbove(WindBag, cp->GetX(), cp->GetY() + cp->GetBottom(), plr);
		windbag->SetUsageCount(2 * cp_behind);
	}
	return;
}