/**
	Chippy
	Small, lovely creatures.
	
	@author Zapper
*/

#include Library_Animal

local Name = "$Name$";
local Description = "$Description$";
local animal_reproduction_area_size = 800;
local animal_reproduction_rate = 200;
local animal_max_count = 10;

// Remember the attachee to be able to detach again.
local attach_object, attached_mesh;
// Remember the energy sucked to frequently spawn offsprings.
local energy_sucked;

private func Place(int amount, proplist rectangle, proplist settings)
{
	var max_tries = 4 * amount;
	var loc_area = nil;
	if (rectangle) 
		loc_area = Loc_InArea(rectangle);
	var chippies = [];
	var loc_mat = Loc_Material("Acid");
	if (settings && settings.mat)
		loc_mat = settings.mat;
	while ((amount > 0) && (--max_tries > 0))
	{
		var spot = FindLocation(loc_mat, Loc_Space(5), loc_area);
		if (!spot)
			continue;
		
		var chippie = CreateObject(this, spot.x, spot.y, NO_OWNER);
		if (!chippie) 
			continue;
		
		if (chippie->Stuck())
		{
			chippie->RemoveObject();
			continue;
		}
		PushBack(chippies, chippie);
		--amount;
	}
	// Return a list of all created chippies.
	return chippies;
}

public func Construction(...)
{
	AddEffect("Activity", this, 1, 10, this);
	SetAction("Walk");
	energy_sucked = 0;
	return _inherited(...);
}

public func Destruction()
{
	if (GetAction() == "Clawing")
		StopClawing();
	return _inherited(...);
}

public func Death(int killed_by)
{
	var particles = 
	{
		Prototype = Particles_Material(RGB(50, 200, 50)),
		OnCollision = PC_Bounce(),
		DampingX = 900, DampingY = 900,
	};
	
	for (var particle_graphics in ["SmokeDirty", "Flash"])
	{
		CreateParticle(particle_graphics, PV_Random(-1, 1), PV_Random(-1, 1),
					PV_Random(-20, 20), PV_Random(-20, 20),
					PV_Random(20, 60), particles, 60);
	}
	Sound("Animals::Chippie::Chirp*", false, 50);
	RemoveObject();
}

public func AttachTargetLost()
{
	SetAction("Jump");
}

private func StartClawing()
{
	AddEffect("Clawing", this, 1, 30, this);
}

private func StopClawing()
{
	if (attach_object && attached_mesh != nil)
		attach_object->DetachMesh(attached_mesh);
	this.Visibility = VIS_All;
	RemoveEffect("Clawing", this);
	SetVertexXY(0, 0, -1);
}

private func FxClawingTimer(target, effect, time)
{
	var e = GetActionTarget();
	if (!e) return SetAction("Jump");
	
	if ((!e->GetAlive()) || GBackSemiSolid())
	{
		SetAction("Jump");
		return;
	}
	
	if (!Random(3))
	{
		var damage = GetCon() * 10;
		e->DoEnergy(-damage, true, FX_Call_EngGetPunched, GetOwner());
		energy_sucked += damage;
		
		if (energy_sucked > 10 * 1000)
		{
			LayEgg();
			energy_sucked -= 10 * 1000;
		}
		
		// Grow and prosper.
		if (GetCon() < 150 && !Random(5))
		{
			DoCon(1);
			DoEnergy(5);
		}
	}
}

private func StartJump()
{
	RotateGraphics(0, false);
	if (!GetEffect("DmgShock", this) && !GetEffect("JumpCheck", this))
		AddEffect("JumpCheck", this, 1, 4, this);
}

private func FxJumpCheckTimer(target, effect, time)
{
	var e = FindObject(Find_AtPoint(), Find_OCF(OCF_Alive), Find_AnimalHostile(GetOwner()));
	if (e)
	{
		ClawTo(e);
		return -1;
	}
}

