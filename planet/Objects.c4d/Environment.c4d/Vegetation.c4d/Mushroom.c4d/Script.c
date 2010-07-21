/*-- Mushroom --*/

#include Library_Plant

private func SeedChance() { return 250; }
private func SeedAreaSize() { return 100; }
private func SeedAmount() { return 6; }

private func Initialize()
{
	SetProperty("MeshTransformation", Trans_Rotate(RandomX(0,359),0,1,0));
}

protected func ControlUse(object clonk, int iX, int iY)
{
	clonk->DoEnergy(10);
	RemoveObject();
}

func Definition(def) {
	SetProperty("Name", "$Name$", def);
}