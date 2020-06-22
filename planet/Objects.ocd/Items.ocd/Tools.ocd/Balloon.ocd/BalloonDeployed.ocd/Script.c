/**
	Deployed Balloon
	Helper object for the balloon.	
*/

local rider;
local cargo, has_cargo = false, old_cargo_action;
local parent;
local deployment_yoffset = 50;

protected func Initialize()
{
	SetAction("Inflate");
	SetComDir(COMD_None);
	// Control direction determines the horizontal movement of the balloon.
	var fx = AddEffect("ControlFloat", this, 100, 1, this);
	fx.control_dir = nil;

	// Create some air particles on inflation.
	CreateParticle("Air", PV_Random(-1, 1), PV_Random(15, 17), PV_Random(-3, 3), PV_Random(0, 2), 18, Particles_Air(), 20);
	return;
}

public func SetRider(object clonk)
{
	rider = clonk;
}

public func SetInflated()
{
	// Skip inflating animation.
	if (GetAction() == "Inflate") 
		SetAction("Float");
}

// Sets a cargo object that is held by the balloon
// (only works with objects that do not themselves have an action)
// attach_vertex: Vertex in attached object to stick to the balloon
// xoff, yoff: Move attachment vertex on balloon by this
// deploy_yoff: Move deployment vertices vertically by this (for e.g. firestones to not drop and explode)
public func SetCargo(object new_cargo, int attach_vertex, int xoff, int yoff, int deploy_yoff)
{
	DropCargo(); // drop any previous
	cargo = new_cargo;
	has_cargo = !!cargo;
	if (has_cargo)
	{
		old_cargo_action = cargo->GetAction();
		if (new_cargo.ActMap)
		{
			if (new_cargo.ActMap == new_cargo.Prototype.ActMap) new_cargo.ActMap = new new_cargo.ActMap {};
		}
		else
		{
			new_cargo.ActMap = { };
		}
		new_cargo.ActMap.BalloonDeployedAttach = new Action {
			Prototype = Action,
			Name = "BalloonDeployedAttach",
			Procedure = DFA_ATTACH,
			Delay = 9999999,
			Length = 1,
			FacetBase = 1,
			};
		cargo->SetAction("BalloonDeployedAttach", this);
		cargo->SetActionData(attach_vertex<<8);
		SetVertex(0, VTX_X, xoff);
		SetVertex(0, VTX_Y, yoff + 30);
		deployment_yoffset = deploy_yoff + 50;
		SetVertex(8, VTX_Y, deploy_yoff + 35);
		SetVertex(9, VTX_Y, deploy_yoff + 30);
		SetVertex(10, VTX_Y, deploy_yoff + 30);
	}
	return true;
}

public func DropCargo()
{
	if (has_cargo && cargo)
	{
		cargo->SetAction(old_cargo_action);
		cargo = nil;
		has_cargo = false;
		return true;
	}
}

public func SetParent(object balloon)
{
	parent = balloon;
}

public func GetParent()
{
	return parent;
}

private func Deflate()
{
	DropCargo();
	if (GetAction() != "Deflate")
	{
		SetAction("Deflate");
		SetComDir(COMD_None);
	}
}

private func DeflateEffect()
{
	var act_time = GetActTime();
	CreateParticle("Air", PV_Random(-1, 1), PV_Random(-1, 5), PV_Random(-act_time, act_time), PV_Random(-act_time, act_time), 18, Particles_Air(), act_time);
	// Release rider before being fully deflated, so the he/she can start scaling/walking right away.
	if (act_time >= 4 && rider)
	{
		rider->SetAction("Jump");
		rider->SetSpeed(GetXDir(), GetYDir());
		rider->SetComDir(COMD_Stop);
		rider = nil;
	}
	return;
}

private func Pack()
{
	// Ensure the rider is released from the balloon.
	if (rider)
	{
		rider->SetAction("Jump");
		rider->SetSpeed(GetXDir(), GetYDir());
		rider->SetComDir(COMD_Stop);
	}
	RemoveObject();
}

public func RejectWindbagForce() { return true; }


/*-- Controls --*/

