/*-- The Cavern --*/

protected func Initialize()
{
	// Create the parkour goal.
	var goal = CreateObject(Goal_Parkour, 0, 0, NO_OWNER);
	// Set start point.
	var x, y;
	var d = 100;
	while(!FindPosInMat(x, y, "Sky", 0, LandscapeHeight() - d - 40, LandscapeWidth(), 40, 20) && d < LandscapeHeight())
		d += 10;
	goal->SetStartpoint(x, y);
	// Set some checkpoints.
	while (d < LandscapeHeight() - 350)
	{
		var mode = PARKOUR_CP_Check;
		d += RandomX(150, 250);
		if (!FindPosInMat(x, y, "Tunnel", 0, LandscapeHeight() - d - 80, LandscapeWidth(), 80, 20) || !Random(3))
			FindPosInMat(x, y, "Sky", 0, LandscapeHeight() - d - 80, LandscapeWidth(), 80, 20);
		else
			mode = mode | PARKOUR_CP_Respawn;
		// All checkpoints ordered.
		mode = mode | PARKOUR_CP_Ordered;
		goal->AddCheckpoint(x, y, mode);
	}
	// Set finish point.
	d = 0;
	while(!FindPosInMat(x, y, "Sky", 0, 20 + d, LandscapeWidth(), 40, 20) && d < LandscapeHeight())
		d += 10;
	goal->SetFinishpoint(x, y);
	// Place chests.
	d = 300; 
	while (d < LandscapeHeight() - 300)
	{
		var i = 0;
		while (!FindPosInMat(x, y, "Tunnel", 0, d, LandscapeWidth(), 300, 15) && i < 25)
			i++; // Max 25 attempts.
		CreateObject(Chest, x, y + 8, NO_OWNER);
		d += RandomX(250, 300); 
	}
	// Fill chests.
	var content_list = [GrappleBow, DynamiteBox, Ropeladder, Boompack, Loam];
	for (var chest in FindObjects(Find_ID(Chest)))
		for (var i = 0; i < 4; i++)
			chest->CreateContents(content_list[Random(GetLength(content_list))]);
	// Create Disasters.
	//CreateObject(Core_Disaster_Earthquake, 0, 0, NO_OWNER)->SetChance(100);
	// Snow
	AddEffect("IntSnow", 0, 100, 1);
	return;
}

protected func FindPosInMat(int &tx, int &ty, string mat, int rx, int ry, int wdt, int hgt, int size)
{
	var x, y;
	for (var i = 0; i < 500; i++)
	{
		x = rx + Random(wdt);
		y = ry + Random(hgt);
		if(GetMaterial(AbsX(x), AbsY(y)) == Material(mat) &&
			GetMaterial(AbsX(x+size), AbsY(y+size)) == Material(mat) &&
			GetMaterial(AbsX(x+size), AbsY(y-size)) == Material(mat) &&
			GetMaterial(AbsX(x-size), AbsY(y-size)) == Material(mat) &&
			GetMaterial(AbsX(x-size), AbsY(y+size)) == Material(mat))
		{
			tx = x; ty = y;
			return true; // Location found.
		}
	}
	return false; // No location found.
}

// Gamecall from parkour goal, on respawning.
protected func PlrHasRespawned(int plr, object cp)
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
global func FxIntSnowStart(object target, int fxnum)
{
	EffectVar(0, nil, fxnum) = 0;
	return FX_OK;
}

global func FxIntSnowTimer(object target, int fxnum)
{
	EffectVar(0, nil, fxnum) += RandomX(-10, 12);
	EffectVar(0, nil, fxnum) %= LandscapeWidth();
	if (GetMaterial(EffectVar(0, nil, fxnum), 1) != Material("Sky"))
		return FX_OK;
	if (Random(3))
		return FX_OK;
	CastPXS("Snow", RandomX(8, 16), 10, EffectVar(0, nil, fxnum), 1, RandomX(160, 200));
	return FX_OK;
}