/* Mountain parkour */

func Initialize()
{
  var pGoal = CreateObject(Core_Goal_Parkour, 0, 0, NO_OWNER);
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
    var mode = RACE_CP_Check | RACE_CP_Respawn;
    pGoal->AddCheckpoint(ix, iy, mode);
    y-=LandscapeHeight()/7;
  }
  x=LandscapeWidth()/2;;
  y=35;
  pGoal->SetFinishpoint(x, y);
}

protected func PlrHasRespawned(int iPlr, object cp)
{
	var clonk = GetCrew(iPlr);
	if (!Random(2))
		clonk->CreateContents(LOAM);
	else
		clonk->CreateContents(DYNA);
	clonk->CreateContents(MJOW);
	return;
}
