/**
	AI
	Controls NPC behaviour.

	@author Sven2
*/


// Include the different parts of the AI.
#include AI_HelperFunctions
#include AI_MeleeWeapons
#include AI_RangedWeapons
#include AI_Vehicles

// General settings of the AI, these can be modified per script for the specific AI clonk.
static const AI_DefMaxAggroDistance = 200, // Lose sight to target if it is this far away (unles we're ranged - then always guard the range rect).
             AI_DefGuardRangeX      = 300, // Search targets this far away in either direction (searching in rectangle).
             AI_DefGuardRangeY      = 150, // Search targets this far away in either direction (searching in rectangle).
             AI_AlertTime           = 800; // Number of frames after alert after which AI no longer checks for projectiles.


/*-- Public interface --*/

// Change whether target Clonk has an AI (used by editor)
public func SetAI(object clonk, bool has_ai)
{
	if (has_ai)
	{
		// Only add if it doesn't have the effect yet
		var ai = GetAI(clonk);
		if (!ai) ai = AddAI(clonk);
		return ai;
	}
	else
	{
		return RemoveEffect("AI", clonk);
	}
}

// Add AI execution timer to target Clonk
public func AddAI(object clonk)
{
	var fx = GetEffect("AI", clonk);
	if (!fx) fx = AddEffect("AI", clonk, 1, 3, nil, AI);
	if (!fx || !clonk)
		return nil;
	fx.ai = AI;
	clonk.ExecuteAI = AI.Execute;
	clonk.ai = fx;
	if (clonk->GetProcedure() == "PUSH")
		fx.vehicle = clonk->GetActionTarget();
	BindInventory(clonk);
	SetHome(clonk);
	SetGuardRange(clonk, fx.home_x-AI_DefGuardRangeX, fx.home_y-AI_DefGuardRangeY, AI_DefGuardRangeX*2, AI_DefGuardRangeY*2);
	SetMaxAggroDistance(clonk, AI_DefMaxAggroDistance);
	SetAutoSearchTarget(clonk, true);
	// AI editor stuff
	// TODO: Should use new-style effects to declare this directly in a base class
	// Whoever does that: Make sure that the derived AIs in our defense scenarios still work
	fx.EditorProps = {};
	fx.EditorProps.guard_range = { Name="$GuardRange$", Type="rect", Storage="proplist", Color=0xff00, Relative=false };
	fx.EditorProps.ignore_allies = { Name="$IgnoreAllies$", Type="bool" };
	fx.EditorProps.max_aggro_distance = { Name="$MaxAggroDistance$", Type="circle", Color=0x808080 };
	fx.EditorProps.active = { Name="$Active$", EditorHelp="$ActiveHelp$", Type="bool", Priority=50, AsyncGet="GetActive", Set="SetActive" };
	fx.EditorProps.auto_search_target = { Name="$AutoSearchTarget$", EditorHelp="$AutoSearchTargetHelp$", Type="bool" };
	fx.SetActive = AI.Fx_SetActive;
	fx.GetActive = AI.Fx_GetActive;
	return fx;
}

public func GetAI(object clonk) { return clonk.ai; }

// Set the current inventory to be removed when the clonk dies. Only works if clonk has an AI.
public func BindInventory(object clonk)
{
	if (!clonk)
		return false;
	var fx = clonk.ai;
	if (!fx)
		return false;
	var cnt = clonk->ContentsCount();
	fx.bound_weapons = CreateArray(cnt);
	for (var i = 0; i < cnt; ++i)
		fx.bound_weapons[i] = clonk->Contents(i);
	return true;
}

// Set the home position the Clonk returns to if he has no target.
public func SetHome(object clonk, int x, int y, int dir)
{
	if (!clonk)
		return false;
	var fx = clonk.ai;
	if (!fx)
		return false;
	// nil/nil defaults to current position.
	if (!GetType(x))
		x = clonk->GetX();
	if (!GetType(y))
		y = clonk->GetY();
	if (!GetType(dir))
		dir = clonk->GetDir();
	fx.home_x = x;
	fx.home_y = y;
	fx.home_dir = dir;
	return true;
}

