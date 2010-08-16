/* Canyons parkour */

func Initialize()
{
	var pGoal = CreateObject(Goal_Parkour, 0, 0, NO_OWNER);
	var x, y;
	y=LandscapeHeight()-400;
	x=10;
	pGoal->SetStartpoint(x, y);
	var ix,iy;
	for(var i=0; i<6;i++)
	{
		iy=y;ix=x;
		var l=0,u=20;
		while(GBackSolid(ix,iy) | GBackSky(ix,iy)) {++l;u+=3;iy=y-u+Random(2*u); ix=x-u+Random(2*u);if(l>100){break;};}
		var mode = PARKOUR_CP_Check | PARKOUR_CP_Respawn;
		pGoal->AddCheckpoint(ix, iy, mode);
		y-=LandscapeHeight()/10;
		x+=LandscapeWidth()/6;
	}
	x=LandscapeWidth()-35;
	y=35;
	pGoal->SetFinishpoint(x, y);
	SetFoW(0);
}

protected func PlrHasRespawned(int iPlr, object cp)
{
	var clonk = GetCrew(iPlr);
	clonk->CreateContents(Shovel);
	clonk->CreateContents(Loam);
	clonk->CreateContents(JarOfWinds);
	return;
}
