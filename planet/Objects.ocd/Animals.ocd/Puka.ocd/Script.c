/**
	Puka
	Cute little electronically active creature.
*/

local Name = "$Name$";
local Description = "$Description$";

// For the teleport trajectory.
local current_high_x, current_high_y;
local current_ground_x, current_ground_y;

// Animations.
local turn_angle;

// The closest enemy that has been found.
local enemy;
// Some positions near water that might be good teleport targets.
local water_positions;
local water_cycle;

public func Place(int amount, proplist rectangle, proplist settings)
{
	var max_tries = 3 * amount;
	var loc_area = nil;
	if (rectangle) loc_area = Loc_InArea(rectangle);
	var animal;
	
	while ((amount > 0) && (--max_tries > 0))
	{
		// Try to find walkable ground near water.
		var water_spot = FindLocation(Loc_Material("Water"));
		var ground_spot = nil;
		if (water_spot)
		{
			var water_rectangle = Shape->Rectangle(water_spot.x - 200, water_spot.y - 200, 400, 400);
			// Make sure the position is inside the required target rectangle.
			water_rectangle = Shape->Intersect(water_rectangle, rectangle)->GetBoundingRectangle();
			ground_spot = FindLocation(Loc_Wall(CNAT_Bottom), Loc_Or(Loc_Sky(), Loc_Tunnel()), Loc_Space(20, CNAT_Top), Loc_InArea(water_rectangle));
		}
		
		// If no hip and cool spot found, just get some generic spot.
		if (!ground_spot)
			ground_spot = FindLocation(Loc_Wall(CNAT_Bottom), Loc_Or(Loc_Sky(), Loc_Tunnel()), Loc_Space(20, CNAT_Top), loc_area);
		if (!ground_spot) continue;
		
		animal = CreateObjectAbove(this, ground_spot.x, ground_spot.y, NO_OWNER);
		if (!animal) continue;

		if (animal->Stuck())
		{
			animal->RemoveObject();
			continue;
		}
		--amount;
	}
	return animal;
}

public func IsAnimal() { return true; }

public func Construction()
{
	turn_angle = 0;
	water_positions = [];
	water_cycle = 0;
	
	AddTimer("ExecuteActivity", 10);
	AddTimer("UpdateEnemy", 30);
	AddTimer("CheckTurn", 10);
	SetAction("Walk");
	SetComDir(COMD_Stop);
	if (Random(2)) SetDir(DIR_Left);
	else SetDir(DIR_Right);
	
	// Two spot layouts are available.
	if (!Random(2))
		SetMeshMaterial("Puka2");
	ScheduleCall(this, "RandomizeColor", 10);
	return true;
}

private func RandomizeColor()
{
	// Normally, Pukas are somewhat blue-ish. Rarely, you can find red ones, too!
	var base_color = 170;
	if (!Random(5)) base_color = 0;
	SetColor(HSL(RandomX(base_color - 45, base_color + 45), 100 + Random(150), 50 + Random(50)));
}

public func CatchBlow(int damage, object from)
{
	Schedule(this, "Sound(\"Animals::Puka::Hurt*\")", RandomX(5, 20));
}

private func CheckStuck()
{

}

private func StartSwim()
{
	this.Activity = this.ActivitySwimming;
}

private func ClearActivity()
{
	this.Activity = nil;
}

private func StartSleep()
{
	this.Activity = this.ActivitySleeping;
}

private func StopSwim()
{

}

private func EndSwim()
{

}

private func StartJump()
{

}

private func EndJump()
{
	RemoveEffect("JumpCheck", this);
}

private func StopWalk()
{

}

private func StartScale()
{
	this.Activity = this.ActivityScaling;
}

private func StopScale()
{

}

private func StartHangle()
{
	
}

private func StopHangle()
{

}

public func Death()
{
	AddEffect("IntDeathSparks", this, 1, 1, this);
	SoundAt("Animals::Puka::Die");
}

private func FxIntDeathSparksStart(object target, effect fx, temp)
{
	if (temp) return;
	
	fx.particles = 
	{
		Prototype = Particles_ElectroSpark1(),
		G = 150, B = 150, Alpha = PV_Linear(255, 0),
		Stretch = 2000,
		DampingX = 999, DampingY = 999,
		ForceY = -GetGravity()
	};
}

