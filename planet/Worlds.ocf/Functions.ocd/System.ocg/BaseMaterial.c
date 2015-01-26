/**
	Base Material 
	Provides global functions to give players base material for scenarios.
	
	@author Maikel
*/

// Gives the player specific base materials as given in the materials array.
global func GivePlayerSpecificBaseMaterial(int plr, array materials)
{
	for (var mat in materials)
	{
		SetBaseMaterial(plr, mat[0], mat[1]);
		SetBaseProduction(plr, mat[0], mat[2]);
	}
	return;
}

// Gives the player elementary base materials.
global func GivePlayerElementaryBaseMaterial(int plr)
{
	// Production of material of clonk.
	SetBaseMaterial(plr, Clonk, 5);
	SetBaseProduction(plr, Clonk, 2);
	
	// Production of material of bread.
	SetBaseMaterial(plr, Bread, 5);
	SetBaseProduction(plr, Bread, 2);
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
		[Loam, 4, 2],
		[Dynamite, 4, 2]
	];
	for (var mat in materials)
	{
		SetBaseMaterial(plr, mat[0], mat[1]);
		SetBaseProduction(plr, mat[0], mat[2]);
	}
	return;
}