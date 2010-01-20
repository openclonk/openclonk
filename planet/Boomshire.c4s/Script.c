/* Sky race */

func Initialize()
{
 
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
  obj->CreateContents(MJOW);
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
