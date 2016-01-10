#include AI

// How far must someone be away from an airship to be independent from it
local AIRSHIP_LOST_DIST = 50;
// How near must an airship be to the target to dispatch its troops
local AIRSHIP_BOARD_DIST = 60;
// How near must neighbouring airships be to trigger evasive maneuvers
local AIRSHIP_VICINITY_DIST = 60;
// How near must an archer be to a target to shoot. Theoretically, archers can shoot from much farer away but that's too brutal for the players
local ARCHER_SHOOT_DIST = 500;

func AddAI(object clonk)
{
	var fx = AI->AddAI(clonk);
	if (fx)
	{
		clonk.ExecuteAI = CustomAI.Execute;
		fx.ai = CustomAI;
		fx.ignore_allies = true;
	}
	return fx;
}

func SetEnemyData(object clonk, proplist data)
{
	var fx = GetEffect("AI", clonk);
	if (fx)
	{
		if (data.Siege) fx.is_siege = true;
		return true;
	}
	return false;
}

func FindTarget(fx, bool parent)
{
	if (parent) return _inherited(fx);
	return this->GetNearestWindmill();
}

private func FindInventoryWeapon(fx)
{
	// Extra weapons
	if (fx.weapon = FindContents(Axe))
		{ fx.strategy = fx.ai.ExecuteMelee; return true; }
	if (fx.vehicle && fx.vehicle->GetID() == Boomattack)
		{ fx.weapon = FindContents(Bow); fx.strategy = fx.ai.ExecuteRanged; fx.projectile_speed = 100; fx.aim_wait = 0; fx.ammo_check = fx.ai.HasArrows; fx.ranged=true; return true; }
	if (inherited(fx, ...)) return true;
	// no weapon :(
	return false;
}

func Execute(proplist fx, int time)
{
	if (!fx.target) fx.target = GetNearestWindmill();
	if (GetAction() == "Ride")
	{
		var action_target = GetActionTarget();
		if (action_target && action_target->GetID() == Balloon)
			return ExecuteIdle(); // do nothing if hanging from a balloon
		// else do everything! (action_target == Boomattack)
	}
	if (fx.parachute_lost)
	{
		if (GetAction() != "Walk")
			return inherited(fx, time, ...);
		fx.target = nil;
		fx.parachute_lost = nil;
		SetCommand("None");
	}

	return inherited(fx, time, ...);
}

private func ExecuteVehicle(fx)
{
	if (!fx.vehicle) return false;
	if (fx.vehicle->GetID() == Catapult) return _inherited(fx, ...);
	if (fx.vehicle->GetID() == Airship) return ExecutePilot(fx);
	return false;
}

private func ExecutePilot(fx) // this function name doesn't sound right
{
	var target = fx.target ?? GetNearestWindmill();
	if (!target) return false; // ???

	if (GetProcedure() != "PUSH" || GetActionTarget() != fx.vehicle)
	{
		if (ObjectDistance(fx.vehicle) > CustomAI.AIRSHIP_LOST_DIST) // We lost the airship (maybe we were flung from it)
		{
			fx.strategy = nil;
			fx.weapon = nil;
			fx.vehicle->PromoteNewCaptain();
			fx.vehicle = nil;
			return true;
		}
		if (!GetCommand() || !Random(4)) SetCommand("Grab", fx.vehicle);
		return true;
	}

	// Close to the target
	if (ObjectDistance(target) < CustomAI.AIRSHIP_BOARD_DIST)
	{
		fx.vehicle->PrepareToBoard(this);
		return true;
	}

	// No command check
	if (!fx.vehicle->GetCommand())
	{
		var point = this->GetBoardingPoint(target);
		fx.vehicle->SetCommand("MoveTo", nil, point[0], point[1]);
		fx.target = FindTarget(fx);
	// Unmovable check
	} else if (fx.vehicle->GetXDir() == 0 && fx.vehicle->GetYDir() == 0)
	{
		fx.stuck_time++;
		if (fx.stuck_time > 20)
			fx.vehicle->SetYDir(10);
		if (fx.stuck_time > 40)
			fx.vehicle->SetXDir(RandomX(-10,10));
		if (fx.stuck_time > 60)
		{
			if(InsideIslandRectangle(fx.vehicle))
				return fx.vehicle->PrepareToBoard(this);
			else
				fx.vehicle->Sink();
		}
	} else if(fx.stuck_time)
	{
		fx.stuck_time = 0;
	}

	// Vicinity check
	var ships = FindObjects(Find_Distance(CustomAI.AIRSHIP_VICINITY_DIST), Find_ID(Airship), Find_Exclude(fx.vehicle));
	if (GetLength(ships) && !InsideIslandRectangle(fx.vehicle))
	{
		var centerX = 0, centerY = 0;
		for (var ship in ships)
		{
			centerX += ship->GetX();
			centerY += ship->GetY();
		}
		centerX += fx.vehicle->GetX();
		centerY += fx.vehicle->GetY();
		centerX /= GetLength(ships) + 1;
		centerY /= GetLength(ships) + 1;
		// Move away from mass center
		var targetX = fx.vehicle->GetX() - centerX;
		var targetY = fx.vehicle->GetY() - centerY;
		fx.vehicle->SetCommand("MoveTo", nil, BoundBy(GetX() + targetX * 2, 30, LandscapeWidth()-30), BoundBy(GetY() + targetY * 2, 30, LandscapeHeight()-30));
	}

	return true;
}

