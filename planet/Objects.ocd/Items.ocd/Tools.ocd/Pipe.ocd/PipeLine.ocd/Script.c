/*-- Pipe line

	Author: ST-DDT, Marky
--*/

local Name = "$Name$";

local pipe_kit;

local ActMap = {
	Connect = {
		Prototype = Action,
		Name = "Connect",
		Procedure = DFA_CONNECT,
		NextAction = "Connect"
	}
};

private func Initialize()
{
	SetAction("Connect");
	SetVertexXY(0, GetX(), GetY());
	SetVertexXY(1, GetX(), GetY());
	SetNeutral();
}

// Greyish colour
public func SetNeutral()
{
	SetProperty("LineColors", [RGB(80, 80, 120), RGB(80, 80, 120)]);
}

// Reddish colour
public func SetDrain()
{
	SetProperty("LineColors", [RGB(110, 80, 80), RGB(110, 80, 80)]);
}

// Greenish colour
public func SetSource()
{
	SetProperty("LineColors", [RGB(80, 110, 80), RGB(80, 110, 80)]);
}

/** Returns true if this object is a functioning pipe. */
public func IsPipeLine()
{
	return GetAction() == "Connect";
}

/** Returns whether this pipe is connected to an object.
    Returns only actually connected objects if the parameter 'strict' is true */
public func IsConnectedTo(object obj, bool strict)
{
	return GetActionTarget(0) == obj || GetActionTarget(1) == obj || (!strict && pipe_kit == obj);
}


/** Returns the object which is connected to obj through this pipe. */
public func GetConnectedObject(object obj)
{
	if (GetActionTarget(0) == obj)
		return GetActionTarget(1);
	if (GetActionTarget(1) == obj)
		return GetActionTarget(0);
	return;
}

/** Switches connection from one object to another. */
public func SwitchConnection(object connected_to, object obj)
{
	var target0 = GetActionTarget(0), target1 = GetActionTarget(1);

	if (target0 == connected_to) target0 = obj;
	if (target1 == connected_to) target1 = obj;
	
	SetActionTargets(target0, target1);
}

/** Saves the pipe object that created this line. */
public func SetPipeKit(object obj)
{
	pipe_kit = obj;
}

public func GetPipeKit()
{
	if (pipe_kit)
	{
		return pipe_kit;
	}
	else
	{
		if (GetActionTarget(0)->GetID() == Pipe) return GetActionTarget(0);
		if (GetActionTarget(1)->GetID() == Pipe) return GetActionTarget(1);
		
		FatalError("Unexpected error: This pipe has lost its pipe kit!");
	}
}


private func LineBreak(bool no_msg)
{
	Sound("Objects::LineSnap");
	if (!no_msg)
		BreakMessage();

	var line_end = GetPipeKit();
	if (line_end) line_end->SetNeutralPipe();
	return;
}

private func Destruction()
{
	var line_end = GetPipeKit();
	if (line_end) line_end->SetNeutralPipe();
	return;
}

private func BreakMessage()
{
	var line_end = GetPipeKit();
	if (line_end) line_end->Report("$TxtPipeBroke$");
	return;
}

public func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	SaveScenarioObjectAction(props);
	return true;
}