private func FxIntDeathSparksTimer(object target, effect fx, int time)
{
	CreateParticle("ElectroSpark", PV_Random(-5, 5), PV_Random(-5, 5),
		PV_Random(-20, 20), PV_Random(-5, 10),
		PV_Random(2, 10), fx.particles, 20);
	if (time > this.ActMap.Dead.Length) return -1;
	return 1;
} 

private func FxIntDeathSparksStop(object target, effect fx, int reason, temp)
{
	if (temp) return;
	fx.particles.Stretch *= 2;
	CreateParticle("ElectroSpark", PV_Random(-5, 5), PV_Random(-5, 5),
		PV_Random(-60, 60), PV_Random(-60, 60),
		PV_Random(2, 10), fx.particles, 800);
	if (this) RemoveObject();
	return 1;
} 

private func Jump()
{
	SetAction("Jump");
	SetSpeed(GetXDir(), -10 - GetXDir());
}

private func StartWalk()
{
	// Coming from a swim? Remember this location.
	if (this.Activity == this.ActivitySwimming)
	{
		SetComDir(COMD_Stop);
		
		water_positions[water_cycle] = [GetX(), GetY()];
		water_cycle = (water_cycle + 1) % 5;
	}
	
	this.Activity = this.ActivityWalking;
}

private func ActivityScaling()
{
	if (!Random(10)) return TryStartTeleport();
	
	if (!Random(2))
	{
		if (Random(3)) SetComDir(COMD_Up);
		else SetComDir(COMD_Down);
	}
}

private func ActivitySwimming()
{
	if (!Random(6))
	{
		Turn(nil, true);
	
		if (GetComDir() == COMD_Left) SetComDir(COMD_UpLeft);
		else if (GetComDir() == COMD_Right) SetComDir(COMD_UpRight);
	}
	
	if (!Random(20)) TryStartTeleport();
}

private func ActivityWalking()
{
	var comd = GetComDir();
	
	if (enemy)
	{
		if (enemy->GBackLiquid())
		{
			if (ShockWater()) return;
		}
		
		if (!Random(10)) SetComDir(COMD_Stop);
		var distance = ObjectDistance(this, enemy);
		
		if (distance < 50 && Random(2))
			return TryStartTeleport();
	}
	
	// If not walking, randomly sleep.
	if (comd == COMD_Stop)
	{
		if (!enemy)
		{
			if (!Random(15)) return SetAction("Sleep");
			if (!Random(15)) // Idle?
			{
				if (Random(3))
				{
					Schedule(this, "Sound(\"Animals::Puka::Gulp\")", 40);
					return SetAction("LookAround");
				}
				return SetAction("ScratchFace");
			}
		}
		
		if (!Random(5))
		{
			if (Random(2)) SetComDir(COMD_Left);
			else SetComDir(COMD_Right);
			return;
		}
		if (!Random(20))
			TryStartTeleport();
		return;
	}
	
	if (!Random(5))
	{
		SetComDir(COMD_Stop);
		return;
	}
	
	if (!Random(5))
	{
		Turn();
		return;
	}
	
	// Anticipate holes in the landscape while walking.
	
}

private func ActivitySleeping()
{
	if (!Random(20))
		SetAction("Walk");
	if (!Random(5))
		AddEffect("IntDream", this, 1, 1, this);
}

private func FxIntDreamStart(object target, effect fx, temp)
{
	if (temp) return;
	fx.x = GetX();
	fx.y = GetY() - 5;
	
	fx.particles =
	{
		Prototype = Particles_ElectroSpark2(),
		Size = PV_Linear(PV_Random(0, 1), PV_Random(1, 2)),
		Rotation = PV_Random(360)
	};
}

private func FxIntDreamTimer(object target, effect fx, int time)
{
	if (time > RandomX(30, 200)) return -1;
	fx.x += RandomX(-1, 1);
	fx.y += RandomX(-1, 1);
	CreateParticle("ElectroSpark", fx.x - GetX(), fx.y - GetY(), 0, 0, PV_Random(5, 20), fx.particles, 2);
}

private func ExecuteActivity()
{
	if (this.Activity) this->Activity();
}

