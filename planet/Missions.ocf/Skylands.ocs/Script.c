/**
	Skylands
	Assemble a plane on some floating islands
	
	@authors Sven2
*/

static g_is_initialized;
static g_intro_initialized;

func DoInit(int first_player)
{
	// Test
	//CreateObjectAbove(LiftTower, 178, 405, first_player);
	// Set time of day to evening and create some clouds and celestials.
	Cloud->Place(15);
	EnsureObject(Rule_BuyAtFlagpole, 0, 0,-1);
	SetSkyAdjust(0xff000000);
	var storm = EnsureObject(Storm, 0, 0, NO_OWNER);
	storm->SetStorm(-20, 0, 1000);
	SetSkyParallax(1); // move background with the wind
	var time = EnsureObject(Time, 0, 0,-1);
	time->SetTime(600);
	time->SetCycleSpeed(20);
	// Goal
	CreateObject(Goal_Plane);
	// Plane part restore
	for (var part in FindObjects(Find_Func("IsPlanePart"))) part->AddRestoreMode();
	return true;
}

func EnsureObject(id def, int x, int y, int owner)
{
	var obj = FindObject(Find_ID(def));
	if (!obj) obj = CreateObjectAbove(def, x, y, owner);
	return obj;
}

func InitializePlayer(int plr)
{
	// Scenario init
	if (!g_is_initialized) g_is_initialized = DoInit(plr);
	// Move clonks to location and give them a shovel.
	var index = 0, crew;
	while (crew = GetCrew(plr, index))
	{
		var x = 150 + Random(50);
		crew->SetPosition(x , 390);
		crew->CreateContents(Shovel);
		// one clonk can construct, another can mine.
		if (index == 1)
			crew->CreateContents(Hammer);
		else
			crew->CreateContents(Axe);
		index++;
	}
	return;
}

func OnPlaneFinished(object plane)
{
  // todo: outro
  plane->CreateObjectAbove(Airplane, 0, 0, NO_OWNER);
  plane->RemoveObject();
}

func OnGoalsFulfilled()
{
	GainScenarioAchievement("Done");
	return false;
}
