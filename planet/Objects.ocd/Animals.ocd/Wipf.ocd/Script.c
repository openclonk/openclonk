/**
	Wipf
	A furry creature which is the clonk's biggest friend.
	
	@author Maikel
*/

// Wipfs can be carried by the clonk.
#include Library_CarryHeavy


public func Initialize()
{
	// Add an effect to control the wipf's activities.
	CreateEffect(IntActivity, 1, 10);
	return;
}

public func Place(int amount, proplist area)
{
	if (this != Wipf)
		return;
	if (!area) 
		area = Shape->LandscapeRectangle();
	var wipfs = [];
	for (var i = 0 ; i < amount ; i++)
	{
		var position = FindLocation(Loc_InArea(area), Loc_Wall(CNAT_Bottom), Loc_Or(Loc_Sky(), Loc_Tunnel()));
		if (position)
		{
			var wipf = CreateObjectAbove(Wipf, position.x, position.y, NO_OWNER);
			if (wipf->Stuck())
				wipf->RemoveObject();
			if (wipf) 
				PushBack(wipfs, wipf);
		}
	}
	return wipfs;
}

public func IsPrey() { return true; }


/*-- Activity --*/

local Activity = new Effect {
    Start = func(int temp) {
        if (temp)
		return FX_OK;
    	// Set the correct interval.
	    this.Interval = 10;		
	    // Set action and a face a random direction.
	    this.Target->SetAction("Stand");
	    this.Target->SetDir([DIR_Left, DIR_Right][Random(2)]);
	    return FX_OK;
    },
    Timer = func(int time) {
        // Don't do anything if contained.
	    if (this.Target->Contained())
		    return FX_OK;

	    // Start digging if stuck.
	    if (this.Target->Stuck())
	    {
		    this.Target->SetAction("Dig");
		    this.Target->SetDigDirection(Random(2) * 2 - 1, Random(2) * 2 - 1);
		    return FX_OK;
	    }

	    // Actions when standing.
	    if (this.Target->IsStanding())
	    {
		    // Start digging if above diggable material.
		    var mat = this.Target->GetMaterial(0, 8);
		    if (!Random(100) && GetMaterialVal("DigFree", "Material", mat))
		    {
		    	this.Target->SetAction("Dig");
		    	this.Target->SetDigDirection(Random(2) * 2 - 1, 1);
		    	return FX_OK;
		    }		
		    // Start walking from time to time.
		    if (!Random(10))
		    {
		    	this.Target->SetAction("Walk");
		    	this.Target->SetComDir([COMD_Left, COMD_Right][Random(2)]);
		    	return FX_OK;
		    }
		    return FX_OK;
	    }

	    // Actions on walking.
	    if (this.Target->IsWalking()) 
	    {
		    // Change direction of walking from time to time.
		    if (!Random(5))
		    {
		    	this.Target->SetComDir([COMD_Left, COMD_Right][Random(2)]);
		    	return FX_OK;	
		    }
		    // Do some random jumps.
		    if (!Random(20))
		    {
		    	this.Target->Jump();
		    	return FX_OK;		
		    }
		    // Stop walking from time to time.
		    if (!Random(20) && this.Target->GetComDir() != COMD_Stop)
		    {
		    	this.Target->SetComDir(COMD_Stop);
		    	return FX_OK;
		    }
		    // Start standing when com dir is stop and speed is zero.
		    if (this.Target->GetComDir() == COMD_Stop && this.Target->GetYDir() == 0)
		    {
		    	this.Target->SetAction("Stand");
		    	return FX_OK;
		    }
		    return FX_OK;
	    }
	
	    // Actions on digging.
	    if (this.Target->IsDigging())
	    {
		    // If the wipf if stuck while digging digfree the full wipf's shape.
		    if (this.Target->Stuck())
			    this.Target->DigFree(GetX(), GetY(), 8);
		
		    // Change digging direction.
		    if (!Random(3))
		    {
			    var dir = this.Target->GetDigDirection();
			    if (!dir)
			    	return FX_OK;
			    var xdir = dir.x + RandomX(-this.Target.ActMap.Dig.Speed, this.Target.ActMap.Dig.Speed) / 3;
			    var ydir = dir.y + RandomX(-this.Target.ActMap.Dig.Speed, this.Target.ActMap.Dig.Speed) / 3;
			    ydir = BoundBy(ydir, -Abs(xdir), Abs(xdir));
			    this.Target->SetDigDirection(xdir, ydir);
			    return FX_OK;
		    }
		    // Stop digging from time to time.
		    if (!Random(3) && this.Target->GetComDir() != COMD_Stop)
		    {
		    	this.Target->SetComDir(COMD_Stop);
		    	return FX_OK;
		    }
		    // Start standing when com dir is stop and speed is zero.
		    if (this.Target->GetComDir() == COMD_Stop && this.Target->GetYDir() == 0)
		    {
		    	this.Target->SetAction("Stand");
		    	return FX_OK;
		    }
		    return FX_OK;
	    }
	
	    // Actions on swimming.
	    if (this.Target->IsSwimming())
	    {
		    // Swim up if under water.
		    if (this.Target->GBackLiquid(0, -4))
		    {
		    	this.Target->SetComDir(COMD_Up);
		    	return FX_OK;
		    }		
		    // Change direction of walking from time to time.
		    if (!Random(5))
		    {
		    	this.Target->SetComDir([COMD_Left, COMD_Right][Random(2)]);
		    	return FX_OK;	
		    }
		    // If at an edge do jump out of the liquid.
		    if (!this.Target->GBackLiquid(0, -6))
		    {
			    if (((this.Target->GBackSolid(16, 2) || this.Target->GBackSolid(24, 2)) && PathFree(this.Target->GetX(), this.Target->GetY(), this.Target->GetX() + 16, this.Target->GetY() - 16) && this.Target->GetDir() == DIR_Right) ||
			        ((this.Target->GBackSolid(-16, 2) || this.Target->GBackSolid(-24, 2)) && PathFree(this.Target->GetX(), this.Target->GetY(), this.Target->GetX() - 16, this.Target->GetY() - 16) && this.Target->GetDir() == DIR_Left))
			    {
				    this.Target->SetPosition(this.Target->GetX(), this.Target->GetY() - 2);
				    this.Target->Jump();
			    }
			    return FX_OK;
		    }
	    }
	    return FX_OK;
    },
};