// Put into clonk proplist.
public func AI_BindInventory() { return this.ai.ai->BindInventory(this); }
public func AI_SetHome() { return this.ai.ai->SetHome(this); }
public func AI_SetIgnoreAllies() { return (this.ai.ignore_allies = true); }

// Set active state: Enables/Disables timer
public func SetActive(object clonk, bool active)
{
	if (!clonk)
		return false;
	var fx = clonk.ai;
	if (!fx)
		return false;
	if (!active)
	{
		// Inactive: Stop any activity.
		clonk->SetCommand("None");
		clonk->SetComDir(COMD_Stop);
	}
	return fx->SetActive(active);
}

public func Fx_SetActive(bool active)
{
	// In effect context: Activity controlled by execution interval
	this.Interval = active * 3;
	return true;
}

public func Fx_GetActive()
{
	return !!this.Interval;
}

// Enable/disable auto-searching of targets.
public func SetAutoSearchTarget(object clonk, bool new_auto_search_target)
{
	if (!clonk)
		return false;
	var fx = clonk.ai;
	if (!fx)
		return false;
	fx.auto_search_target = new_auto_search_target;
	return true;
}

// Set the guard range to the provided rectangle.
public func SetGuardRange(object clonk, int x, int y, int wdt, int hgt)
{
	if (!clonk)
		return false;
	var fx = clonk.ai;
	if (!fx)
		return false;
	// Clip to landscape size.
	if (x < 0)
	{
		wdt += x;
		x = 0;
	}
	if (y < 0)
	{
		hgt += y;
		y = 0;
	}
	wdt = Min(wdt, LandscapeWidth() - x);
	hgt = Min(hgt, LandscapeHeight() - y);
	fx.guard_range = {x = x, y = y, wdt = wdt, hgt = hgt};
	return true;
}

// Set the maximum distance the enemy will follow an attacking clonk.
public func SetMaxAggroDistance(object clonk, int max_dist)
{
	if (!clonk)
		return false;
	var fx = clonk.ai;
	if (!fx)
		return false;
	fx.max_aggro_distance = max_dist;
	return true;
}

// Set range in which, on first encounter, allied AI clonks get the same aggro target set.
public func SetAllyAlertRange(object clonk, int new_range)
{
	if (!clonk)
		return false;
	var fx = clonk.ai;
	if (!fx)
		return false;
	fx.ally_alert_range = new_range;
	return true;
}

// Set callback function name to be called in game script when this AI is first encountered
// Callback function first parameter is (this) AI clonk, second parameter is player clonk.
// The callback should return true to be cleared and not called again. Otherwise, it will be called every time a new target is found.
public func SetEncounterCB(object clonk, string cb_fn)
{
	if (!clonk)
		return false;
	var fx = clonk.ai;
	if (!fx)
		return false;
	fx.encounter_cb = cb_fn;
	return true;
}


/*-- Scenario saving --*/

public func FxAISaveScen(clonk, fx, props)
{
	if (!clonk) return false;
	props->AddCall("AI", AI, "AddAI", clonk);
	if (!fx.Interval)
		props->AddCall("AI", AI, "SetActive", clonk, false);
	if (fx.home_x != clonk->GetX() || fx.home_y != clonk->GetY() || fx.home_dir != clonk->GetDir())
		props->AddCall("AI", AI, "SetHome", clonk, fx.home_x, fx.home_y, GetConstantNameByValueSafe(fx.home_dir, "DIR_"));
	props->AddCall("AI", AI, "SetGuardRange", clonk, fx.guard_range.x, fx.guard_range.y, fx.guard_range.wdt, fx.guard_range.hgt);
	if (fx.max_aggro_distance != AI_DefMaxAggroDistance)
		props->AddCall("AI", AI, "SetMaxAggroDistance", clonk, fx.max_aggro_distance);
	if (fx.ally_alert_range)
		props->AddCall("AI", AI, "SetAllyAlertRange", clonk, fx.ally_alert_range);
	if (!fx.auto_search_target)
		props->AddCall("AI", AI, "SetAutoSearchTarget", clonk, false);
	if (fx.encounter_cb)
		props->AddCall("AI", AI, "SetEncounterCB", clonk, Format("%v", fx.encounter_cb));
	return true;
}


