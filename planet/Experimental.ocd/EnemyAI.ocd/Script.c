static const S2AI_DefMaxAggroDistance = 200, // lose sight to target if it is this far away (unles we're ranged - then always guard the range rect)
             S2AI_DefGuardRangeX = 300,  // search targets this far away in either direction (searching in rectangle)
             S2AI_DefGuardRangeY = 150;  // search targets this far away in either direction (searching in rectangle)
             
/* Public interface */

// Add AI execution timer to target Clonk
func AddAI(object clonk)
{
	var fx = GetEffect("S2AI", clonk);
	if (!fx) fx = AddEffect("S2AI", clonk, 1, 10, nil, S2AI);
	if (!fx || !clonk) return nil;
	clonk.ExecuteS2AI = S2AI.Execute;
	if (clonk->GetProcedure() == "PUSH") fx.vehicle = clonk->GetActionTarget();
	BindInventory(clonk);
	SetHome(clonk);
	SetGuardRange(clonk, fx.home_x-S2AI_DefGuardRangeX, fx.home_y-S2AI_DefGuardRangeY, S2AI_DefGuardRangeX*2, S2AI_DefGuardRangeY*2);
	SetMaxAggroDistance(clonk, S2AI_DefMaxAggroDistance);
	return fx;
}

func GetAI(object clonk) { return GetEffect("S2AI", clonk); }

// Set the current inventory to be removed when the clonk dies. Only works if clonk has an AI.
func BindInventory(object clonk)
{
	var fx = GetEffect("S2AI", clonk);
	if (!fx || !clonk) return false;
	var cnt = clonk->ContentsCount();
	fx.bound_weapons = CreateArray(cnt);
	for (var i=0; i<cnt; ++i) fx.bound_weapons[i] = clonk->Contents(i);
	clonk->Call(S2AI.UpdateDebugDisplay, fx);
	return true;
}

// Set the home position the Clonk returns to if he has no target
func SetHome(object clonk, int x, int y, int dir)
{
	var fx = GetEffect("S2AI", clonk);
	if (!fx || !clonk) return false;
	// nil/nil defaults to current position
	if (!GetType(x)) x=clonk->GetX();
	if (!GetType(y)) y=clonk->GetY();
	if (!GetType(dir)) dir=clonk->GetDir();
	fx.home_x = x; fx.home_y = y;
	fx.home_dir = dir;
	return true;
}

// Set the guard range to the provided rectangle
func SetGuardRange(object clonk, int x, int y, int wdt, int hgt)
{
	var fx = GetEffect("S2AI", clonk);
	if (!fx || !clonk) return false;
	fx.guard_range_check = Find_AtRect(x-GetX(),y-GetY(),wdt,hgt);
	fx.guard_range = {x=x, y=y, wdt=wdt, hgt=hgt};
	clonk->Call(S2AI.UpdateDebugDisplay, fx);
	return true;
}

// Set the maximum distance the enemy will follow an attacking Clonk
func SetMaxAggroDistance(object clonk, int max_dist)
{
	var fx = GetEffect("S2AI", clonk);
	if (!fx || !clonk) return false;
	fx.max_aggro_distance = max_dist;
	return true;
}

/* Internal functions */

func FxS2AITimer(clonk, fx, int time) { clonk->ExecuteS2AI(fx, time); return FX_OK; }

func FxS2AIStop(clonk, fx, int reason)
{
	// remove debug display
	if (fx.debug) clonk->Call(S2AI.EditCursorDeselection, fx);
	// remove weapons on death
	if (reason == FX_Call_RemoveDeath)
	{
		if (fx.bound_weapons) for (var obj in fx.bound_weapons) if (obj && obj->Contained()==clonk) obj->RemoveObject();
	}
	return FX_OK;
}


// called in context of the Clonk that is being controlled
func Execute(proplist fx, int time)
{
	fx.time = time;
	// Find something to fight with
	if (!fx.weapon) { CancelAiming(fx); if (!ExecuteArm(fx)) return ExecuteIdle(fx); else if (!fx.weapon) return true; }
	// Weapon out of ammo?
	if (fx.ammo_check && !Call(fx.ammo_check, fx.weapon)) { fx.weapon=nil; return false; }
	// Find an enemy
	if (fx.target) if (!fx.target->GetAlive() || (!fx.ranged && ObjectDistance(fx.target) >= fx.max_aggro_distance)) fx.target = nil;
	if (!fx.target) { CancelAiming(fx); if (!(fx.target = FindTarget(fx))) return ExecuteIdle(fx); }
	// Attack it!
	return Call(fx.strategy, fx);
}

