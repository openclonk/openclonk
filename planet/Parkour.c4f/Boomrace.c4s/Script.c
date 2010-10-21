/*-- 
	Boomrace 
	Author: Newton
	
	A parkour on boompacks.
--*/

protected func Initialize()
{
	var goal = CreateObject(Goal_Parkour, 0, 0, NO_OWNER);
	goal->SetStartpoint(90, 820);
	goal->AddCheckpoint(660, 580, 28);
	goal->AddCheckpoint(500, 270, 28);
	goal->AddCheckpoint(1850, 90, 28);
	goal->AddCheckpoint(1650, 740, 28);
	goal->AddCheckpoint(2200, 870, 28);
	goal->AddCheckpoint(3300, 240, 28);
	goal->AddCheckpoint(3830, 710, 28);
	goal->SetFinishpoint(3650, 180);

	SetSkyAdjust(RGB(255,128,0), RGB(0,0,0));
}

// Gamecall from parkour goal, on respawning.
protected func PlrHasRespawned(int iPlr, object cp)
{
	var clonk = GetCrew(iPlr);
	clonk->CreateContents(Boompack);
}

global func FxRespawnBoomTimer(object target, int num, int time)
{
	target->CreateContents(Boompack);
	return -1;
}