private func ClawTo(object obj)
{
	// Play the sound on the object, because the chippie turns invisible.
	obj->Sound("Animals::Chippie::Bite*", false, 50);
	
	energy_sucked += 5 * 1000;
	obj->DoEnergy(-5, false, FX_Call_EngGetPunched, GetOwner());
	// Woops, I broke it.
	if (!obj || !obj->GetAlive()) return;
	
	SetAction("Claw", obj);
	SetVertexXY(0, obj->GetX() - GetX(), obj->GetY() - GetY());
	
	// Special handling for the Clonk - attach directly to a random bone.
	if (obj->~IsClonk())
	{
		var bone_name = ["skeleton_leg_upper.R", "skeleton_leg_upper.L",
						"skeleton_arm_upper.R", "skeleton_arm_upper.L",
						"pos_back2"][Random(5)];
		attach_object = obj;
		var scale = GetCon() * 1000 / 100;
		attached_mesh = attach_object->AttachMesh(this, bone_name, "back", 
						Trans_Mul(Trans_Scale(scale, scale, scale), Trans_Rotate(Random(360), 0, 1, 0)));
		// Hide the attached object.
		this.Visibility = VIS_None;
	}
	else
	{
		this.MeshTransformation = Trans_Mul(Trans_Rotate(90, 0, 1, 0), Trans_Rotate(Random(360), 0, 0, 1));
	}
}

private func EndJump()
{
	RemoveEffect("JumpCheck", this);
}

private func StartWalk()
{
	RotateGraphics(0, false);
}

private func StartScale()
{
	RotateGraphics(90, false);
}

private func RotateGraphics(int r, bool mirror)
{
	var transform = Trans_Rotate(r, 0, 0, 1);
	if (mirror) transform = Trans_Mul(transform, Trans_Scale(-1000, 1000, 1000));
	this.MeshTransformation = transform;
}

private func StartHangle()
{
	RotateGraphics(180, true);
}

private func Jump()
{
	SetAction("Jump");
	SetSpeed(GetXDir(), -10 - GetXDir());
}

private func FxActivityTimer(target, effect, time)
{
	if ((GetAction() == "Jump") || (GetAction() == "Claw"))
	{
		return 1;
	}
	
	if (GetComDir() != COMD_Stop)
		if (!Random(5)) Jump();
	
	if (!GetEffect("DmgShock", this) && !GBackSemiSolid())
	{
		for (var enemy in FindObjects(Find_Distance(100), Find_OCF(OCF_Alive), Find_AnimalHostile(GetOwner()), Sort_Distance()))
		{
			if (!PathFree(GetX(), GetY(), enemy->GetX(), enemy->GetY())) continue;
			
			if (enemy->GetX() < GetX())
				SetComDir(COMD_Left);
			else SetComDir(COMD_Right);
			
			if (ObjectDistance(this, enemy) < 20)
			{
				SetAction("Jump");
				var a = Angle(GetX(), GetY(), enemy->GetX(), enemy->GetY());
				SetSpeed(Sin(a, 20), -Cos(a, 20) - 2);
			}
			
			return 1;
		}

		if (GetEffect("DoDance", this))
		{
			if (GetAction() == "Walk")
			{
				SetComDir(COMD_Stop);
				Jump();
				return 1;
			}
		}
		else
		if (!Random(20) && !GetEffect("DanceCooldown", this))
		{
			Sound("Animals::Chippie::Talk*");
			
			var cnt = 0;
			for (var obj in FindObjects(Find_Distance(100), Find_ID(GetID()), Find_Allied(GetOwner()), Sort_Distance()))
			{
				if (++cnt > 5) break;
				obj->AddEffect("DoDance", obj, 1, 35*5, obj);
				obj->AddEffect("DanceCooldown", obj, 1, 35*10, obj);
			}
		}
	}
	
	if (!Random(5))
	{
		if (Random(2))
			SetComDir(COMD_Right);
		else SetComDir(COMD_Left);
	} else SetComDir(COMD_Stop);

	return 1;
}

private func FxActivityDamage(target, effect, dmg)
{
	if (dmg > 0) return dmg;
	AddEffect("DmgShock", this, 1, 35*5, this);
	return dmg;
}

