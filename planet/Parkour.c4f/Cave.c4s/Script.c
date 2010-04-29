/* Cave parkour */

func Initialize()
{
	var pGoal = CreateObject(Goal_Parkour, 0, 0, NO_OWNER);
	var x, y;
	y=LandscapeHeight()/2;
	x=10;
	pGoal->SetStartpoint(x, y);
	var ix,iy;
	for(var i=0; i<7;i++)
	{
		iy=y-250+Random(500); ix=x-75+Random(150);
		var l=0;
		while(GBackSolid(ix,iy)) {++l;iy=y-250+Random(500); ix=x-75+Random(150);if(l>50){break;};}
		var mode = PARKOUR_CP_Check | PARKOUR_CP_Respawn;
		pGoal->AddCheckpoint(ix, iy, mode);
		x+=LandscapeWidth()/7;
	}
	x=LandscapeWidth()-35;
	pGoal->SetFinishpoint(x, y);
}

protected func PlrHasRespawned(int iPlr, object cp)
{
	var clonk = GetCrew(iPlr);
	if (!Random(2))
		clonk->CreateContents(Loam);
	else
		clonk->CreateContents(Dynamite);
	clonk->CreateContents(JarOfWinds);
	return;
}
