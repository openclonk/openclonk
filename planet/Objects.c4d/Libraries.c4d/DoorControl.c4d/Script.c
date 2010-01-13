/*-- Door control --*/

#strict 2

/*
  This object supplies the base functionality for structures with doors.
  The buildings have to have OpenDoor, DoorOpen and CloseDoor in their act map
*/

protected func ActivateEntrance(pObj)
{
  if(this->~IsBase() && this->~CanBlockEnemies())
    if(Hostile(GetOwner(), pObj->GetOwner()))
    {
      Sound("Error", 0, 100, pObj->GetOwner()+1);
      PlayerMessage(pObj->GetOwner(), "$TxtNoEntryEnemy$", this, GetPlayerName(GetOwner()));
      return 1;
    }
  if (ActIdle()) SetAction("OpenDoor");
  return 1;
}
  
private func OpenEntrance()
{
  SetEntrance(1);
}

private func CloseEntrance()
{
  SetEntrance(0);
}

private func SoundOpenDoor()
{
  Sound("DoorOpen"); // TODO: Get sound
}

private func SoundCloseDoor()
{
  Sound("DoorClose"); // TODO: get sound
}
  
private func DoorClosed()
{
  return 1;
}

protected func Completion()
{
  SetEntrance(0);
  return _inherited();
}

func Ejection(obj)
{
  if (GetAction () == "DoorOpen") SetAction ("DoorOpen");
  return _inherited (obj);
}

func Collection2(obj)
{
  if (GetAction () == "DoorOpen") SetAction ("DoorOpen");
 return _inherited (obj);
}

func Definition(def) {
  SetProperty("Name", "$Name$", def);
}
