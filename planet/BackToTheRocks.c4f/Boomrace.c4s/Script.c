/*-- 
	Boomrace 
	Author: Newton
	
	A parkour on boompacks.
--*/

protected func Initialize()
{
	// Create parkour goal & checkpoints.
	var goal = CreateObject(Goal_Parkour, 0, 0, NO_OWNER);
	var mode = PARKOUR_CP_Respawn | PARKOUR_CP_Check | PARKOUR_CP_Ordered;
	goal->SetStartpoint(90, 820);
	goal->AddCheckpoint(660, 580, mode);
	goal->AddCheckpoint(500, 270, mode);
	goal->AddCheckpoint(1850, 90, mode);
	goal->AddCheckpoint(1650, 740, mode);
	goal->AddCheckpoint(2200, 870, mode);
	goal->AddCheckpoint(3300, 240, mode);
	goal->AddCheckpoint(3830, 710, mode);
	goal->SetFinishpoint(3650, 180);

	// Red sky.
	SetSkyAdjust(RGB(255, 128, 0), RGB(0, 0, 0));
}

// Gamecall from parkour goal, on respawning.
protected func PlrHasRespawned(int plr, object cp)
{
	var clonk = GetCrew(plr);
	clonk->CreateContents(Boompack);
}

global func FxRespawnBoomTimer(object target, int num, int time)
{
	target->CreateContents(Boompack);
	return -1;
}
