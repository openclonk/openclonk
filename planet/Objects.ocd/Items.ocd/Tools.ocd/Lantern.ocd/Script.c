/**
	Lantern
	Basic light source. Emits fire when breaking.

	@author Armin, Maikel, Clonkonaut
*/

private func Construction()
{
	SetProperty("MeshTransformation", Trans_Mul(Trans_Scale(3500), Trans_Rotate(280,0,1,0)));
}

/*-- Convenient calls --*/

public func TurnOn()
{
	if (lit) return false;
	SetLightRange(80, 60);
	SetMeshMaterial("LanternLit", 1);
	lit = true;
	return true;
}

public func TurnOff()
{
	if (!lit) return false;
	SetLightRange(0, 0);
	SetMeshMaterial("LanternGlass", 1);
	lit = false;
	return true;
}

/*-- Usage --*/

local lit = false;

public func ControlUse(object clonk)
{
	// Only do something if the clonk can do an action.
	if (!clonk->HasHandAction())
		return true;
	// Turn on or off
	if (lit) {
		TurnOff();
	} else {
		TurnOn();
	}
	Sound("Click2");
	return true;
}

/*-- Entrance --*/

local dummy;

public func ForceEntry(object container)
{
	if (container->~LanternPosition())
		return true;
	return false;
}

public func Entrance(object container)
{
	if (container->~LanternPosition())
	{
		dummy = CreateObject(DummyLantern,0,0,NO_OWNER);
		dummy->Set(container, container->LanternPosition(), this);
	}
}

public func Departure()
{
	if (dummy)
		dummy->RemoveObject();
}

public func Destruction()
{
	if (dummy)
		dummy->RemoveObject();
}

/*-- Ground Hitting --*/

private func Hit2()
{
	Sound("GlassHit?");
}

private func Hit2()
{
	// Cast flames on impact.
	for (var i = 0; i < 20; i++)
		CastObjects(Flame, 1, 20, RandomX(-3, 3), RandomX(-4, 0));
	// Cast some particles.
		// TODO?
	// Sound effects.
	Sound("GlassHit?");
	Sound("Inflame");
	Explode(10, true);
}

/*-- Visual --*/

public func GetCarryMode() { return CARRY_HandBack; }
public func GetCarryBone() { return "main"; }
public func GetCarryTransform()
{
	return Trans_Mul(Trans_Scale(3500), Trans_Rotate(280,0,1,0));
}
private func Definition(def)
{
	SetProperty("PictureTransformation", Trans_Mul(Trans_Rotate(280,0,1,0), Trans_Rotate(35,0,0,1), Trans_Rotate(10,1,0,0), Trans_Translate(0,0,250)),def);
}

/*-- Status --*/

public func IsTool() { return true; }
public func IsToolProduct() { return true; }

local Name = "$Name$";
local Description = "$Description$";
local UsageHelp = "$UsageHelp$";
local Collectible = true;