/* Sky race */

protected func Initialize()
{
	for (var i=0; i<20; ++i) CreateObject(LOAM, 1560+Random(11)-5, 200+Random(11)-5, NO_OWNER);
	for (var i=0; i<20; ++i) CreateObject(DYNA, 2730+Random(11)-5, 660+Random(11)-5, NO_OWNER);
	// Create the race goal.
	var pGoal = CreateObject(Core_Goal_Parkour, 0, 0, NO_OWNER);
	pGoal->SetStartpoint(50, 475);
	pGoal->AddCheckpoint(930, 510, RACE_CP_Respawn);
	pGoal->AddCheckpoint(1700, 300, RACE_CP_Respawn);
	pGoal->AddCheckpoint(2730, 730, RACE_CP_Respawn);
	pGoal->SetFinishpoint(4950, 475);
}

// Gamecall from Race-goal, on respawning.
protected func PlrHasRespawned(int iPlr, object cp)
{
	var clonk = GetCrew(iPlr);
	clonk->CreateContents(BOW1);
	clonk->CreateContents(DYNB);
	clonk->CreateObject(CLUB);
	return;
}