func CheckTargetInGuardRange(fx)
{
	// if target is not in guard range, reset it and return false
	if (!Inside(fx.target->GetX()-fx.guard_range.x, -10, fx.guard_range.wdt+9)
		||!Inside(fx.target->GetY()-fx.guard_range.y, -10, fx.guard_range.hgt+9))
		{ fx.target=nil; return false; }
	return true;
}

func ExecuteVehicle(fx)
{
	// only knows how to use catapult
	if (!fx.vehicle || fx.vehicle->GetID() != Catapult) { fx.vehicle = nil; return false; }
	// still pushing it?
	if (GetProcedure() != "PUSH" || GetActionTarget() != fx.vehicle)
	{
		if (!GetCommand() || !Random(4)) SetCommand("Grab", fx.vehicle);
		return true;
	}
	// Target still in guard range?
	if (!CheckTargetInGuardRange(fx)) return false;
	// turn in correct direction
	var x=GetX(), y=GetY(), tx=fx.target->GetX(), ty=fx.target->GetY()-4;
	if (tx>x && !fx.vehicle.dir) { fx.vehicle->ControlRight(this); return true; }
	if (tx<x &&  fx.vehicle.dir) { fx.vehicle->ControlLeft (this); return true; }
	// make sure we're aiming
	if (!fx.aim_weapon)
	{
		if (!fx.vehicle->~ControlUseStart(this)) return false;
		fx.aim_weapon = fx.vehicle;
		fx.aim_time = fx.time;
		return true;
	}
	// update catapult animation
	fx.vehicle->~ControlUseHolding(this, tx-x, ty-y);
	// project target position
	var d = Distance(x,y,tx,ty);
	fx.projectile_speed = fx.vehicle->DefinePower(tx-x, ty-y);
	var dt = d * 10 / fx.projectile_speed; // projected travel time of the object
	tx += fx.target->GetXDir(dt);
	ty += fx.target->GetYDir(dt);
	if (!fx.target->GetContact(-1)) ty += GetGravity()*dt*dt/200;
	// Can shoot now?
	if (fx.time >= fx.aim_time + fx.aim_wait) if (PathFree(x,y,tx,ty))
	{
		fx.aim_weapon->~ControlUseStop(this, tx-x,ty-y);
		fx.aim_weapon = nil;
	}
	return true;
}

func CancelAiming(fx)
{
	if (fx.aim_weapon)
	{
		fx.aim_weapon->~ControlUseCancel(this);
		fx.aim_weapon = nil;
	}
	return true;
}

func IsAimingOrLoading() { return !!GetEffect("IntAim*", this); }

func ExecuteRanged(fx)
{
	// Still carrying the bow?
	if (fx.weapon->Contained() != this) { fx.weapon=nil; return false; }
	// Target still in guard range?
	if (!CheckTargetInGuardRange(fx)) return false;
	// Make sure we can shoot
	if (!IsAimingOrLoading() || !fx.aim_weapon)
	{
		CancelAiming(fx);
		if (!CheckHandsAction(fx)) return true;
		// Start aiming
		if (!fx.weapon->ControlUseStart(this, fx.target->GetX()-GetX(), ty=fx.target->GetY()-GetY())) return false; // something's broken :(
		fx.aim_weapon = fx.weapon;
		fx.aim_time = fx.time;
		// Enough for now
		return;
	}
	// Calculate offset to target. Take movement into account
	// Also aim for the head (y-4) so it's harder to evade by jumping
	var x=GetX(), y=GetY(), tx=fx.target->GetX(), ty=fx.target->GetY()-4;
	var d = Distance(x,y,tx,ty);
	var dt = d * 10 / fx.projectile_speed; // projected travel time of the arrow
	tx += fx.target->GetXDir(dt);
	ty += fx.target->GetYDir(dt);
	if (!fx.target->GetContact(-1)) ty += GetGravity()*dt*dt/200;
	// Path to target free?
	if (PathFree(x,y,tx,ty))
	{
		// Get shooting angle
		var shooting_angle = GetBallisticAngle(tx-x, ty-y, fx.projectile_speed, 160);
		if (GetType(shooting_angle) != C4V_Nil)
		{
			//Message("Bow @ %d!!!", shooting_angle);
			// Aim/Shoot there
			x = Sin(shooting_angle,100);
			y = -Cos(shooting_angle,100);
			fx.aim_weapon->ControlUseHolding(this, x,y);
			if (this->IsAiming() && fx.time >= fx.aim_time + fx.aim_wait)
			{
				//Log("Throw angle %v speed %v to reach %d %d", shooting_angle, fx.projectile_speed, tx-GetX(), ty-GetY());
				fx.aim_weapon->ControlUseStop(this, x,y);
				fx.aim_weapon = nil;
			}
			return true;
		}
	}
	// Path not free or out of range. Just wait for enemy to come...
	//Message("Bow @ %s!!!", fx.target->GetName());
	fx.aim_weapon->ControlUseHolding(this,tx-x,ty-y);
	return true;
}

