/*
	Zaphive

	Author: Clonkonaut
*/

/* Placement */

public func Place(int amount, proplist rectangle)
{
	// No calls to objects, only definitions
	if (GetType(this) == C4V_C4Object) return;
	// Default parameters
	if (!amount) amount = 1;
	if (!rectangle) rectangle = Rectangle(0,0, LandscapeWidth(), LandscapeHeight());

	var trees = FindObjects(Find_InRect(rectangle.x, rectangle.y, rectangle.w, rectangle.h), Find_OCF(OCF_Fullcon), Find_Func("IsTree"), Find_Func("IsStanding"), Sort_Random());
	var hives = CreateArray(), hive;
	while (amount)
	{
		hive = nil;
		for (var tree in trees)
		{
			hive = tree->CreateObjectInTreetop(this);
			if (hive) break;
		}
		if (hive)
			hives[GetLength(hives)] = hive;
		amount--;
	}
	return hives;
}

local tree;

// Called by trees
public func AttachToTree(object to_attach)
{
	tree = to_attach;
	// Move down (this might break placement by the tree but...yeah!)
	SetPosition(GetX(), GetY()+4);
}

// Called by trees
public func DeattachFromTree()
{
	// Fall down
	SetCategory(GetCategory() & ~C4D_StaticBack);
}

/* Creation */

private func Initialize()
{
	SetProperty("MeshTransformation", Trans_Rotate(RandomX(-50,50),0,1,0));
	// Create a little swarm
	var zap = CreateObject(Zap);
	// Could be instantly dead
	if (zap)
		zap->SetHome(this);
}

/* Destruction */

private func Hit2()
{
	// Create an enraged swarm
	var zap = CreateObject(Zap,0, -3);
	// Could be instantly dead
	if (zap)
		zap->SetEnraged();
	RemoveObject();
}

local Name = "$Name$";