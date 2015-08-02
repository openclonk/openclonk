/*-- Coconut Tree --*/

#include Library_Plant
#include Library_Tree

private func SeedChance() {	return 100; }
private func SeedArea() { return 400; }
private func SeedAmount() { return 12; }

private func Construction()
{
	inherited(...);
	// -12000 offset to fix model origin which is aligned to geometry centre on export instead of blender's given origin :(
	SetProperty("MeshTransformation", Trans_Mul(Trans_Translate(-12000), Trans_Rotate(RandomX(0,359),0,1,0)));
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

/*-- Properties --*/

private func Definition(def) 
{
	SetProperty("PictureTransformation", Trans_Translate(0, 0, 20000), def);
}

local Name = "$Name$";
local Touchable = 0;
local BlastIncinerate = 1;
local ContactIncinerate = 3;
