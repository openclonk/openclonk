#include AI

public func SetEnemyData(object clonk, proplist data)
{
	var fx = clonk->~GetAI();
	if (fx)
	{
		if (data.Siege)
			fx.is_siege = true;
		return true;
	}
	return false;
}

public func FindTarget(effect fx)
{
	// Attack doors and statue unless an enemy clonk is closer than these objectives
	var objective;
	if (g_doorleft && fx.Target->GetX() <= g_doorleft->GetX() + 5)
		objective = g_doorleft.dummy_target;
	else if (g_doorright && fx.Target->GetX() >= g_doorright->GetX()-5)
		 objective = g_doorright.dummy_target;
	else
		objective = g_statue;
	var target = inherited(fx, ...);
	if (objective)
		if (fx.is_siege || !target || fx.Target->ObjectDistance(target) > fx.Target->ObjectDistance(objective))
			target = objective;
	return target;
}

private func CheckVehicleAmmo(effect fx, object catapult)
{
	// Ammo is auto-refilled
	return true;
}

private func ExecuteBomber(effect fx)
{
	// Still carrying the bomb?
	if (fx.weapon->Contained() != fx.Target)
	{
		fx.weapon=nil;
		return false;
	}
	// Are we in range?
	if (fx.Target->ObjectDistance(fx.target) < 20)
	{
		// Suicide!
		fx.weapon->GoBoom();
		fx.Target->Kill();
	}
	else
	{
		// Not in range. Walk there.
		if (!fx.Target->GetCommand() || !Random(10))
			fx.Target->SetCommand("MoveTo", fx.target);
	}
	return true;
}

func Execute(effect fx, int time)
{
	// Execution: No defensive AI. All enemies move towards target
	var target = fx.target ?? g_statue;
	if (!Random(10) && target)
	{
		var tx = target->GetX();
		if (Abs(tx-fx.Target->GetX())>30)
		{
			fx.Target->SetCommand("MoveTo", nil, BoundBy(fx.Target->GetX(), tx - 30, tx + 30), target->GetY());
			return true;
		}
	}
	return inherited(fx, time, ...);
}

func ExecuteIdle(effect fx)
{
	// Idle execution overridden by Execute
	return true;
}


//======================================================================
/* Enemy creation */

func Departure_WeaponRespawn(object container)
{
	// Weapon used? Schedule to respawn a new one!
	if (container->GetAlive() || container->GetID()==Catapult) container->ScheduleCall(container, CustomAI.DoWeaponRespawn, 5, 1, GetID());
	this.Departure = nil;
}

func DoWeaponRespawn(id_weapon)
{
	if (GetAlive() || GetID()==Catapult)
	{
		var re_weapon = CreateContents(id_weapon);
		if (re_weapon) re_weapon.Departure = CustomAI.Departure_WeaponRespawn;
		return re_weapon;
	}
}

func Inventory_GetCarryTransform()
{
	if (GetID().GetCarryTransform)
		return Trans_Mul(Call(GetID().GetCarryTransform, ...), this.ExtraTransform);
	else
		return this.ExtraTransform;
}