/*-- Standing --*/

public func IsStanding()
{
	return GetAction() == "Stand";
}

public func StartStand()
{
	if (!GetEffect("IntStanding", this))
		CreateEffect(IntStanding, 1, 1);

	return;
}

public func StopStand()
{
	if (!IsStanding()) 
		RemoveEffect("IntStanding", this);
	return;
}

local IntStanding = new Effect {
    Start = func(int temp) {
        if (temp)
		    return FX_OK;
	    this.Target->SetComDir(COMD_Stop);
	    this.standing_anim = ["Idle", "Idle2"][Random(2)];
	    this.anim_number = this.Target->PlayAnimation(this.Target.IntStanding.standing_anim, 5, Anim_Linear(0, 0, GetAnimationLength(this.Target.IntStanding.standing_anim), 35, ANIM_Hold), Anim_Const(1000));
	    return FX_OK;
    },
    
    Timer = func(int time) { return FX_OK; },
    
    Stop = func(int reason, bool temp) {
        if(temp)
            return FX_OK;
        this.Target->StopAnimation(this.Target.IntStanding.anim_number);
        return FX_OK;
    },
};

/*-- Walking --*/

public func IsWalking()
{
	return GetAction() == "Walk";
}

public func StartWalk()
{
	if (!GetEffect("IntWalking", this))
		CreateEffect(IntWalking, 1, 1);
	return;
}

public func StopWalk()
{
	if (!IsWalking()) 
		RemoveEffect("IntWalking", this);
	return;
}

local IntWalking = new Effect {
    Start = func(int temp) {
    	if (temp)
		    return FX_OK;
	    var dir = -1;
	    if (this.Target->GetComDir() == COMD_Right) 
		    dir = +1;
    	this.anim_number = this.Target->PlayAnimation("Hop", 5, Anim_X(0, 0, GetAnimationLength("Hop"), 50 * dir), Anim_Const(1000));
	    return FX_OK;
    },
    
    Timer = func(int time) {
        if ((time % 6) == 0)
		    this.Target->Footstep();
	    return FX_OK;
    },
    
    Stop = func(int reason, bool temp) {
        if (temp)
		    return FX_OK;
	    this.Target->StopAnimation(this.Target.IntWalking.anim_number);
	    return FX_OK;
    },
};

private func Footstep()
{
	if (GetMaterialVal("DigFree", "Material", GetMaterial(0, 7)) == 0)
		return;

	var dir = Sign(GetXDir());
	var clr = GetAverageTextureColor(GetTexture(0, 7));
	var particles =
	{
		Prototype = Particles_Dust(),
		Size = PV_KeyFrames(0, 0, 3, 100, 7, 1000, 5),
		R = (clr >> 16) & 0xff,
		G = (clr >> 8) & 0xff,
		B = clr & 0xff,
	};
	CreateParticle("Dust", PV_Random(dir * -2, dir * -1), 5, PV_Random(dir * 2, dir * 1), PV_Random(-2, -3), PV_Random(12, 24), particles, 5);
	return;
}