private func UpdateEnemy()
{
	// Already disposed of the last one?
	if (enemy && !enemy->GetAlive()) enemy = nil;
	// Last one too far away now?
	if (enemy && ObjectDistance(this, enemy) > 150) enemy = nil;
	
	var x = GetX();
	var y = GetY();
	for (var obj in FindObjects(Find_Distance(100), Find_OCF(OCF_Alive), Find_AnimalHostile(GetOwner()), Sort_Distance()))
	{
		if (!PathFree(x, y, obj->GetX(), obj->GetY())) continue;
		enemy = obj;
		return;
	}
}

private func DoElectroCircle()
{
	for (var angle = 0; angle < 360; angle += 10)
	{
		var speed_x = Cos(angle, 15);
		var speed_y = -Sin(angle, 15);
		CreateParticle("ElectroSpark", PV_Random(-5, 5), PV_Random(-5, 5),
			PV_Random(speed_x - 10, speed_x + 10), PV_Random(speed_y - 10, speed_y + 10),
			PV_Random(5, 10),
			Particles_ElectroSpark2(),
			10);
	}
	
	var particles = 
	{
		Prototype = Particles_ElectroSpark1(),
		Stretch = 6000,
		DampingX = 999, DampingY = 999
	};
	
	// Punish all close enemies (not allied animals, though).
	for (var obj in FindObjects(Find_Distance(30), Find_OCF(OCF_Alive), Find_AnimalHostile(GetOwner()), Find_Exclude(this)))
	{
		var delta_x = 3 * (obj->GetX() - GetX());
		var delta_y = 3 * (obj->GetY() - GetY());
		
		obj->DoEnergy(-15, false, FX_Call_EngGetPunched, GetOwner());
		
		CreateParticle("ElectroSpark", 0, 0, 
			PV_Random(delta_x - 10, delta_x + 10), PV_Random(delta_y - 10, delta_y + 10),
			PV_Random(1, 5),
			particles,
			40);
	}
}

private func FxIntTeleportStart(object target, effect fx, temp)
{
	if (temp) return;
	DoElectroCircle();
	fx.start_x = GetX();
	fx.start_y = GetY();
	this.Visibility = VIS_None;
	
	fx.particles = 
	{
		Prototype = Particles_ElectroSpark1(),
		Stretch = 6000,
		DampingX = 999, DampingY = 999
	};
	
	var xdir, ydir;
	if (current_high_x != nil)
	{
		xdir = current_high_x - fx.start_x;
		ydir = current_high_y - fx.start_y;
	}
	else
	{
		xdir = 0;
		ydir = -30;
	}
	CreateParticle("ElectroSpark", PV_Random(-5, 5), PV_Random(-5, 5),
		PV_Random(xdir - 5, xdir + 5), PV_Random(ydir - 5, ydir + 5),
		PV_Random(1, 5), fx.particles, 200);
		
	SoundAt("Animals::Puka::TeleportOut");
}

private func FxIntTeleportTimer(object target, effect fx, int time)
{
	// Instant teleport if no midpoint set.
	if (current_high_x == nil)
	{
		var old_x = GetX();
		var old_y = GetY();
		// Try target position if not stuck.
		SetPosition(current_ground_x, current_ground_y);
		var counter = 3;
		while (Stuck() && counter > 0)
		{
			SetPosition(GetX(), GetY() - 5);
		}
		// Otherwise abort!
		if (Stuck())
			SetPosition(old_x, old_y);
		return -1;
	}
	
	// Actually fly the way if in "normal" mode.
	time *= 40;
	if (time >= 1000) return -1;
	var half_time = 500;
	var w0 = Max(0, half_time - time);
	var w1 = half_time - Abs(time - half_time);
	var w2 = Max(0, time - half_time);
	
	var point_x = fx.start_x * w0 + current_high_x * w1 + current_ground_x * w2;
	var point_y = fx.start_y * w0 + current_high_y * w1 + current_ground_y * w2;
	point_x /= w0 + w1 + w2;
	point_y /= w0 + w1 + w2;
	
	var old_stuck = Stuck();
	var x = GetX();
	var y = GetY();
	SetPosition(point_x, point_y);
	if (Stuck() && !old_stuck)
		SetPosition(x, y);
	SetSpeed(0, 0);
}

