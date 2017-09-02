/*--- Coconut ---*/

#include Library_Seed
#include Library_Edible

local lib_seed_plant = Tree_Coconut;
local lib_seed_lifetime = 10000;
local lib_seed_planttime = 2500;
local lib_seed_can_plant_manually = false;

local mother;


/* Hanging state */

private func GetHangingTime() { return 4200; }

public func AttachToTree(object tree)
{
	SetCategory(GetCategory() | C4D_StaticBack);
	mother = tree;
	ScheduleCall(this, "DetachFromTree", 4200 + Random(500));
}

public func DetachFromTree(bool no_bounce)
{
	ClearScheduleCall(this, "DetachFromTree");
	SetCategory(GetCategory() ^ C4D_StaticBack);
	SetXDir(Random(3)-1);
	if (mother) mother->LostCoconut();
	mother = nil;
	AddEffect("Bouncy", this, 1, 175);
}

private func Entrance(...)
{
	if (mother) DetachFromTree(true);
	return _inherited(...);
}

private func Destruction(...)
{
	if (mother) DetachFromTree(true);
	return _inherited(...);
}


/* Custom coconut tree creation */

public func Sprout()
{
	// No duplicate sprout
	if (GetEffect("IntGerminate", this)) return false;
	// Try to sprout a coconut tree.
	var d = 8, tree;
	if (tree = PlaceVegetation(Tree_Coconut, -d/2, -d/2, d, d, 100))
	{
		tree->InitChild(this);
		this.Collectible = 0;
		AddEffect("IntGerminate", this, 100, 1, this); 
		return true;
	}
	return false;
}

public func FxIntGerminateTimer(object coconut, proplist effect, int timer)
{
	// Fade out
	SetObjAlpha(255 - (timer * 255 / 100));
	if (timer == 100)
		coconut->RemoveObject();
	return;
}

/*-- Bounce --*/

protected func Hit(int dx, int dy)
{
	// Bounce: useful for spreading seeds further from parent tree.
	if (dy > 1)
	{
		if (GetEffect("Bouncy", this)) {
			SetXDir(RandomX(-5,5));
			SetYDir(dy * 3 / -4, 100);
		} else
			SetYDir(dy / -2, 100);
	}

	StonyObjectHit(dx, dy);
	return;
}

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local BlastIncinerate = 8;
local ContactIncinerate = 2;
local Confinement;
local ContactCalls = true;
