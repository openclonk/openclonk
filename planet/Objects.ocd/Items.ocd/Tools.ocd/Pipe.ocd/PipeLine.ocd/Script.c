/*-- Pipe line

	Author: ST-DDT
--*/

local Name = "$Name$";

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
	SetProperty("LineColors", [RGB(80, 80, 120), RGB(80, 80, 120)]);
	return;
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

/** Returns whether this pipe is connected to an object. */
public func IsConnectedTo(object obj)
{
	return GetActionTarget(0) == obj || GetActionTarget(1) == obj;
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

private func LineBreak(bool no_msg)
{
	Sound("LineBreak");
	if (!no_msg)
		BreakMessage();

	var line_end = GetActionTarget(0);
	if (!line_end ||line_end->GetID() != Pipe)
		line_end = GetActionTarget(1);
	if (line_end) 
		line_end->~ResetPicture();
	return;
}

private func Destruction()
{
	var line_end = GetActionTarget(0);
	if (!line_end || line_end->GetID() != Pipe)
		line_end = GetActionTarget(1);
	if (line_end) 
		line_end->~ResetPicture();
	return;
}

private func BreakMessage()
{
	var line_end = GetActionTarget(0);
	if (!line_end || line_end->GetID() != Pipe)
		line_end = GetActionTarget(1);
	if (line_end)
		line_end->Message("$TxtPipeBroke$");
	return;
}

public func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	SaveScenarioObjectAction(props);
	return true;
}