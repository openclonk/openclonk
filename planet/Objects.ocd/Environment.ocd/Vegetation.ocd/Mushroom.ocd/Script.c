/*-- Mushroom --*/

#include Library_Plant

private func SeedChance() { return 250; }
private func SeedAreaSize() { return 100; }
private func SeedAmount() { return 6; }

func Construction()
{
	StartGrowth(3);
}

private func Initialize()
{
	SetProperty("MeshTransformation", Trans_Rotate(RandomX(0,359),0,1,0));
}

public func Interact(object clonk)
{
	//Pick mushroom
	this.Collectible = 1;
	if(clonk->ContentsCount() < clonk->MaxContentsCount())
		Enter(clonk);
}

public func IsInteractable(object clonk)
{
	return clonk->GetProcedure() == "WALK" && GetProperty("Collectible") != 1;
}

public func GetInteractionMetaInfo(object clonk)
{
	return { Description = "$PickMushroom$" };
}

protected func ControlUse(object clonk, int iX, int iY)
{
	clonk->DoEnergy(10);
	RemoveObject();
}

local Name = "$Name$";
local Collectible = 0;
