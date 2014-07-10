/**
	Deep Sea Mining
	Mine gems buried deeply below the ocean.
	
	@authors Sven2
*/

// set in Map.c
static main_island_x, main_island_y;
static goal_platform_x, goal_platform_y;

static const SCEN_TEST = false;

protected func Initialize()
{
	// Construction site on goal platform
	var goal_site = CreateObject(ConstructionSite, goal_platform_x+10, goal_platform_y+3);
	goal_site->Set(CrystalCommunicator);
	goal_site->MakeUncancellable();
	if (SCEN_TEST)
	{
		for (var i=0; i<6; ++i)
		{
			goal_site->CreateObject(Metal,-20);
			goal_site->CreateObject(Ruby,0);
			goal_site->CreateObject(Amethyst,20);
		}
		goal_site->CreateContents(Metal,6);
		goal_site->CreateContents(Ruby,6);
		goal_site->CreateContents(Amethyst,5);
	}
	
	// Initialize different parts of the scenario.
	InitEnvironment();
	InitVegetation();
	InitAnimals();
	InitMainIsland();
		
	return;
}

protected func InitializePlayer(int plr)
{
	// Harsh zoom range
	SetPlayerZoomByViewRange(plr, 500, 350, PLRZOOM_LimitMax);
	SetPlayerZoomByViewRange(plr, 500, 350, PLRZOOM_Direct);
	SetPlayerViewLock(plr, true);
	
	// Position and materials
	var i, crew;
	for (i = 0; crew = GetCrew(plr, i); ++i)
	{
		var pos = FindMainIslandPosition();
		crew->SetPosition(pos[0], pos[1] - 11);
		crew->CreateContents(Shovel);
		if (SCEN_TEST)
		{
			var cs = FindObject(Find_ID(ConstructionSite));
			crew->SetPosition(cs->GetX(), cs->GetY()-20);
		}
	}
	
	// Claim ownership of unowned structures
	for (var structure in FindObjects(Find_Or(Find_Category(C4D_Structure), Find_Func("IsFlagpole")), Find_Owner(NO_OWNER)))
	{
		structure->SetOwner(plr);
		structure->~RefreshOwnershipOfSurrounding();
	}
		
	// Should be done in OnOwnerChanged? It doesn't happen ATM.
	RedrawAllFlagRadiuses();
	
	// Goal message for intro
	Dialogue->MessageBox("$MsgIntro$", GetCursor(plr), GetCrew(plr), plr, true); // oh no we're stranded!

	return;
}

// Initializes environment and disasters.
private func InitEnvironment()
{
	// Water refill from sides
	var initial_water_level = 0;
	while (GetMaterial(0,initial_water_level) != Material("Water")) ++initial_water_level;
	ScheduleCall(nil, this.EnsureWaterLevel, 20, 999999999, initial_water_level);

	// Set a certain parallax.
	SetSkyParallax(0, 20, 20);
	
	// No disasters for now
	//Meteor->SetChance(5); Cloud->SetLightning(16);
	
	return;
}

// Ensures that the sea doesn't disappear
func EnsureWaterLevel(int level, bool no_recursion)
{
	var water_mat = Material("Water");
	if (GetMaterial(0,level) != water_mat) CastPXS("Water", 100, 20, 0,level, 90, 10);
	if (GetMaterial(LandscapeWidth()-1,level) != water_mat) CastPXS("Water", 100, 20, LandscapeWidth()-1,level, 270, 10);
	// Extra insertion at a lower level so it's not easy to block off
	if (!no_recursion && !Random(3)) EnsureWaterLevel(level + 50 + Random(450), true);
	return true;
}

private func InitVegetation()
{
	// Grass on starting island.
	PlaceGrass(85);
	
	// Place some cocont trees all around the main island
	for (var i = 0; i < 10 + Random(8); i++)
		PlaceVegetation(Tree_Coconut, 0, 0, LandscapeWidth(), LandscapeHeight(), 1000 * (61 + Random(40)));
		
	// Create an effect to make sure there will always grow some new trees.
	ScheduleCall(nil, this.EnsureTrees, 100, 999999999);
	
	// Some objects in the earth.	
	PlaceObjects(Rock, 10 + Random(10),"Earth");
	PlaceObjects(Firestone, 35 + Random(5), "Earth");
	PlaceObjects(Loam, 25 + Random(5), "Earth");

	// Underwater vegetation
	Seaweed->Place(20);
	Coral->Place(30);
	
	return;
}

