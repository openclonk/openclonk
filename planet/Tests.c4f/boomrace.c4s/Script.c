/* Sky race */

func Initialize()
{
	var pGoal = CreateObject(Goal_Parkour, 0, 0, NO_OWNER);
	pGoal->SetStartpoint(90, 820);
	pGoal->AddCheckpoint(660, 580, 28);
	pGoal->AddCheckpoint(500, 270, 28);
	pGoal->AddCheckpoint(1850, 90, 28);
	pGoal->AddCheckpoint(1650, 740, 28);
	pGoal->AddCheckpoint(2200, 870, 28);
	pGoal->AddCheckpoint(3300, 240, 28);
	pGoal->AddCheckpoint(3830, 710, 28);
	pGoal->SetFinishpoint(3650, 180);

	SetSkyAdjust (RGB(255,128,0), RGB(0,0,0)); 
}

// Gamecall from Race-goal, on respawning.
func PlrHasRespawned(int iPlr, object cp)
{
	var clonk = GetCrew(iPlr);
	if(clonk->Contents())
		clonk->Contents()->RemoveObject();
	clonk->CreateContents(Boompack);
}

global func FxRespawnBoomTimer(object target, int num, int time)
{
	target->CreateContents(Boompack);
	return -1;
}