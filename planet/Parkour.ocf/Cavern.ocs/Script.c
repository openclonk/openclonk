/*-- 
	Cool Cavern
	Author: Maikel
	
	An upwards parkour where basic clonking skills are required to get out of cold cavern.
--*/

protected func Initialize()
{
	// Create the parkour goal.
	var goal = CreateObject(Goal_Parkour, 0, 0, NO_OWNER);
	// Set start point.
	var x, y, pos;
	var d = 100;
	while (!(pos = FindPosInMat("Sky", 0, LandscapeHeight() - d - 40, LandscapeWidth(), 40, 20)) && d < LandscapeHeight())
		d += 10;
	x = pos[0]; y = pos[1];
	goal->SetStartpoint(x, y);
	// Set some checkpoints.
	while (d < LandscapeHeight() - 350)
	{
		// All checkpoints ordered, only respawn in tunnel.
		var mode = PARKOUR_CP_Check | PARKOUR_CP_Ordered;
		d += RandomX(150, 250);
		if (!(pos = FindPosInMat("Tunnel", 0, LandscapeHeight() - d - 80, LandscapeWidth(), 80, 20)) || !Random(3))
			pos = FindPosInMat("Sky", 0, LandscapeHeight() - d - 80, LandscapeWidth(), 80, 20);
		else
			mode = mode | PARKOUR_CP_Respawn;
		if (!pos)
			continue;
		x = pos[0]; y = pos[1];
		goal->AddCheckpoint(x, y, mode);
	}
	// Set finish point.
	d = 0;
	while (!(pos = FindPosInMat("Sky", 0, 20 + d, LandscapeWidth(), 40, 20)) && d < LandscapeHeight())
		d += 10;
	x = pos[0]; y = pos[1];
	goal->SetFinishpoint(x, y);
	// Place chests.
	d = 300;
	while (d < LandscapeHeight() - 300)
	{
		var i = 0;
		while (!(pos = FindPosInMat("Tunnel", 0, d, LandscapeWidth(), 300, 15)) && i < 25)
			i++; // Max 25 attempts.
		if (!pos)
			continue;
		x = pos[0]; y = pos[1];
		CreateObjectAbove(Chest, x, y + 8, NO_OWNER);
		d += RandomX(250, 300);
	}
	// Fill chests.
	var content_list = [GrappleBow, DynamiteBox, Ropeladder, Boompack, Loam, Torch];
	for (var chest in FindObjects(Find_ID(Chest)))
		for (var i = 0; i < 4; i++)
			chest->CreateContents(content_list[Random(GetLength(content_list))]);
	// Create Disasters.
	Earthquake->SetChance(50);
	// Snow
	AddEffect("IntSnow", nil, 100, 1);
	return;
}

protected func FindPosInMat(string mat, int rx, int ry, int wdt, int hgt, int size)
{
	for (var i = 0; i < 500; i++)
	{
		var x = rx + Random(wdt);
		var y = ry + Random(hgt);
		if (GetMaterial(AbsX(x), AbsY(y)) == Material(mat) &&
			GetMaterial(AbsX(x + size), AbsY(y + size)) == Material(mat) &&
			GetMaterial(AbsX(x + size), AbsY(y-size)) == Material(mat) &&
			GetMaterial(AbsX(x-size), AbsY(y-size)) == Material(mat) &&
			GetMaterial(AbsX(x-size), AbsY(y + size)) == Material(mat))
		{
			return [x, y];
		}
	}
	return;
}

// Gamecall from parkour goal, on respawning.
protected func OnPlayerRespawn(int plr, object cp)
{
	var clonk = GetCrew(plr);
	clonk->CreateContents(Shovel);
	clonk->CreateContents(GrappleBow);
	clonk->CreateContents(Loam);
	return;
}

// Gamecall from parkour goal, on reaching a bonus cp.
protected func GivePlrBonus(int plr, object cp)
{
	// No bonus.
	return;
}

// Snow effect.
global func FxIntSnowStart(object target, effect)
{
	effect.snowpos_x = 0;
	return FX_OK;
}

global func FxIntSnowTimer(object target, effect)
{
	effect.snowpos_x += RandomX(-10, 12);
	effect.snowpos_x %= LandscapeWidth();
	if (GetMaterial(effect.snowpos_x, 1) != Material("Sky"))
		return FX_OK;
	if (Random(3))
		return FX_OK;
	CastPXS("Snow", RandomX(8, 16), 10, effect.snowpos_x, 1, RandomX(160, 200));
	return FX_OK;
}