/*-- Swimming --*/

public func IsSwimming()
{
	return GetAction() == "Swim";
}

public func StartSwim()
{
	if (!GetEffect("IntSwimming", this))
		CreateEffect(IntSwimming, 1, 1);
	return;
}

public func StopSwim()
{
	if (!IsSwimming()) 
		RemoveEffect("IntSwimming", this);
	return;
}

local IntSwimming = new Effect {
    Start = func(int temp) {
        if (temp)
		    return FX_OK;
	    var dir = -1;
	    if (this.Target->GetComDir() == COMD_Right) 
		    dir = +1;
	    this.anim_number = this.Target->PlayAnimation("Swim", 5, Anim_X(0, 0, GetAnimationLength("Swim"), 40 * dir), Anim_Const(1000));
	    return FX_OK;
    },
    
    Timer = func(int time) { return FX_OK; },
    
    Stop = func(int reason, bool temp) {
        if (temp)
		    return FX_OK;
	    this.Target->StopAnimation(this.Target.IntSwimming.anim_number);
	    return FX_OK;
    },
};

/*-- Jumping --*/

public func IsJumping()
{
	return GetAction() == "Jump";
}

public func StartJump()
{
	if (!GetEffect("IntJumping", this))
		CreateEffect(IntJumping, 1, 1);
	return;
}

public func StopJump()
{
	if (!IsJumping()) 
		RemoveEffect("IntJumping", this);
	return;
}

local IntJumping = {
    Start = func(int temp) {
        if (temp)
		    return FX_OK;
        var dir = -1;
	    if (GetDir() == DIR_Right) 
		    dir = +1;
	    this.anim_number = this.Target->PlayAnimation("Hop", 5, Anim_X(0, 0, GetAnimationLength("Hop"), 50 * dir), Anim_Const(1000));
	    return FX_OK;
    },
    
    Timer = func(int time) { return FX_OK; },
    
    Stop = func(int reason, bool temp) {
        if (temp)
		    return FX_OK;
    	this.Target->StopAnimation(this.Target.IntJumping.anim_number);
	    return FX_OK;
    },
};

public func Jump(int xdir, int ydir)
{
	SetAction("Jump");
	
	if (xdir == nil)
		xdir = 10;
	if (GetDir() == DIR_Left)
		xdir *= -1;
	if (ydir == nil)
		ydir = -20;

	SetSpeed(GetXDir() + xdir, ydir - GetXDir() / 2);
	
	if (GetMaterialVal("DigFree", "Material", GetMaterial(0, 7)) != 0)
	{
		for (var c = 0; c < 5; ++c)
		{
			var dir = Sign(GetXDir());
			var clr = GetAverageTextureColor(GetTexture(0, 7));
			var particles =
			{
				Prototype = Particles_Dust(),
				R = (clr >> 16) & 0xff,
				G = (clr >> 8) & 0xff,
				B = clr & 0xff,
			};
			CreateParticle("Dust", PV_Random(dir * -2, dir * -1), 5, PV_Random(dir * 2, dir * 1), PV_Random(-2, -3), PV_Random(36, 2 * 36), particles, 5);
		}
	}
	return;
}


/*-- Digging --*/

public func IsDigging()
{
	return GetAction() == "Dig";
}

public func StartDig()
{
	SetComDir(COMD_None);
	if (!GetEffect("IntDigging", this))
		CreateEffect(IntDigging, 1, 1);
	return;
}

public func StopDig()
{
	if (!IsDigging()) 
		RemoveEffect("IntDigging", this);
	return;
}

public func SetDigDirection(int x, int y)
{
	var effect = GetEffect("IntDigging", this);
	if (effect)
	{
		effect.dig_x = x;
		effect.dig_y = y;
	}
	return;
}

public func GetDigDirection()
{
	var effect = GetEffect("IntDigging", this);
	if (effect)
	{
		return { x = effect.x_dir, y = effect.y_dir };
	}
	return;
}

