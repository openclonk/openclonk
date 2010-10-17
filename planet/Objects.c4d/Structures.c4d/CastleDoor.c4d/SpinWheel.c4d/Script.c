/*-- Spin Wheel --*/

local targetdoor;

public func Initialize()
{
	SetAction("Still");
}

public func SetCastleDoor(object door)
{
	targetdoor = door;
	return;
}

public func ControlUp(object clonk)
{
	if (GetAction() == "Still" && targetdoor)
	{
		targetdoor->OpenGateDoor();
		SetAction("SpinLeft");
		Sound("Chain.ogg");
	}
}

public func ControlDown(object clonk)
{
	if (GetAction() == "Still" && targetdoor)
	{
		targetdoor->CloseGateDoor();
		SetAction("SpinRight");
		Sound("Chain.ogg");
	}
}

func Definition(def) {
	SetProperty("ActMap", {
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
	}, def);
	SetProperty("Name", "$Name$", def);
	SetProperty("PictureTransformation", Trans_Mul(Trans_Scale(800), Trans_Translate(0,0,0),Trans_Rotate(-20,1,0,0),Trans_Rotate(-30,0,1,0)), def);
	SetProperty("MeshTransformation", Trans_Rotate(-13,0,1,0));
}
