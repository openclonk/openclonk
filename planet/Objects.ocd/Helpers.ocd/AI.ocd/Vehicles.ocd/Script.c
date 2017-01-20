/**
	AI Vehicles
	Functionality that helps the AI use vehicles. Handles:
	 * Catapult
	
	@author Sven2, Maikel
*/


/*-- General Vehicle --*/

private func ExecuteVehicle(effect fx)
{
	// Do we have a vehicle?
	if (!fx.vehicle)
		return false;
	
	// Use the catapult.
	if (fx.vehicle->GetID() == Catapult)
		return this->ExecuteCatapult(fx);

	// Don't know how to use this vehicle, so reset it.
	fx.vehicle = nil;
	return false;
}

private func CheckVehicleAmmo(effect fx, object vehicle)
{
	// Check for specific vehicle.
	if (vehicle->GetID() == Catapult)
	{
		if (this->CheckCatapultAmmo(fx, vehicle))
			return true;		
	}
	// Vehicle out of ammo: Can't really be refilled. Stop using that weapon.
	fx.vehicle = nil;
	fx.Target->ObjectCommand("UnGrab");
	return false;
}


/*-- Catapult --*/

private func ExecuteCatapult(effect fx)
{
	// Still pushing it?
	if (fx.Target->GetProcedure() != "PUSH" || fx.Target->GetActionTarget() != fx.vehicle)
	{
		if (!fx.Target->GetCommand() || !Random(4))
			fx.Target->SetCommand("Grab", fx.vehicle);
		return true;
	}
	// Target still in guard range?
	if (!this->CheckTargetInGuardRange(fx))
		return false;
	// Turn in correct direction.
	var x = fx.Target->GetX(), y = fx.Target->GetY(), tx = fx.target->GetX(), ty = fx.target->GetY() - 4;
	if (tx > x && !fx.vehicle.dir)
	{
		fx.vehicle->ControlRight(fx.Target);
		return true;
	}
	if (tx < x &&  fx.vehicle.dir)
	{
		fx.vehicle->ControlLeft(fx.Target);
		return true;
	}
	// Make sure we're aiming.
	if (!fx.aim_weapon)
	{
		if (!fx.vehicle->~ControlUseStart(fx.Target))
			return false;
		fx.aim_weapon = fx.vehicle;
		fx.aim_time = fx.time;
		return true;
	}
	// Update catapult animation.
	fx.vehicle->~ControlUseHolding(fx.Target, tx - x, ty - y);
	// Determine power needed to hit target.
	var dx = tx - x, dy = ty - y + 20;
	var power = Sqrt((GetGravity() * dx * dx) / Max(Abs(dx) + dy, 1));
	var dt = dx * 10 / power;
	tx += this->GetTargetXDir(fx.target, dt);
	ty += this->GetTargetYDir(fx.target, dt);
	if (!fx.target->GetContact(-1))
		dy += GetGravity() * dt * dt / 200;
	power = Sqrt((GetGravity() * dx * dx) / Max(Abs(dx) + dy, 1));
	power = power + Random(11) - 5;
	 // Limits imposed by catapult.
	fx.projectile_speed = power = BoundBy(power, 20, 100);
	// Can shoot now?
	if (fx.time >= fx.aim_time + fx.aim_wait && PathFree(x, y - 20, x + dx, y + dy - 20))
	{
		fx.aim_weapon->~DoFire(fx.Target, power, 0);
		fx.aim_weapon = nil;
	}
	return true;
}

private func CheckCatapultAmmo(effect fx, object vehicle)
{
	return vehicle->ContentsCount() > 0;
}