/*-- AI effect callback functions --*/

public func FxAITimer(object clonk, effect fx, int time)
{
	clonk->ExecuteAI(fx, time);
	return FX_OK;
}

public func FxAIStop(object clonk, effect fx, int reason)
{
	// Remove weapons on death.
	if (reason == FX_Call_RemoveDeath)
	{
		if (fx.bound_weapons)
			for (var obj in fx.bound_weapons)
				if (obj && obj->Contained()==clonk)
					obj->RemoveObject();
	}
	// Remove AI reference.
	if (clonk && clonk.ai == fx)
		clonk.ai = nil;
	return FX_OK;
}

public func FxAIDamage(object clonk, effect fx, int dmg, int cause)
{
	// AI takes damage: Make sure we're alert so evasion and healing scripts are executed!
	// It might also be feasible to execute encounter callbacks here (in case an AI is shot from a position it cannot see).
	// However, the attacking clonk is not known and the callback might be triggered e.g. by an unfortunate meteorite or lightning blast.
	// So let's just keep it at alert state for now.
	if (dmg < 0) 
		fx.alert = fx.time;
	return dmg;
}


/*-- AI execution timer functions --*/

// Called in context of the clonk that is being controlled
private func Execute(effect fx, int time)
{
	fx.time = time;
	// Evasion, healing etc. if alert.
	if (fx.alert)
		if (ExecuteProtection(fx))
			return true;
	// Current command override.
	if (fx.command)
	{
		if (Call(fx.command, fx))
			return true;
		else
			fx.command = nil;
	}
	// Find something to fight with.
	if (!fx.weapon) 
	{
		CancelAiming(fx);
		if (!ExecuteArm(fx))
			return ExecuteIdle(fx);
		else if (!fx.weapon)
			return true;
	}
	// Weapon out of ammo?
	if (fx.ammo_check && !Call(fx.ammo_check, fx, fx.weapon))
	{
		fx.weapon = nil;
		return false;
	}
	// Find an enemy.
	if (fx.target) 
		if (!fx.target->GetAlive() || (!fx.ranged && ObjectDistance(fx.target) >= fx.max_aggro_distance))
			fx.target = nil;
	if (!fx.target)
	{
		CancelAiming(fx);
		if (!fx.auto_search_target || !(fx.target = FindTarget(fx)))
			return ExecuteIdle(fx);
		// First encounter callback. might display a message.
		if (fx.encounter_cb)
			if (GameCall(fx.encounter_cb, this, fx.target))
				fx.encounter_cb = nil;
		// Wake up nearby allies.
		if (fx.ally_alert_range)
		{
			var ally_fx;
			for (var ally in FindObjects(Find_Distance(fx.ally_alert_range), Find_Exclude(this), Find_OCF(OCF_CrewMember), Find_Owner(GetOwner())))
				if (ally_fx = AI->GetAI(ally))
					if (!ally_fx.target)
					{
						ally_fx.target = fx.target;
						ally_fx.alert = ally_fx.time;
						if (ally_fx.encounter_cb) 
							if (GameCall(ally_fx.encounter_cb, ally, fx.target))
								ally_fx.encounter_cb = nil;
					}
			// Waking up works only once. after that, AI might have moved and wake up clonks it shouldn't.
			fx.ally_alert_range = nil;
		}
	}
	// Attack it!
	return Call(fx.strategy, fx);
}

// Selects an item the clonk is about to use.
private func SelectItem(object item)
{
	if (!item)
		return;
	if (item->Contained() != this)
		return;
	this->SetHandItemPos(0, this->GetItemPos(item));
}

