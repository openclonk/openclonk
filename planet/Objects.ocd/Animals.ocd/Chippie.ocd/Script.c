/**
	Chippy
	Small, lovely creatures.
*/

local Name = "$Name$";
local Description = "$Description$";

// Remember the attachee to be able to detach again.
local attach_object, attached_mesh;
// Remember the energy sucked to frequently spawn offsprings.
local energy_sucked;

public func Construction()
{
	CreateEffect(Activity, 1, 10);
	SetAction("Walk");
	if (GetOwner() == NO_OWNER)
		SetCreatureControlled();
	energy_sucked = 0;
	return true;
}

public func Destruction()
{
	if (GetAction() == "Clawing")
		StopClawing();
}

public func Death()
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
	CreateEffect(Clawing, 1, 30);
}

private func StopClawing()
{
	if (attach_object && attached_mesh != nil)
		attach_object->DetachMesh(attached_mesh);
	this.Visibility = VIS_All;
	RemoveEffect("Clawing", this);
	SetVertexXY(0, 0, -1);
}

local Clawing = new Effect {
    Timer = func(int time) {
        var e = this.Target->GetActionTarget();
        if(!e || (!e->GetAlive()) || this.Target->GBackSemiSolid())
        {
            return this.Target->SetAction("Jump");
        }
        
        if(!Random(3))
        {
            var damage = this.Target->GetCon() * 10;
            e->DoEnergy(-damage, true, FX_Call_EngGetPunched, this.Target->GetOwner());
            this.Target.energy_sucked += damage;
            
            if(this.Target.energy_sucked > 10 * 1000)
            {
                this.Target->LayEgg();
                this.Target.energy_sucked -= 10 * 1000;
            }
            
            //Grow and prosper.
            if(this.Target->GetCon() < 150 && !Random(10))
            {
                this.Target->DoCon(1);
                this.Target->DoEnergy(5);
            }
        }
    },
};

private func StartJump()
{
	RotateGraphics(0, false);
	if(!GetEffect("DmgShock", this) && !GetEffect("JumpCheck", this))
		CreateEffect(JumpCheck, 1, 4);
}

local JumpCheck = new Effect {
    Timer = func(int time) {
        var e = this.Target->FindObject(Find_AtPoint(),Find_OCF(OCF_Alive), Find_Hostile(this.Target->GetOwner()));
        if(e)
        {
            this.Target->ClawTo(e);
            return -1;
        }
    },
};

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

local Activity = new Effect {
    Timer = func(int time) {
        if((this.Target->GetAction() == "Jump") || (this.Target->GetAction() == "Claw"))
        {
            return 1;
        }
        
        if(this.Target->GetComDir() != COMD_Stop)
        {
            if(!Random(5)) this.Target->Jump();
        }
        
        if(!GetEffect("DmgShock",this.Target) && !this.Target->GBackSemiSolid())
        {
            for(var enemy in this.Target->FindObjects(this.Target->Find_Distance(100),Find_OCF(OCF_Alive),Find_Hostile(this.Target->GetOwner()), Sort_Distance()))
            {
                if(!this.Target->PathFree(this.Target->GetX(), this.Target->GetY(), enemy->GetX(), enemy->GetY())) continue;
                
                if(enemy->GetX() < this.Target->GetX())
                {
                   this.Target->SetComDir(COMD_Left);
                }
                else this.Target->SetComDir(COMD_Right);
                
                if(ObjectDistance(this.Target,enemy) < 20)
                {
                    this.Target->SetAction("Jump");
                    var a = Angle(this.Target->GetX(), this.Target->GetY(), enemy->GetX(), enemy->GetY());
                    this.Target->SetSpeed(Sin(a, 20), -Cos(a, 20) - 2);
                }
                
                return 1;
            }
            
            if(GetEffect("DoDance",this.Target))
            {
                if(GetAction() == "Walk")
                {
                    this.Target->SetComDir(COMD_Stop);
                    this.Target->Jump();
                    return 1;
                }
            }
            else if(!Random(20) && !GetEffect("DanceCooldown", this.Target))
            {
                this.Target->Sound("Animals::Chippie::Talk*");
                
                var cnt = 0;
                for(var obj in this.Target->FindObjects(this.Target->Find_Distance(100),Find_ID(Chippie), Find_Allied(this.Target->GetOwner()), Sort_Distance()))
                {
                    if(++cnt > 5) break;
                    obj->CreateEffect(obj.DoDance,1,35*5);
                    obj->CreateEffect(obj.DanceCooldown,1,35*10);
                }
                
                if(!GetEffect("EggCooldown", this.Target) && !Random(10)) this.Target->LayEgg();
            }
        }
        
        if(!Random(5))
        {
            if(Random(2))
            {
                this.Target->SetComDir(COMD_Right);
            }
            else
            {
                this.Target->SetComDir(COMD_Left);
            }
        }
        else this.Target->SetComDir(COMD_Stop);
        
        return 1;
    },
    
    Damage = func(int dmg) {
        if(dmg > 0) return dmg;
        CreateEffect(this.Target.DmgShock, 1, 35*5);
        return dmg;
    },
};

private func LayEgg()
{
	if(GetEffect("DmgShock", this)) return;
	var o = CreateObject(Chippie_Egg, 0, 0, GetOwner());
	o->SetSpeed(-GetXDir(), -GetYDir());
	o->SetCon(50);
	o->StartGrowth(10);
	CreateEffect(EggCooldown, 1, 35*30);
}
local EggCooldown = new Effect {};
local DmgShock = new Effect {};
local DoDance = new Effect {};
local DanceCooldown = new Effect {};

local MaxEnergy = 10000;
local MaxBreath = 10000;
local NoBurnDecay = 1;
local ContactIncinerate = 15;
local CorrosionResist = 1;
local BorderBound = C4D_Border_Sides;
local ContactCalls = true;

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