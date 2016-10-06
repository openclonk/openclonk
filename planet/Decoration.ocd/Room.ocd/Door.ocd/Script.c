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