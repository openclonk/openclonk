/*-- 
	Shiver Peak
	Authors: Ringwaul, Asmageddon
	
	Parkour on a dynamic map.
--*/

func Initialize()
{
	var goal = CreateObject(Goal_Parkour, 0, 0, NO_OWNER);
	var x, y;
	y = LandscapeHeight() - 120;
	x = LandscapeWidth() / 2;
	goal->SetStartpoint(x, y);
	var ix, iy;
	y = 6 * LandscapeHeight() / 7;
	var mode = PARKOUR_CP_Check | PARKOUR_CP_Respawn | PARKOUR_CP_Ordered;
	for (var i = 1; i < 7; i++)
	{
		iy = y - 50 + Random(100);
		ix = x - 125 + Random(250);
		var l = 0, u = 125;
		while (GBackSolid(ix, iy))
		{
			++l; u += 5;
			iy = y - 50 + Random(100);
			ix = x - u + Random(2 * u);
			if (l > 50)
				break;
		}
		goal->AddCheckpoint(ix, iy, mode);
		y -= LandscapeHeight() / 7;
	}
	x = LandscapeWidth() / 2;
	y = 50;
	goal->SetFinishpoint(x, y);
	
	//Environmental Effects
	var time = CreateObject(Environment_Time);
	time->SetCycleSpeed(0);
	time->SetTime(900);

	//Clouds
	for (var i = 0; i < 30; i++)
		CreateObject(CloudEffect, Random(LandscapeWidth()), Random(LandscapeHeight()))->Show(nil, nil, 5000, true);
	//Snow
	AddEffect("Snowfall", 0, 1, 2);

	MapBottomFix();

	//Place powderkegs and dynamite boxes
	for (var i = 0; i < 25; i++)
	{
		var pos = FindPosInMat("Tunnel", 0, 0, LandscapeWidth(), LandscapeHeight());
		if (!pos)
			continue;
		if (Random(2)) 
			CreateObject(PowderKeg, pos[0], pos[1], NO_OWNER);
		else 
			CreateObject(DynamiteBox, pos[0], pos[1], NO_OWNER);
	}
}

protected func PlrHasRespawned(int plr, object cp)
{
	var clonk = GetCrew(plr);
	clonk->CreateContents(Shovel);
	clonk->CreateContents(Loam);
	clonk->CreateContents(Dynamite);
	return;
}

global func FxSnowfallTimer(object target, int num, int timer)
{
	CastPXS("Snow", 5, 1, RandomX(0, LandscapeWidth()), 1);
}

global func MapBottomFix()
{
	for (var i = 1; i < LandscapeWidth(); i++)
	{
		var sway = Sin(i, 10);
		if (GetMaterial(i, LandscapeHeight() - 1) == Material("Tunnel"))
			DrawMaterialQuad("Granite", i - 1, LandscapeHeight() - 13 + sway, i + 1, LandscapeHeight() - 13 + sway, i + 1, LandscapeHeight(), i - 1, LandscapeHeight());
	}
}