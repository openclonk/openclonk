/**
	Pipe line

	@author ST-DDT, Marky
*/

local pipe_kit;
local is_air_pipe = false;

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
	this.LineColors = [RGB(80, 80, 120), RGB(80, 80, 120)];
	is_air_pipe = false;
}

// Reddish colour.
// Please to not change otherwise people with dyschromatopsia will hunt you down.
public func SetDrain()
{
	this.LineColors = [RGB(238, 102, 0), RGB(238, 102, 0)];
	is_air_pipe = false;
}

// Greenish colour.
// Please to not change otherwise people with dyschromatopsia will hunt you down.
public func SetSource()
{
	this.LineColors = [RGB(102, 136, 34), RGB(102, 136, 34)];
	is_air_pipe = false;
}

// Blueish colour.
public func SetAir()
{
	this.LineColors = [RGB(0, 153, 255), RGB(0, 153, 255)];
	is_air_pipe = true;
}

// Returns true if this object is a functioning pipe.
public func IsPipeLine()
{
	return GetAction() == "Connect";
}

public func IsAirPipe()
{
	return this.is_air_pipe;
}

// Returns whether this pipe is connected to an object.
// Returns only actually connected objects if the parameter 'strict' is true.
public func IsConnectedTo(object obj, bool strict)
{
	return GetActionTarget(0) == obj || GetActionTarget(1) == obj || (!strict && pipe_kit == obj);
}

// Returns the object which is connected to obj through this pipe.
public func GetConnectedObject(object obj)
{
	if (GetActionTarget(0) == obj)
		return GetActionTarget(1);
	if (GetActionTarget(1) == obj)
		return GetActionTarget(0);
	return;
}

// Switches connection from connected_to to another obj.
public func SwitchConnection(object connected_to, object obj)
{
	var target0 = GetActionTarget(0), target1 = GetActionTarget(1);

	if (target0 == connected_to) target0 = obj;
	if (target1 == connected_to) target1 = obj;
	
	SetActionTargets(target0, target1);
}

// Saves the pipe object that created this line.
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
		FatalError("Unexpected error: This pipe has lost its pipe kit!");
	}
}


private func OnLineBreak(bool no_msg)
{
	Sound("Objects::LineSnap");
	if (!no_msg)
		BreakMessage();

	if (GetPipeKit())
	{
		GetPipeKit()->SetNeutralPipe();
		if (GetActionTarget(0)) GetActionTarget(0)->~OnPipeDisconnect(GetPipeKit());
		if (GetActionTarget(1)) GetActionTarget(1)->~OnPipeDisconnect(GetPipeKit());
	}
	
	return;
}

private func OnLineChange()
{
	// Notify action targets about line change.
	var act1 = GetActionTarget(0);
	var act2 = GetActionTarget(1);	
	if (act1) act1->~OnPipeLengthChange(this);
	if (act2) act2->~OnPipeLengthChange(this);
	
	// Break line if it is too long.
	if (GetPipeLength() > this.PipeMaxLength)
	{
		OnLineBreak();
		RemoveObject();
	}
	return;
}

// Returns the length between all the vertices.
public func GetPipeLength()
{
	var current_length = 0;
	for (var index = 0; index < GetVertexNum() - 1; index++)
		current_length += Distance(GetVertex(index, VTX_X), GetVertex(index, VTX_Y), GetVertex(index + 1, VTX_X), GetVertex(index + 1, VTX_Y));
	return current_length;
}

private func Destruction()
{
	if (GetActionTarget(0)) GetActionTarget(0)->~OnPipeLineRemoval();
	if (GetActionTarget(1)) GetActionTarget(1)->~OnPipeLineRemoval();
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
	if (pipe_kit) props->AddCall("PipeKit", this, "SetPipeKit", pipe_kit);
	if (IsAirPipe()) props->AddCall("AirPipe", this, "SetAir");
	return true;
}

/*-- Properties --*/

local Name = "$Name$";
local PipeMaxLength = 1200;
local BlockPipeCutting = false;

local ActMap = {
	Connect = {
		Prototype = Action,
		Name = "Connect",
		Procedure = DFA_CONNECT,
		NextAction = "Connect"
	}
};