private func ExecuteProtection(effect fx)
{
	// Search for nearby projectiles. Ranged AI also searches for enemy clonks to evade.
	var enemy_search;
	if (fx.ranged)
		enemy_search = Find_And(Find_OCF(OCF_CrewMember), Find_Not(Find_Owner(GetOwner())));
	var projectiles = FindObjects(Find_InRect(-150,-50,300,80), Find_Or(Find_Category(C4D_Object), Find_Func("IsDangerous4AI"), Find_Func("IsArrow"), enemy_search), Find_OCF(OCF_HitSpeed2), Find_NoContainer(), Sort_Distance());
	for (var obj in projectiles)
	{
		var dx = obj->GetX() - GetX(), dy = obj->GetY() - GetY();
		var vx = obj->GetXDir(), vy = obj->GetYDir();
		if (Abs(dx) > 40 && vx)
			dy += (Abs(10 * dx / vx)**2) * GetGravity() / 200;
		var v2 = Max(vx * vx + vy * vy, 1);
		var d2 = dx * dx + dy * dy;
		if (d2 / v2 > 4)
		{
			// Won't hit within the next 20 frames.
			continue;
		}
		// Distance at which projectile will pass clonk should be larger than clonk size (erroneously assumes clonk is a sphere).
		var l = dx * vx + dy * vy;
		if (l < 0 && Sqrt(d2 - l * l / v2) <= GetCon() / 8)
		{
			// Not if there's a wall between.
			if (!PathFree(GetX(), GetY(), obj->GetX(), obj->GetY()))
				continue;
			// This might hit.
			fx.alert=fx.time;
			// Use a shield if the object is not explosive.
			if (fx.shield && !obj->~HasExplosionOnImpact())
			{
				// Use it!
				SelectItem(fx.shield);
				if (fx.aim_weapon == fx.shield)
				{
					// Continue to hold shield.
					fx.shield->ControlUseHolding(this, dx,dy);
				}
				else
				{
					// Start holding shield.
					if (fx.aim_weapon)
						CancelAiming(fx);
					if (!CheckHandsAction(fx))
						return true;
					if (!fx.shield->ControlUseStart(this, dx,dy))
						return false; // Something's broken :(
					fx.shield->ControlUseHolding(this, dx, dy);
					fx.aim_weapon = fx.shield;
				}
				return true;
			}
			// No shield. try to jump away.
			if (dx < 0)
				SetComDir(COMD_Right);
			else
				SetComDir(COMD_Left);
			if (ExecuteJump())
				return true;
			// Can only try to evade one projectile.
			break;
		}
	}
	// Stay alert if there's a target. Otherwise alert state may wear off.
	if (!fx.target)
		fx.target = FindEmergencyTarget(fx);
	if (fx.target)
		fx.alert = fx.time;
	else if (fx.time - fx.alert > AI_AlertTime)
		fx.alert = nil;
	// Nothing to do.
	return false;
}

private func CheckTargetInGuardRange(effect fx)
{
	// If target is not in guard range, reset it and return false.
	if (!Inside(fx.target->GetX() - fx.guard_range.x, -10, fx.guard_range.wdt + 9) || !Inside(fx.target->GetY() - fx.guard_range.y, -10, fx.guard_range.hgt + 9)) 
	{
		if (ObjectDistance(fx.target) > 250)
		{
			fx.target = nil;
			return false;
		}
	}
	return true;
}

private func CancelAiming(effect fx)
{
	if (fx.aim_weapon)
	{
		fx.aim_weapon->~ControlUseCancel(this);
		fx.aim_weapon = nil;
	}
	else
	{
		// Also cancel aiming done outside AI control.
		this->~CancelAiming();
	}
	return true;
}

private func ExecuteLookAtTarget(effect fx)
{
	// Set direction to look at target, we can assume this is instantanuous.
	if (fx.target->GetX() > GetX())
		SetDir(DIR_Right);
	else
		SetDir(DIR_Left);
	return true;
}

