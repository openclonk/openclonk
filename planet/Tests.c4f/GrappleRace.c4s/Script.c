/*-- 
	GrappleRace 
	Author: Mimmo
	
	You got two grapplers. Get to the goal as fast as possible!
--*/


/*-- Scenario properties --*/

protected func Initialize()
{
	// Create the parkour.
	CreateParkour();
	
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
	x = 50; y = LandscapeHeight() / 2 -120;
	goal->SetStartpoint(x, y)->SetCPSize(40);
	
	var mode = PARKOUR_CP_Respawn | PARKOUR_CP_Ordered | PARKOUR_CP_Team;
		goal->AddCheckpoint(2325, 1300, mode)->SetCPSize(30);;
		goal->AddCheckpoint(4400, 930, mode)->SetCPSize(30);;
	
	// Create finish checkpoint.
	x = LandscapeWidth() - 600; y = LandscapeHeight() -300;
	goal->SetFinishpoint(x, y)->SetCPSize(60);
	return;
}

/*-- Player respawn --*/

// Callback from parkour goal, on player respawn.
protected func PlrHasRespawned(int plr, object cp)
{
	var clonk = GetCrew(plr);

	clonk->SetPosition(cp->GetX(),cp->GetY());
	clonk->CreateContents(GrappleBow,2);
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


protected func InitializePlayer(int plr)
{
	// No fog ow war in this scenario.
	SetFoW(false, plr);
	return;
}
