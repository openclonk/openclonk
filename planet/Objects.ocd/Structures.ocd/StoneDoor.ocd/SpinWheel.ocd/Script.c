/*-- Spin Wheel --*/

local targetdoor, temp_light;

public func Initialize()
{
	SetAction("Still");
}

public func SetStoneDoor(object door)
{
	targetdoor = door;
	return true;
}

public func ControlUp(object clonk)
{
	if (GetAction() == "Still" && targetdoor)
	{
		if (clonk)
		{
			SetPlrView(clonk->GetController(), targetdoor);
			if (temp_light) temp_light->RemoveObject();
			var y_off = targetdoor->~GetFloorOffset();
			temp_light = Global->CreateLight(targetdoor->GetX(), targetdoor->GetY() + y_off, 30, Fx_Light.LGT_Temp, clonk->GetController(), 30, 50);
		}
		targetdoor->OpenDoor();
		SetAction("SpinLeft");
		Sound("Chain");
	}
}

public func ControlDown(object clonk)
{
	if (GetAction() == "Still" && targetdoor)
	{
		if (clonk)
		{
			SetPlrView(clonk->GetController(), targetdoor);
			if (temp_light) temp_light->RemoveObject();
			var y_off = targetdoor->~GetFloorOffset();
			temp_light = Global->CreateLight(targetdoor->GetX(), targetdoor->GetY() + y_off, 30, Fx_Light.LGT_Temp, clonk->GetController(), 30, 50);
		}
		targetdoor->CloseDoor();
		SetAction("SpinRight");
		Sound("Chain");
	}
}

public func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	if (targetdoor) props->AddCall("Target", this, "SetStoneDoor", targetdoor);
	return true;
}

func ConnectNearestDoor()
{
	// EditCursor helper command: Connect to nearest door. Return connected door.
	var door = FindObject(Find_ID(StoneDoor), Sort_Distance());
	if (door) SetStoneDoor(door);
	return door;
}

local ActMap = {
	Still = {
		Prototype = Action,
		Name = "Still",
		Procedure = DFA_NONE,
		Length = 1,
		Delay = 1,
		NextAction = "Still",
		Animation = "SpinLeft",
	},
	SpinLeft = {
		Prototype = Action,
		Name = "SpinLeft",
		Procedure = DFA_NONE,
		Length = 36,
		Delay = 1,
		NextAction = "Still",
		Animation = "SpinLeft",
	},
	SpinRight = {
		Prototype = Action,
		Name = "SpinRight",
		Procedure = DFA_NONE,
		Length = 36,
		Delay = 1,
		NextAction = "Still",
		Animation = "SpinRight",
	},
};
local Name = "$Name$";
local Touchable = 2;
local EditCursorCommands = ["ControlUp()", "ControlDown()", "ConnectNearestDoor()"];

func Definition(def)
{
	SetProperty("PictureTransformation", Trans_Mul(Trans_Scale(800), Trans_Translate(0,0,0),Trans_Rotate(-20,1,0,0),Trans_Rotate(-30,0,1,0)), def);
	SetProperty("MeshTransformation", Trans_Rotate(-13,0,1,0), def);
}


