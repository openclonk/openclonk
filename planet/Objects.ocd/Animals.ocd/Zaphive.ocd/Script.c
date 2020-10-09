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
	if (!rectangle) rectangle = Rectangle(0, 0, LandscapeWidth(), LandscapeHeight());

	var trees = FindObjects(Find_InRect(rectangle.x, rectangle.y, rectangle.wdt, rectangle.hgt), Find_OCF(OCF_Fullcon), Find_Func("IsTree"), Find_Func("IsStanding"));
	var hives = CreateArray(), hive;
	while (amount)
	{
		hive = nil;
		ShuffleArray(trees);
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
	// Constant offset. The old comment didn't say why this was necessary.
	var additional_y_offset = 4;
	// Calculate the offset from the tree, taking into account the target vertex' offset.
	var offset_x = GetX() - to_attach->GetX() - to_attach->GetVertex(0, VTX_X);
	var offset_y = GetY() - to_attach->GetY() - to_attach->GetVertex(0, VTX_Y) + additional_y_offset;
	SetAction("Attach", to_attach);
	// And specify/set the vertex to attach.
	SetActionData(4 << 8);
	SetVertexXY(4, -offset_x, -offset_y);
}

// Called by trees
public func DetachFromTree()
{
	AttachTargetLost();
}

public func AttachTargetLost()
{
	// Fall down
	SetAction("Idle");
	SetVertexXY(4, 0, 0);
	SetRDir(RandomX(-3, 3));
}

/* Creation */

private func Initialize()
{
	SetProperty("MeshTransformation", Trans_Rotate(RandomX(-50, 50),0, 1, 0));
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
	var zap = CreateObject(Zap, 0, -3);
	// Could be instantly dead
	if (zap)
		zap->SetEnraged();
	Sound("Hits::OrganicHit?");
	CreateParticle("WoodChip", 0, 0, PV_Random(-2, 2), -4, PV_Random(36 * 3, 36 * 10), Particles_Straw(), 5);
	RemoveObject();
}

local ActMap = {
	Attach = {
		Prototype = Action,
		Procedure = DFA_ATTACH,
		FacetBase = 1,
		Length = 1,
		Delay = 0
	},
};

local Name = "$Name$";