private func ExecuteThrow(effect fx)
{
	// Still carrying the weapon to throw?
	if (fx.weapon->Contained() != this)
	{
		fx.weapon = nil;
		return false;
	}
	// Path to target free?
	var x = GetX(), y = GetY(), tx = fx.target->GetX(), ty = fx.target->GetY();
	if (PathFree(x, y, tx, ty))
	{
		var throw_speed = this.ThrowSpeed;
		if (fx.weapon->GetID() == Javelin)
			throw_speed *= 2;
		var rx = (throw_speed * throw_speed) / (100 * GetGravity()); // horizontal range for 45 degree throw if enemy is on same height as we are
		var ry = throw_speed * 7 / (GetGravity() * 10); // vertical range of 45 degree throw
		var dx = tx - x, dy = ty - y + 15 * GetCon() / 100; // distance to target. Reduce vertical distance a bit because throwing exit point is not at center
		// Check range
		// Could calculate the optimal parabulum here, but that's actually not very reliable on moving targets
		// It's usually better to throw straight at the target and only throw upwards a bit if the target stands on high ground or is far away
		// Also ignoring speed added by own velocity, etc...
		if (Abs(dx) * ry - Min(dy) * rx <= rx*ry)
		{
			// We're in range. Can throw?
			if (!CheckHandsAction(fx))
				return true;
			// OK. Calc throwing direction.
			dy -= dx * dx / rx;
			// And throw!
			SetCommand("None");
			SetComDir(COMD_Stop);
			SelectItem(fx.weapon);
			return this->ControlThrow(fx.weapon, dx, dy);
		}
	}
	// Can't reach target yet. Walk towards it.
	if (!GetCommand() || !Random(3))
		SetCommand("MoveTo", fx.target);
	return true;
}

private func CheckHandsAction(effect fx)
{
	// Can use hands?
	if (this->~HasHandAction())
		return true;
	// Can't throw: Is it because e.g. we're scaling?
	if (!this->HasActionProcedure())
	{
		ExecuteStand(fx);
		return false;
	}
	// Probably hands busy. Just wait.
	return false;
}

private func ExecuteStand(effect fx)
{
	SetCommand("None");
	if (GetProcedure() == "SCALE" || GetAction() == "Climb")
	{
		var tx;
		if (fx.target)
			tx = fx.target->GetX() - GetX();
		// Scale: Either scale up if target is beyond this obstacle or let go if it's not.
		if (GetDir() == DIR_Left)
		{
			if (tx < -20)
				SetComDir(COMD_Left);
			else
				ObjectControlMovement(GetOwner(), CON_Right, 100); // let go
		}
		else
		{
			if (tx > -20)
				SetComDir(COMD_Right);
			else
				ObjectControlMovement(GetOwner(), CON_Left, 100); // let go
		}
	}
	else if (GetProcedure() == "HANGLE")
	{
		ObjectControlMovement(GetOwner(), CON_Down, 100);
	}
	else
	{
		// Hm. What could it be? Let's just hope it resolves itself somehow...
		SetComDir(COMD_Stop);
	}
	return true;
}

private func ExecuteEvade(effect fx, int threat_dx, int threat_dy)
{
	// Evade from threat at position delta threat_dx, threat_dy.
	if (threat_dx < 0)
		SetComDir(COMD_Left);
	else
		SetComDir(COMD_Right);
	if (threat_dy >= -5 && !Random(2))
		if (ExecuteJump(fx))
			return true;
	// Shield? Todo.
	return true;
}

private func ExecuteJump(effect fx)
{
	// Jump if standing on floor.
	if (GetProcedure() == "WALK")
	{
		if (this->~ControlJump())
			return true; // For clonks.
		return this->Jump(); // For others.
	}
	return false;
}

private func ExecuteArm(effect fx)
{
	// Find shield.
	fx.shield = FindContents(Shield);
	// Find a weapon. For now, just search own inventory.
	if (FindInventoryWeapon(fx) && fx.weapon->Contained() == this)
	{
		SelectItem(fx.weapon);
		return true;
	}
	// No weapon.
	return false;
}

