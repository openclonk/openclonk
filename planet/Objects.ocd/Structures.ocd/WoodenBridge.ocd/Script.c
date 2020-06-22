/**
	Wooden Bridge
	A bridge which can be placed using the hammer.

	@author Pyrit, Randrian, Maikel	
*/

#include Library_Structure


local connection_left;
local connection_right;

protected func Construction()
{
	SetCategory(C4D_StaticBack);
	return _inherited(...);
}

protected func Initialize()
{
	// Move objects out of the bridge to prevent them being stuck on completion of the bridge.
	 MoveOutOfSolidMask();
	return _inherited(...);
}


/*-- Construction --*/

public func NoConstructionFlip() { return true; }

// Is a construction that is built just below the surface.
public func IsBelowSurfaceConstruction() { return true; }

public func IsHammerBuildable() { return true; }

public func ConstructionCombineWith()
{
	return "ConnectWoodenBridge";
}

public func ConnectWoodenBridge(object previewer)
{
	if (!previewer) 
		return true;
	if (previewer-> GetX() > GetX() && !connection_right)
		return true;
	if (previewer-> GetX() < GetX() && !connection_left)
		return true;
	return false;
}

public func ConstructionCombineDirection() { return CONSTRUCTION_STICK_Left | CONSTRUCTION_STICK_Right; }

public func IsStructureWithoutBasement() { return false; }

/* Called when the wooden bridge construction site is created.
   Returns the parameter "other_bridge", so that you can place
   multiple bridges via script:
   A->CombineWith(CreateObject(B))->...->CombineWith(CreateObject(C))
*/
public func CombineWith(object other_bridge)
{
	// Store the connected bridge.
	SetConnectedBridge(other_bridge);
	other_bridge->SetConnectedBridge(this);
	return other_bridge;
}

public func SetConnectedBridge(object other_bridge)
{
	if (other_bridge->GetX() > GetX())
		connection_right = other_bridge;
	else
		connection_left = other_bridge;
	return;
}

public func RemoveConnectedBridge(object other_bridge)
{
	if (other_bridge == connection_left)
		connection_left = nil;
	if (other_bridge == connection_right)
		connection_right = nil;
	return;
}


/*-- Destruction --*/

public func Destruction()
{
	// Notify connected bridges about destruction.
	if (connection_left)
		connection_left->RemoveConnectedBridge(this);
	if (connection_right)
		connection_right->RemoveConnectedBridge(this);
		
	// Create global particles, so they don't get removed when the bridge is removed.
	Global->CreateParticle("WoodChip", GetX(), GetY() + 5, PV_Random(-15, 15), PV_Random(-13, -6), PV_Random(36 * 3, 36 * 10), Particles_WoodChip(), 20);
	var particles =
	{
		Prototype = Particles_Dust(),
		R = 50, G = 50, B = 50,
		Size = PV_KeyFrames(0, 0, 0, 200, PV_Random(2, 10), 1000, 0),
	};
	for (var cnt = 0; cnt < 24; ++cnt)
	{
		var x = RandomX(-36, 36);
		var y = RandomX(-6, 6);
		Global->CreateParticle("Dust", GetX() + x, GetY() + y, PV_Random(-3, 3), PV_Random(-3, -3), PV_Random(18, 36), particles, 2);
		CastPXS("Ashes", 5, 30, x, y);
	}
	return _inherited(...);
}

public func Damage(int change, int cause, int cause_plr)
{
	// Damaged and burnt bridges appear darker.
	var darkness = 255 - 180 * GetDamage() / this.HitPoints;
	SetClrModulation(RGB(darkness, darkness, darkness));
	// Let the structure library handle the rest.
	return _inherited(change, cause, cause_plr, ...);
}


/*-- Properties --*/

// this.MeshTransformation = Trans_Mul(Trans_Rotate(90, 0, 0, 1), Trans_Scale(1000, 916, 1000), Trans_Translate(-6000, -37800, 0))
local MeshTransformation = [0, -916, 0, 34624, 1000, 0, 0, -6000, 0, 0, 1000, 0];
// def.PictureTransformation = Trans_Mul(Trans_Rotate(90, 0, 0, 1), Trans_Rotate(30, 1, 0, 0), Trans_Rotate(10, 0, 1, 0), Trans_Scale(1280), Trans_Scale(1000, 916, 1000), Trans_Translate(-6000, -12000, 0))
local PictureTransformation = [-111, -1014, 629, 12834, 1260, 0, 222, -7560, -192, 586, 1091, -5880];
local Name = "$Name$";
local Description = "$Description$";
local BlastIncinerate = 2;
local ContactIncinerate = 8;
local NoBurnDecay = true;
local HitPoints = 80;
local Components = {Wood = 3};
