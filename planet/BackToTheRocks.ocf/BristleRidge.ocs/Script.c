/*-- 
	Bristle Ridge
	Authors: Mimmo_O, Asmageddon, Maikel
	
	Parkour on a dynamic map, the player starts on the bottom left and has to make it to the upper right.
	The landscape consists of several pillers seperated by abyss, the player must use the grappler, jar of winds
	and shovel to cover the abyss and climb the pillars.
--*/


protected func Initialize()
{
	// Parkour goal: direction bottom left to upper right.
	var goal = CreateObject(Goal_Parkour, 0, 0, NO_OWNER);
	// Start at bottom left corner of the map.
	var sx = 80, sy = LandscapeHeight() - 350;
	goal->SetStartpoint(sx, sy);
	// Finish at the top right corner of the map.
	var fx = LandscapeWidth() - 150, fy = 80;
	goal->SetFinishpoint(fx, fy);
	// All checkpoints are ordered and provide respawn.
	// Checkpoints form a more or less straight line from start to finish.
	var cp_mode = PARKOUR_CP_Check | PARKOUR_CP_Respawn | PARKOUR_CP_Ordered;
	var cp_count = 5;
	var dx = (fx - sx) / (cp_count + 1);
	var dy = (fy - sy) / (cp_count + 1);
	var x, y;
	// Create cp_count checkpoints.
	for (var cp_num = 1; cp_num <= cp_count; cp_num++)
	{
		x = sx + cp_num * dx;
		y = sy + cp_num * dy;
		// Move around a little to find a decent location (100) tries.
		var move = 15;
		for (var i = 0; i < 100; i++)
		{
			x += Random(2 * move) - move;
			y += Random(2 * move) - move;
			if (!GBackSolid(x, y) && !GBackSky(x, y))
				break;
		}
		goal->AddCheckpoint(x, y, cp_mode);
		CreateObject(Dynamite, x, y, NO_OWNER)->Explode(25);
		CreateObject(Dynamite, x, y, NO_OWNER)->Explode(25);
	}
	
	// Create a little mood.
	Sound("BirdsLoop.ogg", true, 100, nil, 1);
	CreateObject(Environment_Clouds);
	PlaceGrass(200);
	return;
}

// Callback from parkour goal: give the player its tools on respawn.
protected func OnPlayerRespawn(int plr, object cp)
{
	var clonk = GetCrew(plr);
	clonk->CreateContents(GrappleBow);
	clonk->CreateContents(JarOfWinds)->DoFullLoad();
	clonk->CreateContents(Shovel);
	return;
}
