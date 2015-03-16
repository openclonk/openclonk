/*-- Coconut Tree --*/

#include Library_Plant
#include Library_Tree

private func SeedChance() {	return 100; }
private func SeedArea() { return 400; }
private func SeedAmount() { return 12; }

func Construction()
{
	StartGrowth(5);
	// set random rotation so trees don't look alike too much
	// -12000 offset to fix model origin which is aligned to geometry centre on export instead of blender's given origin :(
	SetProperty("MeshTransformation", Trans_Mul(Trans_Translate(-12000), Trans_Rotate(RandomX(0,359),0,1,0)));
	inherited(...);
}

public func Seed()
{
	if(!IsStanding()) return false;

	// Find number of plants in seed area.
	var size = SeedArea();
	var amount = SeedAmount();
	var offset = size / -2;	
	var plant_cnt = ObjectCount(Find_ID(GetID()), Find_InRect(offset, offset, size, size));
	// If there are not much plants in the seed area compared to seed amount
	// the chance of seeding is improved, if there are much the chance is reduced.
	var chance = SeedChance();
//	var chance = chance / Max(1, amount - plant_cnt) + chance * Max(0, plant_cnt - amount);
	// Place a plant if we are lucky, in principle there can be more than seed amount.
	if (!Random(chance) && GetCon() >= 100)
	{
		// Place the plant but check if it is not close to another one.	
//		var plant = PlaceVegetation(GetID(), offset, offset, size, size, 3);
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
	return;
}

public func IsTree() { return true; }

public func ChopDown()
{
	// Remove the bottom vertex
	SetVertex(0, VTX_Y, 0, 1);
	RemoveVertex(0);

	_inherited(...);
}

protected func Damage(int change, int cause)
{
	_inherited(change, cause, ...);

	if (cause == FX_Call_DmgChop && IsStanding())
		ShakeTree();
	return;
}

private func ShakeTree()
{
	var effect = AddEffect("IntShakeTree", this, 100, 1, this);
	effect.current_trans = this.MeshTransformation;
	return;
}

protected func FxIntShakeTreeTimer(object target, proplist effect, int time)
{
	if (time > 24)
		return FX_Execute_Kill;
	var angle = Sin(time * 45, 2);
	var r = Trans_Rotate(angle, 0, 0, 1);
	target.MeshTransformation = Trans_Mul(r, effect.current_trans);
	return FX_OK;
}

/*-- Properties --*/

protected func Definition(def) 
{
	SetProperty("PictureTransformation", Trans_Translate(0, 0, 20000), def);
}

local Name = "$Name$";
local Touchable = 0;
local BlastIncinerate = 1;
local ContactIncinerate = 3;