// Ensures that there will always grow some trees on the main island.
func EnsureTrees()
{
	var wdt = LandscapeWidth();
	var hgt = LandscapeHeight();
	// Place a tree if there are less than eight trees, with increasing likelihood for lower amounts of trees.
	var nr_trees = ObjectCount(Find_Func("IsTree"), Find_Func("IsStanding"));
	if (Random(9) >= nr_trees)
		if (!Random(20))
			PlaceVegetation(Tree_Coconut, main_island_x - 300, main_island_y - 200, 600, 400, 3);
	return true;
}

private func InitAnimals()
{
	// Place fish in upper ocean area (there tend to be small basins below, where lots of fish could get stuck)
	var fish_area = GetFishArea();
	Fish->Place(50, fish_area);
	Piranha->Place(25, fish_area);
	ScheduleCall(nil, this.EnsureAnimals, 237, 999999999);
	return true;
}

private func GetFishArea() { return Rectangle(50, main_island_y, LandscapeWidth() - 100, LandscapeHeight()/2 - main_island_y); }

private func EnsureAnimals()
{
	if (ObjectCount(Find_ID(Fish), Find_Not(Find_Action("Dead"))) < 50) DoFishSpawn(Fish);
	if (ObjectCount(Find_ID(Piranha), Find_Not(Find_Action("Dead"))) < 25) DoFishSpawn(Piranha);
	return true;
}

private func DoFishSpawn(fish_type)
{
	// Try placement away from Clonks. If a Clonk was nearby, just remove it immediately.
	var fish = fish_type->Place(1, GetFishArea());
	if (fish)
		if (fish->FindObject(fish->Find_Distance(300), Find_ID(Clonk), Find_OCF(OCF_Alive)))
			fish->RemoveObject();
	return fish;
}

// Initializes the main island according to the material specification.
private func InitMainIsland()
{
	var amount = 3;
	var pos;
	
	// Always start with a lorry filled with: hammer(x2), axe(x2), wood(x6) and metal(x4).
	var lorry_pos = FindMainIslandPosition(0, 80);
	var lorry = CreateObject(Lorry, lorry_pos[0], lorry_pos[1] - 8);
	lorry->CreateContents(Hammer, 2);
	lorry->CreateContents(Axe, 2);
	lorry->CreateContents(Wood, 6);
	lorry->CreateContents(Metal, 4);
	
	// If more material is specified, create a small settlement: flag(x2) and windmill.
	// Also fill lorry a bit more with: pickaxe(x1), dynamite(x4), wood(x4), metal(x2).
	if (amount >= 2)
	{
		pos = FindMainIslandPosition(-120, 20);
		CreateObject(Flagpole, pos[0], pos[1]);
		pos = FindMainIslandPosition(120, 20);
		CreateObject(Flagpole, pos[0], pos[1]);
		pos = FindMainIslandPosition(nil, nil, true);
		CreateObject(WindGenerator, pos[0], pos[1]);
		lorry->CreateContents(Wood, 4);
		lorry->CreateContents(Metal, 2);
		lorry->CreateContents(Pickaxe, 1);
		lorry->CreateContents(Dynamite, 4);
	}
	
	// If still more material is specified, create a larger settlement: sawmill, chemical lab, tools workshop.
	// Also fill lorry a bit more with: Barrel (x1), Bucket(x1), Loam(x4), DynamiteBox(x2).
	if (amount >= 3)
	{
		pos = FindMainIslandPosition(nil, nil, true);
		CreateObject(Sawmill, pos[0], pos[1]);
		pos = FindMainIslandPosition(nil, nil, true);
		CreateObject(ChemicalLab, pos[0], pos[1]);
		pos = FindMainIslandPosition(nil, nil, true);
		CreateObject(ToolsWorkshop, pos[0], pos[1]);
	
		lorry->CreateContents(Barrel, 1);
		lorry->CreateContents(Bucket, 1);
		lorry->CreateContents(Loam, 4);
		lorry->CreateContents(DynamiteBox, 1);
		lorry->CreateContents(WallKit, 4);
		//lorry->CreateContents(Boompack, 1);	
	}
	
	return;
}

// Tries to find a position on the main island.
private func FindMainIslandPosition(int xpos, int sep, bool no_struct)
{
	if (xpos == nil)
		xpos = 0;
	if (sep == nil) 
		sep = 250;

	for (var i = 0; i < 100; i++)
	{
		var x = main_island_x + xpos + Random(sep*2+1)-sep;
		var y = main_island_y / 2 - 220;
		
		while (!GBackSolid(x, y) && y < LandscapeHeight()*3/4)
			y++;
		
		if (GetMaterial(x,y) == Material("Brick")) continue; // not on goal platform
			
		if (!no_struct || !FindObject(Find_Or(Find_Category(C4D_Structure), Find_Func("IsFlagpole")), Find_Distance(60, x, y)))
			break;
	}

	return [x, y];
}


