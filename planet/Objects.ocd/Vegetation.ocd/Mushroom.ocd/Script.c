/** 
	Mushroom 
	Can be picked and eaten.
	
	@author
*/

#include Library_Plant
#include Library_Crop

private func SeedChance() { return 600; }
private func SeedArea() { return 150; }
private func SeedAmount() { return 4; }
private func SeedOffset() { return 10; }

/*-- Initialization --*/

protected func Construction()
{
	StartGrowth(3);
	RootSurface();
	this.MeshTransformation = Trans_Mul(Trans_Translate(0, 10000, 0), Trans_Rotate(RandomX(0, 359), 0, 1, 0));
	return _inherited(...);
}

public func RootSurface()
{
	// First move up until unstuck.
	var max_move = 30;
	while (Stuck() && --max_move >= 0)
		SetPosition(GetX(), GetY() - 1);	
	// Then move down until stuck.
	max_move = 30;
	while (!Stuck() && --max_move >= 0)
		SetPosition(GetX(), GetY() + 1);
	return;
}

/*-- Harvesting --*/

private func IsCrop() { return true; }
private func SickleHarvesting() { return false; }

public func IsHarvestable()
{
	// The mushroom is harvestable if it has grown a little.
	return GetCon() >= 50;
}

public func Harvest(object clonk)
{
	this.Collectible = true;
	clonk->Collect(this);
	return true;
}

public func IsInteractable(object clonk)
{
	return !this.Collectible && inherited(clonk);
}

public func GetInteractionMetaInfo(object clonk)
{
	return { Description = "$PickMushroom$" };
}

/*-- Eating --*/

protected func ControlUse(object clonk, int iX, int iY)
{
	clonk->Eat(this);
}

// Nutritional value depends on the completion of the mushroom.
public func NutritionalValue() { return 3 * GetCon() / 20; }

local Name = "$Name$";
local Description = "$Description$";
local UsageHelp = "$UsageHelp$";
local Collectible = false;
local Placement = 4;