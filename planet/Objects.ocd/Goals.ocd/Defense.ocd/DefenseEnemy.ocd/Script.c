/**
	Defense Enemy
	Defines standard enemies which attack the player or its base. The enemy is a list of properties, the main ones are:
	 * Type (id)           - The type of enemy (defaults to Clonk).
	 * Amount (int)        - The amount of this type of enemy (use to create large groups).
	 * Interval (int)      - Spawn an enemy every X frames.
	 * Bounty (int)        - The amount of clunkers received for killing the enemy.
	 * Score (int)         - The amount of points obtained for beating this enemy.
	 * Position (proplist) - The position where the enemy should be spawned {X = ??, Y = ??}.
	Secondary properties are:
	 * Energy
	 * Skin
	 * Vehicle
	
	@author Maikel
 */


/*-- Enemy Launching --*/

// Definition call which can be used to launch an enemy.
public func LaunchEnemy(proplist prop_enemy, int wave_nr, int enemy_plr)
{
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
	var amount = Max(prop_enemy.Amount, 1);
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
	// Enemy visuals
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
	if (prop_enemy.Scale) enemy->SetMeshTransformation(Trans_Scale(prop_enemy.Scale[0], prop_enemy.Scale[1], prop_enemy.Scale[2]), 6);
	if (prop_enemy.Name) enemy->SetName(prop_enemy.Name);
	enemy->SetColor(prop_enemy.Color);
	// Physical properties
	enemy.MaxEnergy = (prop_enemy.Energy ?? 50) * 1000;
	enemy->DoEnergy(10000);
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
	enemy.MaxContentsCount = 2;
	// Reward for killing enemy
	enemy.Bounty = prop_enemy.Bounty;
	enemy.Score = prop_enemy.Score;
	// Vehicles
	var vehicle;
	if (prop_enemy.Vehicle)
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
		else
		{
			vehicle = CreateObjectAbove(prop_enemy.Vehicle, x, y + 10, enemy_plr);
			enemy->SetAction("Push", vehicle);
		}
	}
	// Enemy inventory
	if (prop_enemy.Inventory)
	{
		for (var inv in ForceVal2Array(prop_enemy.Inventory))
		{
			var inv_obj = enemy->CreateContents(inv);
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
	}

	return enemy;
}

private func ForceVal2Array(/*any*/ v)
{
	if (GetType(v) != C4V_Array)
		return [v];
	return v;
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

// Default enemy, all other enemies inherit from this.
static const DefaultEnemy =
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
	Inventory = Sword,
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
	Skin = 3
};

// A clonk with javelins.
local Spearman = new DefaultEnemy
{
	Name = "$EnemySpearman$",
	Inventory = Javelin,
	Energy = 15,
	Bounty = 5,
	Color = 0xff0000ff,
	Skin = 1
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