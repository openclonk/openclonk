/*--- Terraflint ---*/

#include Library_CarryHeavy

public func GetCarryMode(clonk) { return CARRY_BothHands; }
public func GetCarryPhase() { return 800; }

func Hit()
{
	Sound("Fuse");
	CreateParticle("Fire", 0, 0, PV_Random(-5, 5), PV_Random(-15, 5), PV_Random(10, 40), Particles_Glimmer(), 5);
}

func Hit2()
{
	Explode(25);
}

public func IsChunk() { return true; }

func Definition(def) {
	SetProperty("PictureTransformation", Trans_Mul(Trans_Rotate(30,0,0,1),Trans_Rotate(-30,1,0,0),Trans_Scale(1300)),def);
}

local Name = "$Name$";
local Description = "$Description$";
local Rebuy = true;
local Touchable = 2;
local Plane = 529; // cause it's explosive, players should see it in a pile of stuff
