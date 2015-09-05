/**
	Deployed Balloon
	Helper object for the balloon.	
*/

local rider;
local parent;

protected func Initialize()
{
	SetAction("Inflate");
	SetComDir(COMD_None);
	// Control direction determines the horizontal movement of the balloon.
	var effect = AddEffect("ControlFloat", this, 100, 1, this);
	effect.control_dir = 0;

	// Create some air particles on inflation.
	CreateParticle("Air", PV_Random(-1, 1), PV_Random(15, 17), PV_Random(-3, 3), PV_Random(0, 2), 18, Particles_Air(), 20);
	return;
}

public func SetRider(object clonk)
{
	rider = clonk;
}

public func SetParent(object balloon)
{
	parent = balloon;
}

private func Deflate()
{
	if (GetAction() != "Deflate")
	{
		SetAction("Deflate");
		SetComDir(COMD_None);
	}
	//Schedule(this, "Pack()", 20); //EndCall doesn't work. >:(
}

private func DeflateEffect()
{
	var act_time = GetActTime();
	CreateParticle("Air", PV_Random(-1, 1), PV_Random(-1, 5), PV_Random(-act_time, act_time), PV_Random(-act_time, act_time), 18, Particles_Air(), act_time);
}

private func Pack()
{
	RemoveEffect("NoDrop", parent);
	rider->SetAction("Jump");
	rider->SetSpeed(GetXDir(), GetYDir());
	rider->SetComDir(COMD_Down);
	RemoveObject();
}


/*-- Controls --*/

public func ControlLeft()
{
	var effect = GetEffect("ControlFloat", this);
	if (effect)
		effect.control_dir = -1;
	return true;
}

public func ControlRight()
{
	var effect = GetEffect("ControlFloat", this);
	if (effect)
		effect.control_dir = 1;
	return true;
}

public func ControlStop()
{
	var effect = GetEffect("ControlFloat", this);
	if (effect)
		effect.control_dir = 0;
	return true;
}

public func ControlJump()
{
	Deflate();
	return true;
}

public func FxControlFloatTimer(object target, proplist effect, int time)
{
	var speed = 7;
	if (GetYDir() > speed) SetYDir(GetYDir() - 1);
	if (GetYDir() < speed) SetYDir(GetYDir() + 1);
	if (GetXDir() > speed * 3) SetXDir(GetXDir() - 1);
	if (GetXDir() < -speed * 3) SetXDir(GetXDir() + 1);

	// Forward the control direction into movement.
	SetXDir(GetXDir() + effect.control_dir);

	// Has a bottom vertex hit? Is the balloon stuck in material? Then deflate.
	if (GetContact(-1) & CNAT_Bottom || Stuck()) 
	{
		Deflate();
		return FX_Execute_Kill;
	}
	if (GBackSolid(0, 50) || GBackLiquid(0, 50))
	{
		Deflate();
		return FX_OK;
	}
}

public func IsProjectileTarget()
{
	return true;
}

public func OnProjectileHit()
{
	// Pop the balloon and tumble the rider.
	CreateParticle("Air", 0, -10, PV_Random(-10, 10), PV_Random(-10, 10), 10, Particles_Air(), 30);
	Sound("BalloonPop");
	if (rider)
	{
		rider->SetAction("Tumble");
		rider->SetSpeed(GetXDir(),GetYDir());
	}
	parent->RemoveObject();
	RemoveObject();
}

// Could store and restore the deployed balloon, but all the
// dependencies to be set when recreating this mid-animation
// will probably cause more upwards incompatibilities than benefit
public func SaveScenarioObject() { return false; }


/*-- Properties --*/

local ActMap = {
	Float = {
		Prototype = Action,
		Name = "Float",
		Procedure = DFA_FLOAT,
		Directions = 1,
		Length = 144,
		Delay = 1,
		NextAction = "Float",
		Animation = "Fly",
	},
	Inflate = {
		Prototype = Action,
		Name = "Inflate",
		Procedure = DFA_FLOAT,
		Directions = 1,
		Length = 20,
		Delay = 1,
		NextAction = "Float",
		Animation = "Inflate",
	},
	Deflate = {
		Prototype = Action,
		Name = "Deflate",
		Procedure = DFA_FLOAT,
		Directions = 1,
		Length = 20,
		Delay = 1,
		PhaseCall = "DeflateEffect",
		EndCall = "Pack",
		AbortCall = "Pack",
		NextAction = "Idle",
		Animation = "Deflate",
	},
};
local Name = "$Name$";
