/**
	Defense AI
	Controls enemy behaviour in defense scenarios. Modified version of normal AI.

	@author Sven2, Clonkonaut, Maikel
*/

#include AI


// AI Settings.
local AltTargetDistance = 400; // Use the scenario given target if normal AI target is further away than this distance.


private func FindTarget(effect fx)
{
	var target = _inherited(fx, ...);
	// Focus on defense target if normal target is too far away.
	if (!target || ObjectDistance(target, fx.Target) > fx.control.AltTargetDistance)
		target = GetRandomAttackTarget(fx.Target);
	// If target can't be attacked just take normal target again.
	if (!this->HasWeaponForTarget(fx, target))
		target = _inherited(fx, ...);
	return target;
}

private func FindInventoryWeapon(effect fx)
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
	return _inherited(fx, ...);
}
