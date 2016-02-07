/* Melting Castle */

static g_respawn_flags, g_had_intro_msg;

static const EDIT_MAP = false; // Set to true to edit map and Objects.c; avoids map resize and object duplication

func Initialize()
{
	if (EDIT_MAP) return true;
	// Mirror map objects by moving them to the other side, then re-running object initialization
	for (var o in FindObjects(Find_NoContainer(), Find_Not(Find_Category(C4D_Goal | C4D_Rule))))
	{
		var oid = o->GetID();
		if (oid == Ambience) continue; // Can't filter by C4D_Environment, because we do want waterfalls and item spawns
		o->SetPosition(LandscapeWidth() - o->GetX(), o->GetY());
		// Catapults and cannons should point left on the right island
		
		if (oid == Catapult)
			o->TurnLeft();
		else if (oid == Cannon)
			o->TurnCannon(DIR_Left, true);
	}
	ScenarioObjects->InitializeObjects();
	// Some adjustments to the bridges
	for (var bridge in FindObjects(Find_ID(WoodenBridge)))
	{
		bridge->SetClrModulation(0xff00ffff);
		bridge->SetHalfVehicleSolidMask(true);
		bridge->SetCategory(C4D_Vehicle);
		bridge->SetVertex(,,-37); // prevent self-stuck
	}
	// Item spawns by team
	for (var item_spawn in FindObjects(Find_ID(ItemSpawn)))
	{
		item_spawn->SetTeam(1 + (item_spawn->GetX() > LandscapeWidth()/2));
	}
	// Remember flags for respawn
	g_respawn_flags = CreateArray(3);
	for (var flag in FindObjects(Find_ID(Goal_Flag)))
	{
		flag->DisablePickup(); // Not using CTF goal
		flag.Destruction = Scenario.OnFlagDestruction;
		flag.team = 1;
		flag.Plane = 274; // cannot be moved by airship
		if (flag->GetX() > LandscapeWidth()/2) ++flag.team;
		g_respawn_flags[flag.team] = flag;
	}
	// Weapon drops timer
	ScheduleCall(nil, Scenario.DoBalloonDrop, 36*8, 99999);
	return true;
}

local weapon_list = [ // id, vertex, offx, offy, deployy
	[Firestone, 0, -3, -6, -18],
	[IronBomb, 0, 0, -6, -10],
	[IceWallKit, 2, 0, -5, -10],
	[DynamiteBox, 0, 3, -5, -10],
	[BombArrow, 2, 0, -8, -10],
	[Boompack, 0, 0, 0, 0],
	[GrenadeLauncher, 3, 0, -4, -10]
];

func DoBalloonDrop()
{
	// Random weapon
	var wp = Scenario.weapon_list[Random(GetLength(Scenario.weapon_list))];
	var x = LandscapeWidth()/4 + Random(LandscapeWidth()/2);
	var y = 0;
	var balloon = CreateObject(BalloonDeployed, x, y);
	if (!balloon) return;
	balloon->SetInflated();
	var cargo = CreateObject(wp[0]);
	if (!cargo) return;
	balloon->SetCargo(cargo, wp[1], wp[2], wp[3], wp[4]);
	if (wp[0] == GrenadeLauncher) cargo->CreateContents(IronBomb);
	return balloon;
}

func OnFlagDestruction()
{
	// Callback from flag after it dropped
	Log("$FlagDownMsg$", ["$TeamLeft$", "$TeamRight$"][this.team-1]);
	return true;
}

func InitializePlayer(int plr)
{
	// Everything freely visible (to allow aiming with the cannon)
	SetFoW(false, plr);
	SetPlayerZoomByViewRange(plr,LandscapeWidth(),LandscapeHeight(),PLRZOOM_LimitMax);
	SetPlayerViewLock(plr, false);
	// Acquire base
	var team = GetPlayerTeam(plr);
	var flag = g_respawn_flags[team];
	if (flag && flag->GetOwner() == NO_OWNER) AcquireBase(plr, team);
	// Intro message. Delayed to be visible to all players.
	if (!g_had_intro_msg)
	{
		g_had_intro_msg = true;
		ScheduleCall(nil, Scenario.IntroMsg, 10, 1);
	}
	// Initial launch
	RelaunchPlayer(plr);
	return true;
}

func IntroMsg()
{
	var speaker = FindObject(Find_ID(Goal_Flag));
	if (speaker)
	{
		var c = speaker->GetColor();
		var n = speaker->GetName();
		speaker->SetColor(0xffffff);
		speaker->SetName("$IntroName$");
		Dialogue->MessageBoxAll("$IntroMsg$", speaker, true);
		speaker->SetName(n);
		speaker->SetColor(c);
	}
	return true;
}

func LaunchPlayer(int plr)
{
	// Position at flag
	var flagpole = g_respawn_flags[GetPlayerTeam(plr)];
	if (!flagpole) return EliminatePlayer(plr); // Flag lost and clonk died? Game over!
	var crew = GetCrew(plr), start_x = flagpole->GetX(), start_y = flagpole->GetY();
	crew->SetPosition(start_x, start_y);
	// Make sure clonk can move
	DigFreeRect(start_x-6,start_y-10,13,18,true);
	// Crew setup
	crew.MaxEnergy = 100000;
	crew->DoEnergy(1000);
	crew->CreateContents(WindBag);
	return true;
}

func RelaunchPlayer(int plr)
{
	// Find flag for respawn
	var flagpole = g_respawn_flags[GetPlayerTeam(plr)];
	if (!flagpole) return EliminatePlayer(plr); // Flag lost and clonk died? Game over!
	// Player positioning. 
	var start_x = flagpole->GetX(), start_y = flagpole->GetY();
	// Relaunch: New clonk
	var crew = GetCrew(plr);
	var is_relaunch = (!crew || !crew->GetAlive());
	if (is_relaunch)
	{
		crew = CreateObject(Clonk, 10,10, plr);
		if (!crew) return false; // wat?
		crew->MakeCrewMember(plr);
		SetCursor(plr, crew, false);
	}
	// Relaunch near current flag pos (will be adjusted on actual relaunch)
	crew->SetPosition(start_x, start_y);
	var relaunch = CreateObjectAbove(RelaunchContainer, start_x, start_y, plr);
	if (relaunch)
	{
		relaunch->StartRelaunch(crew);
		relaunch->SetRelaunchTime(8, is_relaunch);
	}
	return true;
}

// GameCall from RelaunchContainer.
func OnClonkLeftRelaunch(object clonk)
{
	if (clonk) return LaunchPlayer(clonk->GetOwner());
}

func RelaunchWeaponList() { return [Bow, Sword, Club, Javelin, Musket, Firestone, IceWallKit]; }


func AcquireBase(int plr, int team)
{
	// Change ownership of some stuff in the base to the first player of a team joining
	// Note that this may be called late into the game in case all players of one team join late into the game
	var base_x0 = 0;
	if (team == 2) base_x0 = LandscapeWidth()/2;
	var base_ids = [Catapult, Cannon, Goal_Flag, Chest];
	for (var obj in FindObjects(Find_InRect(base_x0, 100, LandscapeWidth()/2, LandscapeHeight()-100), Find_NoContainer(), Find_Owner(NO_OWNER)))
	{
		var idobj = obj->GetID();
		if (GetIndexOf(base_ids, idobj))
		{
			obj->SetOwner(plr);
			if (idobj == Goal_Flag) obj->SetClrModulation(0xff000000 | GetPlayerColor(plr));
		}
	}
	return true;
}
