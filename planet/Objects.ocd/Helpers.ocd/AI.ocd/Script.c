/**
	AI
	Controls Enemy NPC behaviour.

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
	var ai = GetAI(clonk);
	if (has_ai)
	{
		// Only add if it doesn't have the effect yet
		if (!ai)
			ai = AddAI(clonk);
		return ai;
	}
	else
	{
		if (ai)
			ai->Remove();
	}
}

// Add AI execution timer to target Clonk
public func AddAI(object clonk)
{
	var fx = clonk->~GetAI();
	if (!fx)
		fx = clonk->CreateEffect(FxAI, 1, 3, this);
	if (!fx || !clonk)
		return nil;
	clonk.ai = fx;
	// Add AI default settings.	
	BindInventory(clonk);
	SetHome(clonk);
	SetGuardRange(clonk, fx.home_x - AI_DefGuardRangeX, fx.home_y - AI_DefGuardRangeY, AI_DefGuardRangeX * 2, AI_DefGuardRangeY * 2);
	SetMaxAggroDistance(clonk, AI_DefMaxAggroDistance);
	SetAutoSearchTarget(clonk, true);
	return fx;
}

public func GetAI(object clonk) { return clonk->~GetAI(); }

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


/*-- AI Effect --*/

local FxAI = new Effect
{
	Construction = func(id control_def)
	{
		// Execute AI every 3 frames.
		this.Interval = 3;
		// Store the definition that controls this AI.
		this.control = control_def;
		// Init editor properties.
		this->InitEditorProps();
		// Store the vehicle the AI is using.
		if (this.Target->GetProcedure() == "PUSH")
			this.vehicle = this.Target->GetActionTarget();
		// Give the AI a helper function to get the AI control effect.
		this.Target.GetAI = this.GetAI;
		return FX_OK;
	},
	
	GetAI = func()
	{
		return this;
	},

	Timer = func(int time)
	{
		// Execute the AI in the clonk.
		this.control->Execute(this, time);
		return FX_OK;
	},

	Destruction = func(int reason)
	{
		// Remove weapons on death.
		if (reason == FX_Call_RemoveDeath)
		{
			if (this.bound_weapons)
				for (var obj in this.bound_weapons)
					if (obj && obj->Contained() == Target)
						obj->RemoveObject();
		}
		// Remove AI reference.
		if (Target && Target.ai == this)
			Target.ai = nil;
		return FX_OK;	
	},
	
	Damage = func(int dmg, int cause)
	{
		// AI takes damage: Make sure we're alert so evasion and healing scripts are executed!
		// It might also be feasible to execute encounter callbacks here (in case an AI is shot from a position it cannot see).
		// However, the attacking clonk is not known and the callback might be triggered e.g. by an unfortunate meteorite or lightning blast.
		// So let's just keep it at alert state for now.
		if (dmg < 0) 
			this.alert = this.time;
		return dmg;
	},
	
	InitEditorProps = func()
	{
		// Editor properties for the AI.
		this.EditorProps = {};
		this.EditorProps.guard_range = { Name = "$GuardRange$", Type = "rect", Storage = "proplist", Color = 0xff00, Relative = false };
		this.EditorProps.ignore_allies = { Name = "$IgnoreAllies$", Type = "bool" };
		this.EditorProps.max_aggro_distance = { Name = "$MaxAggroDistance$", Type = "circle", Color = 0x808080 };
		this.EditorProps.active = { Name = "$Active$", EditorHelp = "$ActiveHelp$", Type = "bool", Priority = 50, AsyncGet = "GetActive", Set = "SetActive" };
		this.EditorProps.auto_search_target = { Name = "$AutoSearchTarget$", EditorHelp = "$AutoSearchTargetHelp$", Type = "bool" };
	},
	SetActive = func(bool active)
	{
		this.Interval = 3 * active;	
	},
	GetActive = func()
	{
		return this.Interval != 0;	
	},
	
	// Save this effect and the AI for scenarios.
	SaveScen = func(proplist props)
	{
		if (!this.Target)
			return false;
		props->AddCall("AI", AI, "AddAI", this.Target);
		if (!this.Interval)
			props->AddCall("AI", AI, "SetActive", this.Target, false);
		if (this.home_x != this.Target->GetX() || this.home_y != this.Target->GetY() || this.home_dir != this.Target->GetDir())
			props->AddCall("AI", AI, "SetHome", this.Target, this.home_x, this.home_y, GetConstantNameByValueSafe(this.home_dir, "DIR_"));
		props->AddCall("AI", AI, "SetGuardRange", this.Target, this.guard_range.x, this.guard_range.y, this.guard_range.wdt, this.guard_range.hgt);
		if (this.max_aggro_distance != AI_DefMaxAggroDistance)
			props->AddCall("AI", AI, "SetMaxAggroDistance", this.Target, this.max_aggro_distance);
		if (this.ally_alert_range)
			props->AddCall("AI", AI, "SetAllyAlertRange", this.Target, this.ally_alert_range);
		if (!this.auto_search_target)
			props->AddCall("AI", AI, "SetAutoSearchTarget", this.Target, false);
		if (this.encounter_cb)
			props->AddCall("AI", AI, "SetEncounterCB", this.Target, Format("%v", this.encounter_cb));
		return true;
	}
};


