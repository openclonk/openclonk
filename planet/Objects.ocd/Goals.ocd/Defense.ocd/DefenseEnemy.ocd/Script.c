/**
	Defense Enemy
	
	Defines standard enemies which attack the player or its base. The enemy is a list of properties, the main ones are:
	 * Type (id)           - The type of enemy (defaults to Clonk).
	 * Amount (int)        - The amount of this type of enemy (use to create large groups).
	 * Interval (int)      - Spawn an enemy every X frames.
	 * Bounty (int)        - The amount of clunkers received for killing the enemy.
	 * Score (int)         - The amount of points obtained for beating this enemy.
	 * Position (proplist) - The position where the enemy should be spawned {X = ??, Y = ??, Exact = true/false}.
	Secondary properties are:
	 * Energy              - Alternative amount of hitpoints of the clonk.
	 * Skin                - Alternative skin for the clonk.
	 * Inventory           - Items for the clonk, either ID or list of IDs.
	 * Vehicle             - Vehicle the enemy is controlling or riding.
	 * VehicleHP           - Hitpoints for the enemy vehicle, can be made stronger.
	 * VehicleInventory    - Items for the vehicle, either ID or list of IDs.
	 * IsCrew              - Is part of a crew of the AI that is controlling the vehicle.
	
	This also defines standard waves which attack the player or its base. The wave is a list of properties, the main ones are:
	 * Name (string)       - The name of the attack wave.
	 * Duration (int)      - Duration of the wave in seconds, if nil the next wave is only launched when this is fully eliminated.
	 * Bounty (int)        - The amount of clunkers received for beating the wave.
	 * Score (int)         - The amount of points obtained for beating this wave.
	 * Enemies (array)     - Array of enemies, see DefenseEnemy for more information.	
	
	@author Maikel
 */


/*-- Enemy Launching --*/

// Definition call which can be used to launch an enemy.
public func LaunchEnemy(proplist prop_enemy, int wave_nr, int enemy_plr)
{
	if (GetType(this) != C4V_Def)
		Log("WARNING: LaunchEnemy(%v, %d, %d) not called from definition context but from %v.", prop_enemy, wave_nr, enemy_plr, this);
		
	// Don't launch the enemy when amount equals zero.
	if (!prop_enemy || prop_enemy.Amount <= 0)
		return;	
	// Determine enemy size.
	var def = prop_enemy.Type ?? prop_enemy.Vehicle;
	if (!def) 
		def = Clonk;
	var width = def->GetDefWidth();
	var height = def->GetDefWidth();

	// If no position spawn at the top of the map and log a warning.
	var pos = prop_enemy.Position;
	if (!pos)
	{
		pos = {X = Random(LandscapeWidth()), Y = 0};
		Log("WARNING: launching enemy %v, but has no position specified and will be created at (%d, %d).", prop_enemy, pos.X, pos.Y);
	}

	// Determine where to spawn the enemy with some variation.	
	var xmin, xmax, ymin, ymax;
	var variation = 100;
	if (pos.Exact)
		variation = 0;
	xmin = BoundBy(pos.X - variation, 0 + width / 2, LandscapeWidth() - width / 2);
	xmax = BoundBy(pos.X + variation, 0 + width / 2, LandscapeWidth() - width / 2);
	ymin = BoundBy(pos.Y - variation, 0 + height / 2, LandscapeHeight() - height / 2);
	ymax = BoundBy(pos.Y + variation, 0 + height / 2, LandscapeHeight() - height / 2);
	var rect = Rectangle(xmin, ymin, xmax - xmin, ymax - ymin);
	
	// Show an arrow for the enemy position.
	CreateArrowForPlayers((xmin + xmax) / 2, (ymin + ymax) / 2);
		
	// Schedule spawning of enemy definition for the given amount.
	var interval = Max(prop_enemy.Interval, 1);
	var amount = prop_enemy.Amount;
	ScheduleCall(nil, DefenseEnemy.LaunchEnemyAt, interval, amount, prop_enemy, wave_nr, enemy_plr, rect);	
	return;	
}

