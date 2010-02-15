/* Sky race */

func Initialize()
{
	var pGoal = CreateObject(Core_Goal_Parkour, 0, 0, NO_OWNER);
	pGoal->SetStartpoint(90, 820);
	pGoal->AddCheckpoint(660, 580, 28);
	pGoal->AddCheckpoint(500, 270, 28);
	pGoal->AddCheckpoint(1850, 90, 28);
	pGoal->AddCheckpoint(1650, 740, 28);
	pGoal->AddCheckpoint(2200, 870, 28);
	pGoal->AddCheckpoint(3300, 240, 28);
	pGoal->AddCheckpoint(3830, 710, 28);
	pGoal->SetFinishpoint(3650, 180);

	ScriptGo(1);
}

// Gamecall from Race-goal, on respawning.
func PlrHasRespawned(int iPlr, object cp)
{
	var clonk = GetCrew(iPlr);
	if(clonk->Contents())
		clonk->Contents()->RemoveObject();
}

func Script1()
{
	for(var i=0; i<GetPlayerCount(); i++)
	{
		var clonk = GetCursor(GetPlayerByIndex(i));
		if(clonk)
			clonk->CreateContents(BOOM);
	}
}

func Script15()
{
	goto(1);
}