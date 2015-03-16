/*-- Coniferous Tree --*/

#include Library_Plant
#include Library_Tree

// Overloaded from the plant library to add the foreground parameter, foreground = true will roughly make every 3rd tree foreground (not the offspring though)
func Place(int amount, proplist rectangle, proplist settings, bool foreground)
{
	// Default behaviour
	var trees = inherited(amount, rectangle, settings);
	if (GetLength(trees) < 1) return trees;

	for (var tree in trees)
		if (!Random(3))
			tree.Plane = 510;
	return trees;
}

private func SeedChance() {	return 500; }
private func SeedArea() { return 400; }
private func SeedAmount() { return 10; }

func Construction()
{
	StartGrowth(5);
	// set random rotation so trees don't look alike too much
	SetProperty("MeshTransformation", Trans_Rotate(RandomX(0,359),0,1,0));
	inherited(...);
}

public func ChopDown()
{
	// Use Special Vertex Mode 1 (see documentation) so the removed vertex won't come back when rotating the tree.
	SetVertex(0, VTX_Y, 0, 1);
	// Remove the bottom vertex
	RemoveVertex(0);

	_inherited(...);
}

protected func Damage(int change, int cause)
{
	_inherited(change, cause, ...);

	if (GetDamage() > MaxDamage() && OnFire())
	{
		var burned = CreateObjectAbove(Tree_Coniferous_Burned, 0, 0, GetOwner());
		burned->SetCategory(GetCategory());
		burned.Touchable = this.Touchable;
		burned->SetCon(GetCon());
		if (burned)
		{
			burned->SetR(GetR());
			burned->Incinerate(OnFire());
			burned->SetPosition(GetX(), GetY());
			Sound("TreeCrack", false);
		}
		RemoveObject();
		return;
	}
	
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

// This is gives buggy results for the mesh.
//protected func Definition(def) 
//{
//	SetProperty("PictureTransformation", Trans_Translate(0, 0, 20000), def);
//}

local Name = "$Name$";
local Touchable = 0;
local BlastIncinerate = 2;
local ContactIncinerate = 6;
local NoBurnDecay = 1;