private func FindInventoryWeapon(effect fx)
{
	fx.ammo_check = nil;
	fx.ranged = false;
	// Find weapon in inventory, mark it as equipped and set according strategy, etc.
	if (fx.weapon = fx.vehicle)
	{
		if (CheckVehicleAmmo(fx, fx.weapon))
		{
			fx.strategy = fx.ai.ExecuteVehicle;
			fx.ranged = true;
			fx.aim_wait = 20;
			fx.ammo_check = fx.ai.CheckVehicleAmmo;
			return true;
		}
		else
			fx.weapon = nil;
	}
	if (fx.weapon = FindContents(GrenadeLauncher))
	{
		if (HasBombs(fx, fx.weapon))
		{
			fx.strategy = fx.ai.ExecuteRanged;
			fx.projectile_speed = 75;
			fx.aim_wait = 85;
			fx.ammo_check = fx.ai.HasBombs;
			fx.ranged = true;
			return true;
		}
		else
			fx.weapon = nil;
	}
	if (fx.weapon = FindContents(Bow))
	{
		if (HasArrows(fx, fx.weapon))
		{
			fx.strategy = fx.ai.ExecuteRanged;
			fx.projectile_speed = 100;
			fx.aim_wait = 0;
			fx.ammo_check = fx.ai.HasArrows;
			fx.ranged = true;
			return true;
		}
		else
			fx.weapon = nil;
	}
	if (fx.weapon = FindContents(Blunderbuss))
	{
		if (HasAmmo(fx, fx.weapon))
		{
			fx.strategy = fx.ai.ExecuteRanged;
			fx.projectile_speed = 200;
			fx.aim_wait = 85;
			fx.ammo_check = fx.ai.HasAmmo;
			fx.ranged = true;
			fx.ranged_direct = true;
			return true;
		}
		else
			fx.weapon = nil;
	}
	if (fx.weapon = FindContents(Javelin)) 
	{
		fx.strategy = fx.ai.ExecuteRanged;
		fx.projectile_speed = this.ThrowSpeed * 21 / 100;
		fx.aim_wait = 16;
		fx.ranged=true;
		return true;
	}
	// Throwing weapons.
	if ((fx.weapon = FindContents(Firestone)) || (fx.weapon = FindContents(Rock)) || (fx.weapon = FindContents(Lantern))) 
	{
		fx.strategy = fx.ai.ExecuteThrow;
		return true;
	}
	// Melee weapons.
	if (fx.weapon = FindContents(Sword)) 
	{
		fx.strategy = fx.ai.ExecuteMelee;
		return true;
	}
	// No weapon.
	return false;
}

private func ExecuteIdle(effect fx)
{
	if (!Inside(GetX() - fx.home_x, -5, 5) || !Inside(GetY() - fx.home_y, -15, 15))
	{
		return SetCommand("MoveTo", nil, fx.home_x, fx.home_y);
	}
	else
	{
		SetCommand("None");
		SetComDir(COMD_Stop);
		SetDir(fx.home_dir);
	}
	// Nothing to do.
	return false;
}

private func FindTarget(effect fx)
{
	// Consider hostile clonks, or all clonks if the AI does not have an owner.
	var hostile_criteria = Find_Hostile(GetOwner());
	if (GetOwner() == NO_OWNER)
		hostile_criteria = Find_Not(Find_Owner(GetOwner()));
	for (var target in FindObjects(Find_InRect(fx.guard_range.x-GetX(),fx.guard_range.y-GetY(),fx.guard_range.wdt,fx.guard_range.hgt), Find_OCF(OCF_CrewMember), hostile_criteria, Find_NoContainer(), Sort_Random()))
		if (PathFree(GetX(),GetY(),target->GetX(),target->GetY()))
			return target;
	// Nothing found.
	return;
}

private func FindEmergencyTarget(effect fx)
{
	// Consider hostile clonks, or all clonks if the AI does not have an owner.
	var hostile_criteria = Find_Hostile(GetOwner());
	if (GetOwner() == NO_OWNER)
		hostile_criteria = Find_Not(Find_Owner(GetOwner()));
	// Search nearest enemy clonk in area even if not in guard range, used e.g. when outside guard range (AI fell down) and being attacked.
	for (var target in FindObjects(Find_Distance(200), Find_OCF(OCF_CrewMember), hostile_criteria, Find_NoContainer(), Sort_Distance()))
		if (PathFree(GetX(), GetY(), target->GetX(), target->GetY()))
			return target;
	// Nothing found.
	return;
}


