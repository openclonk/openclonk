/*-- Coniferous Tree --*/

#include Library_Plant

private func SeedAreaSize() { return 500; }
private func SeedAmount() { return 10; }

private func Initialize()
{
	SetProperty("MeshTransformation", Trans_Rotate(RandomX(0,359),0,1,0));
}

func Definition(def) {
	SetProperty("Name", "$Name$", def);
}