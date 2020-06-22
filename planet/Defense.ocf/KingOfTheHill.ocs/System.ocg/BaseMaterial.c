// Base material for the player.

global func GivePlayerBaseMaterial(int plr)
{
	var base_mats = [
		// Clonks & health.
		[Clonk,          20,  20],
		[Bread,          20,  20],
		// Materials & mining.
		[Wood,           16,  12],
		[Metal,          16,  12],
		[Rock,           16,  12],
		[Ore,            16,  12],
		[Coal,           16,  12],
		[Loam,           12,  8],
		[Firestone,      12,  8],
		[Cloth,           8,  4],
		[Dynamite,        8,  4],
		[DynamiteBox,     8,  4],
		// Essential tools.
		[Shovel,          4,  4],
		[Hammer,          2,  2],
		[Axe,             2,  2],
		[Pickaxe,         2,  2],
		// Additional tools.
		[Barrel,          2,  2],
		[MetalBarrel,     2,  2],
		[Bucket,          2,  2],
		[Torch,           2,  2],
		[Pipe,            2,  2],
		[WallKit,         1,  1],
		// Advanced tools.
		[Ropeladder,      2,  1],
		[GrappleBow,      2,  1],
		[Balloon,         2,  1],
		[TeleGlove,       1,  1],
		[WindBag,         1,  1],
		[Boompack,        1,  1],
		// Weapons.
		[Bow,             2,  1],
		[Arrow,          12,  8],
		[Shield,          2,  1],
		[Javelin,         2,  1],
		[Club,            2,  1],
		[Sword,           2,  1],
		[Helmet,          2,  1],
		// Advanced weapons.
		[FireArrow,       6,  4],
		[BombArrow,       6,  4],
		[Blunderbuss,     2,  1],
		[LeadBullet,      8,  8],
		[GrenadeLauncher, 1,  1],
		[IronBomb,        6,  6],
		[SmokeBomb,       2,  2],
		[Lantern,         2,  2],
		[Catapult,        1,  1],
		[PowderKeg,       4,  1],
		[Cannon,          1,  1]
	];
	for (var mat in base_mats)
	{
		DoBaseMaterial(plr, mat[0], mat[1]);
		DoBaseProduction(plr, mat[0], mat[2]);
	}
	return;
}
