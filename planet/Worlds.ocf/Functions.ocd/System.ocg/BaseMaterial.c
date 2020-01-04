/**
	Base Material 
	Provides global functions to give players base material for scenarios.
	
	@author Maikel
*/

// Gives the player elementary base materials.
global func GivePlayerElementaryBaseMaterial(int plr)
{
	// Production of material of clonk.
	// Production of material of bread.
	GivePlayerBaseMaterial(plr, [[Clonk, 5, 2], [Bread, 5, 2]]);
	return;
}

// Gives the player some basic tools.
global func GivePlayerToolsBaseMaterial(int plr)
{
	// List of tools with [id, mat, prod] entries.
	var materials = [
		[Shovel, 4, 2],
		[Hammer, 2, 1],
		[Axe, 2, 1],
		[Pickaxe, 2, 1],
		[Sickle, 2, 1],
		[Loam, 4, 2],
		[Dynamite, 4, 2]
	];
	GivePlayerBaseMaterial(plr, materials);
	return;
}
