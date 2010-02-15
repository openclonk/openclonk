/*-- Sky Parkour --*/
func Initialize()
{
  var pGoal = CreateObject(Core_Goal_Parkour, 0, 0, NO_OWNER);
  var x, y;
  y=LandscapeHeight()/2;
  x=10;
  pGoal->SetStartpoint(x, y);
  var mode = RACE_CP_Check | RACE_CP_Respawn;
  y=100; x=LandscapeWidth()/2;
  pGoal->AddCheckpoint(x, y, mode);
  x=LandscapeWidth()-50;
  pGoal->SetFinishpoint(x, y);
}

protected func PlrHasRespawned(int iPlr, object cp)
{
	var clonk = GetCrew(iPlr);
	clonk->CreateContents(LOAM);
	clonk->CreateContents(MJOW);
	return;
}
