/**
	Defense AI
	Controls enemy behaviour in defense scenarios. Modified version of normal AI.

	@author Sven2, Clonkonaut, Maikel
*/

#include AI


// AI Settings.
local AltTargetDistance = 400; // Use the scenario given target if normal AI target is further away than this distance.


// Special settings for the defense AI.
public func AddAI(object clonk, id type)
{
	var fx_ai = inherited(clonk, type, ...);
	// Make AI aggressive.
	fx_ai.is_aggressive = true;
	return fx_ai;
}

// Alternative target finding for defense scenarios: partially controlled by the scenario script.
public func FindTarget(effect fx)
{
	var target = _inherited(fx, ...);
	// Focus on defense target if normal target is too far away.
	if (!target || ObjectDistance(target, fx.Target) > fx->GetControl().AltTargetDistance)
	{
		if (fx.is_siege)
			target = GetRandomSiegeTarget(fx.Target);
		else		
			target = GetRandomAttackTarget(fx.Target);
	}
	// If target can't be attacked just take normal target again.
	if (!this->HasWeaponForTarget(fx, target))
		target = _inherited(fx, ...);
	this->LogAI_Info(fx, Format("Defense AI %v found %v as a target to attack.", fx.Target, target));
	this->LogAI_CallStack(fx);
	return target;
}

// Move to a target if idle.
public func ExecuteIdle(effect fx)
{
	if (!fx.target)
		fx.target = this->FindEmergencyTarget(fx);
	if (!Random(5) && fx.target)
	{
		var tx = fx.target->GetX();
		if (Abs(tx - fx.Target->GetX())>30)
		{
			fx.Target->SetCommand("MoveTo", nil, BoundBy(fx.Target->GetX(), tx - 30, tx + 30), fx.target->GetY());
			return true;
		}
	}
	return true;
}

public func FindInventoryWeapon(effect fx)
{
	// Alternative behavior when riding the boom attack.
	if (fx.vehicle && fx.vehicle->GetID() == DefenseBoomAttack)
	{
		fx.weapon = FindContents(Bow);
		fx.strategy = this.ExecuteRanged;
		fx.projectile_speed = 100;
		fx.aim_wait = 0;
		fx.ammo_check = this.HasArrows;
		fx.ranged = true;
		return true; 
	}
	// Make a bomber out of those who carry the powderkeg.
	if (fx.weapon = fx.Target->FindContents(PowderKeg)) 
	{
		fx.is_siege = true;
		fx.strategy = this.ExecuteBomber;
		return true;
	}
	return _inherited(fx, ...);
}


/*-- Bomber --*/

public func ExecuteBomber(effect fx)
{
	// Still carrying the bomb?
	if (fx.weapon->Contained() != fx.Target)
	{
		this->LogAI_Info(fx, Format("ExecuteBomber %v lost its bomb.", fx.Target));
		fx.weapon = nil;
		return false;
	}
	// Find a different target at random times, because the current target could not be the optimal any more.
	if (!Random(40) && !PathFree(fx.Target->GetX(), fx.Target->GetY(), fx.target->GetX(), fx.target->GetY()))
	{
		fx.target = this->FindTarget(fx);
		return true;
	}
	// Are we in range?
	if (fx.Target->ObjectDistance(fx.target) < 16 || Distance(fx.Target->GetX(), fx.Target->GetY(), fx.target->GetX(), fx.target->GetY() + fx.target->GetBottom()) < 16)
	{
		// Suicide!
		fx.weapon->Explode(fx.weapon->GetExplosionStrength());
		fx.Target->Kill();
	}
	// Are we stuck?
	else if (fx.Target->Stuck())
	{
		// Also perform suicide, because we can safely assume the player somehow trapped this AI.
		fx.weapon->Explode(fx.weapon->GetExplosionStrength());
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

