/**
	Shark
	Very strong water animal.

	@author Armin
*/

local BiteStrength = 25;
local attacking, walking, swimming, swim_animation, base_transform, turned;

private func Construction()
{
	StartGrowth(15);
	SetAction("Swim");
	if (!Random(2))
		SetComDir(COMD_Right);
	else
		SetComDir(COMD_Left);
	if (GetAlive())
		AddTimer(this.Activity, 10);

	return _inherited(...);
}

public func IsAnimal() { return true; }

private func Attack(object target, int x, int y)
{
	if (!attacking || (target->GetAction() != "Swim" && target->GetAction() != "Jump") || !target->GetAlive() || !PathFree(GetX(), GetY(), target->GetX(), target->GetY()))
	{
		SetCommand("None");
		attacking = false;
		SetAction("Swim");
		return 1;
	}
	var dis = Distance(GetX(), GetY(), target->GetX(), target->GetY());
	if (dis > 30)
	{
		var current_angle = Angle(GetX(), GetY(), target->GetX(), target->GetY());
		if (current_angle < 0) current_angle = 360 + current_angle;
		var t = current_angle - 270;
		var t2 = Cos(t, 90) - 90; // Expand the areas around 0deg and 180deg a bit so you see fish from the side more
		this.MeshTransformation = Trans_Mul(Trans_Rotate(t, 0, 0, 1), Trans_Rotate(t2, 1, 0, 0));
	}
	this->SetCommand("MoveTo", target);
	if (dis < 20)
	{
		// Victim near. Bite him.
		SetAction("Bite");
		Sound("Animals::Fish::Munch*");
		if (target->GetAlive())
			target->DoEnergy(-BiteStrength);
		DoEnergy(BiteStrength);

		// Mission succesful. Swim back to the initial position.
		SetCommand("None");
		this["MeshTransformation"] = Trans_Identity();
		this->SetCommand("MoveTo", nil, x, y);
		ScheduleCall(this, "ResetHunger", 300);
		return 1;
	}
	ScheduleCall(this, "Attack", 5, 0, target, x, y);
}

private func Turn()
{
	if (GetCommand() != nil)
		return 1;
	SetComDir(COMD_None);
	SetXDir(0);
	SetYDir(0);
	SetAction("Turn");

	// The shark should now turn around in very short intervalls.
	turned = true;
	ScheduleCall(this, "ResetTurn", 80);
}

private func Activity()
{
	if (GetAction() == "Turn")
		return;
	if (swimming)
	{
		if (Inside(GetXDir(), -6, 6) && !turned && GetAction() != "FastSwim")
		{
			SetCommand("None");
			Turn();
		}
		else if (!attacking)
		{
			var target = FindObject(Find_ID(Clonk), Find_Distance(380), Find_Action("Swim"));
			if (target)
				if (PathFree(GetX(), GetY(), target->GetX(), target->GetY()))
				{
					attacking = true;
					SetAction("FastSwim");
					Attack(target, GetX(), GetY());
					return 1;
				}

			// Avoid surface.
			if (!GBackLiquid(0, -10)) 
			{
				if (GetDir())
					SetComDir(COMD_DownRight);
				else
					SetComDir(COMD_DownLeft);
				return 1;
			}

			// Avoid ground.
			if (!GBackLiquid(0, 25))
			{
				if (GetDir())
					SetComDir(COMD_UpRight);
				else
					SetComDir(COMD_UpLeft);
				return 1;
			}

			if (!turned)
			{
				// Wall?
				if ((GBackSolid(+75, 0) && GetDir() == DIR_Right) ^ (GBackSolid(-75, 0) && GetDir() == DIR_Left))
				{
					Turn();
					return;
				}

				if (!Random(40))
				{
					Turn();
					return 1;
				}
			}
		}
	}
	else if (walking)
	{
		// PANIC WHERE THE WATER AT
		var rx = RandomX(10, 25);
		if (!Random(2)) rx *= -1;
		
		var has_water = false;
		for (var y = 0; y <= 12; y += 4)
		{
			if (!GBackLiquid(rx, y)) continue;
			has_water = true;
			break;
		}
		
		if (has_water)
		{
			if (rx < 0) SetComDir(COMD_Left);
			else SetComDir(COMD_Right);
			
			if (!Random(2))
				DoJump();
			return true;
		}
		else
		{
			if (!Random(2))
			{
				if (!Random(2)) SetComDir(COMD_Left);
				else SetComDir(COMD_Right);
			}
			if (!Random(3))
				DoJump();
		}
	}
	return 1;
}