private func CheckVehicleAmmo(fx, object catapult)
{
	// Ammo is auto-refilled
	return true;
}

func ExecuteIdle(proplist fx)
{
	// Idle execution overridden by Execute
	return true;
}

// Wait until airship arrives
func ExecuteMelee(fx)
{
	// Don't attack if still on the carrier
	if (fx.carrier)
	{
		if (!fx.carrier->IsInsideGondola(this)) fx.carrier = nil;
		if (GetCommand()) SetCommand("None");
		if (fx.shield) return ExecuteProtection(fx);
		return ExecuteIdle();
	}

	// Still carrying the melee weapon?
	if (fx.weapon->Contained() != this) { fx.weapon=nil; return false; }
	// Are we in range?
	var x=GetX(), y=GetY(), tx=fx.target->GetX(), ty=fx.target->GetY();
	var dx = tx-x, dy = ty-y;
	var dy_tolerance = -15;
	if (fx.target->~IsMainObjective()) dy_tolerance = -40; // Don't jump in front of the windmills you will fall off the islands

	if (Abs(dx) <= 10 && PathFree(x,y,tx,ty))
	{
		if (dy >= dy_tolerance)
		{
			// target is under us - sword slash downwards!
			if (!CheckHandsAction(fx)) return true;
			// Stop here
			SetCommand("None"); SetComDir(COMD_None);
			// cooldown?
			if (!fx.weapon->CanStrikeWithWeapon(this))
			{
				return true;
			}
			// OK, slash!
			SelectItem(fx.weapon);
			return fx.weapon->ControlUse(this, tx,ty);
		}
		if (fx.target->~IsMainObjective())
		{
			// Don't jump for higher windmills, get a new target!
			fx.target = nil;
			SetCommand("None");
			return true;
		}
		// Clonk is above us - jump there
		ExecuteJump();
		if (dx<-5) SetComDir(COMD_Left); else if (dx>5) SetComDir(COMD_Right); else SetComDir(COMD_None);
	}
	// Not in range. Walk there.
	if (!GetCommand() || !Random(10)) SetCommand("MoveTo", fx.target);
	//Message("Melee %s @ %s!!!", fx.weapon->GetName(), fx.target->GetName());
	return true;
}

// Use shields only if still on the airship
func ExecuteProtection(fx)
{
	if (!fx.carrier)
		return false;
	if (!fx.shield)
		return false;
	return _inherited(fx);
}

// Don't evade you will fall off the islands
func ExecuteEvade(fx, int foo, int bar)
{
	return false;
}

