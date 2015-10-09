/*-- Coconut Tree --*/

#include Library_Plant
#include Library_Tree

private func SeedChance() {	return 100; }
private func SeedArea() { return 400; }
private func SeedAmount() { return 12; }

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
	if(!IsStanding()) return false;

	// Coconut trees always create coconut seeds
	// The seed area checks are done by the coconut before creating the tree.
	var chance = this->SeedChance();
	if (!Random(chance) && GetCon() >= 100 && ObjectCount(Find_ID(Coconut)) < ObjectCount(Find_ID(Tree_Coconut)))
	{
		var seed = CreateObjectAbove(Coconut, 0, -35);
		seed->SetXDir(-5 + Random(11));
		seed->SetR(Random(360));
		seed->SetRDir(RandomX(-5,5));
		seed->SetConfinement(this.Confinement);
	}
}

public func GetTreetopPosition(pos)
{
	var offset = Sin(mesh_rotation/2, 20);
	return Shape->Rectangle(-45+offset,-25, 50,10)->GetRandomPoint(pos);
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
