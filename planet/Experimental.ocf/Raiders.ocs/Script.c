/**
	Raiders of the Lost Castle
	A settlement scenario in which the player exploits an old castle ruin and will finally face supernatural forces.
	
	@authors Clonkonaut
*/

static is_initialized;

// Decoration and environmental objects, basic settings

func Initialize()
{
	// Goal: Script goal.
	var goal = CreateObject(Goal_Script);
	goal.Description = "$GoalDesc1$";

	// Vegetation
	Tree_Coniferous->Place(20, Shape->Rectangle(106, 121, 826, 335), { keep_area = true });
	PlaceGrass(20, 0, 106);

	// The castle
	CreateObjectAbove(Raiders_CastleBack, 1068, 256, NO_OWNER);
}

// Player related supplies and structures

func DoInit(int plr)
{
	if (is_initialized) return;

	// Supplies
	var crate = CreateObjectAbove(Crate, 30, FindHeight(30), plr);
	crate->CreateContents(Shovel);
	crate->CreateContents(Hammer);
	crate->CreateContents(Axe);
	crate = CreateObjectAbove(Crate, 45, FindHeight(30), plr);
	crate->CreateContents(Bread, 3);

	// A flag
	CreateConstruction(Flagpole, 70, FindHeight(70), plr, 100, true);

	is_initialized = true;
}

// Player positioning

protected func InitializePlayer(int plr)
{
	// Move crew to starting location
	var crew = GetCrew(plr, 0);
	var x = 80 + Random(10);
	crew->SetPosition(x, FindHeight(x)-10);

	DoInit(plr);
}