private func LaunchEnemyAt(proplist prop_enemy, int wave_nr, int enemy_plr, proplist rect)
{
	// Create enemy (per default a Clonk) at the given location.
	var x = rect.x + Random(rect.wdt);
	var y = rect.y + Random(rect.hgt);
	var enemy = CreateObjectAbove(prop_enemy.Type ?? Clonk, x, y, enemy_plr);
	if (!enemy)
		return nil;
	enemy->MakeCrewMember(enemy_plr);
	enemy->SetController(enemy_plr);
	GameCallEx("OnEnemyCreation", enemy, wave_nr);
	// Enemy visuals.
	UpdateEnemyVisuals(enemy, prop_enemy);
	// Update physical properties.
	UpdateEnemyPhysicals(enemy, prop_enemy);
	// Reward for killing enemy: clunker bounty and points.
	enemy.Bounty = prop_enemy.Bounty;
	enemy.Score = prop_enemy.Score;
	// Vehicles for enemies that are not a crew of a vehicle.
	var vehicle;
	if (prop_enemy.Vehicle && !prop_enemy.IsCrew)
	{
		if (prop_enemy.Vehicle == Balloon)
		{
			var balloon = enemy->CreateContents(Balloon);
			balloon->ControlUseStart(enemy);
		}
		else if (prop_enemy.Vehicle == DefenseBoomAttack)
		{
			vehicle = CreateObjectAbove(prop_enemy.Vehicle, x, y + 10, enemy_plr);
			enemy->SetAction("Ride", vehicle);
			// Add boom attack to enemy list.
			GameCallEx("OnEnemyCreation", vehicle, wave_nr);
		}
		else if (prop_enemy.Vehicle == Airplane)
		{
			vehicle = CreateObjectAbove(prop_enemy.Vehicle, x, y + 10, enemy_plr);
			enemy->Enter(vehicle);
			vehicle->PlaneMount(enemy);
			// Assume the plane is at one landscape side and wants to fly to the opposite side.
			if (vehicle->GetX() > LandscapeWidth() / 2)
				vehicle->FaceLeft();
			vehicle->StartInstantFlight(vehicle->GetR(), 15);
		}
		else
		{
			vehicle = CreateObjectAbove(prop_enemy.Vehicle, x, y + 10, enemy_plr);
			enemy->SetAction("Push", vehicle);
			vehicle.pilot = enemy;
		}
		// Give the vehicle more hitpoints.
		if (prop_enemy.VehicleHP)
			vehicle.HitPoints = prop_enemy.VehicleHP;
		// Add the enemy vehicle to no friendly fire rule (must be done by hand, noFF rule is for crew only be default).
		if (vehicle)
			GameCallEx("OnCreationRuleNoFF", vehicle);
	}
	// Move crew members onto their vehicles.
	if (prop_enemy.Vehicle && prop_enemy.IsCrew)
	{
		var crew_vehicle = enemy->FindObject(Find_Distance(50), Find_ID(prop_enemy.Vehicle), Sort_Distance());
		if (crew_vehicle)
		{
			// Set commander for crew.
			enemy.commander = crew_vehicle.pilot;		
		}	
	}
	// Vehicle inventory.
	if (vehicle && prop_enemy.VehicleInventory)
	{
		for (var inv in ForceToInventoryArray(prop_enemy.VehicleInventory))
		{
			var inv_obj = vehicle->CreateContents(inv);
			// Infinite ammo.
			if (inv_obj)
				inv_obj->~SetInfiniteStackCount();
		}
	}
	// Enemy inventory.
	if (prop_enemy.Inventory)
	{
		for (var inv in ForceToInventoryArray(prop_enemy.Inventory))
		{
			// Special way to pick up carry heavy objects instantly.
			var inv_obj;
			if (inv->~IsCarryHeavy() && (enemy->GetOCF() & OCF_CrewMember))
				inv_obj = enemy->CreateCarryHeavyContents(inv);
			else
				inv_obj = enemy->CreateContents(inv);
			// Infinite ammo.
			if (inv_obj)
				inv_obj->~SetInfiniteStackCount();
		}
	}
	// Add AI.
	if (!enemy->~HasNoNeedForAI())
	{
		DefenseAI->AddAI(enemy);
		DefenseAI->SetMaxAggroDistance(enemy, LandscapeWidth());
		DefenseAI->SetGuardRange(enemy, 0, 0, LandscapeWidth(), LandscapeHeight());
		// Add vehicle to AI.
		if (vehicle)
			DefenseAI->SetVehicle(enemy, vehicle);
	}
	return enemy;
}

