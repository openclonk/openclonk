/**
	Flower
	The beauty in nature

	@author Nachtfalter, Armin
*/

#include Library_Plant

local is_explicit_skin = false;

local plant_seed_chance = 33;
local plant_seed_area = 120;
local plant_seed_amount = 6;
local plant_seed_offset = 5;

public func Construction()
{
	StartGrowth(1);
	SetSkin(Random(4));
	is_explicit_skin = false;
	SetProperty("MeshTransformation", Trans_Mul(Trans_Scale(RandomX(850, 1200)), Trans_Rotate(RandomX(0, 359),0, 1, 0)));
	
	inherited(...);
}

// Set one of four possible flower skins (skin from 0 to 3)
// Also makes the skin explicit for saved scenarios
public func SetSkin(int skin)
{
	var skin_name = "flower";
	if (skin) skin_name = Format("flower%d", skin);
	SetMeshMaterial(skin_name);
}

public func SetMeshMaterial(...)
{
	is_explicit_skin = true;
	return inherited(...);
}

public func Incineration()
{
	CreateParticle("Grass", 0, 0, PV_Random(-20, 20), PV_Random(-20, 10), PV_Random(30, 100), Particles_Straw(), 10);
	RemoveObject();
}

public func SaveScenarioObject(proplist props, ...)
{
	if (!inherited(props, ...)) return false;
	// Do not set automatic skin, but set it if made explicit (or e.g. an external flower skin was used)
	if (!is_explicit_skin)
		props->Remove("MeshMaterial");
	else
	{
		var mat = GetMeshMaterial();
		var flower_skins = ["flower", "flower1", "flower2", "flower3"];
		var skin = GetIndexOf(flower_skins, mat);
		if (skin >= 0)
		{
			props->Remove("MeshMaterial");
			props->AddCall("MeshMaterial", this, "SetSkin", skin);
		}
	}
	return true;
}

local Name = "$Name$";
local BlastIncinerate = 1;
local ContactIncinerate = 3;
local Placement = 4;
