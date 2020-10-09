/* Melting Castle */

static g_respawn_flags, g_had_intro_msg;

static const EDIT_MAP = false; // Set to true to edit map and Objects.c; avoids map resize and object duplication

func Initialize()
{
	if (EDIT_MAP) return true;
	GetRelaunchRule()->SetDefaultRelaunchCount(nil);
	GetRelaunchRule()->SetRespawnDelay(8);
	GetRelaunchRule()->SetLastWeaponUse(false);
	GetRelaunchRule()->SetAllowPlayerRestart(true);
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
		flag.RejectWindbagForce = Clonk.IsClonk;
		if (flag->GetX() > LandscapeWidth()/2) ++flag.team;
		g_respawn_flags[flag.team] = flag;
	}
	// Weapon drops timer
	g_num_avail_weapons = 6;
	ScheduleCall(nil, Scenario.DoBalloonDrop, 36*8, 99999);
	return true;
}

local weapon_list = [ // id, vertex, offx, offy, deployy
	[Firestone, 0, -3, -6, -18],
	[IronBomb, 0, 0, -6, -10],
	[IceWallKit, 2, 0, -5, -10],
	[DynamiteBox, 0, 3, -5, -10],
	[BombArrow, 2, 0, -8, -10],
	[GrenadeLauncher, 3, 0, -4, -10],
	[Boompack, 0, 0, 0, 0]
];

static g_num_avail_weapons; // boompack available later

func DoBalloonDrop()
{
	// Random weapon
	if (FrameCounter() > 36*60*4) g_num_avail_weapons = 7; // enable boompacks after some time
	var wp = Scenario.weapon_list[Random(g_num_avail_weapons)];
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
	SetPlayerZoomByViewRange(plr, LandscapeWidth(),LandscapeHeight(),PLRZOOM_LimitMax);
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

func LaunchPlayer(object clonk, int plr)
{
	// Make sure clonk can move
	DigFreeRect(clonk->GetX()-6, clonk->GetY()-10, 13, 18, true);
	// Crew setup
	clonk.MaxEnergy = 100000;
	clonk->DoEnergy(1000);
	clonk->CreateContents(WindBag);
	return true;
}

public func OnPlayerRelaunch(int plr)
{
	if (!g_respawn_flags[GetPlayerTeam(plr)])
		return EliminatePlayer(plr);
}

public func RelaunchPosition(int iPlr, int iTeam)
{
	if (!g_respawn_flags[iTeam]) return;
	return [g_respawn_flags[iTeam]->GetX(), g_respawn_flags[iTeam]->GetY()];
}

public func OnClonkLeftRelaunch(object clonk, int plr)
{
	// Find flag for respawn
	var flagpole = g_respawn_flags[GetPlayerTeam(plr)];
	if (!flagpole) return EliminatePlayer(plr); // Flag lost and clonk died? Game over!
	
	// Reset available items in spawns
	for (var item_spawn in FindObjects(Find_ID(ItemSpawn))) item_spawn->Reset(plr);
	// Relaunch near current flag pos (will be adjusted on actual relaunch)
	return LaunchPlayer(clonk, plr);
}

func RelaunchWeaponList() { return [Bow, Sword, Club, Javelin, Blunderbuss, Firestone, IceWallKit]; }


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