// Always shoot
private func ExecuteRanged(fx)
{
	// Still carrying the bow?
	if (fx.weapon->Contained() != this) { fx.weapon=fx.post_aim_weapon=nil; return false; }
	// Finish shooting process
	if (fx.post_aim_weapon)
	{
		// wait max one second after shot (otherwise may be locked in wait animation forever if something goes wrong during shot)
		if (FrameCounter() - fx.post_aim_weapon_time < 36)
			if (IsAimingOrLoading()) return true;
		fx.post_aim_weapon = nil;
	}
	// Target still in guard range?
	//if (!CheckTargetInGuardRange(fx)) return false;
	// Don't check for this guard range crap, fire if near enough
	if (ObjectDistance(fx.target, this) > CustomAI.ARCHER_SHOOT_DIST) return false;

	// Shooting at windmill
	if (fx.target->~IsMainObjective())
	{
		// Look for clonks in range
		if (!Random(25))
		{
			var target = FindTarget(fx, true);
			if (target)
				fx.target = target;
		}
	} else {
		// Firing at a clonk, maybe fire at a windmill
		if (!Random(50) || !PathFree(GetX(), GetY(), fx.target->GetX(), fx.target->GetY()))
			fx.target = FindTarget(fx);
		if (!fx.target) return; // ???
	}

	// Look at target
	ExecuteLookAtTarget(fx);
	// Make sure we can shoot
	if (!IsAimingOrLoading() || !fx.aim_weapon)
	{
		CancelAiming(fx);
		if (!CheckHandsAction(fx)) return true;
		// Start aiming
		SelectItem(fx.weapon);
		if (!fx.weapon->ControlUseStart(this, fx.target->GetX()-GetX(), fx.target->GetY()-GetY())) return false; // something's broken :(
		fx.aim_weapon = fx.weapon;
		fx.aim_time = fx.time;
		fx.post_aim_weapon = nil;
		// Enough for now
		return;
	}
	// Stuck in aim procedure check?
	if (GetEffect("IntAimCheckProcedure", this) && !this->ReadyToAction())
		return ExecuteStand(fx);
	// Calculate offset to target. Take movement into account
	// Also aim for the head (y-4) so it's harder to evade by jumping
	var x=GetX(), y=GetY(), tx=fx.target->GetX(), ty=fx.target->GetY()-4;
	var d = Distance(x,y,tx,ty);
	var dt = d * 10 / fx.projectile_speed; // projected travel time of the arrow
	tx += GetTargetXDir(fx.target, dt);
	ty += GetTargetYDir(fx.target, dt);
	if (!fx.target->GetContact(-1)) if (!fx.target->GetCategory() & C4D_StaticBack) ty += GetGravity()*dt*dt/200;

	// Get shooting angle
	var shooting_angle;
	if (fx.ranged_direct)
		shooting_angle = Angle(x, y, tx, ty, 10);
	else {
		if (PathFree(x,y,tx,ty))
			shooting_angle = GetBallisticAngle(tx-x, ty-y, fx.projectile_speed, 160);
		else
			shooting_angle = GetUpperBallisticAngle(tx-x, ty-y, fx.projectile_speed, 160);
	}
	if (GetType(shooting_angle) != C4V_Nil)
	{
		// No ally on path? Also search for allied animals, just in case.
		var ally;
		if (!fx.ignore_allies) ally = FindObject(Find_OnLine(0,0,tx-x,ty-y), Find_Exclude(this), Find_OCF(OCF_Alive), Find_Owner(GetOwner()));
		if (ally)
		{
			if (ExecuteJump()) return true;
			// can't jump and ally is in the way. just wait.
		}
		else
		{
			//Message("Bow @ %d!!!", shooting_angle);
			// Aim/Shoot there
			x = Sin(shooting_angle, 1000, 10);
			y = -Cos(shooting_angle, 1000, 10);
			fx.aim_weapon->ControlUseHolding(this, x,y);
			if (this->IsAiming() && fx.time >= fx.aim_time + fx.aim_wait)
			{
				//Log("Throw angle %v speed %v to reach %d %d", shooting_angle, fx.projectile_speed, tx-GetX(), ty-GetY());
				fx.aim_weapon->ControlUseStop(this, x,y);
				fx.post_aim_weapon = fx.aim_weapon; // assign post-aim status to allow slower shoot animations to pass
				fx.post_aim_weapon_time = FrameCounter();
				fx.aim_weapon = nil;
			}
			return true;
		}
	}

	// Path not free or out of range. Just wait for enemy to come...
	fx.aim_weapon->ControlUseHolding(this,tx-x,ty-y);
	// Might also change target if current is unreachable
	var new_target;
	if (!Random(3)) if (new_target = FindTarget(fx)) fx.target = new_target;
	return true;
}

// Used when no free path to target (presumably because of the airship gondola floor)
// Returns the 'upper' shooting angle, see GetBallisticAngle etc.
private func GetUpperBallisticAngle(int dx, int dy, int v, int max_angle)
{
	// v is in 1/10 pix/frame
	// gravity is in 1/100 pix/frame^2
	var g = GetGravity();
	// correct vertical distance to account for integration error
	// engine adds gravity after movement, so targets fly higher than they should
	// thus, we aim lower. we don't know the travel time yet, so we assume some 90% of v is horizontal
	// (it's ~2px correction for 200px shooting distance)
	dy += Abs(dx)*g*10/(v*180);
	//Log("Correction: Aiming %d lower!", Abs(dx)*q*10/(v*180));
	// q is in 1/10000 (pix/frame)^4
	var q = v**4 - g*(g*dx*dx-2*dy*v*v); // dy is negative up
	if (q<0) return nil; // out of range
	var a = (Angle(0, 0, g * dx, -Sqrt(q) - v * v, 10) + 1800) % 3600 - 1800;
	// Check bounds
	if(!Inside(a, -10 * max_angle, 10 * max_angle)) return nil;
	return a;
}

