/**
	LostMine
	
	@authors ck
*/

static g_is_initialized;

func DoInit(int first_player)
{
	var goal = CreateObject(Goal_ElevatorEnergy);
	var elevator = FindObject(Find_ID(Elevator));
	goal->SetTarget(elevator.case);

	elevator->CreateShaft(240);
	elevator->SetOwner(first_player);
	elevator.case->SetPosition(elevator.case->GetX(), elevator.case->GetY()+190);
	// Create the start buildings: 2 x flag, windmill, armory, cabin
	CreateObject(Flagpole, 193*8, 43*8, first_player);
	CreateObject(Flagpole, 221*8, 46*8, first_player);
	CreateObject(WoodenCabin, 228*8, 47*8, first_player);
	CreateObject(Armory, 212*8, 47*8, first_player);
	CreateObject(WindGenerator, 197*8, 44*8, first_player);
	// Create start material: Hammer, shovel, axe
	var clonk1 = GetCrew(first_player, 0);
	var clonk2 = GetCrew(first_player, 1);
	clonk1->CreateContents(Shovel);
	clonk1->CreateContents(Hammer);
	clonk2->CreateContents(Shovel);
	clonk2->CreateContents(Axe);

	// Earth objects
	for(var i = 0; i < 70; ++i)
	{
		var stuff = [Firestone, Firestone, Rock, Rock, Rock, Nugget];
		var location = FindLocation(Loc_Material("Earth"));//, Loc_Space(5, false), Loc_Space(5, true));
		if(location)
		{
			CreateObject(stuff[Random(GetLength(stuff))], location.x, location.y);
		}
	}

	return true;
}

func InitializePlayer(int plr)
{
	// Players only
	if (GetPlayerType(plr)!=C4PT_User) return;
	// Scenario init
	if (!g_is_initialized) g_is_initialized = DoInit(plr);
	return true;
}

func OnGoalsFulfilled()
{
	GainScenarioAchievement("Done");
	return false;
}