/* Outro */

// Goal fulfilled
public func OnGoalsFulfilled()
{
	var outro = {};
	outro.communicator = FindObject(Find_Func("IsCrystalCommunicator"));
	if (!outro.communicator) return false; // what?
	// Stop Clonks and disable player controls
	Dialogue->StartCinematics(outro.communicator);
	// Outro
	ScheduleCall(nil, this.Fade2Darkness, 15, 32, {});
	Dialogue->MessageBoxAll("$MsgOutro1$", GetOutroTalker(outro), true); // ok turn it on
	ScheduleCall(nil, Scenario.Outro0, 100, 1, outro);
	// Return true to force goal rule to not call GameOver() yet
	return true;
}

private func Outro0(proplist outro)
{
	outro.communicator->StartCommunication(); // 250 frames
	ScheduleCall(nil, Scenario.Outro1, 650, 1, outro);
}

private func Outro1(proplist outro)
{
	Dialogue->MessageBoxAll("$MsgOutro2$", GetOutroTalker(outro), true); // let's see if it works
	ScheduleCall(nil, Scenario.Outro2, 50, 1, outro);
}

private func Outro2(proplist outro)
{
	outro.communicator->SendCode("...---..."); // 159 frames
	return ScheduleCall(nil, Scenario.Outro3, 200, 1, outro);
}

private func Outro3(proplist outro)
{
	outro.communicator->StopCommunication();
	Dialogue->MessageBoxAll("$MsgOutro3$", GetOutroTalker(outro), true); // i wonder if anyone has heard us
	outro.plane = CreateObject(Plane, 100, main_island_y-100);
	outro.plane->SetContactDensity(85); // only collision with brick for proper landing
	outro.pilot = CreateObject(Clonk, 100, 100);
	outro.pilot->MakeInvincible();
	outro.pilot->SetSkin(2);
	outro.pilot->Enter(outro.plane);
	outro.pilot->SetAction("Walk");
	outro.pilot->SetName("$NamePilot$");
	outro.pilot->SetColor(RGB(55, 65, 75));
	outro.pilot->SetDir(DIR_Right);
	outro.plane->FaceRight();
	outro.plane->StartInstantFlight(90, 15);
	return ScheduleCall(nil, Scenario.Outro4, 5, 99999999, outro);
}

private func Outro4(proplist outro)
{
	// Wait for plane to arrive
	if (outro.plane->GetX() < outro.communicator->GetX() - 200) return true;
	ClearScheduleCall(nil, Scenario.Outro4);
	// Plane in range! Ensure players see it.
	SetPlayerZoomByViewRange(NO_OWNER, 500, 350, PLRZOOM_Direct);
	Dialogue->MessageBoxAll("$MsgOutro4$", outro.pilot, true); // hey, our friends!
	return ScheduleCall(nil, Scenario.Outro5, 100, 1, outro);
}

private func Outro5(proplist outro)
{
	Dialogue->MessageBoxAll("$MsgOutro5$", GetOutroTalker(outro), true); // we're saved!
	outro.plane->StartInstantFlight(245, 15);
	outro.plane->SetContactDensity(C4M_Solid);
	return ScheduleCall(nil, Scenario.Outro6, 60, 1, outro);
}

private func Outro6(proplist outro)
{
	outro.plane->StartInstantFlight(280, 5);
	return ScheduleCall(nil, Scenario.Outro7, 15, 1, outro);
}

private func Outro7(proplist outro)
{
	outro.plane->CancelFlight();
	return ScheduleCall(nil, Scenario.Outro8, 40, 1, outro);
}

private func Outro8(proplist outro)
{
	outro.pilot->Exit();
	Dialogue->MessageBoxAll("$MsgOutro6$", outro.pilot, true); // hop on everyone!
	return ScheduleCall(nil, Scenario.Outro9, 100, 1, outro);
}

private func Outro9(proplist outro)
{
	// Reenable crew in case players want to continue playing after round
	outro.plane->FaceRight();
	Dialogue->StopCinematics();
	return ScheduleCall(nil, Scenario.Outro10, 100, 1, outro);
}

private func Outro10(proplist outro)
{
	Sound("Fanfare");
	return GameOver();
}

private func Fade2Darkness(proplist v)
{
	v.t += 8;
	var fade_val = Max(0xff-v.t);
	SetSkyAdjust(RGB(fade_val,fade_val,fade_val));
}	

private func GetOutroTalker(outro)
{
	return outro.communicator->FindObject(Find_ID(Clonk), Find_OCF(OCF_Alive), outro.communicator->Sort_Distance());
}