local IntDigging = new Effect {
    Start = func(int temp) {
        if (temp)
		    return FX_OK;
	    var dir = -1;
	    if (this.Target->GetDir() == DIR_Right) 
		    dir = +1;
	    this.anim_number = this.Target->PlayAnimation("Dig", 5, Anim_X(0, 0, GetAnimationLength("Dig"), 50 * dir), Anim_Const(1000));
	    this.x_dir = 0;
	    this.y_dir = 0;
	    return FX_OK;
    },
    
    Timer = func(int time) {
        if (this.Target->GetComDir() == COMD_Stop)
	    {
		    this.Target->SetXDir(0);
		    this.Target->SetYDir(0);
		    return FX_OK;
	    }
	    var xdir = this.dig_x * this.Target.ActMap.Dig.Speed;
	    var ydir = this.dig_y * this.Target.ActMap.Dig.Speed;
	    var speed = Max(1, Distance(xdir, ydir));
	    xdir = xdir * this.Target.ActMap.Dig.Speed / speed;
	    ydir = ydir * this.Target.ActMap.Dig.Speed / speed;
	    if (xdir != this.x_dir || ydir != this.y_dir)
	    {
	    	this.x_dir = xdir;
	    	this.y_dir = ydir;
	    	this.Target->SetXDir(xdir, 100);
	    	this.Target->SetYDir(ydir, 100);
	    }
	    return FX_OK;
    },
    
    Stop = func(int reason, bool temp) {
        if (temp)
		    return FX_OK;
	    this.Target->StopAnimation(this.Target.IntDigging.anim_number);
	    return FX_OK;
    },
};

public func DigOutObject(object obj)
{
	// Wipf's do not dig out bucket materials like earth.
	if (obj->~IsBucketMaterial())
		return obj->RemoveObject();
	return;
}


/*-- Eating --*/

public func IsEating()
{
	return GetAction() == "Eat";
}

public func Eat(object food)
{
	if (IsWalking() || IsStanding())
	{
		DoEnergy(food->~NutritionalValue());
		food->RemoveObject();
		Sound("Clonk::Action::Munch?");
		SetAction("Eat");
	}
	return;
}

