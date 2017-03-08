/**
	Room Door
	A door which can be entered.

	@author Apfelclonk, Maikel
*/


// Door opens and closes by use of the door library.
#include Library_DoorControl

local target_door; // Anything that enters is transferred to the connected door
local entrance_action; // Action to be performed when something/someone enters the door

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

public func SetKey(object set_key)
{
	// Set or clear key
	if (set_key)
	{
		MakeLockedDoor(set_key);
	}
	else
	{
		door_key = nil;
	}
}

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
	if (door_key)
	{
		if (door_key->Contained() != entering_obj)
		{
			entering_obj->~PlaySoundDecline();
			PlayerMessage(entering_obj->GetOwner(), "$MsgDoorLocked$");
			return false;
		}
		// Use key
		Sound("Structures::DoorUnlock");
		Message("$KeyUsed$");
		door_key->RemoveObject(); // Will reset door_key to nil
	}
	return _inherited(entering_obj, ...);
}



/* Entrance actions */

public func SetTargetDoor(object ntd) { target_door = ntd; }

public func SetEntranceAction(proplist nea) { entrance_action = nea; }

local ignore_collection;

public func Collection2(object entering_object, ...)
{
	// Ignore if transferring from another door
	if (!ignore_collection)
	{
		// Handle target transfer
		if (target_door)
		{
			++target_door.ignore_collection;
			entering_object->Enter(target_door);
			if (target_door)
			{
				--target_door.ignore_collection;
				if (!entering_object) return;
				entering_object->SetCommand("Exit");
			}
		}
		// Handle entrance action
		UserAction->EvaluateAction(entrance_action, this, entering_object);
	}
	return _inherited(entering_object, ...);
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

/* Editor definitions */

public func Definition(def, ...)
{
	if (!def.EditorProps) def.EditorProps = {};
	def.EditorProps.door_key = { Name = "$Key$", EditorHelp="$KeyHelp$", Type = "object", Filter = "IsKey", Set="SetKey", Save="Key" };
	def.EditorProps.target_door = { Name = "$ConnectedDoor$", EditorHelp="$ConnectedDoorHelp$", Type = "object", Filter = "IsDoorTarget", Set="SetTargetDoor", Save="TargetDoor" };
	def.EditorProps.entrance_action = new UserAction.Prop { Name="$EntranceAction$", EditorHelp="$EntranceActionHelp$", Set="SetEntranceAction", Save="EntranceAction" };
	return _inherited(def, ...);
}



/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Plane = 200;
local MeshTransformation = [1050, 0, 0, -550, 0, 1050, 0, -400, 0, 0, 1050, 0]; // Trans_Mul(Trans_Translate(-550, -400, 0), Trans_Scale(1050))

public func IsDoorTarget() { return true; }
public func IsDoor() { return true; }