/*-- AI Execution --*/

private func Execute(effect fx, int time)
{
	fx.time = time;
	// Evasion, healing etc. if alert.
	if (fx.alert)
		if (this->ExecuteProtection(fx))
			return true;
	// Current command override.
	if (fx.command)
	{
		if (this->Call(fx.command, fx))
			return true;
		else
			fx.command = nil;
	}
	// Find something to fight with.
	if (!fx.weapon) 
	{
		this->CancelAiming(fx);
		if (!this->ExecuteArm(fx))
			return this->ExecuteIdle(fx);
		else if (!fx.weapon)
			return true;
	}
	// Weapon out of ammo?
	if (fx.ammo_check && !this->Call(fx.ammo_check, fx, fx.weapon))
	{
		fx.weapon = nil;
		return false;
	}
	// Find an enemy.
	if (fx.target) 
		if ((fx.target->GetCategory() & C4D_Living && !fx.target->GetAlive()) || (!fx.ranged && fx.Target->ObjectDistance(fx.target) >= fx.max_aggro_distance))
			fx.target = nil;
	if (!fx.target)
	{
		this->CancelAiming(fx);
		if (!fx.auto_search_target || !(fx.target = this->FindTarget(fx)))
			return ExecuteIdle(fx);
		// First encounter callback. might display a message.
		if (fx.encounter_cb)
			if (GameCall(fx.encounter_cb, fx.Target, fx.target))
				fx.encounter_cb = nil;
		// Wake up nearby allies.
		if (fx.ally_alert_range)
		{
			var ally_fx;
			for (var ally in fx.Target->FindObjects(Find_Distance(fx.ally_alert_range), Find_Exclude(fx.Target), Find_OCF(OCF_CrewMember), Find_Owner(fx.Target->GetOwner())))
				if (ally_fx = this->GetAI(ally))
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
	return this->Call(fx.strategy, fx);
}

// Selects an item the clonk is about to use.
private func SelectItem(effect fx, object item)
{
	if (!item)
		return;
	if (item->Contained() != fx.Target)
		return;
	fx.Target->SetHandItemPos(0, fx.Target->GetItemPos(item));
}

private func ExecuteProtection(effect fx)
{
	// Search for nearby projectiles. Ranged AI also searches for enemy clonks to evade.
	var enemy_search;
	if (fx.ranged)
		enemy_search = Find_And(Find_OCF(OCF_CrewMember), Find_Not(Find_Owner(fx.Target->GetOwner())));
	var projectiles = fx.Target->FindObjects(Find_InRect(-150, -50, 300, 80), Find_Or(Find_Category(C4D_Object), Find_Func("IsDangerous4AI"), Find_Func("IsArrow"), enemy_search), Find_OCF(OCF_HitSpeed2), Find_NoContainer(), Sort_Distance());
	for (var obj in projectiles)
	{
		var dx = obj->GetX() - fx.Target->GetX(), dy = obj->GetY() - fx.Target->GetY();
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
		if (l < 0 && Sqrt(d2 - l * l / v2) <= fx.Target->GetCon() / 8)
		{
			// Not if there's a wall between.
			if (!PathFree(fx.Target->GetX(), fx.Target->GetY(), obj->GetX(), obj->GetY()))
				continue;
			// This might hit.
			fx.alert=fx.time;
			// Use a shield if the object is not explosive.
			if (fx.shield && !obj->~HasExplosionOnImpact())
			{
				// Use it!
				SelectItem(fx, fx.shield);
				if (fx.aim_weapon == fx.shield)
				{
					// Continue to hold shield.
					fx.shield->ControlUseHolding(fx.Target, dx,dy);
				}
				else
				{
					// Start holding shield.
					if (fx.aim_weapon)
						this->CancelAiming(fx);
					if (!this->CheckHandsAction(fx))
						return true;
					if (!fx.shield->ControlUseStart(fx.Target, dx, dy))
						return false; // Something's broken :(
					fx.shield->ControlUseHolding(fx.Target, dx, dy);
					fx.aim_weapon = fx.shield;
				}
				return true;
			}
			// No shield. try to jump away.
			if (dx < 0)
				fx.Target->SetComDir(COMD_Right);
			else
				fx.Target->SetComDir(COMD_Left);
			if (this->ExecuteJump(fx))
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
		fx.aim_weapon->~ControlUseCancel(fx.Target);
		fx.aim_weapon = nil;
	}
	else
	{
		// Also cancel aiming done outside AI control.
		fx.Target->~CancelAiming();
	}
	return true;
}

private func ExecuteLookAtTarget(effect fx)
{
	// Set direction to look at target, we can assume this is instantanuous.
	if (fx.target->GetX() > fx.Target->GetX())
		fx.Target->SetDir(DIR_Right);
	else
		fx.Target->SetDir(DIR_Left);
	return true;
}

private func ExecuteThrow(effect fx)
{
	// Still carrying the weapon to throw?
	if (fx.weapon->Contained() != fx.Target)
	{
		fx.weapon = nil;
		return false;
	}
	// Path to target free?
	var x = fx.Target->GetX(), y = fx.Target->GetY(), tx = fx.target->GetX(), ty = fx.target->GetY();
	if (PathFree(x, y, tx, ty))
	{
		var throw_speed = fx.Target.ThrowSpeed;
		var rx = (throw_speed * throw_speed) / (100 * GetGravity()); // horizontal range for 45 degree throw if enemy is on same height as we are
		var ry = throw_speed * 7 / (GetGravity() * 10); // vertical range of 45 degree throw
		var dx = tx - x, dy = ty - y + 15 * fx.Target->GetCon() / 100; // distance to target. Reduce vertical distance a bit because throwing exit point is not at center
		// Check range
		// Could calculate the optimal parabulum here, but that's actually not very reliable on moving targets
		// It's usually better to throw straight at the target and only throw upwards a bit if the target stands on high ground or is far away
		// Also ignoring speed added by own velocity, etc...
		if (Abs(dx) * ry - Min(dy) * rx <= rx * ry)
		{
			// We're in range. Can throw?
			if (!this->CheckHandsAction(fx))
				return true;
			// OK. Calc throwing direction.
			dy -= dx * dx / rx;
			// And throw!
			fx.Target->SetCommand("None");
			fx.Target->SetComDir(COMD_Stop);
			this->SelectItem(fx, fx.weapon);
			return fx.Target->ControlThrow(fx.weapon, dx, dy);
		}
	}
	// Can't reach target yet. Walk towards it.
	if (!fx.Target->GetCommand() || !Random(3))
		fx.Target->SetCommand("MoveTo", fx.target);
	return true;
}

private func CheckHandsAction(effect fx)
{
	// Can use hands?
	if (fx.Target->~HasHandAction())
		return true;
	// Can't throw: Is it because e.g. we're scaling?
	if (!fx.Target->HasActionProcedure())
	{
		this->ExecuteStand(fx);
		return false;
	}
	// Probably hands busy. Just wait.
	return false;
}

private func ExecuteStand(effect fx)
{
	fx.Target->SetCommand("None");
	if (fx.Target->GetProcedure() == "SCALE" || fx.Target->GetAction() == "Climb")
	{
		var tx;
		if (fx.target)
			tx = fx.target->GetX() - fx.Target->GetX();
		// Scale: Either scale up if target is beyond this obstacle or let go if it's not.
		if (fx.Target->GetDir() == DIR_Left)
		{
			if (tx < -20)
				fx.Target->SetComDir(COMD_Left);
			else
				fx.Target->ObjectControlMovement(fx.Target->GetOwner(), CON_Right, 100); // let go
		}
		else
		{
			if (tx > -20)
				fx.Target->SetComDir(COMD_Right);
			else
				fx.Target->ObjectControlMovement(fx.Target->GetOwner(), CON_Left, 100); // let go
		}
	}
	else if (fx.Target->GetProcedure() == "HANGLE")
	{
		fx.Target->ObjectControlMovement(fx.Target->GetOwner(), CON_Down, 100);
	}
	else
	{
		// Hm. What could it be? Let's just hope it resolves itself somehow...
		fx.Target->SetComDir(COMD_Stop);
	}
	return true;
}

private func ExecuteEvade(effect fx, int threat_dx, int threat_dy)
{
	// Evade from threat at position delta threat_dx, threat_dy.
	if (threat_dx < 0)
		fx.Target->SetComDir(COMD_Left);
	else
		fx.Target->SetComDir(COMD_Right);
	if (threat_dy >= -5 && !Random(2))
		if (this->ExecuteJump(fx))
			return true;
	// Shield? Todo.
	return true;
}

private func ExecuteJump(effect fx)
{
	// Jump if standing on floor.
	if (fx.Target->GetProcedure() == "WALK")
	{
		if (fx.Target->~ControlJump())
			return true; // For clonks.
		return fx.Target->Jump(); // For others.
	}
	return false;
}

private func ExecuteArm(effect fx)
{
	// Find shield.
	fx.shield = fx.Target->FindContents(Shield);
	// Find a weapon. For now, just search own inventory.
	if (this->FindInventoryWeapon(fx) && fx.weapon->Contained() == fx.Target)
	{
		SelectItem(fx, fx.weapon);
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
		if (this->CheckVehicleAmmo(fx, fx.weapon))
		{
			fx.strategy = this.ExecuteVehicle;
			fx.ranged = true;
			fx.aim_wait = 20;
			fx.ammo_check = this.CheckVehicleAmmo;
			return true;
		}
		else
			fx.weapon = nil;
	}
	if (fx.weapon = fx.Target->FindContents(GrenadeLauncher))
	{
		if (this->HasBombs(fx, fx.weapon))
		{
			fx.strategy = this.ExecuteRanged;
			fx.projectile_speed = 75;
			fx.aim_wait = 85;
			fx.ammo_check = this.HasBombs;
			fx.ranged = true;
			return true;
		}
		else
			fx.weapon = nil;
	}
	if (fx.weapon = fx.Target->FindContents(Bow))
	{
		if (this->HasArrows(fx, fx.weapon))
		{
			fx.strategy = this.ExecuteRanged;
			fx.projectile_speed = 100;
			fx.aim_wait = 0;
			fx.ammo_check = this.HasArrows;
			fx.ranged = true;
			return true;
		}
		else
			fx.weapon = nil;
	}
	if (fx.weapon = fx.Target->FindContents(Blunderbuss))
	{
		if (this->HasAmmo(fx, fx.weapon))
		{
			fx.strategy = this.ExecuteRanged;
			fx.projectile_speed = 200;
			fx.aim_wait = 85;
			fx.ammo_check = this.HasAmmo;
			fx.ranged = true;
			fx.ranged_direct = true;
			return true;
		}
		else
			fx.weapon = nil;
	}
	if (fx.weapon = fx.Target->FindContents(Javelin)) 
	{
		fx.strategy = this.ExecuteRanged;
		fx.projectile_speed = fx.Target.ThrowSpeed * fx.weapon.shooting_strength / 100;
		fx.aim_wait = 16;
		fx.ranged=true;
		return true;
	}
	// Throwing weapons.
	if ((fx.weapon = fx.Target->FindContents(Firestone)) || (fx.weapon = fx.Target->FindContents(Rock)) || (fx.weapon = fx.Target->FindContents(Lantern))) 
	{
		fx.strategy = this.ExecuteThrow;
		return true;
	}
	// Melee weapons.
	if (fx.weapon = fx.Target->FindContents(Sword)) 
	{
		fx.strategy = this.ExecuteMelee;
		return true;
	}
	// No weapon.
	return false;
}

private func ExecuteIdle(effect fx)
{
	if (!Inside(fx.Target->GetX() - fx.home_x, -5, 5) || !Inside(fx.Target->GetY() - fx.home_y, -15, 15))
	{
		return fx.Target->SetCommand("MoveTo", nil, fx.home_x, fx.home_y);
	}
	else
	{
		fx.Target->SetCommand("None");
		fx.Target->SetComDir(COMD_Stop);
		fx.Target->SetDir(fx.home_dir);
	}
	// Nothing to do.
	return false;
}

private func FindTarget(effect fx)
{
	// Consider hostile clonks, or all clonks if the AI does not have an owner.
	var hostile_criteria = Find_Hostile(fx.Target->GetOwner());
	if (fx.Target->GetOwner() == NO_OWNER)
		hostile_criteria = Find_Not(Find_Owner(fx.Target->GetOwner()));
	for (var target in fx.Target->FindObjects(Find_InRect(fx.guard_range.x - fx.Target->GetX(), fx.guard_range.y - fx.Target->GetY(), fx.guard_range.wdt, fx.guard_range.hgt), Find_OCF(OCF_CrewMember), hostile_criteria, Find_NoContainer(), Sort_Random()))
		if (PathFree(fx.Target->GetX(), fx.Target->GetY(), target->GetX(), target->GetY()))
			return target;
	// Nothing found.
	return;
}

private func FindEmergencyTarget(effect fx)
{
	// Consider hostile clonks, or all clonks if the AI does not have an owner.
	var hostile_criteria = Find_Hostile(fx.Target->GetOwner());
	if (fx.Target->GetOwner() == NO_OWNER)
		hostile_criteria = Find_Not(Find_Owner(fx.Target->GetOwner()));
	// Search nearest enemy clonk in area even if not in guard range, used e.g. when outside guard range (AI fell down) and being attacked.
	for (var target in fx.Target->FindObjects(Find_Distance(200), Find_OCF(OCF_CrewMember), hostile_criteria, Find_NoContainer(), Sort_Distance()))
		if (PathFree(fx.Target->GetX(), fx.Target->GetY(), target->GetX(), target->GetY()))
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