public func StartEat()
{
	PlayAnimation("Nibble", 5, Anim_Linear(0, 0, GetAnimationLength("Nibble"), 36, ANIM_Remove), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	return;
}


/*-- Hanging --*/

public func IsHanging()
{
	return GetAction() == "Hang";
}

public func StartHang()
{
	if (!GetEffect("IntHanging", this))
		CreateEffect(IntHanging, 1, 1);

	return;
}

public func StopHang()
{
	if (!IsStanding()) 
		RemoveEffect("IntHanging", this);
	return;
}

local IntHangling = new Effect {
    Start = func(int temp) {
        if (temp)
		    return FX_OK;
	    this.Target->SetComDir(COMD_Stop);
	    this.current_transform = this.Target.MeshTransformation;
	    this.Target.MeshTransformation = Trans_Mul(this.Target.MeshTransformation, Trans_Translate(-750, -1000, 0), Trans_Rotate(90, 0, 1, 0), Trans_Rotate(35, 0, 0, 1));
	    this.anim_number = this.Target->PlayAnimation("Idle", 5, Anim_Linear(0, 0, GetAnimationLength("Idle"), 35, ANIM_Hold), Anim_Const(1000));
	    return FX_OK;
    },
    
    Timer = func(int time) { return FX_OK; }
    
    Stop = func(int reason, bool temp) {
        if (temp)
		    return FX_OK;
	    this.Target.MeshTransformation = this.current_transform;
	    this.Target->StopAnimation(this.Target.IntHangling.anim_number);
	    return FX_OK;
    },
};


/*-- Dead --*/

public func IsDead()
{
	return GetAction() == "Dead";
}

protected func Death(int killed_by)
{
	Sound("Animals::Wipf::Aroof");
	return;
}

public func StartDead()
{
	PlayAnimation("Dead", 5, Anim_Linear(0, 0, GetAnimationLength("Dead"), 16, ANIM_Hold));
	return;
}


/*-- Being Carried --*/

public func Entrance()
{
	SetAction("Stand");
	return;
}

public func Departure()
{
	SetAction("Jump");
	return;
}


/*-- Other Actions --*/

protected func CatchBlow()
{
	if (IsDead()) 
		return;
	Sound("Animals::Wipf::Weep?");
	return;
}


/*-- Carry Heavy --*/

public func GetCarryTransform(clonk)
{
	if (GetCarrySpecial(clonk))
		return Trans_Mul(Trans_Scale(1300), Trans_Rotate(180, 0, 1, 0), Trans_Rotate(180, 1, 0, 0), Trans_Rotate(20, 0, 0, 1), Trans_Translate(0, -5000, 0));
		
	return Trans_Mul(Trans_Scale(1300), Trans_Rotate(90, 1, 0, 0), Trans_Rotate(160, 0, 0, 1));
}

public func GetCarryPhase() { return 450; }

public func GetCarryBone() { return "spine lower"; }


/*-- DefCore --*/

local Name = "$Name$";
local Description = "$Description$";
local MaxEnergy = 50000;
local MaxBreath = 720;
local NoBurnDecay = true;
local Collectible = true;
local ContactIncinerate = 10;
local BorderBound = C4D_Border_Sides;
local ContactCalls = true;

protected func Definition(proplist def)
{
	def.MeshTransformation = Trans_Mul(Trans_Scale(1300), Trans_Translate(0, 2500, 0));
	def.PictureTransformation = Trans_Mul(Trans_Rotate(65, 0, 1, 0), Trans_Rotate(-20, 0, 0, 1));
	return _inherited(def, ...);
}


/*-- ActMap --*/

local ActMap = {
	Walk = {
		Prototype = Action,
		Name = "Walk",
		Procedure = DFA_WALK,
		Accel = 16,
		Decel = 24,
		Speed = 200,
		Directions = 2,
		FlipDir = 1,
		Length = 1,
		Delay = 0,
		X = 0,
		Y = 0,
		Wdt = 14,
		Hgt = 14,
		StartCall = "StartWalk",
		AbortCall = "StopWalk",
		EndCall = "StopWalk",
		InLiquidAction = "Swim",
	},
	Stand = {
		Prototype = Action,
		Name = "Stand",
		Procedure = DFA_THROW,
		Directions = 2,
		FlipDir = 1,
		Length = 1,
		Delay = 0,
		X = 0,
		Y = 0,
		Wdt = 14,
		Hgt = 14,
		StartCall = "StartStand",
		AbortCall = "StopStand",
		EndCall = "StopStand",
		InLiquidAction = "Swim",
	},
	Dig = {
		Prototype = Action,
		Name = "Dig",
		Procedure = DFA_DIG,
		Accel = 10,
		Decel = 10,
		Speed = 80,
		Directions = 2,
		FlipDir = 1,
		Length = 28,
		Delay = 1,
		X = 0,
		Y = 0,
		Wdt = 14,
		Hgt = 14,
		DigFree = 8,
		NextAction = "Dig",
		StartCall = "StartDig",
		AbortCall = "StopDig",
		InLiquidAction = "Swim",
	},
	Swim = {
		Prototype = Action,
		Name = "Swim",
		Procedure = DFA_SWIM,
		Speed = 80,
		Accel = 8,
		Decel = 12,
		Directions = 2,
		FlipDir = 1,
		Length = 1,
		Delay = 0,
		X = 0,
		Y = 0,
		Wdt = 14,
		Hgt = 14,
		StartCall = "StartSwim",
		AbortCall = "StopSwim",
		EndCall = "StopSwim",
	},
	Jump = {
		Prototype = Action,
		Name = "Jump",
		Procedure = DFA_FLIGHT,
		Speed = 240,
		Accel = 14,
		Directions = 2,
		FlipDir = 1,
		Length = 1,
		Delay = 0,
		X = 0,
		Y = 0,
		Wdt = 14,
		Hgt = 14,
		PhaseCall = "CheckStuck",
		StartCall = "StartJump",
		EndCall = "StopJump",
		AbortCall = "StopJump",
		InLiquidAction = "Swim",
	},
	Dead = {
		Prototype = Action,
		Name = "Dead",
		Directions = 2,
		X = 0,
		Y = 0,
		Wdt = 14,
		Hgt = 14,
		Length = 1,
		Delay = 0,
		NextAction = "Hold",
		StartCall = "StartDead",
		NoOtherAction = 1,
		ObjectDisabled = 1,
	},
	Eat = {
		Prototype = Action,
		Name = "Eat",
		Procedure = DFA_THROW,
		Directions = 2,
		Length = 1,
		Delay = 36,
		X = 0,
		Y = 0,
		Wdt = 14,
		Hgt = 14,
		StartCall = "StartEat",
		NextAction = "Walk",
		InLiquidAction = "Swim",
	},
	Hang = {
		Prototype = Action,
		Name = "Hang",
		Procedure = DFA_ATTACH,
		Directions = 2,
		Length = 1,
		Delay = 0,
		X = 0,
		Y = 0,
		Wdt = 14,
		Hgt = 14,
		StartCall = "StartHang",
		AbortCall = "StopHang",
		InLiquidAction = "Swim",
	},
};