private func FxIntTeleportStop(object target, effect fx, int reason, temp)
{
	if (temp || !this) return;
	this.Visibility = VIS_All;
	SetAction("Jump");
	DoElectroCircle();
	
	var xdir, ydir;
	if (current_high_x != nil)
	{
		xdir = GetX() - current_high_x;
		ydir = GetY() - current_high_y;
	}
	else
	{
		xdir = 0;
		ydir = 30;
	}
	
	var start_x = -xdir/2;
	var start_y = -ydir/2;
	CreateParticle("ElectroSpark", PV_Random(start_x - 5, start_x + 5), PV_Random(start_y - 5, start_y + 5),
		PV_Random(xdir - 5, xdir + 5), PV_Random(ydir - 5, ydir + 5),
		PV_Random(1, 5), fx.particles, 200);
	
	SoundAt("Animals::Puka::TeleportIn");
}

public func TryStartTeleport()
{
	// Already teleporting?
	if (GetAction() == "Teleport" || GetEffect("IntTeleport", this)) return;
	
	// Go to the most distant water?
	var teleport_ok = false;
	
	if (!Random(3))
	{
		var best_position = nil, best_distance = nil;
		var x = GetX(), y = GetY();
		for (var position in water_positions)
		{
			if (GBackSolid(AbsX(position[0]), AbsY(position[1]))) continue;
			
			var distance = Distance(x, y, position[0], position[1]);
			if (best_position == nil || distance > best_distance)
			{
				best_position = position;
				best_distance = distance;
			}
		}
		
		if (best_position != nil)
		{
			current_ground_x = best_position[0];
			current_ground_y = best_position[1];
			current_high_x = nil;
			current_high_y = nil;
			teleport_ok = true;
		}
	}
	
	teleport_ok = teleport_ok || GetTeleportPosition();
	
	// Go to a new random position?
	if (teleport_ok)
	{
		SetComDir(COMD_Stop);
		if (GetAction() == "Walk")
			SetAction("Teleport");
		else EndTeleport();
	}
}

private func StartTeleport()
{
	this.Activity = nil;
}

// When the teleport action has played.	
private func EndTeleport()
{
	AddEffect("IntTeleport", this, 1, 1, this);
}

private func GetTeleportPosition()
{
	// Allow multiple tries to increase the chance of a good position.
	for (var try = 0; try < 2; ++try)
	{
		// Project a line upwards and from the collision point (if any) project downwards at an angle.
		var upwards_angle = RandomX(-45, 45);
		
		if (GetAction() == "Scale")
		{
			if (GetDir() == DIR_Left) upwards_angle += 90;
			else upwards_angle -= 90;
		}
		var upwards_length = 100;
		var upwards_target_x = GetX() + Sin(upwards_angle, upwards_length);
		var upwards_target_y = GetY() - Cos(upwards_angle, upwards_length);
		var point = PathFree2(GetX(), GetY(), upwards_target_x, upwards_target_y);
		
		var high_x, high_y;
		if (point)
		{
			high_x = point[0];
			high_y = point[1];
		}
		else
		{
			high_x = upwards_target_x;
			high_y = upwards_target_y;
		}
		high_x = 2 * high_x / 3;
		high_y = 2 * high_y / 3;
		
		// Now project down again.
		var downwards_angle = RandomX(135, 160);
		if (Random(2)) downwards_angle *= -1;
		
		var down_target_x = high_x + Sin(downwards_angle, 75 + upwards_length);
		var down_target_y = high_y - Cos(downwards_angle, 75 + upwards_length);
		point = PathFree2(high_x, high_y, down_target_x, down_target_y);
		if (!point) continue;
		
		// Don't stay at the same position..
		if (Distance(GetX(), GetY(), point[0], point[1]) < 50) continue;
		
		current_high_x = high_x;
		current_high_y = high_y;
		current_ground_x = point[0];
		current_ground_y = point[1];
		return true;
	}
	return false;
}

