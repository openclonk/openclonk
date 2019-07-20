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

local transform; // for debugging

/*-- Engine Callbacks --*/

func Initialize()
{
	SetState(TRCH_Normal);
	SetMeshMaterial("Torch");
}

func Hit()
{
	Sound("Hits::Materials::Wood::WoodHit?");
}

public func Selection(object container)
{
	if (container && container->~IsClonk())
	{
		SetState(TRCH_InHand);
	}
	return _inherited(container, ...);
}

public func Deselection(object container)
{
	if (container->~IsClonk())
	{
		SetState(TRCH_Normal);
	}
	return _inherited(container, ...);
}

public func SaveScenarioObject(proplist props, ...)
{
	if (!_inherited(props, ...)) return false;
	
	props->AddCall("SetState", this, "SetState", state);
	if (state == TRCH_Attached || state == TRCH_Fixed)
	{
		props->AddCall("Attach", this, "AttachToWall", state == TRCH_Fixed);
		props->Remove("Category");
		props->Remove("Plane");
	}
	return true;
}

/*-- Callbacks --*/

// Returns whether the torch currently is a source of light.
public func IsLightSource()
{
	return !!GetEffect("IntBurning", this);
}

public func IsInteractable(object clonk)
{
	return state == TRCH_Attached;
}

public func GetInteractionMetaInfo(object clonk)
{
	return { Description = "$MsgTorchDetach$", IconName = nil, IconID = nil, Selected = false };
}

/*-- Usage --*/

public func RejectUse(object clonk)
{
	return !clonk->HasHandAction();
}

public func ControlUse(object clonk)
{
	// Attach the torch if the clonk stands in front of tunnel material.
	if (GetMaterial() == Material("Tunnel"))	
	{
		AttachToWall();
		return true;
	}
	// Otherwise log a message about where one can attach torches.
	Message("$MsgTorchAttach$");
	return true;
}

public func Interact(object clonk)
{
	// detach the torch from the wall.
	DetachFromWall();
	clonk->Collect(this, false, nil, true);
	return true;
}

// Attaches the torch to the wall.
public func AttachToWall(bool fixed)
{
	// Exit the torch and make it a non-collectible static back, also change its state.
	if (Contained()) Exit(0, 3);
	SetCategory(C4D_StaticBack);
	this.Collectible = false;
	SetState(TRCH_Attached);
	if (fixed)
		SetState(TRCH_Fixed);
	// Set plane so that it is in the background.
	this.Plane = 1;
	// Rotate the head of the torch a little into the screen.
	this.MeshTransformation = Trans_Rotate(-20, 1, 0, 0);
	return;
}

// Detaches the torch from the wall.
public func DetachFromWall()
{
	// Make the torch a collectible object, also change its state.
	SetCategory(C4D_Object);
	this.Collectible = true;
	SetState(TRCH_Normal);
	return;
}

// Set the state of the torch.
public func SetState(int to_state)
{
	state = to_state;
	if (state == TRCH_Normal) Burn(false);
	else Burn(true);
	return;
}

/*-- Burning Effect --*/

func FxIntBurningStart(object target, effect fx, int temporary)
{
	if (temporary)
		return 1;
	// Starting interval
	fx.Interval = 4;
	// Fire particle
	fx.flame = 
	{
		R = PV_KeyFrames(0, 0, 0, 200, 255, 800, 255, 1000, 255),
		G = PV_KeyFrames(0, 0, 0, 200, 210, 800, 70, 1000, 70),
		B = PV_KeyFrames(0, 0, 255, 200, 100, 800, 0, 1000, 0),
		
		Alpha = PV_KeyFrames(1000, 0, 0, 10, 255, 500, 255, 1000, 0),
		Size = PV_Linear(PV_Random(2, 3), PV_Random(4, 5)),
		Stretch = 1000,
		Phase = PV_Random(0, 4),
		Rotation = PV_Random(0, 359),
		DampingX = 900,
		DampingY = 1000,
		BlitMode = GFX_BLIT_Additive,
		CollisionVertex = 0,
		OnCollision = PC_Die()
	};
	fx.smoke = 
	{
		Prototype = Particles_Smoke(),
		Size = PV_Linear(PV_Random(2, 3), PV_Random(3, 5))
	};
	// Set the light range for this torch.
	SetLightRange(80, 60);
	SetLightColor(FIRE_LIGHT_COLOR);
	return 1;
}

func FxIntBurningTimer (object target, effect fx, int time)
{
	// If the torched is attached or fixed it should emit some fire and smoke particles.
	if (state != TRCH_Normal)
	{
		if (Contained() && (Contained()->~InLiquid() || !Contained()->~HasHandAction())) return -1;
		var pos = GetParticleOffset();
		// Fire effects.
		CreateParticle("FireSharp", PV_Random(-1 + pos[0], 2 + pos[0]), PV_Random(0 + pos[1], -3 + pos[1]), PV_Random(-2, 2), PV_Random(-3, -5), 10 + Random(3), fx.flame, 12);
		// Smoke effects.
		CreateParticle("Smoke", PV_Random(-1 + pos[0], 2 + pos[0]), PV_Random(-7, -9), PV_Random(-2, 2), PV_Random(-2, 2), 24 + Random(12), fx.smoke, 4);
		// Interval jitter
		if (!Random(10)) fx.Interval = 3 + Random(3);
	}
	return 1;
}

func FxIntBurningStop(object target, proplist effect, int reason, bool temporary)
{
	if (temporary)
		return 1;
	// Remove the light from this torch.
	SetLightRange(0);
	return 1;
}

private func Burn(bool burn)
{
	if (burn)
	{
		if (!GetEffect("IntBurning", this))
		{
			AddEffect("IntBurning", this, 100, 4, this);
		}
	}
	else
	{
		if (GetEffect("IntBurning", this))
		{
			RemoveEffect("IntBurning", this);
		}
	}
}

/*-- Production --*/

public func IsTool() { return true; }
public func IsToolProduct() { return true; }

/*-- Display --*/

public func GetCarryMode(object clonk)
{
	if (clonk && !clonk->~HasHandAction())
	{
		Burn(false);
		return CARRY_Back;
	}
	else
	{
		Burn(true);
		return CARRY_Spear;
	}
}

public func GetCarryTransform(object clonk, int sec, bool nohand)
{
	return Trans_Rotate(90, 0, 0, 1);
}

private func GetParticleOffset()
{
	if (state != TRCH_InHand || !Contained() || !(Contained() && Contained()->~IsClonk())) return [0, 0];
	
	// Special handling for clonks
	var off = [-4, -6];
	if (Contained()->GetDir() == DIR_Right) off[0] = 4;
	
	return off;
}

func Definition(def)
{
	SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(2500, -1500, 0), Trans_Rotate(-30, 0, 0, 1)), def);
}

/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Collectible = true;
local Components = {Wood = 1, Coal = 1};