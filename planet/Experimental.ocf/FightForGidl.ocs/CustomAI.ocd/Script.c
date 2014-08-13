#include S2AI

static g_statue;

func AddAI(object clonk)
{
	var fx = S2AI->AddAI(clonk);
	if (fx)
	{
		clonk.ExecuteS2AI = CustomAI.Execute;
		fx.ai = CustomAI;
	}
	return fx;
}

func SetEnemyData(object clonk, proplist data)
{
	var fx = GetEffect("S2AI", clonk);
	if (fx)
	{
		if (data.Siege) fx.is_siege = true;
		return true;
	}
	return false;
}


func FindTarget(fx)
{
	// Attack doors and statue unless an enemy clonk is closer than these objectives
	var objective;
	     if (g_doorleft  && GetX()<=g_doorleft ->GetX()+5) objective = g_doorleft;
	else if (g_doorright && GetX()>=g_doorright->GetX()-5) objective = g_doorright;
	else objective = g_statue;
	var target = inherited(fx, ...);
	if (objective)
		if (fx.is_siege || !target || ObjectDistance(target) > ObjectDistance(objective))
			target = objective;
	return target;
}

private func FindInventoryWeapon(fx)
{
	// Extra weapons
	if (fx.weapon = FindContents(PowderKeg)) { fx.strategy = CustomAI.ExecuteBomber; return true; }
	if (fx.weapon = FindContents(Club)) { fx.strategy = CustomAI.ExecuteClub; return true; }
	if (inherited(fx, ...)) return true;
	// no weapon :(
	return false;
}

private func ExecuteBomber(fx)
{
	// Still carrying the bomb?
	if (fx.weapon->Contained() != this) { fx.weapon=nil; return false; }
	// Are we in range?
	if (ObjectDistance(fx.target) < 20)
	{
		// Suicide!
		fx.weapon->GoBoom();
		Kill();
	}
	else
	{
		// Not in range. Walk there.
		if (!GetCommand() || !Random(3)) SetCommand("MoveTo", fx.target);
	}
	return true;
}

private func ExecuteClub(fx)
{
	// Still carrying the melee weapon?
	if (fx.weapon->Contained() != this) { fx.weapon=nil; return false; }
	// Are we in range?
	var x=GetX(), y=GetY(), tx=fx.target->GetX(), ty=fx.target->GetY();
	var dx = tx-x, dy = ty-y;
	if (Abs(dx) <= 10 && PathFree(x,y,tx,ty))
	{
		if (Abs(dy) >= 15)
		{
			// Clonk is above or below us - wait
			if (dx<-5) SetComDir(COMD_Left); else if (dx>5) SetComDir(COMD_Right); else SetComDir(COMD_None);
			return true;
		}
		if (!CheckHandsAction(fx)) return true;
		// Stop here
		SetCommand("None"); SetComDir(COMD_None);
		// cooldown?
		if (!fx.weapon->CanStrikeWithWeapon(this))
		{
			//Message("MeleeWAIT %s @ %s!!!", fx.weapon->GetName(), fx.target->GetName());
			// While waiting for the cooldown, we try to evade...
			ExecuteEvade(fx,dx,dy);
			return true;
		}
		// OK, attack! Prefer upwards strike
		dy -= 16;
		fx.weapon->ControlUseStart(this, dx,dy);
		fx.weapon->ControlUseHolding(this, dx,dy);
		fx.weapon->ControlUseStop(this, dx,dy);
		return true;
	}
	// Not in range. Walk there.
	if (!GetCommand() || !Random(3)) SetCommand("MoveTo", fx.target);
	//Message("Melee %s @ %s!!!", fx.weapon->GetName(), fx.target->GetName());
	return true;
}

private func CheckVehicleAmmo(fx, object catapult)
{
	// Ammo is auto-refilled
	return true;
}

func PathFree()
{
	// ignore path che4cks to doors because of solidmask)
	var fx = GetEffect("S2AI", this);
	if (fx && fx.target && fx.target->GetID()==StoneDoor) return true;
	return inherited(...);
}

func Execute(proplist fx, int time)
{
	// Execution: No defensive AI. All enemies move towards target
	var target = fx.target ?? g_statue;
	if (!Random(10) && target)
	{
		var tx = target->GetX();
		if (Abs(tx-GetX())>30)
		{
			SetCommand("MoveTo", nil, BoundBy(GetX(), tx-30, tx+30), target->GetY());
			return true;
		}
	}
	return inherited(fx, time, ...);
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

func LaunchEnemy(proplist enemy, int x, int y)
{
	// Create enemy (usually a Clonk)
	var obj = CreateObject(enemy.Type ?? Clonk, x,y), clonk;
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
	if (enemy.Scale) obj->SetMeshTransformation(Trans_Scale(enemy.Scale[0], enemy.Scale[1], enemy.Scale[2]), 6);
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
	}
	obj.MaxContentsCount = CustomAI.Clonk_MaxContentsCount;
	obj.MaxContentsCountVal = 1;
	// Reward for killing enemy
	obj.Bounty = enemy.Bounty;
	// Vehicles
	var vehicle;
	if (enemy.Vehicle)
	{
		vehicle = CreateObject(enemy.Vehicle, x,y);
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
			if (GetIndexOf(g_respawning_weapons, inv_type)) inv_obj.Departure = CustomAI.Departure_WeaponRespawn;
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
	// Init AI: Run towards statue
	CustomAI->AddAI(obj);
	CustomAI->SetMaxAggroDistance(obj, LandscapeWidth());
	var fx = GetEffect("S2AI", obj);
	if (fx) fx.vehicle = vehicle;
	if (g_statue)
	{
		CustomAI->SetHome(obj, g_statue->GetX(), g_statue->GetY(), Random(2));
		CustomAI->SetGuardRange(obj, 0,0,LandscapeWidth(),LandscapeHeight()); // nowhere to run!
		CustomAI->SetEnemyData(obj, enemy);
		//	CustomAI->SetGuardRange(obj, g_statue->GetX()-200, g_statue->GetY()-50, 400, 100);
	}
	// Remember this clonk to end wave when all enemies have been killed
	g_spawned_enemies[GetLength(g_spawned_enemies)] = obj;
	return obj;
}

// helper: put single elements into array
// 
global func ForceVal2Array(v) {  if (GetType(v) != C4V_Array) return [v]; else return v; }

// forward max contents count to property
func Clonk_MaxContentsCount() { return this.MaxContentsCountVal; }


