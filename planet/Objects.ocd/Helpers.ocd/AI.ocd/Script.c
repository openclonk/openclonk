/**
	AI
	Controls enemy NPC behaviour. Different parts of the AI are in the different include
	files below. This AI can be overloaded by making a new AI object and including this
	one and then add the needed changes. The relevant settings are all local constants 
	which can be directly changed in the new AI object, or functions can be overloaded.
	
	Optionally, a custom AI can also be implemented by including AI_Controller and
	all desired AI components.

	@author Sven2, Maikel, Marky
*/

// Include the basic functionality
#include AI_Controller
// Include the different parts of the AI.
#include AI_Appearance
#include AI_Debugging
#include AI_HelperFunctions
#include AI_MeleeWeapons
#include AI_Movement
#include AI_Protection
#include AI_RangedWeapons
#include AI_TargetFinding
#include AI_Vehicles
#include AI_AttackModes
#include AI_HelperClonk
#include AI_HomePosition
#include AI_AttackEnemy
#include AI_Inventory

/*-- Callbacks --*/

// Timer interval for the effect
public func GetTimerInterval() { return 3; }


/*-- Editor Properties --*/

// Callback from the Definition()-call
public func OnDefineAI(proplist def)
{
	_inherited(def);
	
	// Can be added to Clonk
	AddEditorProp_AISelection(Clonk, AI);
}

local EditorHelp = "$EditorHelp$";


/*-- AI Execution --*/

public func Execute(effect fx, int time)
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
		fx.can_attack_structures = false;
		this->CancelAiming(fx);
		if (!this->ExecuteArm(fx))
			return this->ExecuteIdle(fx);
		else if (!fx.weapon)
			return true;
	}
	// Weapon out of ammo?
	if (fx.ammo_check && !this->Call(fx.ammo_check, fx, fx.weapon))
	{
		this->LogAI_Warning(fx, Format("Weapon %v is out of ammo, AI won't do anything.", fx.weapon));
		fx.weapon = nil;
		return false;
	}
	// Find an enemy.
	if (fx.target) 
		if ((fx.target->GetCategory() & C4D_Living && !fx.target->GetAlive()) || (!fx.ranged && fx.Target->ObjectDistance(fx.target) >= fx.max_aggro_distance))
		{
			this->LogAI_Info(fx, Format("Forgetting target %v, because it is dead or out of range", fx.target));
			fx.target = nil;
		}
	if (!fx.target)
	{
		this->CancelAiming(fx);
		if (!fx.auto_search_target || !(fx.target = this->FindTarget(fx)))
		{
			this->LogAI_Warning(fx, Format("Will call ExecuteIdle, because there is no target. Auto-searching for target %v", fx.auto_search_target));
			return ExecuteIdle(fx);
		}
		// First encounter callback. might display a message.
		if (fx.encounter_cb)
			if (GameCall(fx.encounter_cb, fx.Target, fx.target))
				fx.encounter_cb = nil;
		// Wake up nearby allies.
		if (fx.ally_alert_range)
		{
			for (var ally in fx.Target->FindObjects(Find_Distance(fx.ally_alert_range), Find_Exclude(fx.Target), Find_OCF(OCF_CrewMember), Find_Owner(fx.Target->GetOwner())))
			{
				var ally_fx = this->GetAI(ally);
				if (ally_fx && !ally_fx.target)
				{
					ally_fx.target = fx.target;
					ally_fx.alert = ally_fx.time;
					if (ally_fx.encounter_cb) 
						if (GameCall(ally_fx.encounter_cb, ally, fx.target))
							ally_fx.encounter_cb = nil;
				}
			}
			// Do some messages.
			this->ExecuteIntruderMessage(fx);		
			// Waking up works only once. after that, AI might have moved and wake up clonks it shouldn't.
			fx.ally_alert_range = nil;
		}
	}
	// Do stuff on the appearance of the enemy like displaying a message.
	this->ExecuteAppearance(fx);
	// Attack it!
	if (!this->IsWeaponForTarget(fx))
		this->LogAI_Warning(fx, Format("Weapon of type %i is not fit to attack %v (type: %i).", fx.weapon->GetID(), fx.target, fx.target->GetID()));

	this->LogAI_Info(fx, Format("Calling strategy: %v", fx.strategy));
	return this->Call(fx.strategy, fx);
}


public func ExecuteThrow(effect fx)
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


public func ExecuteArm(effect fx)
{
	
	// Find shield.
	fx.shield = fx.Target->FindContents(Shield);
	// Vehicle control overrides all other weapons
	if ((fx.weapon = fx.vehicle) != nil)
	{
		if (this->CheckVehicleAmmo(fx, fx.weapon))
		{
			this->LogAI_Info(fx, "Vehicle ammo is ok");
			fx.strategy = this.ExecuteVehicle;
			fx.ranged = true;
			fx.aim_wait = 20;
			fx.ammo_check = this.CheckVehicleAmmo;
			return true;
		}
		else
		{
			this->LogAI_Info(fx, Format("Vehicle ammo is not ok. Weapon is %v, vehicle is %v", fx.weapon, fx.vehicle));
			fx.weapon = nil;
		}
	}
	// Find a weapon. Depends on attack mode
	if (Call(fx.attack_mode.FindWeapon, fx))
	{
		// Select unless it's e.g. a vehicle or a spell
		SelectItem(fx, fx.weapon);
		return true;
	}
	// No weapon.
	return false;
}