private func UpdateEnemyVisuals(object enemy, proplist prop_enemy)
{
	if (prop_enemy.Skin)
	{
		if (GetType(prop_enemy.Skin) == C4V_Array)
		{
			enemy->SetSkin(prop_enemy.Skin[0]);
			enemy->SetMeshMaterial(prop_enemy.Skin[1]);
		}
		else
			enemy->SetSkin(prop_enemy.Skin);
	}
	if (GetType(prop_enemy.Backpack)) enemy->~RemoveBackpack();
	if (prop_enemy.Scale) enemy->SetMeshTransformation(Trans_Scale(prop_enemy.Scale[0], prop_enemy.Scale[1], prop_enemy.Scale[2]), CLONK_MESH_TRANSFORM_SLOT_Scale);
	if (prop_enemy.Name) enemy->SetName(prop_enemy.Name);
	enemy->SetColor(prop_enemy.Color);
	return;
}

private func UpdateEnemyPhysicals(object enemy, proplist prop_enemy)
{
	enemy.MaxEnergy = (prop_enemy.Energy ?? 50) * 1000;
	enemy->DoEnergy(enemy.MaxEnergy / 1000);
	if (prop_enemy.Speed)
	{
		// Speed: Modify Speed in all ActMap entries
		if (enemy.ActMap == enemy.Prototype.ActMap) enemy.ActMap = new enemy.ActMap {};
		for (var action in /*obj.ActMap->GetProperties()*/ ["Walk", "Scale", "Dig", "Swim", "Hangle", "Jump", "WallJump", "Dive", "Push"]) // obj.ActMap->GetProperties() doesn't work :(
		{
			if (action == "Prototype") continue;
			if (enemy.ActMap[action] == enemy.Prototype.ActMap[action]) enemy.ActMap[action] = new enemy.ActMap[action] {};
			enemy.ActMap[action].Speed = enemy.ActMap[action].Speed * enemy.Speed / 100;
		}
		enemy.JumpSpeed = enemy.JumpSpeed * prop_enemy.Speed / 100;
		enemy.FlySpeed = enemy.FlySpeed * prop_enemy.Speed / 100;
	}
	return;
}

private func ForceToInventoryArray(/*any*/ list)
{
	// Convert single ID to array.
	if (GetType(list) != C4V_Array)
		return [list];
	// Check in array if entries of the form [ID, amount] appear and convert them.
	for (var i = 0; i < GetLength(list); )
	{
		var element = list[i];
		if (GetType(element) == C4V_Array)
		{
			for (var j = 0; j < element[1]; j++)
				PushBack(list, element[0]);
			RemoveArrayValue(list, element);
			continue;
		}
		i++;
	}
	return list;
}

// Create an arrow which show the direction the enemy is coming from.
private func CreateArrowForPlayers(int x, int y)
{
	for (var plr in GetPlayers(C4PT_User))
	{
		var cursor = GetCursor(plr);
		if (!cursor) 
			continue;
		var arrow = CreateObject(GUI_GoalArrow, cursor->GetX(), cursor->GetY(), plr);
		if (!arrow) 
			continue;
		arrow->SetAction("Show", cursor);
		arrow->SetR(Angle(cursor->GetX(), cursor->GetY(), x, y));
		arrow->SetClrModulation(RGBa(255, 50, 0, 128));
		Schedule(arrow, "RemoveObject()", 8 * 36);
	}
	return;
}


/*-- Enemies --*/

static const CSKIN_Default   = 0,
             CSKIN_Steampunk = 1,
             CSKIN_Alchemist = 2,
             CSKIN_Farmer    = 3;

// Default enemy, all other enemies inherit from this.
local DefaultEnemy =
{
	Type = nil,
	Inventory = nil,
	Energy = nil,
	Amount = 1,
	Interval = 1,
	Bounty = 0,	
	Score = 0,
	Position = nil
};

// A clonk with a sword.
local Swordsman = new DefaultEnemy
{
	Name = "$EnemySwordsman$",
	Inventory = [Sword, Helmet],
	Energy = 30,
	Bounty = 20,
	Color=0xff0000ff
};

// A clonk with bow and arrow.
local Archer = new DefaultEnemy
{
	Name = "$EnemyArcher$",
	Inventory = [Bow, Arrow],
	Energy = 10,
	Bounty = 5,
	Color = 0xff00ff00,
	Skin = CSKIN_Farmer
};