func ExecuteThrow(fx)
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
			return this->ControlThrow(fx.weapon, dx, dy);
		}
	}
	// Can't reach target yet. Walk towards it.
	if (!GetCommand() || !Random(3)) SetCommand("MoveTo", fx.target);
	//Message("Throw %s @ %s!!!", fx.weapon->GetName(), fx.target->GetName());
	return true;
}

func CheckHandsAction(fx)
{
	// can use hands?
	if (this->~HasHandAction()) return true;
	// Can't throw: Is it because e.g. we're scaling?
	if (!this->HasActionProcedure()) { ExecuteStand(fx); return false; }
	// Probably hands busy. Just wait.
	//Message("HandsBusy");
	return false;
}

func ExecuteStand(fx)
{
	//Message("Stand");
	SetCommand("None");
	if (GetProcedure() == "SCALE")
	{
		if (GetDir()==DIR_Left) SetComDir(COMD_Right); else SetComDir(COMD_Left);
	}
	else if (GetProcedure() == "HANGLE")
	{
		SetComDir(COMD_Down);
	}
	else
	{
		// Hm. What could it be? Let's just hope it resolves itself somehow...
		SetComDir(COMD_Stop);
		//Message("???");
	}
	return true;
}

func ExecuteMelee(fx)
{
	// Still carrying the melee weapon?
	if (fx.weapon->Contained() != this) { fx.weapon=nil; return false; }
	// Are we in range?
	var x=GetX(), y=GetY(), tx=fx.target->GetX(), ty=fx.target->GetY();
	var dx = tx-x, dy = ty-y;
	if (Abs(dx) <= 10 && PathFree(x,y,tx,ty))
	{
		if (dy >= -15)
		{
			// target is under us - sword slash downwards!
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
			// OK, slash!
			//Message("MeleeSLASH %s @ %s!!!", fx.weapon->GetName(), fx.target->GetName());
			return fx.weapon->ControlUse(this, tx,ty);
		}
		// Clonk is above us - jump there
		//Message("MeleeJump %s @ %s!!!", fx.weapon->GetName(), fx.target->GetName());
		this->ControlJump();
		if (dx<-5) SetComDir(COMD_Left); else if (dx>5) SetComDir(COMD_Right); else SetComDir(COMD_None);
	}
	// Not in range. Walk there.
	if (!GetCommand() || !Random(3)) SetCommand("MoveTo", fx.target);
	//Message("Melee %s @ %s!!!", fx.weapon->GetName(), fx.target->GetName());
	return true;
}

func ExecuteEvade(fx,int threat_dx,int threat_dy)
{
	// Evade from threat at position delta threat_*
	if (threat_dx < 0) SetComDir(COMD_Left); else SetComDir(COMD_Right);
	if (threat_dy >= -5 && !Random(2)) this->ControlJump();
	// shield? todo
	return true;
}

func ExecuteArm(fx)
{
	fx.ammo_check = nil; fx.ranged = false;
	// Find a weapon. For now, just search own inventory
	if (fx.weapon = fx.vehicle)
		if (CheckVehicleAmmo(fx.weapon))
			{ fx.strategy = S2AI.ExecuteVehicle; fx.ranged=true; fx.aim_wait = 20; fx.ammo_check = S2AI.CheckVehicleAmmo; return true; }
		else
			fx.weapon = nil;
	if (fx.weapon = FindContents(Bow))
		if (HasArrows(fx.weapon))
			{ fx.strategy = S2AI.ExecuteRanged; fx.projectile_speed = 100; fx.aim_wait = 0; fx.ammo_check = S2AI.HasArrows; fx.ranged=true; return true; }
		else
			fx.weapon = nil;
	if (fx.weapon = FindContents(Javelin)) { fx.strategy = S2AI.ExecuteRanged; fx.projectile_speed = this.ThrowSpeed*21/100; fx.aim_wait = 16; fx.ranged=true; return true; }
	if (fx.weapon = FindContents(Firestone)) { fx.strategy = S2AI.ExecuteThrow; return true; }
	if (fx.weapon = FindContents(Sword)) { fx.strategy = S2AI.ExecuteMelee; return true; }
	if (fx.weapon = Contents(0)) { fx.strategy = S2AI.ExecuteThrow; return true; }
	// no weapon :(
	return false;
}

