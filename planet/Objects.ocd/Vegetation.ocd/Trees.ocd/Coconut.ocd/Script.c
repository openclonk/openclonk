/*-- Coconut Tree --*/

#include Library_Plant
#include Library_Tree

local plant_seed_chance = 100;
local plant_seed_area = 400;
local plant_seed_amount = 12;
local plant_seed_offset = 30;

local coconuts;
// Saved for GetTreetopPosition
local mesh_rotation;

private func Construction()
{
	inherited(...);
	mesh_rotation = RandomX(0,359);
	// -12000 offset to fix model origin which is aligned to geometry centre on export instead of blender's given origin :(
	SetProperty("MeshTransformation", Trans_Mul(Trans_Translate(-12000), Trans_Rotate(mesh_rotation,0,1,0)));
}

private func Seed()
{
	if(!IsStanding()) return;
	if(OnFire()) return;
	if(GetGrowthValue()) return; // still growing
	// Limit coconuts by attached and dropped coconuts (issue #1916)
	if(coconuts >= MaxCoconuts) return;
	if(ObjectCount(Find_ID(Coconut), Find_InRect(GetLeft(), GetTop(), GetObjWidth(), GetObjHeight())) >= MaxCoconuts) return;

	// Always create coconuts; the coconut will determine if it seeds
	if (Random(10000) < SeedChance())
	{
		var seed = CreateObjectInTreetop(Coconut);
		if (!seed) return;
		coconuts++;
		seed->SetConfinement(this.Confinement);
		seed->SetSeedChance(this->SeedChance());
		seed->SetSeedArea(this->SeedArea());
		seed->SetSeedAmount(this->SeedAmount());
		seed->SetSeedOffset(this->SeedOffset());
		seed.Plane = this.Plane - 2; // coconuts should always be behind the tree
	}
}

public func GetTreetopPosition(pos)
{
	var offset = Sin(mesh_rotation/2, 20);
	return Shape->Rectangle(-25+offset,-25, 30,5)->GetRandomPoint(pos);
}

public func LostCoconut()
{
	coconuts--;
}

/*-- Properties --*/

public func Definition(def, ...) 
{
	SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(-27000, -000, 22000), Trans_Rotate(40,0,0,1), Trans_Rotate(-10,1)), def);
	_inherited(def, ...);
	def.EditorProps.plant_seed_area = nil; // Area doesn't make sense because it's seeding via coconuts
}

local Name = "$Name$";
local Touchable = 0;
local BlastIncinerate = 1;
local ContactIncinerate = 3;
local MaxCoconuts = 3;
local Components = {Wood = 4};