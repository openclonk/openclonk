/*-- Coconut Tree --*/

#include Library_Plant
#include Library_Tree

private func SeedChance() {	return 100; }
private func SeedArea() { return 400; }
private func SeedAmount() { return 12; }

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
	if(GetCon() < 100) return;
	if(coconuts >= MaxCoconuts) return;

	if (CheckSeedChance())
	{
		var seed = CreateObjectInTreetop(Coconut);
		if (!seed) return;
		coconuts++;
		seed->SetConfinement(this.Confinement);
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

private func Definition(def) 
{
	SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(-27000, -000, 22000), Trans_Rotate(40,0,0,1), Trans_Rotate(-10,1)), def);
}

local Name = "$Name$";
local Touchable = 0;
local BlastIncinerate = 1;
local ContactIncinerate = 3;
local MaxCoconuts = 3;