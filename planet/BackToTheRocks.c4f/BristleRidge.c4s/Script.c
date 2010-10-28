/*-- 
	Bristle Ridge
	Authors: Mimmo_O, Asmageddon
	
	Parkour on a dynamic map.
--*/

protected func Initialize()
{
	var goal = CreateObject(Goal_Parkour, 0, 0, NO_OWNER);
	var x, y;
	y = LandscapeHeight() - 400;
	x = 10;
	goal->SetStartpoint(x, y);
	var ix,iy;
	var mode = PARKOUR_CP_Check | PARKOUR_CP_Respawn | PARKOUR_CP_Ordered;
	for (var i = 0; i < 5; i++)
	{
		y -= LandscapeHeight()/10;
		x += LandscapeWidth()/6;
		iy = y; ix = x;
		var l = 0, u = 20;
		while (GBackSolid(ix, iy) || GBackSky(ix, iy)) 
		{
			++l; u += 3; 
			iy = y - u + Random(2 * u);
			ix = x - u + Random(2 * u);
			if (l > 100)
				break;
		}
		goal->AddCheckpoint(ix, iy, mode);
		CreateObject(Dynamite, ix, iy, NO_OWNER)->Explode(20);
	}
	x = LandscapeWidth() - 130;
	y = 50;
	goal->SetFinishpoint(x, y);
	SetFoW(false);
	Sound("BirdsLoop.ogg", true, 100, nil, 1);
	CreateObject(Environment_Clouds);
	PlaceGrass(200);
}

protected func PlrHasRespawned(int plr, object cp)
{
	var clonk = GetCrew(plr);
	clonk->CreateContents(GrappleBow);
	clonk->CreateContents(JarOfWinds);
	clonk->CreateContents(Loam);
	clonk->CreateContents(Shovel);
	return;
}
