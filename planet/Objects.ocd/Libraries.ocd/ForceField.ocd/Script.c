/**
	ForceField
	
	@author Zapper
	
	
	Objects using this library must call _inherited on:
		- Construction
		
	This library provides force fields of two types (attractor/repulsor) that can be placed arbitrarily.
	The force for a X | Y point can then be calculated, taking into account all forces and their specific falloff.
*/


static const Library_ForceField_Attractor = 0;
static const Library_ForceField_Repulsor = 1;

local ForceField;


public func Construction()
{
	ForceField =
	{
		emitters = [], // can be both attractors and repulsors
		default_max_distance = nil,
		inversed = 0,
		max_emitter_count = nil
	};
	return _inherited(...);
}

public func SetDefaultForceFieldMaxDistance(int max_distance)
{
	ForceField.default_max_distance = max_distance;
}

public func SetDefaultForceFieldTTD(int time_to_die)
{
	ForceField.default_time_to_die = time_to_die;
}

public func SetInverseForceFields(bool is_inversed)
{
	if (is_inversed)
		ForceField.inversed = 1;
	else ForceField.inversed = 0;
}

public func SetMaxEmitterNumber(int amount)
{
	ForceField.max_emitter_count = amount;
}

private func Helper_GetTargetX() { return this.target->GetX(); }
private func Helper_GetTargetY() { return this.target->GetY(); }
private func Helper_GetX() { return this.X; }
private func Helper_GetY() { return this.Y; }

public func AddRepulsor(x_or_obj, y, int strength, int max_distance, int time_to_die)
{
	max_distance = max_distance ?? ForceField.default_max_distance;
	time_to_die = time_to_die ?? ForceField.default_time_to_die;
	strength = strength ?? 1000;
	
	var new_emitter = 
	{
		target = 0, // must be different from "nil"
		type = Library_ForceField_Repulsor, 
		max_distance = max_distance,
		time_to_die = time_to_die,
		time_start = FrameCounter(),
		strength = strength
	};
		
	
	if (GetType(x_or_obj) == C4V_C4Object)
	{
		new_emitter.target = x_or_obj;
		new_emitter["GetX"] = Library_ForceField.Helper_GetTargetX;
		new_emitter["GetY"] = Library_ForceField.Helper_GetTargetY;
	}
	else
	{
		new_emitter["X"] = x_or_obj;
		new_emitter["Y"] = y;
		new_emitter["GetX"] = Library_ForceField.Helper_GetX;
		new_emitter["GetY"] = Library_ForceField.Helper_GetY;
	}
	
	// if we have too many emitters, clean up the oldest one
	var emitter_count = 0, len = GetLength(ForceField.emitters);
	var inserted = false, oldest = new_emitter.time_start, oldest_index = 0;
	for (var i = 0; i < len; ++i)
	{
		var insert = false;
		if (ForceField.emitters[i]) 
		{
			++emitter_count;
			if (ForceField.emitters[i].time_start < oldest)
			{
				oldest = ForceField.emitters[i].time_start;
				oldest_index = i;
			}
			
			// overwrite old emitters with the same target
			if (ForceField.emitters[i].target && ForceField.emitters[i].target == new_emitter.target)
				insert = true;
		}
		else
			insert = true;
		
		if (insert && !inserted)
		{
			ForceField.emitters[i] = new_emitter;
			inserted = true;
		}
	}
	if (!inserted)
		PushBack(ForceField.emitters, new_emitter);
	
	// is it necessary to delete one emitter?
	if (emitter_count + 1 > ForceField.max_emitter_count)
	{
		ForceField.emitters[oldest_index] = nil;
	}
	
	return new_emitter;
}

public func AddAttractor(...)
{
	var att = AddRepulsor(...);
	att.type = Library_ForceField_Attractor;
	return att;
}

public func CalculateForce(int x, int y)
{
	if (x == nil)
	{
		x = GetX();
		y = GetY();
	}
	
	var total_x_dir = 0, total_y_dir = 0;
	var total_force = 0;
	var frame_counter = FrameCounter();
	
	for (var i = 0; i < GetLength(ForceField.emitters); ++i)
	{	
		var emitter = ForceField.emitters[i];
		if (!emitter) continue;
		
		// kill old emitters
		var remaining_time = (emitter.time_start + emitter.time_to_die) - frame_counter;
		if (remaining_time <= 0 || emitter.target == nil)
		{
			ForceField.emitters[i] = nil;
			continue;
		}
		
		var type = emitter.type ^ ForceField.inversed; // this is an exclusive OR
		var x_dir = emitter->GetX() - x;
		var y_dir = emitter->GetY() - y;
		var distance = Sqrt(x_dir * x_dir + y_dir * y_dir);
		distance = Max(1, distance);
		// normalize further
		x_dir = 1000 * x_dir / distance;
		y_dir = 1000 * y_dir / distance;
		var normalized_distance = 1000 * distance / emitter.max_distance;
		
		// calculate actual force of the emitter
		var force = 0;
		if (type == Library_ForceField_Repulsor)
			force = -BoundBy(50000 / normalized_distance, 0, 1000);
		else // attractor
		{
			// -((x - 300)^2)/250 + x*2
			normalized_distance = BoundBy(normalized_distance, 0, 1000);
			force = - ((normalized_distance - 300)**2) / 250 + normalized_distance*2;
		}
		
		// now adjust the strength depending on several factors
		force = emitter.strength * force / 1000;
		force = remaining_time * force / emitter.time_to_die;
		emitter.last_force = force;
		
		total_x_dir += x_dir * force;
		total_y_dir += y_dir * force;
		total_force += Abs(force);
	}
	var divisor = total_force;
	if (!total_force) divisor = 1;
	
	return [total_x_dir / divisor, total_y_dir / divisor, total_force];
}

public func DebugShowForceField(int amount)
{
	if (amount == nil) amount = 200;
	
	for (var emitter in ForceField.emitters)
	{
		 if (!emitter) continue;
		 var force = Abs(emitter.last_force);
		 var speed = force / 60;
		 if (emitter.type == Library_ForceField_Repulsor)
		 {
		 	CreateParticle("SphereSpark", emitter->GetX() - GetX() + RandomX(-2, 2), emitter->GetY() - GetY() + RandomX(-2, 2), PV_Random(-speed, speed), PV_Random(-speed, speed), 60, {Prototype = Particles_Flash(), Size = PV_Linear(5, 0), R = 255, G = 0, B = 0}, force / 20);
		 }
		 else
		 	CreateParticle("SphereSpark", emitter->GetX() - GetX() + RandomX(-2, 2), emitter->GetY() - GetY() + RandomX(-2, 2), PV_Random(-speed, speed), PV_Random(-speed, speed), 60, {Prototype = Particles_Flash(), Size = PV_Linear(5, 0), R = 0, G = 0, B = 255}, force / 20);
	}
	
	if (amount > 0)
		ScheduleCall(this, "DebugShowForceField", 5, 1, amount - 1);
}