func LaunchEnemy(proplist enemy, int xmin, int xrange, int y)
{
	// Create enemy (usually a Clonk)
	var x = xmin+Random(xrange);
	var obj = CreateObjectAbove(enemy.Type ?? Clonk, x,y, ENEMY), clonk;
	if (!obj) return nil;
	obj->SetController(ENEMY);
	obj->MakeCrewMember(ENEMY);
	// Enemy visuals
	if (enemy.Skin)
	{
		if (GetType(enemy.Skin) == C4V_Array)
		{
			obj->SetSkin(enemy.Skin[0]);
			obj->SetMeshMaterial(enemy.Skin[1]);
		}
		else
			obj->SetSkin(enemy.Skin);
	}
	if (GetType(enemy.Backpack)) obj->~RemoveBackpack();
	if (enemy.Scale) obj->SetMeshTransformation(Trans_Scale(enemy.Scale[0], enemy.Scale[1], enemy.Scale[2]), CLONK_MESH_TRANSFORM_SLOT_Scale);
	if (enemy.Name) obj->SetName(enemy.Name);
	obj->SetColor(enemy.Color);
	// Physical properties
	obj.MaxEnergy = (enemy.Energy ?? 50) * 1000;
	obj->DoEnergy(10000);
	if (enemy.Speed)
	{
		// Speed: Modify Speed in all ActMap entries
		if (obj.ActMap == obj.Prototype.ActMap) obj.ActMap = new obj.ActMap {};
		for (var action in /*obj.ActMap->GetProperties()*/ ["Walk", "Scale", "Dig", "Swim", "Hangle", "Jump", "WallJump", "Dive", "Push"]) // obj.ActMap->GetProperties() doesn't work :(
		{
			if (action == "Prototype") continue;
			if (obj.ActMap[action] == obj.Prototype.ActMap[action]) obj.ActMap[action] = new obj.ActMap[action] {};
			obj.ActMap[action].Speed = obj.ActMap[action].Speed * enemy.Speed / 100;
		}
		obj.JumpSpeed = obj.JumpSpeed * enemy.Speed / 100;
		obj.FlySpeed = obj.FlySpeed * enemy.Speed / 100;
	}
	obj.MaxContentsCount = 1;
	// Reward for killing enemy
	obj.Bounty = enemy.Bounty;
	// Vehicles
	var vehicle;
	if (enemy.Vehicle)
	{
		vehicle = CreateObjectAbove(enemy.Vehicle, x,y);
		obj->SetAction("Push", vehicle);
	}
	// Enemy inventory
	if (enemy.Inventory) for (var inv in ForceVal2Array(enemy.Inventory))
	{
		var inv_type = inv.InvType;
		if (!inv_type) inv_type = inv;
		var inv_obj = obj->CreateContents(inv_type);
		if (inv_obj)
		{
			// Marker for custom weapon behaviour
			inv_obj.IsAIWeapon = true;
			// Infinite ammo
			inv_obj->~SetInfiniteStackCount();
			if (GetIndexOf(g_respawning_weapons, inv_type) >= 0) inv_obj.Departure = CustomAI.Departure_WeaponRespawn;
			// Extra settings?
			if (inv.InvType)
			{
				// Visuals
				if (inv.Scale) { inv_obj.GetCarryTransform = CustomAI.Inventory_GetCarryTransform; inv_obj.ExtraTransform = Trans_Scale(inv.Scale, inv.Scale, inv.Scale); }
				if (inv.Material) inv_obj->SetMeshMaterial(inv.Material);
				// Strength
				if (inv.Strength) inv_obj->SetStrength(inv.Strength);
			}
		}
	}
	// Flying AI
	if (obj->~HasNoNeedForAI())
	{
		// Flying enemies all init themselves to fly at the statue at the moment
	}
	else
	{
		// Init AI: Run towards statue
		CustomAI->AddAI(obj);
		CustomAI->SetMaxAggroDistance(obj, LandscapeWidth());
		var fx = obj.ai;
		if (fx) fx.vehicle = vehicle;
		if (g_statue)
		{
			CustomAI->SetHome(obj, g_statue->GetX(), g_statue->GetY(), Random(2));
			CustomAI->SetGuardRange(obj, 0,0,LandscapeWidth(),LandscapeHeight()); // nowhere to run!
			CustomAI->SetEnemyData(obj, enemy);
			//	CustomAI->SetGuardRange(obj, g_statue->GetX()-200, g_statue->GetY()-50, 400, 100);
		}
	}
	// Remember this clonk to end wave when all enemies have been killed
	g_spawned_enemies[GetLength(g_spawned_enemies)] = obj;
	return obj;
}

// helper: put single elements into array
// 
global func ForceVal2Array(v) {  if (GetType(v) != C4V_Array) return [v]; else return v; }