private func SpecialReproduction()
{
	LayEgg();
	// Always return true even if laying egg failed. Otherwise mammalian reproduction is performed.
	return true;
}

private func LayEgg()
{
	if (GetEffect("DmgShock", this)) return;
	if (GetEffect("EggCooldown", this)) return;
	var o = CreateObject(Chippie_Egg, 0, 0, GetOwner());
	o->SetSpeed(-GetXDir(), -GetYDir());
	o->SetCon(50);
	o->StartGrowth(10);
	AddEffect("EggCooldown", this, 1, 35*30, this);
	return o;
}

// Chippies grow (solely) via sucking blood (their size influences the damage they do).
public func GrowthSpeed() { return 0; }

local MaxEnergy = 10000;
local MaxBreath = 10000;
local NoBurnDecay = true;
local ContactIncinerate = 15;
local CorrosionResist = true;
local BorderBound = C4D_Border_Sides;
local ContactCalls = true;

public func Definition(proplist def)
{
	def.PictureTransformation = Trans_Mul(Trans_Translate(1000, -700, 0), Trans_Scale(1400), Trans_Rotate(-10, 1, 0, 0), Trans_Rotate(50, 0, 1, 0));
	return _inherited(def, ...);
}

local ActMap = {
Walk = {
	Prototype = Action,
	Name = "Walk",
	Procedure = DFA_WALK,
	Accel = 16,
	Decel = 22,
	Speed = 100,
	Directions = 2,
	FlipDir = 1,
	Length = 20,
	Delay = 1,
	Animation = "Move",
	StartCall = "StartWalk"
},
Swim = {
	Prototype = Action,
	Name = "Swim",
	Procedure = DFA_SWIM,
	Accel = 16,
	Decel = 22,
	Speed = 100,
	Directions = 2,
	FlipDir = 1,
	Length = 20,
	Delay = 1,
	Animation = "Move",
	StartCall = "StartWalk"
},
Scale = {
	Prototype = Action,
	Name = "Scale",
	Procedure = DFA_SCALE,
	Accel = 16,
	Decel = 22,
	Speed = 50,
	Directions = 2,
	FlipDir = 1,
	Length = 20,
	Delay = 2,
	Animation = "Move",
	StartCall = "StartScale"
},
Hangle = {
	Prototype = Action,
	Name = "Hangle",
	Procedure = DFA_HANGLE,
	Accel = 16,
	Decel = 22,
	Speed = 50,
	Directions = 2,
	FlipDir = 1,
	Length = 20,
	Delay = 2,
	Animation = "Move",
	StartCall = "StartHangle"
},
Kneel = {
	Prototype = Action,
	Name = "Kneel",
	Procedure = DFA_KNEEL,
	Directions = 2,
	FlipDir = 1,
	Length = 20,
	Delay = 2,
	Animation = "Move",
	NextAction = "Walk",
},
Jump = {
	Prototype = Action,
	Name = "Jump",
	Procedure = DFA_FLIGHT,
	Speed = 200,
	Accel = 14,
	Directions = 2,
	FlipDir = 1,
	Length = 20,
	Delay = 2,
	Animation = "Move",
	StartCall = "StartJump",
	EndCall = "EndJump",
	AbortCall = "EndJump"
},
Dead = {
	Prototype = Action,
	Name = "Dead",
	Directions = 2,
	FlipDir = 1,
	Length = 10,
	Delay = 3,
	Animation = "Move",
	NextAction = "Hold",
	NoOtherAction = 1,
	ObjectDisabled = 1,
},
Claw = {
	Prototype = Action,
	Name = "Claw",
	Procedure = DFA_ATTACH,
	Directions = 2,
	FlipDir = 1,
	Length = 1,
	Delay = 0,
	Animation = "Move",
	NextAction = "Hold",
	StartCall = "StartClawing",
	EndCall = "StopClawing",
	AbortCall = "StopClawing",
},
};