func HasArrows(object weapon)
{
	if (weapon->Contents(0)) return true;
	if (FindObject(Find_Container(this), Find_Func("IsArrow"))) return true;
	return false;
}

func CheckVehicleAmmo(object catapult)
{
	return catapult->ContentsCount();
}

func ExecuteIdle(fx)
{
	if (!Inside(GetX()-fx.home_x, -5,5) || !Inside(GetY()-fx.home_y, -15,15))
	{
		SetCommand("MoveTo", nil, fx.home_x, fx.home_y);
	}
	else
	{
		SetCommand("None"); SetComDir(COMD_Stop); SetDir(fx.home_dir);
	}
	//Message("zZz");
	return true;
}

func FindTarget(fx)
{
	// could search for hostile...for now, just search for all other players
	for (var target in FindObjects(fx.guard_range_check, Find_OCF(OCF_Alive), Find_Not(Find_Owner(GetOwner())), Sort_Random()))
		if (PathFree(GetX(),GetY(),target->GetX(),target->GetY()))
			return target;
	// nothing found
	return nil;
}

// Helper function: Convert target cordinates and bow out speed to desired shooting angle
// Because http://en.wikipedia.org/wiki/Trajectory_of_a_projectile says so
// No SimFlight checks to check upper angle (as that is really easy to evade anyway)
// just always shoot the lower angle if sight is free
private func GetBallisticAngle(int dx, int dy, int v, int max_angle)
{
	// v is in 1/10 pix/frame
	// gravity is in 1/100 pix/frame^2
	var g = GetGravity();
	// correct vertical distance to account for integration error
	// engine adds gravity after movement, so targets fly higher than they should
	// thus, we aim lower. we don't know the travel time yet, so we assume some 90% of v is horizontal
	// (it's ~2px correction for 200px shooting distance)
	dy += Abs(dx)*q*10/(v*180);
	//Log("Correction: Aiming %d lower!", Abs(dx)*q*10/(v*180));
	// q is in 1/10000 (pix/frame)^4
	var q = v**4 - g*(g*dx*dx-2*dy*v*v); // dy is negative up
	if (q<0) return nil; // out of range
	var a = (Angle(0,0,g*dx,Sqrt(q)-v*v)+180)%360-180;
	// Check bounds
	if(!Inside(a, -max_angle, max_angle)) return nil;
	return a;
}

/* Editor display */

// called in clonk context
func UpdateDebugDisplay(fx)
{
	if (fx.debug) { EditCursorDeselection(fx); EditCursorSelection(fx); }
	return true;
}

// called in clonk context
func EditCursorSelection(fx)
{
	if (fx.debug) EditCursorDeselection(fx);
	var msg = "";
	for (var i=0; i<ContentsCount(); ++i)
		msg = Format("%s{{%i}}", msg, Contents(i)->GetID());
	Message("@AI %s", msg);
	fx.debug = {};
	var clr = 0xffff0000;
	fx.debug.r1 = DebugLine->Create(fx.guard_range.x,fx.guard_range.y,fx.guard_range.x+fx.guard_range.wdt,fx.guard_range.y,clr);
	fx.debug.r2 = DebugLine->Create(fx.guard_range.x+fx.guard_range.wdt,fx.guard_range.y,fx.guard_range.x+fx.guard_range.wdt,fx.guard_range.y+fx.guard_range.hgt,clr);
	fx.debug.r3 = DebugLine->Create(fx.guard_range.x+fx.guard_range.wdt,fx.guard_range.y+fx.guard_range.hgt,fx.guard_range.x,fx.guard_range.y+fx.guard_range.hgt,clr);
	fx.debug.r4 = DebugLine->Create(fx.guard_range.x,fx.guard_range.y+fx.guard_range.hgt,fx.guard_range.x,fx.guard_range.y,clr);
	return true;
}

// called in clonk context
func EditCursorDeselection(fx)
{
	if (fx.debug)
	{
		if (fx.debug.r1) fx.debug.r1->RemoveObject();
		if (fx.debug.r2) fx.debug.r2->RemoveObject();
		if (fx.debug.r3) fx.debug.r3->RemoveObject();
		if (fx.debug.r4) fx.debug.r4->RemoveObject();
		Message("");
	}
	fx.debug = nil;
	return true;
}