public func ControlLeft()
{
	var fx = GetEffect("ControlFloat", this);
	if (fx)
		fx.control_dir = -1;
	return true;
}

public func ControlRight()
{
	var fx = GetEffect("ControlFloat", this);
	if (fx)
		fx.control_dir = 1;
	return true;
}

public func ControlDown()
{
	Deflate();
	return true;
}

public func ControlUp()
{
	var fx = GetEffect("ControlFloat", this);
	if (fx)
		fx.control_dir = 0;
	return true;
}

public func FxControlFloatTimer(object target, effect fx, int time)
{
	var speed = 7;
	// Normalize vertical speed.
	if (GetYDir() > speed) SetYDir(GetYDir() - 1);
	if (GetYDir() < speed) SetYDir(GetYDir() + 1);
	
	// Adjust horizontal speed, according to control and max speed.
	var xdir = GetXDir();
	var xdir_dev = fx.control_dir;
	if (xdir > speed * 3)
		xdir_dev = -1;
	else if (xdir < -speed * 3)
		xdir_dev = 1;
	else if (xdir_dev == 0)
	{
		if (xdir > 0)
			xdir_dev = -1;
		if (xdir < 0)
			xdir_dev = 1;	
	}
	SetXDir(xdir + xdir_dev);
	
	// Has a bottom vertex hit? Is the balloon stuck in material? Is the cargo gone? Then deflate.
	if (GetContact(-1) & CNAT_Bottom || Stuck() || (has_cargo && (!cargo || cargo->Contained()))) 
	{
		Deflate();
		return FX_Execute_Kill;
	}
	if (GBackSolid(0, deployment_yoffset) || GBackLiquid(0, deployment_yoffset))
	{
		Deflate();
		return FX_OK;
	}
}


/*-- Floating --*/

// Make the balloon float around its position.
public func MakeFloat()
{
	RemoveEffect("ControlFloat", this);
	AddEffect("FloatBalloon", this, 100, 1, this);
	return;
}

public func FxFloatBalloonTimer(object target, effect fx, int time)
{
	var ysin = time % 360;
	target->SetYDir(Sin(ysin, 2));
	return FX_OK;
}


/*-- Event Handling --*/

// Called when the clonk unmounts for whatever reason.
protected func OnUnmount(object clonk)
{
	// Assume that if the clonk is now tumbling he could not have held on to the balloon.
	if (clonk == rider && clonk->GetAction() == "Tumble")
	{
		// Therefore we drop the balloon.
		if (parent)
			parent->Exit(0, 0, 0, clonk->GetXDir(), clonk->GetYDir());
		rider = nil;
		Deflate();	
	}
	return;
}

public func IsProjectileTarget(object projectile, object shooter)
{
	// If there is no projectile assume it is a general request and thus return true.
	if (!projectile)
		return true;
	// Exclude the bottom triangle edges from the hitbox such that projectiles can be shot while hanging on the balloon.
	var dx = GetX() - projectile->GetX();
	var dy = GetY() - projectile->GetY() + GetBottom();
	return dy > Abs(dx);
}

public func OnProjectileHit(object projectile)
{
	// Pop the balloon and tumble the rider.
	CreateParticle("Air", 0, -10, PV_Random(-10, 10), PV_Random(-10, 10), 10, Particles_Air(), 30);
	Sound("Objects::Balloon::Pop");
	// Remove the parent balloon object as it is destroyed. 
	if (parent)
		parent->RemoveObject();
	// Drop the rider and set its killer in case it tumbles out of the map.
	if (rider)
	{
		rider->SetSpeed(GetXDir(), GetYDir());
		rider->SetKiller(projectile->GetController());
		rider->SetAction("Tumble");
	}
	// Drop anything being transported.
	DropCargo();
	// We're done.
	RemoveObject();
}

public func IsLightningStrikable(object lightning) { return true; }

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
		Length = 10,
		Delay = 1,
		PhaseCall = "DeflateEffect",
		EndCall = "Pack",
		AbortCall = "Pack",
		NextAction = "Idle",
		Animation = "Deflate",
	},
};
local Name = "$Name$";
local BorderBound = C4D_Border_Sides;
local ContactCalls = true;
local Plane = 300;