private func Place(int amount, proplist rectangle, proplist settings)
{
	var max_tries = 2 * amount;
	var loc_area = nil;
	if (rectangle) loc_area = Loc_InArea(rectangle);
	var f;
	
	while ((amount > 0) && (--max_tries > 0))
	{
		var spot = FindLocation(Loc_Material("Water"), Loc_Space(20), loc_area);
		if (!spot) continue;
		
		f = CreateObjectAbove(this, spot.x, spot.y, NO_OWNER);
		if (!f) continue;
		// Randomly add some large/slim fish
		if (Random(3))
		{
			// There are naturally smaller and larger sharks.
			f->SetCon(RandomX(80, 120));
			// make sure the smaller ones don't grow larger any more
			f->StopGrowth(); 
			// Slim fish.
			if (f->GetCon() > 100 || !Random(3))
				f->SetYZScale(1000 - Random(300));  
		}
		
		if (f->Stuck())
		{
			f->RemoveObject();
			continue;
		}
		--amount;
	}
	return f; // return last created fish
}

private func SetYZScale(int new_scale)
{
	base_transform = Trans_Scale(1000, new_scale, new_scale);
	return true;
}

private func Death()
{
	RemoveTimer(this.UpdateSwim);
	RemoveTimer(this.Activity);
	this.MeshTransformation = Trans_Rotate(160 + Random(41), 1, 0, 0);
	if (base_transform) this.MeshTransformation = Trans_Mul(base_transform, this.MeshTransformation);
	StopAnimation(swim_animation);
	Decay();
	this.Collectible = true;
	
	// Maybe respawn a new fish if roe is near.
	var roe = FindObject(Find_Distance(200), Find_ID(FishRoe));
	if (roe)
		roe->Hatch(GetID());
	
	return _inherited(...);
}

private func DoJump()
{
	SetAction("Jump");
	Sound("Hits::SoftTouch*");
	
	var x_dir = RandomX(ActMap.Jump.Speed/2, ActMap.Jump.Speed);
	if (GetComDir() == COMD_Left) x_dir *= -1;
	var y_dir = -RandomX(ActMap.Jump.Speed/3, ActMap.Jump.Speed);
	SetSpeed(x_dir, y_dir);
}

private func StartWalk() 
{
	var len = GetAnimationLength("Swim");
	var pos = GetAnimationPosition(swim_animation);
	SetAnimationPosition(swim_animation, Anim_Linear(pos, 0, len, 10, ANIM_Loop));
	this.MeshTransformation = Trans_Mul(Trans_Rotate(90 + RandomX(-10, 10), 1, 0, 0), base_transform);
	SetObjDrawTransform(0, 0, 0, 0, 0, 0);
	swim_animation = PlayAnimation("Swim", 5, Anim_Linear(0, 0, len, 100, ANIM_Loop), Anim_Const(500));
	if (GBackLiquid())
	{
		SetAction("Swim");
		return;
	}
	walking = true;
	swimming = false;

	ResetHunger();
}