private func ShockWater()
{
	// Already shocking? Most splendid.
	if (GetAction() == "ShockWater") return true;
	
	// Still reloading?
	if (GetEffect("IntShockWaterCooldown", this)) return false;
	
	// On which side is the water?
	var dir = nil;
	if (GBackLiquid(-15, 15)) dir = DIR_Right;
	else if (GBackLiquid(+15, 15)) dir = DIR_Left;
	
	// No water?
	if (dir == nil)
	{
		return false;
	}
	
	// Turn to the right direction.
	if (GetDir() != dir)
		Turn(dir, false);
	
	SetComDir(COMD_Stop);
	this.Activity = nil;
	SetAction("ShockWater");
	
	AddEffect("Sparkle", this, 1, 1, this);
	
	Sound("Animals::Puka::Hiss*");
	return true;
}

private func EndShockWater()
{
	// Randomly sample some points and - if water - fire lightning.
	var x = 15;
	if (GetDir() == DIR_Right) x *= -1;
	var y = 15;
	
	for (var obj in FindObjects(Find_Distance(120), Find_OCF(OCF_Alive), Find_AnimalHostile(this->GetOwner())))
	{
		if (!obj->GBackLiquid()) continue;
		var angle = Angle(GetX(), GetY(), obj->GetX(), obj->GetY());
		var xdir = +Sin(angle, 5);
		var ydir = -Cos(angle, 5);
		LaunchLightning(GetX() + x, GetY() + y, RandomX(50, 70), xdir, ydir, 10, 10, true);
	}
	
	// And some particles on the tail.
	var particles = 
	{
		Prototype = Particles_ElectroSpark1(),
		Stretch = 2000,
		DampingX = 950, DampingY = 950
	};
	CreateParticle("ElectroSpark", x, y, PV_Random(-30, 30), PV_Random(-30, 30), PV_Random(1, 5), particles, 30);
	
	// Don't do that again immediately.
	AddEffect("IntShockWaterCooldown", this, 1, 30);
}

private func FxSparkleStart(object target, effect fx, temp)
{
	if (temp) return;
	fx.particles =
	{
		Prototype = Particles_ElectroSpark2(),
		Stretch = 1000,
		DampingX = 999, DampingY = 999
	};
}

private func FxSparkleTimer(object target, effect fx, int time)
{
	if (time > 30) return -1;
	var angle = RandomX(-90, 90);
	var xdir = Sin(angle, 20);
	var ydir = -Cos(angle, 20);
	CreateParticle("ElectroSpark", xdir, ydir, xdir, ydir, 5, fx.particles, 1);
	return;
}

private func CheckTurn()
{
	if (GetXDir() < 0) if (GetDir() != DIR_Left) SetDir(DIR_Left);
	else if (GetXDir() > 0) if (GetDir() != DIR_Right) SetDir(DIR_Right);
	
	var t = false;
	if (turn_angle == 0 && GetDir() == DIR_Left) t = true;
	else if (turn_angle == 180 && GetDir() == DIR_Right) t = true;
	
	if (t)
	{
		if (!GetEffect("IntTurning", this))
			AddEffect("IntTurning", this, 1, 1, this);
	}
}

private func Turn(int dir, bool move)
{
	if (dir == nil)
	{
		if (GetDir() == DIR_Left)
			dir = DIR_Right;
		else
			dir = DIR_Left;
	}
	
	if (move)
	{
		if (dir == DIR_Left) SetComDir(COMD_Left);
		else SetComDir(COMD_Right);
	}
	if (GetDir() == dir) return;
	SetXDir(0);
	SetDir(dir);
	CheckTurn();
}

private func FxIntTurningStart(object target, effect fx, temp)
{
	if (temp)
		return true;
}

private func FxIntTurningTimer(object target, effect fx, int time)
{
	if (GetDir() == DIR_Left)
		turn_angle += 15;
	else turn_angle -= 15;

	if (turn_angle < 0 || turn_angle > 180)
	{
		turn_angle = BoundBy(turn_angle, 0, 180);
		this.MeshTransformation = Trans_Rotate(turn_angle + 180 + 30, 0, 1, 0);
		return -1;
	}
	this.MeshTransformation = Trans_Rotate(turn_angle + 180 + 30, 0, 1, 0);
	return 1;
}

// Immune to lightning.
public func RejectLightningStrike() { return true; }

local MaxEnergy = 30000;
local MaxBreath = 10000;
local NoBurnDecay = true;
local BorderBound = C4D_Border_Sides;
local ContactCalls = true;