// A clonk with javelins.
local Spearman = new DefaultEnemy
{
	Name = "$EnemySpearman$",
	Inventory = Javelin,
	Energy = 15,
	Bounty = 5,
	Color = 0xff0000ff,
	Skin = CSKIN_Steampunk
};

// A clonk with a grenade launcher.
local Grenadier = new DefaultEnemy
{
	Name = "$EnemyGrenadier$",
	Inventory = [GrenadeLauncher, [IronBomb, 8]],
	Energy = 25,
	Bounty = 5,
	Color = 0xffa0a0ff,
	Skin = CSKIN_Alchemist
};

// A rocket which moves to a target.
local BoomAttack = new DefaultEnemy
{
	Name = "$EnemyBoomAttack$",
	Type = DefenseBoomAttack,
	Bounty = 2
};

// A faster rocket which moves to a target.
local RapidBoomAttack = new DefaultEnemy
{
	Name = "$EnemyRapidBoomAttack$",
	Type = DefenseBoomAttack,
	Bounty = 2,
	Speed = 300
};

// A parachutist coming from above with a balloon.
local Ballooner = new DefaultEnemy
{
	Name = "$EnemyBallooner$",
	Inventory = Sword,
	Energy = 30,
	Bounty = 15,
	Color = 0xff008000,
	Skin = CSKIN_Default,
	Vehicle = Balloon
};

// An archer riding a boom attack.
local Rocketeer = new DefaultEnemy
{
	Name = "$EnemyRocketeer$",
	Inventory = [Bow, Arrow],
	Energy = 15,
	Bounty = 15,
	Color = 0xffffffff,
	Skin = CSKIN_Steampunk,
	Vehicle = DefenseBoomAttack
};

// An archer riding a boom attack.
local Bomber = new DefaultEnemy
{
	Name = "$EnemyBomber$",
	Inventory = PowderKeg,
	Energy = 50,
	Bounty = 10,
	Color = 0xff55aaff,
	Skin = CSKIN_Default
};

// Commander of the airship.
local AirshipPilot = new DefaultEnemy
{
	Name = "$EnemyAirshipPilot$",
	Inventory = [[Rock, 5]],
	Energy = 35,
	Bounty = 15,
	Color = 0xffff00ff,
	Skin = CSKIN_Alchemist,
	Vehicle = Airship,
	VehilceHP = 100
};

// Crew of the airship.
local AirshipCrew = new DefaultEnemy
{
	Name = "$EnemyAirshipCrew$",
	Inventory = [Sword, Shield],
	Energy = 35,
	Bounty = 5,
	Color = 0xff0000aa,
	Skin = CSKIN_Farmer,
	Vehicle = Airship,
	IsCrew = true
};

// A pilot and a bomber plane.
local BomberPlane = new DefaultEnemy
{
	Name = "$EnemyBomberPlane$",
	Inventory = [],
	Energy = 35,
	Bounty = 20,
	Color = 0xffaaddff,
	Skin = CSKIN_Alchemist,
	Vehicle = Airplane,
	VehicleHP = 50,
	VehicleInventory = [[IronBomb, 8], [Dynamite, 4]]
};


/*-- Wave Launching --*/

// Definition call which can be used to launch an attack wave.
public func LaunchWave(proplist prop_wave, int wave_nr, int enemy_plr)
{
	if (GetType(this) != C4V_Def)
		Log("WARNING: LaunchWave(%v, %d, %d) not called from definition context but from %v.", prop_wave, wave_nr, enemy_plr, this);

	// Create count down until next wave and play wave start sound.
	GUI_Clock->CreateCountdown(prop_wave.Duration);
	CustomMessage(Format("$MsgWave$", wave_nr, prop_wave.Name));
	Sound("UI::Ding");

	// Launch enemies.
	if (prop_wave.Enemies)
		for (var enemy in prop_wave.Enemies)
			this->LaunchEnemy(enemy, wave_nr, enemy_plr);
	return;
}


/*-- Waves --*/

// Default wave, all other waves inherit from this.
local DefaultWave =
{
	Name = nil,
	Duration = nil,
	Bounty = nil,
	Score = nil,
	Enemies = nil
};

// A wave with no enemies: either at the start or to allow for a short break.
local BreakWave = new DefaultWave
{
	Name = "$WaveBreak$",
	Duration = 60
};
