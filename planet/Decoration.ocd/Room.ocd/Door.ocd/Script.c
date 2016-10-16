/**
	Room Door
	A door which can be entered.

	@author Apfelclonk, Maikel
*/


// Door opens and closes by use of the door library.
#include Library_DoorControl


/*-- Door Control --*/

private func OnOpenDoor()
{
	PlayAnimation("proceed_door", 5, Anim_Linear(0, 0, GetAnimationLength("proceed_door"), this.ActMap.OpenDoor.Length, ANIM_Hold));
	SoundOpenDoor();
	return;
}

private func OnCloseDoor()
{
	PlayAnimation("proceed_door", 5, Anim_Linear(GetAnimationLength("proceed_door"), GetAnimationLength("proceed_door"), 0, this.ActMap.CloseDoor.Length, ANIM_Hold));
	SoundCloseDoor();
	return;	
}


/*-- Door Locking --*/

local door_key;

public func MakeLockedDoor(object set_key)
{
	if (!set_key->~IsKey())
		return;
	door_key = set_key;
	// Color the key according to the door.
	var door_clr = GetColor();
	if (door_clr)
		door_key->SetColor(door_clr);
	return;
}

public func ActivateEntrance(object entering_obj)
{
	// Block the entrance if this door needs a key and the key is not carried.
	if (door_key && door_key->Contained() != entering_obj)
	{
		entering_obj->~PlaySoundDecline();
		PlayerMessage(entering_obj->GetOwner(), "$MsgDoorLocked$");
		return false;
	}
	return _inherited(entering_obj, ...);
}


/*-- ActMap --*/

local ActMap = {
	OpenDoor = {
		Prototype = Action,
		Name = "OpenDoor",
		Procedure = DFA_NONE,
		Length = 20,
		Delay = 1,
		NextAction = "DoorOpen",
		StartCall = "OnOpenDoor",
	},
	DoorOpen = {
		Prototype = Action,
		Name = "DoorOpen",
		Procedure = DFA_NONE,
		Length = 1,
		Delay = 20,
		NextAction = "CloseDoor",
		StartCall = "OpenEntrance",
		EndCall = "CloseEntrance",
	},
	CloseDoor = {
		Prototype = Action,
		Name = "CloseDoor",
		Procedure = DFA_NONE,
		Length = 20,
		Delay = 1,
		NextAction = "Idle",
		StartCall = "OnCloseDoor",
	}
};


/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Plane = 200;
local MeshTransformation = [1050, 0, 0, -550, 0, 1050, 0, -400, 0, 0, 1050, 0]; // Trans_Mul(Trans_Translate(-550, -400, 0), Trans_Scale(1050))