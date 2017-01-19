/**
	AI Vehicles
	Functionality that helps the AI use vehicles.
	
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
		return ExecuteCatapult(fx);

	// Don't know how to use this vehicle, so reset it.
	fx.vehicle = nil;
	return false;
}

private func CheckVehicleAmmo(effect fx, object vehicle)
{
	// Check for specific vehicle.
	if (vehicle->GetID() == Catapult)
	{
		if (CheckCatapultAmmo(fx, vehicle))
			return true;		
	}
	// Vehicle out of ammo: Can't really be refilled. Stop using that weapon.
	fx.vehicle = nil;
	this->ObjectCommand("UnGrab");
	return false;
}


/*-- Catapult --*/

private func ExecuteCatapult(effect fx)
{
	// Still pushing it?
	if (GetProcedure() != "PUSH" || GetActionTarget() != fx.vehicle)
	{
		if (!GetCommand() || !Random(4))
			SetCommand("Grab", fx.vehicle);
		return true;
	}
	// Target still in guard range?
	if (!fx.ai->CheckTargetInGuardRange(fx))
		return false;
	// Turn in correct direction.
	var x = GetX(), y = GetY(), tx = fx.target->GetX(), ty = fx.target->GetY() - 4;
	if (tx > x && !fx.vehicle.dir)
	{
		fx.vehicle->ControlRight(this);
		return true;
	}
	if (tx < x &&  fx.vehicle.dir)
	{
		fx.vehicle->ControlLeft(this);
		return true;
	}
	// Make sure we're aiming.
	if (!fx.aim_weapon)
	{
		if (!fx.vehicle->~ControlUseStart(this))
			return false;
		fx.aim_weapon = fx.vehicle;
		fx.aim_time = fx.time;
		return true;
	}
	// Update catapult animation.
	fx.vehicle->~ControlUseHolding(this, tx - x, ty - y);
	// Determine power needed to hit target.
	var dx = tx - x, dy = ty - y + 20;
	var power = Sqrt((GetGravity() * dx * dx) / Max(Abs(dx) + dy, 1));
	var dt = dx * 10 / power;
	tx += fx.ai->GetTargetXDir(fx.target, dt);
	ty += fx.ai->GetTargetYDir(fx.target, dt);
	if (!fx.target->GetContact(-1))
		dy += GetGravity() * dt * dt / 200;
	power = Sqrt((GetGravity() * dx * dx) / Max(Abs(dx) + dy, 1));
	power = power + Random(11) - 5;
	 // Limits imposed by catapult.
	fx.projectile_speed = power = BoundBy(power, 20, 100);
	// Can shoot now?
	if (fx.time >= fx.aim_time + fx.aim_wait && PathFree(x, y - 20, x + dx, y + dy - 20))
	{
		fx.aim_weapon->~DoFire(this, power, 0);
		fx.aim_weapon = nil;
	}
	return true;
}

private func CheckCatapultAmmo(effect fx, object vehicle)
{
	return vehicle->ContentsCount() > 0;
}
