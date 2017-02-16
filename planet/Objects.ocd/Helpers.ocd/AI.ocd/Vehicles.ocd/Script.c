/**
	AI Vehicles
	Functionality that helps the AI use vehicles. Handles:
	 * Catapult
	
	@author Sven2, Clonkonaut, Maikel
*/


// AI Settings.
local AirshipBoardDistance = 100; // How near must an airship be to the target to dispatch its troops.
local AirshipLostDistance = 50; // How far the pilot must be away from an airship for it to find a new pilot.


/*-- General Vehicle --*/

private func ExecuteVehicle(effect fx)
{
	// Do we have a vehicle?
	if (!fx.vehicle)
		return false;
	
	// Use the catapult.
	if (fx.vehicle->GetID() == Catapult)
		return this->ExecuteCatapult(fx);

	// Steer the airship.
	if (fx.vehicle->GetID() == Airship)
		return this->ExecuteAirship(fx);
		
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
	// These vehicles don't need ammo.
	if (vehicle->GetID() == Airship)
		return true;
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


/*-- Airship --*/

public func ExecuteAirship(effect fx)
{
	// Still steering the airship?
	if (fx.Target->GetProcedure() != "PUSH" || fx.Target->GetActionTarget() != fx.vehicle)
	{
		// Try to find a new pilot if the current pilot lost the airship.
		if (fx.Target->ObjectDistance(fx.vehicle) > fx.control.AirshipLostDistance)
		{
			this->PromoteNewAirshipCaptain(fx);
			fx.strategy = nil;
			fx.weapon = nil;
			fx.vehicle = nil;
			return true;
		}
		if (!fx.Target->GetCommand())
			fx.Target->SetCommand("Grab", fx.vehicle);
		return true;
	}
	
	// Move the airship to the target. Check if no command or is making contact, this means a new control needs to be issued.
	if (!fx.vehicle->GetCommand() || fx.vehicle->GetContact(-1))
	{
		// If close enough (also in y-coordinates, must be above target) release the crew.
		if (fx.vehicle->ObjectDistance(fx.target) < fx.control.AirshipBoardDistance && Inside(fx.vehicle->GetY() - fx.target->GetY(), -fx.control.AirshipBoardDistance / 2, 0))
		{
			// Unboard the crew and let go of airship.
			for (var clonk in this->GetCommanderCrew(fx))
			{
				var clonk_ai = clonk->GetAI();
				clonk_ai.commander = nil;
				if (clonk->GetProcedure() == "PUSH")
					clonk->SetAction("Walk");
				clonk_ai.target = fx.target;
			}
			fx.vehicle = nil;
			fx.weapon = nil;
			fx.Target->SetCommand("UnGrab");
			return true;
		}		
		// Find a boarding point for the target.
		var boarding_point = this->GetAirshipBoardingPoint(fx);
		if (boarding_point)
		{
			fx.vehicle->SetCommand("MoveTo", nil, boarding_point[0], boarding_point[1]);
			return true;	
		}
	}
	return false;
}

// Finds a location where to board the airship close to the target.
public func GetAirshipBoardingPoint(effect fx)
{
	if (!fx.target || !fx.vehicle)
		return nil;
	var vx = fx.vehicle->GetX();
	var vy = fx.vehicle->GetY();
	var tx = fx.target->GetX();
	var ty = fx.target->GetY();
	// Look for a new target if the current path is not free and too far away.
	if (fx.vehicle->ObjectDistance(fx.target) > 250 && !PathFree(vx - 30, vy, tx, ty) && !PathFree(vx + 30, vy, tx, ty))
	{
		fx.target = this->FindTarget(fx);
		if (!fx.target)
			return nil;
		tx = fx.target->GetX();
		ty = fx.target->GetY();
	}
	// Approach from above or the side if possible, so move airship up if below target.
	if (vy > ty)
	{
		var gx = vx + RandomX(-10, 10);
		var gy = vy - RandomX(80, 100);
		// Move to the side first if exactly below target.
		if (Abs(vx - tx) <= 150)
		{
			gx = (2 * Random(2) - 1) * RandomX(80, 100);
			gy = vy + RandomX(-10, 10);
		}
		// Check if path is free to new coordinates (blocking by solid mask is not possible).
		if (PathFree(vx, vy, gx, gy))
			return [gx, gy];
	}
	// Go for a straight line to left/right of the target, roughly in steps of 100px.
	for (var attempts = 0; attempts < 10; attempts++)
	{
		var txr = tx + Sign(vx - tx) * RandomX(40, 60);
		var tyr = ty - 10;
		var d = Distance(vx, vy, txr, tyr);
		var gx = vx + Min(RandomX(80, 100), d) * (txr - vx) / d;
		var gy = vy + Min(RandomX(80, 100), d) * (tyr - vy) / d;
		if (PathFree(vx - 30, vy, gx, gy) || PathFree(vx + 30, vy, gx, gy))
			return [gx, gy];
	}
	return nil;
}

public func PromoteNewAirshipCaptain(effect fx)
{
	var crew_members = fx.vehicle->GetCrewMembers();
	if (!GetLength(crew_members))
		return false;
	var new_pilot = RandomElement(crew_members);
	var fx_ai = new_pilot->~GetAI();
	if (!fx_ai)
		return false;
	// Make this crew the new pilot.
	fx_ai.commander = nil;
	fx_ai.weapon = fx.vehicle;
	fx_ai.vehicle = fx.vehicle;
	fx_ai.strategy = this.ExecuteVehicle;
	// Set new commander for remaining crew members.
	for (var crew in crew_members)
		if (crew != new_pilot)
			if (crew->~GetAI())
				crew->~GetAI().commander = new_pilot;
	return true;
}

// Find all crew members with this AI as their commander.
public func GetCommanderCrew(effect fx)
{
	var crew = [];
	for (var clonk in fx.Target->FindObjects(Find_OCF(OCF_CrewMember), Find_Owner(fx.Target->GetOwner())))
		if (clonk->~GetAI() && clonk->GetAI().commander == fx.Target)
			PushBack(crew, clonk);
	return crew;
}