public func Definition(proplist def)
{
	def.PictureTransformation = Trans_Mul(Trans_Translate(5500, 1000, 0), Trans_Scale(1600), Trans_Rotate(-5, 1, 0, 0), Trans_Rotate(60, 0, 1, 0));
}

local ActMap = {
Walk = {
	Prototype = Action,
	Name = "Walk",
	Procedure = DFA_WALK,
	Accel = 4,
	Decel = 22,
	Speed = 100,
	Directions = 2,
	FlipDir = 0,
	Length = 70,
	Delay = 1,
	Animation = "Walk",
	StartCall = "StartWalk",
	AbortCall = "StopWalk",
	EndCall = "StopWalk",
	InLiquidAction = "Swim",
},
Swim = {
	Prototype = Action,
	Name = "Swim",
	Procedure = DFA_SWIM,
	Accel = 16,
	Decel = 22,
	Speed = 100,
	Directions = 2,
	FlipDir = 0,
	Length = 70,
	Delay = 1,
	Animation = "Swim",
	StartCall = "StartSwim",
	AbortCall = "StopSwim",
	EndCall = "EndSwim",
},
Scale = {
	Prototype = Action,
	Name = "Scale",
	Procedure = DFA_SCALE,
	Accel = 16,
	Decel = 22,
	Speed = 50,
	Directions = 2,
	FlipDir = 0,
	Length = 70,
	Delay = 2,
	Animation = "Scale",
	StartCall = "StartScale",
	AbortCall = "StopScale",
	EndCall = "StopScale",
},
Jump = {
	Prototype = Action,
	Name = "Jump",
	Procedure = DFA_FLIGHT,
	Speed = 200,
	Accel = 14,
	Directions = 2,
	FlipDir = 0,
	Length = 20,
	Delay = 2,
	Animation = "RunJump",
	NextAction = "Hold",
	PhaseCall = "CheckStuck",
	StartCall = "StartJump",
	EndCall = "EndJump",
	AbortCall = "EndJump",
	InLiquidAction = "Swim",
},
Dead = {
	Prototype = Action,
	Name = "Dead",
	Directions = 2,
	FlipDir = 0,
	Length = 30,
	Delay = 1,
	Animation = "Teleport",
	NextAction = "Hold",
	NoOtherAction = 1,
	ObjectDisabled = 1
},
Teleport = {
	Prototype = Action,
	Name = "Teleport",
	Procedure = DFA_THROW,
	Directions = 2,
	FlipDir = 0,
	Length = 10,
	Delay = 3,
	Animation = "Teleport",
	NextAction = "Fly",
	StartCall = "StartTeleport",
	EndCall = "EndTeleport",
},
Sleep = {
	Prototype = Action,
	Name = "Sleep",
	Procedure = DFA_THROW,
	Directions = 2,
	FlipDir = 0,
	Length = 30,
	Delay = 1,
	Animation = "RollSleep",
	NextAction = "Hold",
	StartCall = "StartSleep"
},
LookAround = {
	Prototype = Action,
	Name = "LookAround",
	Procedure = DFA_THROW,
	Directions = 2,
	FlipDir = 0,
	Length = 90,
	Delay = 1,
	Animation = "Idle",
	NextAction = "Walk",
	StartCall = "ClearActivity"
},
ScratchFace = {
	Prototype = Action,
	Name = "ScratchFace",
	Procedure = DFA_THROW,
	Directions = 2,
	FlipDir = 0,
	Length = 30,
	Delay = 1,
	Animation = "IdleHand",
	NextAction = "Walk",
	StartCall = "ClearActivity"
},
ShockWater = {
	Prototype = Action,
	Name = "ShockWater",
	Procedure = DFA_THROW,
	Directions = 2,
	FlipDir = 0,
	Length = 10,
	Delay = 3,
	Animation = "ShockWater",
	NextAction = "Walk",
	EndCall = "EndShockWater",
},
Fly = {
	Prototype = Action,
	Name = "Fly",
	Procedure = DFA_FLOAT,
	Speed = 200,
	Accel = 14,
	Directions = 2,
	FlipDir = 0,
	Length = 20,
	Delay = 2,
	Animation = "RunJump",
	NextAction = "Hold",
	InLiquidAction = "Swim",
}
};

