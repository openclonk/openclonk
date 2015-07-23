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

	// Find number of plants in seed area.
	var size = SeedArea();
	var amount = SeedAmount();
	var area = Rectangle(size / -2, size / -2, size, size);
	if (this.Confinement)
		area = RectangleEnsureWithin(area, this.Confinement);
	var plant_cnt = ObjectCount(Find_ID(GetID()), Find_InRect(area.x, area.y, area.w, area.h));
	// If there are not much plants in the seed area compared to seed amount
	// the chance of seeding is improved, if there are much the chance is reduced.
	var chance = SeedChance();
	var chance = chance / Max(1, amount - plant_cnt) + chance * Max(0, plant_cnt - amount);
	// Place a coconut
	if (!Random(chance) && GetCon() >= 100)
	{
		var seed = CreateObjectAbove(Coconut, 0, -35);
		seed->SetXDir(-5 + Random(11));
		seed->SetR(Random(360));
		seed->SetRDir(RandomX(-5,5));

		//one coconut for each tree
		if(ObjectCount(Find_ID(Coconut)) > ObjectCount(Find_ID(Tree_Coconut)))
		{
			seed->RemoveObject();
		}
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