private func StartSwim()
{	
	StopAnimation(GetRootAnimation(5));
	swimming = true;
	walking = false;
	this["MeshTransformation"] = Trans_Identity();
	if (GetCommand() != nil)
		return 1;
	if (GetDir() == DIR_Left)
	{
		SetDir(DIR_Right);
		if (!Random(5)) SetComDir(COMD_UpRight);
		else if (!Random(6)) SetComDir(COMD_DownRight);
		else SetComDir(COMD_Right);
	}
	else
	{
		SetDir(DIR_Left);
		if (!Random(5)) SetComDir(COMD_UpLeft);
		else if (!Random(6)) SetComDir(COMD_DownLeft);
		else SetComDir(COMD_Left);
	}
}

private func StartJump()
{
	if (GBackLiquid())
	{
		SetAction("Swim");
		return;
	}
	swimming = false;
	walking = false;
}

private func ResetHunger()
{
	attacking = false;
}

private func ResetTurn()
{
	turned = false;
}

local ActMap = {
Swim = {
	Prototype = Action,
	Name = "Swim",
	Procedure = DFA_SWIM,
	Speed = 100,
	Accel = 64,
	Decel = 16,
	Length = 52,
	Delay = 10,
	Directions = 2,
	FlipDir = 1,
	StartCall = "StartSwim",
	Animation = "Swim"
},
FastSwim = {
	Prototype = Action,
	Name = "FastSwim",
	Procedure = DFA_SWIM,
	Speed = 200,
	Accel = 48,
	Decel = 16,
	Length = 20,
	Delay = 5,
	Directions = 2,
	FlipDir = 0,
	NextAction = "FastSwim",
	Animation = "Swim"
},
Turn = {
	Prototype = Action,
	Name = "Turn",
	Procedure = DFA_SWIM,
	Speed = 100,
	Accel = 16,
	Decel = 16,
	Length = 52,
	Delay = 10,
	Directions = 2,
	FlipDir = 1,
	NextAction = "Swim",
	Animation = "Turn"
},
Bite = {
	Prototype = Action,
	Name = "Bite",
	Procedure = DFA_SWIM,
	Speed = 100,
	Accel = 16,
	Decel = 16,
	Length = 2,
	Delay = 10,
	Directions = 2,
	NextAction = "Swim",
	Animation = "Bite"
},
Walk = {
	Prototype = Action,
	Name = "Walk",
	Procedure = DFA_WALK,
	Speed = 30,
	Accel = 16,
	Decel = 16,
	Length = 1,
	Delay = 0,
	FacetBase = 1,
	Directions = 2,
	FlipDir = 1,
	NextAction = "Walk",
	StartCall = "StartWalk",
	InLiquidAction = "Swim",
},
Jump = {
	Prototype = Action,
	Name = "Jump",
	Procedure = DFA_FLIGHT,
	Speed = 30,
	Accel = 16,
	Decel = 16,
	Length = 1,
	Delay = 0,
	FacetBase = 1,
	Directions = 2,
	FlipDir = 1,
	NextAction = "Jump",
	StartCall = "StartJump",
	InLiquidAction = "Swim",
},
Dead = {
	Prototype = Action,
	Name = "Dead",
	Procedure = DFA_NONE,
	Speed = 10,
	Length = 1,
	Delay = 0,
	FacetBase = 1,
	Directions = 2,
	FlipDir = 1,
	NextAction = "Hold",
	NoOtherAction = 1,
	ObjectDisabled = 1,
	Animation = "Death"
}
};
local Name = "$Name$";
local Description = "$Description$";
local MaxEnergy = 150000;
local MaxBreath = 360; // 360 = ten seconds
local Placement = 1;
local NoBurnDecay = true;
local BreatheWater = 1;
local BorderBound = C4D_Border_Sides | C4D_Border_Top | C4D_Border_Bottom;

func IsPrey() { return false; }
func IsPredator() { return true; }

public func Definition(proplist def)
{
	def.PictureTransformation = Trans_Mul(Trans_Translate(12000, 5000, 0), Trans_Scale(1600), Trans_Rotate(20, 1, 0, 0), Trans_Rotate(65, 0, 1, 0));
}
