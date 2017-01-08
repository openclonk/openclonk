/**
	Defense AI
	Controls enemy behaviour in defense scenarios.

	@author Sven2, Clonkonaut, Maikel
*/

#include AI


public func AddAI(object clonk)
{
	// Add normal AI and adapt it.
	var fx = AI->AddAI(clonk);
	if (fx)
	{
		clonk.ExecuteAI = DefenseAI.Execute;
		fx.ai = DefenseAI;
	}
	return fx;
}

private func FindTarget(effect fx)
{
	var target = _inherited(fx, ...);
	// Focus on defense target if normal target is too far away.
	if (!target || ObjectDistance(target, fx.Target) > DefenseAI.AI_AltTargetDistance)
		target = GetRandomAttackTarget(fx.Target);
	return target;
}

private func FindInventoryWeapon(effect fx)
{
	// Alternative behavior when riding the boom attack.
	if (fx.vehicle && fx.vehicle->GetID() == DefenseBoomAttack)
	{
		fx.weapon = FindContents(Bow);
		fx.strategy = fx.ai.ExecuteRanged;
		fx.projectile_speed = 100;
		fx.aim_wait = 0;
		fx.ammo_check = fx.ai.HasArrows;
		fx.ranged = true;
		return true; 
	}
	return _inherited(fx, ...);
}


/*-- Properties --*/

local AI_AltTargetDistance = 400;