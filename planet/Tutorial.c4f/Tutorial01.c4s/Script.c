/* Tutorial */

static g_wasinit;
static g_has_shovel; // Set after shovel has been discovered

static g_cp_start;
static g_cp_shovel;


/* Scenario init */

private func ScenarioInit()
{
	// Checkppints
	var goal = FindObject(Find_ID(Goal_Tutorial));
	if (!goal) { GameOver(); FatalError("Goal missing - no definitions loaded?"); }
	
	g_cp_start = goal->SetStartpoint(50,200);
	
	goal->SetFinishpoint(1900, 100);
	
	g_cp_shovel = goal->AddCheckpoint(400, 600, "Shovel");
	g_cp_shovel->SetBaseGraphics(Shovel);
	
	goal->AddCheckpoint(880, 650);
	
	g_wasinit = true;
	
	// Dialogue options -> repeat round.
	SetNextMission("Tutorial.c4f\\Tutorial01.c4s", "$MsgRepeatRound$", "$MsgRepeatRoundDesc$");
	return true;
}


/* Player init */

func InitializePlayer(int plr)
{
	// Scenario init done when first player joins
	if (!g_wasinit) ScenarioInit();
	// Script progress reset
	goto(1);
	ScriptGo(true);
	g_has_shovel = false;
	for (var checkpoint in FindObjects(Find_ID(ParkourCheckpoint)))
		checkpoint->ResetCleared();
	var goal = FindObject(Find_ID(Goal_Tutorial));
	if (goal)
	{
		// No direction indicator initially
		goal->DisableDirectionHelp();
		// Join position (InitializePlayer callback to goal object might have come too early)
		goal->SetPlrRespawnCP(plr, g_cp_start);
		goal->JoinPlayer(plr);
	}
	return true;
}

func PlrHasSpawned(int plr, object clonk, object cp)
{
	if (!clonk) return;
	// no shovel initially
	if (!g_has_shovel)
	{
		var shovel = clonk->FindContents(Shovel);
		if (shovel) shovel->RemoveObject();
	}
	return true;
}


/* Script progress */

// Part 1: Welcome
func Script1()
{
	TutMsg("@$MsgIntro0$");
}

func Script11()
{
	TutMsg("@$MsgIntro1$");
	TutArrowShowTarget(GetCrew(GetPlayerByIndex()), 225, 10);
}

func Script30()
{
	TutMsg("u go there. WASD, etc., WWWWWWWWWWWWWW WWWWWWWWWWWWWWWWWWWWWWW WWWWWWWWWWWWWWWWWWW WWWWWWWWWWWWWW");
	TutArrowShowTarget(g_cp_shovel, 135, 30);
	ScriptGo();
}

func Checkpoint_Shovel(int plr, object cp)
{
	TutArrowClear();
	// Give a shovel!
	var clonk = GetCrew(plr);
	if (clonk)
	{
		var shovel = CreateObject(Shovel, clonk->GetX(), clonk->GetY(), plr);
		if (shovel) if (!clonk->Collect(shovel,false,1)) shovel->RemoveObject();
	}
	// In the future, respawn with a shovel
	g_has_shovel = true;
	ScriptGo(1); goto(100);
	return true;
}

func Script101()
{
	TutMsg("got shovel OMG, WWWWWWWWWWWWWW WWWWWWWWWWWWWWWWWWWWWWW WWWWWWWWWWWWWWWWWWW WWWWWWWWWWWWWW");
	ScriptGo();
}