// Don't move exacty to target's position (throwing check will fail then)
private func ExecuteThrow(fx)
{
	// Still carrying the weapon to throw?
	if (fx.weapon->Contained() != this) { fx.weapon=nil; return false; }
	// Path to target free?
	var x=GetX(), y=GetY(), tx=fx.target->GetX(), ty=fx.target->GetY();
	if (PathFree(x,y,tx,ty))
	{
		var throw_speed = this.ThrowSpeed;
		if (fx.weapon->GetID() == Javelin) throw_speed *= 2;
		var rx = (throw_speed*throw_speed)/(100*GetGravity()); // horizontal range for 45 degree throw if enemy is on same height as we are
		var ry = throw_speed*7/(GetGravity()*10); // vertical range of 45 degree throw
		var dx = tx-x, dy = ty-y+15*GetCon()/100; // distance to target. Reduce vertical distance a bit because throwing exit point is not at center
		// Check range
		// Could calculate the optimal parabulum here, but that's actually not very reliable on moving targets
		// It's usually better to throw straight at the target and only throw upwards a bit if the target stands on high ground or is far away
		// Also ignoring speed added by own velocity, etc...
		if (Abs(dx)*ry-Min(dy)*rx <= rx*ry)
		{
			// We're in range. Can throw?
			if (!CheckHandsAction(fx)) return true;
			// OK. Calc throwing direction
			dy -= dx*dx/rx; // big math!
			// And throw!
			//Message("Throw!");
			SetCommand("None"); SetComDir(COMD_Stop);
			SelectItem(fx.weapon);
			return this->ControlThrow(fx.weapon, dx, dy);
		}
	}
	// Can't reach target yet. Walk towards it.
	if (!GetCommand() || !Random(3))
	{
		var tx = fx.target->GetX();
		SetCommand("MoveTo", nil, BoundBy(GetX(), tx-30, tx+30), fx.target->GetY());
	}
	//Message("Throw %s @ %s!!!", fx.weapon->GetName(), fx.target->GetName());
	return true;
}

//======================================================================
/* Enemy creation */

static g_last_airship;

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

func LaunchEnemy(proplist enemy, int xmin, int xrange, int ymin, yrange)
{
	// Create enemy (usually a Clonk)
	var x = xmin+Random(xrange);
	var y = ymin+Random(yrange);
	var obj = CreateObjectAbove(enemy.Type ?? Clonk, x,y, ENEMY), clonk;
	if (!obj) return nil;
	obj->SetController(ENEMY);
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
		obj.FlySpeed = obj.FlySpeed * enemy.Speed / 100;
	}
	obj.MaxContentsCount = 2;
	obj->MakeInvincibleToFriendlyFire();
	// Reward for killing enemy
	obj.Bounty = enemy.Bounty;
	// Vehicles
	var vehicle;
	if (enemy.Vehicle)
	{
		if (enemy.Vehicle == Balloon)
		{
			Balloon->ControlUseStart(obj);
		} else if (enemy.Vehicle == Boomattack) {
			vehicle = CreateObjectAbove(enemy.Vehicle, x,y+10, ENEMY);
			// Add boomattack to enemy array
			g_spawned_enemies[GetLength(g_spawned_enemies)] = vehicle;
			obj->SetAction("Ride", vehicle);
		} else {
			vehicle = CreateObjectAbove(enemy.Vehicle, x,y+10, ENEMY);
			obj->SetAction("Push", vehicle);
			if (vehicle && vehicle->GetID() == Airship)
				g_last_airship = vehicle;
		}
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
	if (obj->~IsFlyingEnemy())
	{
		// Flying enemies all init themselves to fly at the statue at the moment
	}
	else
	{
		// Init AI: Run towards statue
		CustomAI->AddAI(obj);
		CustomAI->SetMaxAggroDistance(obj, LandscapeWidth());
		CustomAI->SetGuardRange(obj, 0,0,LandscapeWidth(),LandscapeHeight()); // nowhere to run!
		var fx = GetEffect("AI", obj);
		if (fx) fx.vehicle = vehicle;
		if (enemy.IsCrew) // is airship crew member, spawn on last created airship
			if (g_last_airship)
			{
				obj->SetPosition(g_last_airship->GetX() + RandomX(-15,15), g_last_airship->GetY()+12);
				if (fx)
					fx.carrier = g_last_airship;
			}
	}
	// Remember this clonk to end wave when all enemies have been killed
	g_spawned_enemies[GetLength(g_spawned_enemies)] = obj;
	return obj;
}
