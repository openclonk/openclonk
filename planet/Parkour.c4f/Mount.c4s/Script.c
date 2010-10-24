/* Mountain parkour */

func Initialize()
{
	var pGoal = CreateObject(Goal_Parkour, 0, 0, NO_OWNER);
	var x, y;
	y=LandscapeHeight()-120;
	x=LandscapeWidth()/2;
	pGoal->SetStartpoint(x, y);
	var ix,iy;
	y=LandscapeHeight()/7*6;
	for(var i=1; i<7;i++)
	{
		iy=y-50+Random(100); ix=x-125+Random(250);
		var l=0,u=125;
		while(GBackSolid(ix,iy)) {++l;u+=5;iy=y-50+Random(100); ix=x-u+Random(2*u);if(l>50){break;};}
		var mode = PARKOUR_CP_Check | PARKOUR_CP_Respawn;
		pGoal->AddCheckpoint(ix, iy, mode);
		y-=LandscapeHeight()/7;
	}
	x=LandscapeWidth()/2;;
	y=35;
	pGoal->SetFinishpoint(x, y);
	
	// Place chests.
	var d = 300;
	var pos;
	while (d < LandscapeHeight() - 300)
	{
		var i = 0;
		while (!(pos = FindPosInMat("Tunnel", 0, d, LandscapeWidth(), 300, 15)) && i < 25)
			i++; // Max 25 attempts.
		x = pos[0]; y = pos[1];
		CreateObject(Chest, x, y + 8, NO_OWNER);
		d += RandomX(250, 300);
	}
	// Fill chests.
	var content_list = [PowderKeg, Dynamite, DynamiteBox, Loam, Firestone];
	for (var chest in FindObjects(Find_ID(Chest)))
		for (var i = 0; i < 4; i++)
			chest->CreateContents(content_list[Random(GetLength(content_list))]);

	//Environmental Effects
	var time = CreateObject(Environment_Time);
	time->SetCycleSpeed();
	time->SetTime(900);

	//Clouds
	for(var i; i < 30; i++)
		CreateObject(CloudEffect,Random(LandscapeWidth()),Random(LandscapeHeight()))->Show(nil,nil,5000,true);
	//Snow
	AddEffect("Snowfall",0,1,2);

	MapBottomFix();
}

protected func PlrHasRespawned(int iPlr, object cp)
{
	var clonk = GetCrew(iPlr);
	clonk->CreateContents(Shovel);
	if (!Random(2))
		clonk->CreateContents(Loam);
	else
		clonk->CreateContents(Dynamite);
	return;
}

global func FxSnowfallTimer(object target, int num, int timer)
{
	CastPXS("Snow",5,1,RandomX(0,LandscapeWidth()),1);
}

global func MapBottomFix()
{
	for(var i=1; i < LandscapeWidth(); i++)
	{
		var sway = Sin(i,10);
		if(GetMaterial(i,LandscapeHeight()-1) == Material("Tunnel"))
			DrawMaterialQuad("Granite",i-1,LandscapeHeight() - 13 + sway,i+1,LandscapeHeight() - 13 + sway,i+1,LandscapeHeight(),i-1,LandscapeHeight());
	}
}