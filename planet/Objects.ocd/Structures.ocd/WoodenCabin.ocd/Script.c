/*-- Wooden Cabin --*/

#include Library_Structure
#include Library_Ownable
#include Library_DoorControl

public func Initialize()
{
	SetAction("Idle");
}

protected func IsBuilt()
{
	return GetCon() >= 100;
}

public func NoConstructionFlip() { return true; }

local ActMap = {/*
Idle = {
	Prototype = Action,
	Name = "Idle",
	Procedure = DFA_NONE,
	Length = 1,
	Delay = 1,
	NextAction = "Idle",
	Animation = "Closed",
},*/
OpenDoor = {
	Prototype = Action,
	Name = "OpenDoor",
	Procedure = DFA_NONE,
	Length = 20,
	Delay = 1,
	NextAction = "DoorOpen",
	StartCall = "SoundOpenDoor",
	Animation = "OpenDoor",
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
	Animation = "Open",
},
CloseDoor = {
	Prototype = Action,
	Name = "CloseDoor",
	Procedure = DFA_NONE,
	Length = 40,
	Delay = 1,
	NextAction = "Idle",
	StartCall = "SoundCloseDoor",
	Animation = "CloseDoor",
},
};
func Definition(def) {
	SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(2000,0,7000),Trans_Rotate(-20,1,0,0),Trans_Rotate(30,0,1,0)), def);
	SetProperty("MeshTransformation", Trans_Rotate(-8,0,1,0));
}
local BlastIncinerate = 100;
local HitPoints = 70;
local Name = "$Name$";
local Description = "$Description$";