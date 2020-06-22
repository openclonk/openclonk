/**
	Lantern
	Basic light source. Emits fire when breaking.

	@author Armin, Maikel, Clonkonaut
*/

#include Library_Lamp

/*-- Engine Callbacks --*/

func Hit()
{
	Sound("Hits::Materials::Glass::GlassHit?");
}

func Hit2()
{
	// Cast flames on impact.
	for (var i = 0; i < 20; i++)
		CastObjects(Flame, 1, 20, RandomX(-3, 3), RandomX(-4, 0));
	// Cast some particles.
		// TODO?
	// Sound effects.
	Sound("Hits::Materials::Glass::GlassBreak");
	Sound("Fire::Inflame");
	Explode(10, true);
}

/** Scenario saving: Mesh material is included in lamp on-state
*/
public func SaveScenarioObject(props, ...)
{
	if (!_inherited(props, ...)) return false;
	props->Remove("MeshMaterial"); // stored by lamp state anyway
	return true;
}

/*-- Usage --*/

public func TurnOn()
{
	_inherited(...);
	SetMeshMaterial("LanternLit", 1);
}

public func TurnOff()
{
	_inherited(...);
	SetMeshMaterial("LanternGlass", 1);
}

/*-- Production --*/

public func IsTool() { return true; }
public func IsToolProduct() { return true; }

/*-- Display --*/

public func GetCarryMode()
{
	return CARRY_HandBack;
}

public func GetCarryTransform(object clonk, bool idle, bool nohand)
{
	if (nohand)
		return Trans_Mul(Trans_Rotate(-120, 0, 1), Trans_Translate(-2000, 0, -3000));
	return Trans_Rotate(-90, 0, 1, 0);
}

func Definition(def)
{
	SetProperty("PictureTransformation", Trans_Mul(Trans_Rotate(280, 0, 1, 0), Trans_Rotate(35, 0, 0, 1), Trans_Rotate(10, 1, 0, 0), Trans_Translate(0, 0, 250)),def);
}

/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Collectible = true;
local Components = {Firestone = 1, Metal = 1, Coal = 1};