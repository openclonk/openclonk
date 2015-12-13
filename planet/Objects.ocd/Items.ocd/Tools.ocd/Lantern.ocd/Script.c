/**
	Lantern
	Basic light source. Emits fire when breaking.

	@author Armin, Maikel, Clonkonaut
*/

#include Library_Lamp

private func Construction()
{
	//SetProperty("MeshTransformation", Trans_Mul(Trans_Scale(3500), Trans_Rotate(280,0,1,0)));
}

public func TurnOn()
{
	_inherited();
	SetMeshMaterial("LanternLit", 1);
}

public func TurnOff()
{
	_inherited();
	SetMeshMaterial("LanternGlass", 1);
}

/*-- Ground Hitting --*/

private func Hit()
{
	Sound("Hits::Materials::Glass::GlassHit?");
}

private func Hit2()
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

/*-- Visual --*/

public func GetCarryMode() { return CARRY_HandBack; }
public func GetCarryBone() { return "main"; }
public func GetCarryTransform()
{
	return Trans_Rotate(-90,0,1,0);
}
private func Definition(def)
{
	SetProperty("PictureTransformation", Trans_Mul(Trans_Rotate(280,0,1,0), Trans_Rotate(35,0,0,1), Trans_Rotate(10,1,0,0), Trans_Translate(0,0,250)),def);
}

/** Scenario saving: Mesh material is included in lamp on-state
*/
public func SaveScenarioObject(props, ...)
{
	if (!_inherited(props, ...)) return false;
	props->Remove("MeshMaterial"); // stored by lamp state anyway
	return true;
}


/*-- Status --*/

public func IsTool() { return true; }
public func IsToolProduct() { return true; }

local Name = "$Name$";
local Description = "$Description$";
local UsageHelp = "$UsageHelp$";
local Collectible = true;
