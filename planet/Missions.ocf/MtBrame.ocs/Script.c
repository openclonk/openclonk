/**
	Mt. Brame
	Find a way back to your hut, defeating the dangers of Mt. Brame

	@authors ck
*/

static g_is_initialized;

func Initialize()
{
	if (!ObjectCount(Find_ID(Rule_NoPowerNeed))) CreateObject(Rule_NoPowerNeed, 0, 0, NO_OWNER);
}

func DoInit(int first_player)
{
	// Set time of day to morning and create some clouds and celestials.
	Cloud->Place(20);
	var time = CreateObject(Time);
	time->SetTime(400);
	time->SetCycleSpeed(6);
	
	// Workshop owner
	var workshop = FindObject(Find_ID(ToolsWorkshop));
	if (workshop) workshop->SetOwner(first_player);

	// Goal
	CreateObject(Goal_GetBack);
	
	return true;
}

func InitializePlayer(int plr)
{
	var crew;
	// Scenario init
	if (!g_is_initialized) g_is_initialized = DoInit(plr);
	// Start intro if not yet started
	IntroStart();
	// Add player to intro if recently started
	if (!IntroAddPlayer(plr))
	{
		// Too late for entry? Just start in the valley
		var index = 0;
		for (var index = 0; crew = GetCrew(plr, index); ++index)
		{
			var x = 260*8/10 + Random(50);
			var y = 1350*8/10;
			crew->SetPosition(x , y);
			crew->CreateContents(Shovel);
		}
	}
}

func OnGoalsFulfilled()
{
	GainScenarioAchievement("Done");
	return false;
}
