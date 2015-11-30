/** 
	Mushroom 
	Can be picked and eaten.
	
	@author
*/

#include Library_Plant

private func SeedChance() { return 600; }
private func SeedArea() { return 150; }
private func SeedAmount() { return 4; }
private func SeedOffset() { return 10; }

private func Incineration()
{
	SetClrModulation(RGB(48, 32, 32));
}

/*-- Initialization --*/

protected func Construction()
{
	StartGrowth(3);
	RootSurface();
	this.MeshTransformation = Trans_Rotate(RandomX(0, 359), 0, 1, 0);
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

/*-- Eating --*/

protected func ControlUse(object clonk)
{
	clonk->Eat(this);
	return true;
}

// Nutritional value depends on the completion of the mushroom.
public func NutritionalValue() { return GetCon() / 10; }

local Name = "$Name$";
local Description = "$Description$";
local UsageHelp = "$UsageHelp$";
local Collectible = true;
local BlastIncinerate = 5;
local ContactIncinerate = 1;
local Placement = 4;