/*-- Editor Properties --*/

public func Definition(proplist def)
{
	if (!Clonk.EditorProps) Clonk.EditorProps = {};
	Clonk.EditorProps.AI = { Type = "has_effect", Effect = "AI", Set = "AI->SetAI", SetGlobal = true };
	// Add AI user actions
	var enemy_evaluator = UserAction->GetObjectEvaluator("IsClonk", "$Enemy$", "$EnemyHelp$");
	enemy_evaluator.Priority = 100;
	UserAction->AddEvaluator("Action", "Clonk", "$SetAIActivated$", "$SetAIActivatedHelp$", "ai_set_activated", [def, def.EvalAct_SetActive], { Enemy=nil, AttackTarget = { Function="triggering_clonk" }, Status = { Function="bool_constant", Value=true } }, { Type="proplist", Display="{{Enemy}}: {{Status}} ({{AttackTarget}})", EditorProps = {
		Enemy = enemy_evaluator,
		AttackTarget = UserAction->GetObjectEvaluator("IsClonk", "$AttackTarget$", "$AttackTargetHelp$"),
		Status = new UserAction.Evaluator.Boolean { Name = "$Status$" }
		} } );
	UserAction->AddEvaluator("Action", "Clonk", "$SetAINewHome$", "$SetAINewHomeHelp$", "ai_set_new_home", [def, def.EvalAct_SetNewHome], { Enemy=nil, HomePosition=nil, Status = { Function="bool_constant", Value=true } }, { Type="proplist", Display="{{Enemy}} -> {{NewHome}}", EditorProps = {
		Enemy = enemy_evaluator,
		NewHome = new UserAction.Evaluator.Position { Name = "$NewHome$", EditorHelp = "$NewHomeHelp$" },
		NewHomeDir = { Type="enum", Name="$NewHomeDir$", EditorHelp="$NewHomeDirHelp$", Options = [
			{ Name="$Unchanged$" },
			{ Name="$Left$", Value=DIR_Left },
			{ Name="$Right$", Value=DIR_Right }
			] },
		} } );
}

private func EvalAct_SetActive(proplist props, proplist context)
{
	// User action: Activate enemy AI.
	var enemy = UserAction->EvaluateValue("Object", props.Enemy, context);
	var attack_target = UserAction->EvaluateValue("Object", props.AttackTarget, context);
	var status = UserAction->EvaluateValue("Boolean", props.Status, context);
	if (!enemy)
		return;
	// Ensure enemy AI exists
	var fx = GetAI(enemy);
	if (!fx)
	{
		// Deactivated? Then we don't need an AI effect.
		if (!status)
			return;
		fx = AddAI(enemy);
		if (!fx || !enemy)
			return;
	}
	// Set activation.
	fx->SetActive(status);
	// Set specific target if desired.
	if (attack_target)
		fx.target = attack_target;
}

private func EvalAct_SetNewHome(proplist props, proplist context)
{
	// User action: Set new home.
	var enemy = UserAction->EvaluateValue("Object", props.Enemy, context);
	var new_home = UserAction->EvaluatePosition(props.NewHome, context);
	var new_home_dir = props.NewHomeDir;
	if (!enemy)
		return;
	// Ensure enemy AI exists.
	var fx = GetAI(enemy);
	if (!fx)
	{
		fx = AddAI(enemy);
		if (!fx || !enemy)
			return;
		// Create without attack command.
		SetAutoSearchTarget(enemy, false);
	}
	fx.command = this.ExecuteIdle;
	fx.home_x = new_home[0];
	fx.home_y = new_home[1];
	if (GetType(new_home_dir))
		fx.home_dir = new_home_dir;
}


/*-- Properties --*/

local Plane = 300;
