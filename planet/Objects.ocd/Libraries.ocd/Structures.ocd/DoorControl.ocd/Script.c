/*-- Door control --*/

/*
	This object supplies the base functionality for structures with doors.
	The buildings have to have OpenDoor, DoorOpen and CloseDoor in their act map
*/

protected func ActivateEntrance(object obj)
{
	if (this->~IsBase() && this->~CanBlockEnemies())
	{
		var for_plr = obj->GetOwner();
		if (Hostile(GetOwner(), for_plr))
		{
			Sound("Error", false, 100, for_plr);
			PlayerMessage(for_plr, "$TxtNoEntryEnemy$", GetPlayerName(GetOwner()));
			return false;
		}
	}
	if (ActIdle()) 
		SetAction("OpenDoor");
	return true;
}
	
private func OpenEntrance()
{
	SetEntrance(true);
	return;
}

private func CloseEntrance()
{
	SetEntrance(false);
	return;
}

private func SoundOpenDoor()
{
	// TODO: Get sound
	Sound("DoorOpen");
	return;
}

private func SoundCloseDoor()
{
	// TODO: get sound
	Sound("DoorClose");
	return;
}
	
protected func Initialize()
{
	SetEntrance(false);
	return _inherited(...);
}

protected func Ejection(object obj)
{
	// Let the door stay open longer on ejection.
	if (GetAction () == "DoorOpen") 
		SetAction ("DoorOpen");
	return _inherited(obj, ...);
}

protected func Collection2(object obj)
{
	// Let the door stay open longer on collection.
	if (GetAction () == "DoorOpen") 
		SetAction ("DoorOpen");
	return _inherited(obj, ...);
}
