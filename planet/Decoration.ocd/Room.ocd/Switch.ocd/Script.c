/*-- Switch --*/

local target, handle, last_controlling_clonk;

public func Initialize()
{
	handle = CreateObject(Switch_Handle);
	if (handle) handle->SetSwitch(this);
	SetAction("Idle");
}

public func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	if (target) props->AddCall("Target", this, "SetTarget", target);
	if (handle)
	{
		var pos = GetHandlePosition();
		if (pos) props->AddCall("Handle", this, "SetSwitchDir", (pos>0)-(pos<0));
	}
	return true;
}

public func SetTarget(object trg)
{
	target = trg;
	return true;
}

public func ConnectNearestDoor()
{
	// EditCursor helper command: Connect to nearest door. Return connected door.
	var door = FindObject(Find_ID(StoneDoor), Sort_Distance());
	if (door) SetTarget(door);
	return door;
}

public func ControlUp(object clonk)
{
	var dir = Rot2Dir(0,-1);
	if (!dir) return false;
	return ControlSwitchDir(clonk, dir);
}

public func ControlDown(object clonk)
{
	var dir = Rot2Dir(0,+1);
	if (!dir) return false;
	return ControlSwitchDir(clonk, dir);
}

public func ControlLeft(object clonk)
{
	var dir = Rot2Dir(-1,0);
	if (!dir) return false;
	return ControlSwitchDir(clonk, dir);
}

public func ControlRight(object clonk)
{
	var dir = Rot2Dir(+1,0);
	if (!dir) return false;
	return ControlSwitchDir(clonk, dir);
}

private func ControlSwitchDir(object clonk, int dir)
{
	if (!target || !handle)
	{
		Sound("Structures::SwitchStuck");
		Message("$MsgStuck$");
		return false;
	}
	// remember clonk for clalbacks
	last_controlling_clonk = clonk;
	// move handle
	var handle_pos = GetHandlePosition();
	if (dir<0)
	{
		if (GetAction() == "SwitchLeft") return false;
		if (handle_pos == -MaxHandleAngle) { Sound("Structures::SwitchStuck"); return false; }
		SetAction("SwitchLeft");
	}
	else
	{
		if (GetAction() == "SwitchRight") return false;
		if (handle_pos == MaxHandleAngle) { Sound("Structures::SwitchStuck"); return false; }
		SetAction("SwitchRight");
	}
	Sound("Structures::SwitchMove");
	return true;
}

private func Rot2Dir(int dx, int dy)
{
	// Convert direction dx,dy in world coordinates to horizontal direction in local coordinates
	var r = GetR();
	return BoundBy(dx*Cos(r, 3) + dy*Sin(r, 3), -1,+1);
}

private func GetHandlePosition()
{
	// returns position of handle relative to switch in degrees [-180 to +180]
	return (handle->GetR() - GetR() + 540) % 360 - 180;
}

public func SetSwitchDir(int to_dir)
{
	// Set angle of switch without doing any effects or callbacks
	if (!handle) return false;
	return handle->SetR(GetR() + MaxHandleAngle * to_dir);
}

public func SetR(int to_r)
{
	// Set rotation: Also update handle rotation
	if (handle) handle->SetR(handle->GetR() + to_r - GetR());
	return _inherited(to_r, ...);
}

private func SwitchingTimer(int dir)
{
	if (!handle || !target) { Sound("Structures::SwitchStuck"); return SetAction("Idle"); }
	var handle_pos = GetHandlePosition();
	var handle_pos_new = BoundBy(handle_pos + HandleSpeed * dir, -MaxHandleAngle, +MaxHandleAngle);
	if (!handle_pos_new) handle_pos_new = dir; // avoid direct central position, so player cannot force the same direction twice
	handle->SetR(GetR() + handle_pos_new);
	// Reached end?
	if (handle_pos_new == MaxHandleAngle * dir) SetAction("Idle");
	// Passed by middle? (last call because callback might delete switch)
	if (handle_pos * handle_pos_new <= 0)
	{
		Sound("Structures::SwitchFlip*");
		DoSwitchFlip(last_controlling_clonk, dir);
	}
	return true;
}

private func DoSwitchFlip(object clonk, int dir)
{
	// Perform action associated to this switch
	if (target)
		if (dir>0)
		{
			// Open/close should be aligned to vertical component of direction
			if (GetR() < 0)
			{
				target->~OpenDoor(this);
			}
			else
			{
				target->~CloseDoor(this);
			}
			// Action last; it may delete the door/clonk/etc.
			UserAction->EvaluateAction(right_action, this, clonk);
		}
		else
		{
			// Open/close should be aligned to vertical component of direction
			if (GetR() < 0)
			{
				target->~CloseDoor(this);
			}
			else
			{
				target->~OpenDoor(this);
			}
			UserAction->EvaluateAction(left_action, this, clonk);
		}
	return false;
}

private func SwitchingLeft() { return SwitchingTimer(-1); }
private func SwitchingRight() { return SwitchingTimer(+1); }

local ActMap = {
	SwitchLeft = {
		Prototype = Action,
		Name = "SwitchLeft",
		Procedure = DFA_NONE,
		Length = 1,
		Delay = 1,
		NextAction = "SwitchLeft",
		StartCall = "SwitchingLeft",
		FacetBase = 1,
	},
	SwitchRight = {
		Prototype = Action,
		Name = "SwitchRight",
		Procedure = DFA_NONE,
		Length = 1,
		Delay = 1,
		NextAction = "SwitchRight",
		StartCall = "SwitchingRight",
		FacetBase = 1,
	},
};

local Name = "$Name$";
local Description = "$Description$";
local Touchable = 2;
local Plane = 270;
local MaxHandleAngle = 45;
local HandleSpeed = 6;
local Components = { Rock = 3, Metal=1 };
local left_action, right_action; // Custom editor-selected actions on switch handling

local EditorActions = {
	SwitchLeft = { Name = "$SwitchLeft$", Command = "ControlSwitchDir(nil, -1)" },
	SwitchRight = { Name = "$SwitchRight$", Command = "ControlSwitchDir(nil, +1)" },
	ConnectClosestDoor = { Name = "$ConnectNearestDoor$", Command = "ConnectNearestDoor()" },
	Rotate = { Name = "$Rotate$", Command = "SetR((GetR()+135)/90*90)" }
};

func Definition(def)
{
	SetProperty("PictureTransformation", Trans_Mul(Trans_Scale(800), Trans_Translate(0,0,0),Trans_Rotate(-20,1,0,0),Trans_Rotate(-30,0,1,0)), def);
	SetProperty("MeshTransformation", Trans_Rotate(-13,0,1,0), def);
	if (!def.EditorProps) def.EditorProps = {};
	def.EditorProps.target = { Name = "$Target$", Type = "object", Filter = "IsSwitchTarget" };
	def.EditorProps.left_action = new UserAction.Prop { Name="$LeftAction$" };
	def.EditorProps.right_action = new UserAction.Prop { Name="$RightAction$" };
	return _inherited(def, ...);
}
