/**
	Mooq
	Author: jok
*/

// Animations
local turn_angle;
// Color
local color;
// The closest enemy that has been found.
local enemy;
// Closest food that has been found.
local food;

public func Place(int amount, proplist rectangle, proplist settings)
{
	var max_tries = 3 * amount;
	var loc_area = nil;
	if (rectangle) loc_area = Loc_InArea(rectangle);
	var animal;

	while ((amount > 0) && (--max_tries > 0))
	{
		// Try to find walkable ground near lava.
		var lava_spot = FindLocation(Loc_Material("DuroLava"));
		var ground_spot = nil;
		if (lava_spot)
		{
			var lava_rectangle = Shape->Rectangle(lava_spot.x - 200, lava_spot.y - 200, 400, 400);
			// Make sure the position is inside the required target rectangle.
			lava_rectangle = Shape->Intersect(lava_rectangle, rectangle)->GetBoundingRectangle();
			ground_spot = FindLocation(Loc_Wall(CNAT_Bottom), Loc_Or(Loc_Sky(), Loc_Tunnel()), Loc_Space(20, CNAT_Top), Loc_InArea(lava_rectangle));
		}

		// If no hip and cool spot found, just get some generic spot.
		if (!ground_spot) ground_spot = FindLocation(Loc_Wall(CNAT_Bottom), Loc_Or(Loc_Sky(), Loc_Tunnel()), Loc_Space(20, CNAT_Top), loc_area);
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

func Construction()
{
	turn_angle = -60;
	color = 255;

	AddEffect("IntActivity", this, 1, 10, this);
	AddTimer("UpdateEnemy", 30);
	AddTimer("UpdateFood", 60);

	Stop();
	CheckTurn(GetDir());

	SetTailOnFire();
}

func Death()
{
	RemoveTimer("UpdateEnemy");
	RemoveTimer("UpdateFood");
	RemoveEffect("IntActivity", this);

	Sound("Animals::Mooq::Die*");
}

func CatchBlow(int damage, object from)
{
	Schedule(this, "Sound(\"Animals::Mooq::Hurt*\")", RandomX(5, 20));
}

/* Action Callbacks */

func CheckStuck()
{
	// Prevents getting stuck on middle vertex
	if(!GetXDir())
		if(Abs(GetYDir()) < 5)
			if(GBackSolid(0, 3))
				SetPosition(GetX(), GetY() - 1);
}

func ClearActivity()
{
	this.Activity = nil;
}

func StartSwim()
{
	this.Activity = this.ActivitySwimming;
}

func StartWalk()
{
	SetTailOnFire();
	this.Activity = this.ActivityWalking;
}

func SpitPhase()
{
	if(GetActTime() > 45 && GetActTime() < 65)
	{
		if(!Random(4))
		{
			var iX, iY;
			iX = 5;
			if (!GetDir()) iX = -iX;
			iY = -4;
			Smoke(iX,iY,5);
		}
	}
	if(GetActTime() == 58)
	{
		var iX, iY, iXDir, iYDir;
		iX = 10;
		if (!GetDir()) iX = -iX;
		iY = -4;
		iXDir = 300;
		if (!GetDir()) iXDir = -iXDir;
		iYDir = -300;

		Sound("Animals::Mooq::Spit*");

		var obj = CreateContents(Mooq_Firebomb);
		obj->Exit(iX, iY);
		obj->SetXDir(iXDir,100);
		obj->SetYDir(iYDir,100);
	}
}

func EatPhase()
{
	var actt = GetActTime();

	if(actt > 13 && actt < 25)
	{
		if(!Random(4))
		{
			var iX, iY;
			iX = 5;
			if (!GetDir()) iX = -iX;
			iY = 4;
			Smoke(iX,iY,5);
		}
	}
	if(actt == 22)
	{
		if (!food) return;

		Sound("Animals::Mooq::Munch*");
		DoEnergy(food->GetMass());
		food->RemoveObject();
	}
	if(actt > 43 && actt < 55)
	{
		if(!Random(4))
		{
			var iX, iY;
			iX = 5;
			if (!GetDir()) iX = -iX;
			iY = 1;
			Smoke(iX,iY,5);
		}
	}
}

/* Activity */

func ActivitySwimming()
{
	CheckBreathe();
	if(GetMaterial() == Material("Water"))
		CheckFossilize();

	// Stuck?
	if (GetComDir() && !GetXDir() && !GetYDir())
	{
		DoJump(true);
		return Swim(Random(2));
	}

	// Fossilizing?
	if(GetEffect("IntFossilizing", this))
	{
		var lava_spot = FindLava();
		if (lava_spot)
			return TaskSwimTo(lava_spot);
	}

	if (enemy && Random(2)) return TaskSwimTo(enemy);

	if (!enemy && food) return TaskSwimTo(food);

	if (!Random(6)) return Swim(Random(2));

	if (!Random(6)) return DoJump(true);
}

func ActivityWalking()
{
	// Stuck?
	if (GetComDir() && !GetXDir() && !GetYDir())
	{
		if(GetDir())
			Walk(0);
		else
			Walk(1);
		return DoJump();
	}

	// Fossilizing?
	if(GetEffect("IntFossilizing", this))
	{
		var lava_spot = FindLava();
		if (lava_spot) return TaskWalkTo(lava_spot);
	}

	// If enemy, hopefully attack!
	if (enemy)
	{
		var distance = ObjectDistance(this, enemy);

		if (distance < 50 && Random(2)) return TaskHeadbutt(distance);

		if (Inside(distance, 85, 145) && !Random(5)) return TaskSpit();
	}

	// If no enemy, go to food and eat.
	if (!enemy && food) return TaskFood();

	// If not walking, randomly idle.
	if (GetAction() == "Stand")
	{
		if (!enemy && !food)
		{
			// Idle?
			if (!Random(15)) return TaskIdle();
		}

		if (!Random(5)) return Walk(Random(2));

		return;
	}

	if (!Random(5)) return Stop();

	if (!Random(5)) return Walk(Random(2));

	if (GetAction() == "Walk")
	{
		if (!Random(5)) return DoJump();
	}

	// Anticipate holes in the landscape while walking.

}

func FxIntActivityTimer(target, effect, time)
{
	if (this.Activity) this->Activity();
	if (!GetAlive()) return -1;
	return 1;
}

func FxIntActivityDamage(target, effect, dmg)
{
	if (dmg > 0) return dmg;
	return dmg;
}

/* Tasks */

func TaskIdle()
{
	Sound("Animals::Mooq::Snorting*");

	if (!Random(3)) return SetAction("IdleSit");
	if (!Random(2)) return SetAction("IdleStand");

	return SetAction("IdleTailwave");
}

func TaskFood()
{
	var distance = ObjectDistance(this, food);
	if (distance < 11) return Eat();

	return TaskWalkTo(food);
}

func TaskHeadbutt(distance)
{
	if (distance < 11) return Headbutt();

	return TaskWalkTo(enemy);
}

func TaskSpit()
{
	var eX = enemy->GetX();
	var iX = GetX();

	if (iX < eX)
	{
		if (GetDir() != DIR_Right) return Turn(DIR_Right);
		Stop();
		return Spit();
	} else {
		if (GetDir() != DIR_Left) return Turn(DIR_Left);
		Stop();
		return Spit();
	}
}

func TaskWalkTo(spot)
{
	var iX = GetX();
	var iY = GetY();

	if (GetType(spot) == C4V_C4Object)
	{
		var sX = spot->GetX();
		var sY = spot->GetY();
	}
	else if (GetType(spot) == C4V_PropList)
	{
		var sX = spot.x;
		var sY = spot.y;
	}
	else return;

	if (iX < sX)
	{
		if (GetDir() == DIR_Right)
			if (iY > sY + 6)
				if (Random(2)) if(DoJump())
					return true;

		return Walk(DIR_Right);
	} else {
		if (GetDir() == DIR_Left)
			if(iY > sY + 6)
				if(Random(2)) if(DoJump())
					return true;
		return Walk(DIR_Left);
	}
}

func TaskSwimTo(spot)
{
	var iX = GetX();
	var iY = GetY();

	if (GetType(spot) == C4V_C4Object)
	{
		var sX = spot->GetX();
		var sY = spot->GetY();
	}
	else if (GetType(spot) == C4V_PropList)
	{
		var sX = spot.x;
		var sY = spot.y;
	}
	else return;

	if (iX < sX)
	{
		if (GetDir() == DIR_Right)
			if(iY > sY)
				if(DoJump(true))
					return true;

		return Swim(DIR_Right);
	} else {
		if (GetDir() == DIR_Left)
			if(iY > sY)
				if(DoJump(true))
					return true;
		return Swim(DIR_Left);
	}
}

/* Actions */

func Stop()
{
	SetComDir(COMD_Stop);
	SetXDir(0);
	return SetAction("Stand");
}

func Turn(int dir, bool move)
{
	if (dir == nil)
	{
		if (GetDir() == DIR_Left)
			dir = DIR_Right;
		else
			dir = DIR_Left;
	}

	if(GetDir() == dir) return;

	return CheckTurn(dir, move);
}

func Walk(int dir)
{
	if (GetAction() != "Stand" && GetAction() != "Walk") return;

	if (GetDir() == dir)
	{
		SetAction("Walk");
		if (GetDir())
			return SetComDir(COMD_Right);
		else
			return SetComDir(COMD_Left);
	}

	return Turn(dir, true);
}

func Swim(int dir)
{
	if (GetAction() != "Swim") return;

	if (GetDir() == dir)
	{
		SetAction("Swim");
		if (GetDir())
			return SetComDir(COMD_UpRight);
		else
			return SetComDir(COMD_UpLeft);
	}

	return Turn(dir, true);
}

func DoJump(bool swimming)
{
	if (GetAction() != "Walk" && GetAction() != "Stand" && GetAction() != "Swim") return;

	if (swimming)
	{
		if (GBackSky(0, -2))
			SetPosition(GetX(), GetY() - 2);
		else
			return;

		var iX, iY, iXDir, iYDir;
		iX = 10;
		if (!GetDir()) iX = -iX;
		iY = -4;
		iXDir = 200;
		if (!GetDir()) iXDir = -iXDir;
		iYDir = -200;

		if (Random(2)) Sound("Animals::Mooq::Snort*");

		SetSpeed(iXDir + GetXDir(100), iYDir + GetYDir(100), 100);
		return true;
	}

	if (Random(2)) Sound("Animals::Mooq::Snort*");
	return Jump();
}

func Spit()
{
	if (GetAction() == "Stand") return SetAction("Spit");
}

func Eat(object food)
{
	Stop();
	if (GetAction() == "Stand") return SetAction("Eat");
}

func Headbutt()
{
	if (GetAction() != "Walk") return;

	Punch(enemy, 10);
	return SetAction("Headbutt");
}

/* FindEnemy */

func UpdateEnemy()
{
	// Already disposed of the last one?
	if (enemy && !enemy->GetAlive()) enemy = nil;
	// Last one too far away now?
	if (enemy && ObjectDistance(this, enemy) > 250) enemy = nil;
	// Slid in water?
	if (enemy && enemy->GBackLiquid()) enemy = nil;

	var x = GetX();
	var y = GetY();
	for (var obj in FindObjects(Find_Distance(200), Find_OCF(OCF_Alive), Find_AnimalHostile(GetOwner()), Sort_Distance()))
	{
		if (!PathFree(x, y, obj->GetX(), obj->GetY())) continue;
		if (obj->GBackLiquid()) continue;
		enemy = obj;
		return;
	}
}

/* FindFood */

func UpdateFood()
{
	// Need food?
	if (GetEnergy() >= MaxEnergy/1000) return food = nil;
	// Last one too far away now?
	if (food && ObjectDistance(this, food) > 150) food = nil;
	// Slid in water?
	if (food && food->GBackLiquid()) food = nil;

	var x = GetX();
	var y = GetY();
	var Find_FoodIDs = Find_Or(Find_ID(Rock), Find_ID(Coal), Find_ID(Ore));
	for (var obj in FindObjects(Find_Distance(100), Find_FoodIDs, Sort_Distance()))
	{
		if (!PathFree(x, y, obj->GetX(), obj->GetY())) continue;
		if (obj->GBackLiquid()) continue;
		food = obj;
		return;
	}
}

/* FindLava */

func FindLava()
{
	var lava_spot = FindLocation(Loc_Material("DuroLava"), Loc_Space(20));
	if (lava_spot) return lava_spot;

	var lava_spot = FindLocation(Loc_Material("Lava"), Loc_Space(20));
	if (lava_spot) return lava_spot;

	return;
}

/* Turning */

func CheckTurn(int dir, bool move)
{
	if (!GetEffect("IntTurning", this))
	{
		SetDir(dir);
		AddEffect("IntTurning", this, 1, 1, this, nil, move);
		return true;
	}
	return false;
}

func FxIntTurningStart(object target, effect fx, temp, move)
{
	fx.mvmn = move;

	if (!InLiquid())
	{
		Stop();
		SetAction("Turn");
	} else {
		SetComDir(COMD_Stop);
		SetXDir(0);
		SetAction("SwimTurn");
	}

	if (temp) return true;
}

func FxIntTurningTimer(object target, effect fx, int time)
{
	if (GetDir() == DIR_Left)
		turn_angle += 15;
	else
		turn_angle -= 15;

	if (turn_angle < -60 || turn_angle > 180)
	{
		turn_angle = BoundBy(turn_angle, -60, 180);
		this.MeshTransformation = Trans_Rotate(turn_angle + 180 + 30,0,1,0);
		return -1;
	}
	this.MeshTransformation = Trans_Rotate(turn_angle + 180 + 30,0,1,0);
	return 1;
}

func FxIntTurningStop(object target, effect fx, temp)
{
	if (fx.mvmn)
	{
		if (!InLiquid())
		{
			if (GetDir()) SetComDir(COMD_Right);
			else SetComDir(COMD_Left);
			return SetAction("Walk");
		}

		if (GetDir()) SetComDir(COMD_UpRight);
		else SetComDir(COMD_UpLeft);
		return SetAction("Swim");
	}
}

/* Breathing */

func CheckBreathe()
{
	if (!GetEffect("IntBreathing", this)) AddEffect("IntBreathing", this, 1, 1, this, nil);
}

func FxIntBreathingStart(object target, effect fx, temp)
{
	if (temp) return true;
}

func FxIntBreathingTimer(object target, effect fx, int time)
{
	DoBreath(MaxBreath - GetBreath());
	if (!InLiquid()) return -1;

	return 1;
}

/* Fossilizing */

func CheckFossilize()
{
	if (!GetEffect("IntFossilizing", this))
		// Ca. 60 seconds till death without lava
		AddEffect("IntFossilizing", this, 1, 11, this, nil);
}

func FxIntFossilizingStart(object target, effect fx, temp)
{
	if (temp) return true;
}

func FxIntFossilizingTimer(object target, effect fx, int time)
{
	if (GetMaterial() == Material("DuroLava") || GetMaterial() == Material("Lava"))
	{
		color++;
		if (Speed < MaxSpeed) Speed += Random(2);
		if (JumpSpeed < MaxJumpSpeed)
		{
			JumpSpeed += Random(3);
			if(JumpSpeed > MaxJumpSpeed) JumpSpeed = MaxJumpSpeed;
		}
	} else {
		color--;
		if (Speed > 0) Speed -= Random(2);
		if (JumpSpeed > 0)
		{
			JumpSpeed -= Random(3);
			if(JumpSpeed < 0) JumpSpeed = 0;
		}
	}
	if (color < 45 || color > 255)
	{
		color = BoundBy(color, 45, 255);
		SetClrModulation(RGBa(color, color, color, 255));
		return -1;
	}
	SetClrModulation(RGBa(color, color, color, 255));

	return 1;
}

func FxIntFossilizingStop(object target, effect fx, temp)
{
	if (GetMaterial() == Material("DuroLava") || GetMaterial() == Material("Lava")) return SetAction("Swim");
	else return Kill();
}

/* Burning Tail */

func SetTailOnFire()
{
	if (!GetEffect("IntTailBurning", this))
		AddEffect("IntTailBurning", this, 1, 2, this, nil);
}

func FxIntTailBurningStart(object target, effect fx, temp)
{
	fx.fire = {
		R = 200 + Random(55),
		G = 200 + Random(55),
		B = 200 + Random(55),
		Alpha = PV_Linear(255, 0),
		Size = 4,
		Phase = PV_Linear(0, 9),
		DampingX = 1000,
		DampingY = 1000,
		Attach = ATTACH_MoveRelative
	};

	if (temp) return true;
}

func FxIntTailBurningTimer(object target, effect fx, int time)
{
	if (!GetAlive() || InLiquid())
	{
		var level;
		level = 5 ?? 10;
		var particles = Particles_Smoke();
		particles.Size = PV_Linear(PV_Random(level/2, level), PV_Random(2 * level, 3 * level));
		var pos = [3, -1, 0];
		var dir = [PV_Random(-level/3, level/3), PV_Random(-level/2, -level/3), 0];
		CreateParticleAtBone("Smoke", "tail_3", pos, dir, PV_Random(level * 2, level * 10), particles, BoundBy(level/5, 3, 20));
		return -1;
	}

	var pos = [3, -1, 0];
	var dir = [0, 0, 0];
	CreateParticleAtBone("Fire", "tail_3", pos, dir, 5, fx.fire, 1);
	return 1;
}

/* ActMap */

local ActMap = {

Walk = {
	Prototype = Action,
	Name = "Walk",
	Procedure = DFA_WALK,
	Speed = Speed,
	Accel = 4,
	Decel = 22,
	Directions = 2,
	FlipDir = 0,
	Length = 12,
	Delay = 1,
	Animation = "Walk",
	StartCall = "StartWalk",
	InLiquidAction = "Swim",
},
Swim = {
	Prototype = Action,
	Name = "Swim",
	Procedure = DFA_SWIM,
	Speed = Speed,
	Accel = 16,
	Decel = 22,
	Directions = 2,
	FlipDir = 0,
	Length = 12,
	Delay = 1,
	Animation = "Swim",
	StartCall = "StartSwim",
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
	Delay = 1,
	Animation = "Jump",
	NextAction = "Hold",
	PhaseCall = "CheckStuck",
    StartCall = "ClearActivity",
	InLiquidAction = "Swim",
},
Dead = {
	Prototype = Action,
	Name = "Dead",
	Directions = 2,
	FlipDir = 0,
	Length = 30,
	Delay = 1,
	Animation = "Death",
	NextAction = "Hold",
	StartCall = "ClearActivity",
	NoOtherAction = 1,
	ObjectDisabled = 1,
},
Stand = {
	Prototype = Action,
	Name = "Stand",
	Procedure = DFA_WALK, //DFA_THROW
	Directions = 2,
	FlipDir = 0,
	Length = 90,
	Delay = 1,
	Animation = "Stand",
	NextAction = "Stand",
	StartCall = "StartWalk",
	InLiquidAction = "Swim",
},
Turn = {
    Prototype = Action,
	Name = "Turn",
	Procedure = DFA_THROW,
	Directions = 2,
	FlipDir = 0,
	Length = 16,
	Delay = 1,
	Animation = "Walk",
	NextAction = "Stand",
	StartCall = "ClearActivity",
},
SwimTurn = {
    Prototype = Action,
	Name = "SwimTurn",
	Procedure = DFA_THROW,
	Directions = 2,
	FlipDir = 0,
	Length = 16,
	Delay = 1,
	Animation = "Swim",
	NextAction = "Swim",
	StartCall = "ClearActivity",
},
Spit = {
    Prototype = Action,
	Name = "Spit",
	Procedure = DFA_THROW,
	Directions = 2,
	FlipDir = 0,
	Length = 90,
	Delay = 1,
	Animation = "SitMouthOpen",
	NextAction = "Stand",
	PhaseCall = "SpitPhase",
	StartCall = "ClearActivity",
	InLiquidAction = "Swim",
},
Eat = {
	Prototype = Action,
	Name = "Eat",
	Procedure = DFA_NONE,
	Directions = 2,
	Length = 60,
	Delay = 1,
	Animation = "Eat",
	PhaseCall = "EatPhase",
	StartCall = "ClearActivity",
	NextAction = "Stand",
	InLiquidAction = "Swim",
	Attach=CNAT_Bottom,
},
Headbutt = {
	Prototype = Action,
	Name = "Headbutt",
	Procedure = DFA_THROW,
	Directions = 2,
	FlipDir = 0,
	Length = 12,
	Delay = 1,
	Animation = "Headbutt",
	StartCall = "ClearActivity",
	NextAction = "Stand",
	InLiquidAction = "Swim",
},
IdleStand = {
	Prototype = Action,
	Name = "IdleStand",
	Procedure = DFA_THROW,
	Directions = 2,
	FlipDir = 0,
	Length = 32,
	Delay = 1,
	Animation = "IdleStand",
	NextAction = "Stand",
	InLiquidAction = "Swim",
	StartCall = "ClearActivity",
},
IdleSit = {
	Prototype = Action,
	Name = "IdleSit",
	Procedure = DFA_THROW,
	Directions = 2,
	FlipDir = 0,
	Length = 64,
	Delay = 1,
	Animation = "IdleSit",
	NextAction = "Stand",
	InLiquidAction = "Swim",
	StartCall = "ClearActivity",
},
IdleTailwave = {
	Prototype = Action,
	Name = "IdleTailwave",
	Procedure = DFA_THROW,
	Directions = 2,
	FlipDir = 0,
	Length = 16,
	Delay = 1,
	Animation = "IdleTailwave",
	NextAction = "Stand",
	InLiquidAction = "Swim",
	StartCall = "ClearActivity",
},
};

local Name = "$Name$";
local Description = "$Description$";
local MaxEnergy = 250000;
local MaxBreath = 720; // Mooq can breathe for 20 seconds under water. // But it haz special effects ignoring MaxBreath.
local MaxSpeed = 100;
local Speed = MaxSpeed;
local MaxJumpSpeed = 300;
local JumpSpeed = MaxJumpSpeed;
local NoBurnDecay = true;
local BorderBound = C4D_Border_Sides;
local ContactCalls = true;

public func Definition(proplist def)
{
	def.PictureTransformation = Trans_Mul(Trans_Translate(2000, 3000, 0), Trans_Scale(1400), Trans_Rotate(20, 1, 0, 0), Trans_Rotate(60, 0, 1, 0));
}