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
	// Clear trees at the starting base
	var trees = FindObjects(Find_InRect(180*8, 30*8, 50*8, 20*8), Find_Func("IsTree"));
	for (var tree in trees)
		tree->RemoveObject();
	// Create the start buildings: 2 x flag, 3x wind generator, armory, cabin
	CreateConstruction(Flagpole, 193*8, 43*8, first_player, 100, false);
	CreateConstruction(Flagpole, 221*8, 46*8, first_player, 100, false);
	CreateConstruction(WoodenCabin, 228*8, 46*8, first_player, 100, true);
	CreateConstruction(Armory, 212*8, 46*8, first_player, 100, true);
	CreateConstruction(WindGenerator, 197*8, 44*8, first_player, 100, true);
	CreateConstruction(WindGenerator, 200*8, 44*8, first_player, 100, true);
	CreateConstruction(WindGenerator, 203*8, 45*8, first_player, 100, true);

	// Earth objects
	for (var i = 0; i < 70; ++i)
	{
		var stuff = [Firestone, Firestone, Rock, Rock, Rock, Nugget];
		var location = FindLocation(Loc_Material("Earth"));//, Loc_Space(5, false), Loc_Space(5, true));
		if (location)
		{
			CreateObjectAbove(stuff[Random(GetLength(stuff))], location.x, location.y);
		}
	}
	// Other environment stuff
	Zaphive->Place();

	return true;
}

func InitializePlayer(int plr)
{
	// Players only
	if (GetPlayerType(plr)!=C4PT_User) return;
	// Scenario init
	if (!g_is_initialized) g_is_initialized = DoInit(plr);
	// Create start material: Hammer, shovel, axe
	var clonk1 = GetCrew(plr, 0);
	var clonk2 = GetCrew(plr, 1);
	if (clonk1)
	{
		clonk1->CreateContents(Shovel);
		clonk1->CreateContents(Hammer);
	}
	if (clonk2)
	{
		clonk2->CreateContents(Shovel);
		clonk2->CreateContents(Axe);
	}
}

func OnGoalsFulfilled()
{
	GainScenarioAchievement("Done");
	return false;
}
