/* Sky race */

func Initialize()
{
 CreateObject(DYNA,1050,1150,-1);
 CreateObject(DYNA,1050,1150,-1);
 
 CreateObject(DYNA,500,900,-1);
 CreateObject(DYNA,500,900,-1);
}

func InitializePlayer(int plr)
{
  return JoinPlayer(plr);
}

private func JoinPlayer(int plr)
{
  var obj=GetCrew(plr);
  obj->DoEnergy(100000);
  obj->SetPosition(20+Random(10),1000);
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
