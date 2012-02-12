/*-- 
	AirRace 
	Author: Maikel
	
	Fly with an airplane round a parkour, the first one to reach the finish wins.
	On the way some checkpoints must be passed, also weapons can be picked up.
--*/


/*-- Scenario properties --*/

protected func Initialize()
{
	// Create the parkour.
	CreateParkour();
	
	// Create some weapon spawns.
	CreateWeaponSpawns();

	// Vegetation.
	PlaceGrass(100);
	
	// Environment.
	CreateObject(Environment_Clouds);

	return;
}

// Create parkour, start & finish on the brick islands, random separated checkpoints in-between.
private func CreateParkour()
{
	// Find parkour goal and create on if non-existent.
	var goal = FindObject(Find_ID(Goal_Parkour));
	if (!goal)
		goal = CreateObject(Goal_Parkour, 0, 0, NO_OWNER);
		
	var x, y;	
	// Create start checkpoint.
	x = 50; y = LandscapeHeight() / 2;
	while (!GBackSolid(x, y + 20) && y < LandscapeHeight())
		y += 20;
	goal->SetStartpoint(x, y)->SetCPSize(40);
	
	// Create some checkpoints in between.
	var mode = PARKOUR_CP_Check | PARKOUR_CP_Ordered | PARKOUR_CP_Respawn | PARKOUR_CP_Bonus;
	for (var dx = 500; dx < LandscapeWidth() - 600; dx += RandomX(500, 600))
	{
		var pos = FindCheckpointPosition(dx, 200, 250, LandscapeHeight() - 400, 40);
		if (pos)
			goal->AddCheckpoint(pos[0], pos[1], mode)->SetCPSize(40);
	}
	
	// Create finish checkpoint.
	x = LandscapeWidth() - 200; y = LandscapeHeight() / 2;
	while (!GBackSolid(x, y + 20) && y < LandscapeHeight())
		y += 20;
	goal->SetFinishpoint(x, y)->SetCPSize(40);
	return;
}

// Finds a position a plane - of size - can actually fly through, returns [x, y] as a position.
private func FindCheckpointPosition(int rx, int ry, int wdt, int hgt, int size)
{
	for (var i = 0; i < 500; i++)
	{
		var x = rx + Random(wdt);
		var y = ry + Random(hgt);
		// Not to close to other checkpoints.
		if (FindObject(Find_Distance(800, x, y), Find_ID(ParkourCheckpoint)))
			continue;
		// Check circle of size, with 5x5 raster.
		var found_pos = true;
		for (var cx = -size; cx <= size; cx += 5)
			for (var cy = -size; cy <= size; cy += 5)
				if (Distance(0, 0, cx, cy) < size ** 2)
					if (GBackSemiSolid(x + cx, y + cy))
						found_pos = false;
		if (found_pos)	
			return [x, y];
	}
	return nil;
}

// Creates some weapon spawns, players have to fly through them to pick up the weapon.
private func CreateWeaponSpawns()
{
	for (var i = 0; i < 12; i++)
	{
		var pos = FindWeaponSpawnPosition(300, 200, LandscapeWidth() - 600, LandscapeHeight() - 400, 40);
		if (pos)
			CreateObject(WeaponSpawn, pos[0], pos[1], NO_OWNER)->SetSpawnSize(40);		
	}
	return;
}

// Finds a position a plane - of size - can actually fly through, returns [x, y] as a position.
private func FindWeaponSpawnPosition(int rx, int ry, int wdt, int hgt, int size)
{
	for (var i = 0; i < 500; i++)
	{
		var x = rx + Random(wdt);
		var y = ry + Random(hgt);
		// Not to close to other checkpoints.
		if (FindObject(Find_Distance(700, x, y), Find_ID(WeaponSpawn)))
			continue;
		if (FindObject(Find_Distance(400, x, y), Find_ID(ParkourCheckpoint)))
			continue;
		// Check circle of size, with 5x5 raster.
		var found_pos = true;
		for (var cx = -size; cx <= size; cx += 5)
			for (var cy = -size; cy <= size; cy += 5)
				if (Distance(0, 0, cx, cy) < size ** 2)
					if (GBackSemiSolid(x + cx, y + cy))
						found_pos = false;
		if (found_pos)	
			return [x, y];
	}
	return nil;
}

/*-- Player respawn --*/

// Callback from parkour goal, on player respawn.
protected func OnPlayerRespawn(int plr, object cp)
{
	var clonk = GetCrew(plr);
	var plane = CreateObject(Plane, cp->GetX(), cp->GetY(), plr);
	clonk->Enter(plane);
	plane->CreateContents(Bullet);
	var mode = cp->GetCPMode();
	if (mode & PARKOUR_CP_Start)
		plane->SetR(90);
	else if (mode & PARKOUR_CP_Respawn)
	{
		var angle = FindPlaneAngle(cp);
		plane->StartInstantFlight(angle, 15);
	}
	return;
}

private func FindPlaneAngle(object cp)
{
	// Start to the right.
	var angle = 90, i = 0, dir = 1;
	var x = cp->GetX();
	var y = cp->GetY();	
	while (Abs(i) < 180)
	{
		if (PathFree(x, y, x + Sin(angle + i, 200), y + Cos(angle + i, 200)))
		{
				dir = 1;
				break;
		}
		if (PathFree(x, y, x + Sin(angle - i, 200), y + Cos(angle - i, 200)))
		{
				dir = -1;
				break;
		}
		i += 10;
	}
	return angle + dir * i;
}

protected func GivePlrBonus(int plr, object cp)
{
	var plane = GetCursor(plr)->Contained();
	if (plane && plane->GetID() == Plane)
		plane->CreateContents(Bullet);
	return;
}

protected func InitializePlayer(int plr)
{
	// No fog ow war in this scenario.
	SetFoW(false, plr);
	return;
}
