/* Sky race */

func Initialize()
{
  for (var i=0; i<20; ++i) CreateObject(LOAM, 1560+Random(11)-5, 200+Random(11)-5, NO_OWNER);
  for (var i=0; i<20; ++i) CreateObject(DYNA, 2730+Random(11)-5, 660+Random(11)-5, NO_OWNER);
}

func InitializePlayer(int plr)
{
  return JoinPlayer(plr);
}

private func JoinPlayer(int plr)
{
  var obj=GetCrew(plr);
  obj->DoEnergy(100000);
  obj->SetPosition(10+Random(50), LandscapeHeight()/2-30);
  obj->CreateContents(BOW1);
  obj->CreateContents(ARRW);
  return true;
}


/* Relaunch */

public func RelaunchPlayer(int plr)
{
  var clnk=CreateObject(CLNK,0,0,plr);
  clnk->MakeCrewMember(plr);
  SetCursor(plr,clnk);
  SelectCrew(plr, clnk, 1);
  Log(RndRelaunchMsg(), GetPlayerName(plr));
  return JoinPlayer(plr);
}

private func RndRelaunchMsg()
{
  return Translate(Format("RelaunchMsg%d", Random(4)));
}
