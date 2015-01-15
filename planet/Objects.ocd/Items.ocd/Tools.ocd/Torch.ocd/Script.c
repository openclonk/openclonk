/**
	Torch
	Can be either held or attached to tunnel material to illuminate dark caves.
	
	@author Pyrit, Maikel
 --*/


// State of the torch: Normal (contained, lying around), in hand or attached.
local state;
local TRCH_Normal   = 0;
local TRCH_InHand   = 1;
local TRCH_Attached = 2;
local TRCH_Fixed    = 3;

protected func Initialize()
{
	local state = TRCH_Normal;
	SetMeshMaterial("Torch");
	return;
}

private func Hit()
{
	Sound("WoodHit?");
	return;
}

public func GetCarryMode() { return CARRY_HandBack; }

public func IsWorkshopProduct() { return true; }
public func IsTool() { return true; }
public func IsToolProduct() { return true; }


/*-- Usage --*/

public func ControlUse(object clonk)
{
	// Only do something if the clonk can do an action.
	if (!clonk->HasHandAction())
		return true;
	// Attach the torch if the clonk stands in front of tunnel material.
	if (GetMaterial() == Material("Tunnel"))	
	{
		// Do an attach animation. 
		clonk->DoKneel(); // For now kneel.
		// Attach the torch to the wall.
		AttachToWall();
		return true;
	}
	// Otherwise log a message about where one can attach torches.
	Message("$MsgTorchAttach$");
	return true;
}

public func IsInteractable(object clonk)
{
	return state == TRCH_Attached;
}

func GetInteractionMetaInfo(object clonk)
{
	return { Description = "$MsgTorchDetach$", IconName = nil, IconID = nil, Selected = false };
}

public func Interact(object clonk)
{
	// Do an detach animation. 
	clonk->DoKneel(); // For now kneel.
	// detach the torch from the wall.
	DetachFromWall();
	clonk->Collect(this, false, nil, true);
	return true;
}

// Attaches the torch to the wall.
public func AttachToWall(bool fixed)
{
	// Exit the torch and make it a non-collectible static back, also change its state.
	Exit(0, 3);
	SetCategory(C4D_StaticBack);
	this.Collectible = false;
	state = TRCH_Attached;
	if (fixed)
		state = TRCH_Fixed;
	// Set plane so that it is in the background.
	this.Plane = 1;
	// Rotate the head of the torch a little into the screen.
	this.MeshTransformation = Trans_Rotate(-20, 1, 0, 0);
	// Add a burning effect if not already done.
	if (!GetEffect("IntBurning", this))
		AddEffect("IntBurning", this, 100, 1, this);
	return;
}

// Detaches the torch from the wall.
public func DetachFromWall()
{
	// Make the torch a collectible object, also change its state.
	SetCategory(C4D_Object);
	this.Collectible = true;
	state = TRCH_Normal;
	// Remove the burning effect if active.
	if (!GetEffect("IntBurning", this))
		RemoveEffect("IntBurning", this);
	return;
}

// Set the state of the torch.
public func SetState(int to_state)
{
	state = to_state;
	return;
}

// Set state on entrance of a clonk.
protected func Entrance(object container)
{
	if (container->~IsClonk())
		state = TRCH_InHand;
	return _inherited(container, ...);
}

// Set state on departure from a clonk.
protected func Departure(object container)
{
	if (container->~IsClonk())
		state = TRCH_Normal;
	return _inherited(container, ...);
}


/*-- Burning Effect --*/

protected func FxIntBurningStart(object target, proplist effect, int temporary)
{
	if (temporary)
		return 1;
	// Ensure the interval is always one frame.
	effect.Interval = 1;
	// Set the light range for this torch.
	SetLightRange(80, 60);
	return 1;
}

protected func FxIntBurningTimer (object target, proplist effect, int time)
{
	// If the torched is attached or fixed it should emit some fire and smoke particles.
	if (state == TRCH_Attached || state == TRCH_Fixed)
	{
		var x = 1;
		var y = -1;
		// Fire effects.
		var particle_fire = Particles_Fire();
		particle_fire.Size = PV_KeyFrames(0, 0, PV_Random(2, 4), 500, 2, 1000, 0);
		CreateParticle("Fire", PV_Random(x - 2, x + 2), PV_Random(y - 2, y + 2), PV_Random(-1, 1), PV_Random(-1, 1), 16 + Random(8), particle_fire, 3);
		// Smoke effects.
		var particle_smoke = Particles_Smoke();
		particle_smoke.Size = PV_Linear(PV_Random(2, 3), PV_Random(3, 5));
		CreateParticle("Smoke", PV_Random(x - 1, x + 1), PV_Random(y - 1, y + 1), PV_Random(-2, 2), PV_Random(-2, 2), 24 + Random(12), particle_smoke, 2);
	}
	return 1;
}

protected func FxIntBurningStop(object target, proplist effect, int reason, bool temporary)
{
	if (temporary)
		return 1;
	// Remove the light from this torch.	
	SetLightRange(0);
	return 1;
}


/*-- Properties --*/

protected func Definition(def) 
{
	SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(2500, -1500, 0), Trans_Rotate(-30, 0, 0, 1)), def);
}

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local Rebuy = true;

