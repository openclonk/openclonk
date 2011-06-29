/*-- Pipe line --*/

protected func Initialize()
{
	SetAction("Connect");
	SetVertexXY(0, GetX(), GetY());
	SetVertexXY(1, GetX(), GetY());
	SetProperty("LineColors", [RGB(80,80,120), RGB(80,80,120)]);
	return;
}

// Returns true if this object is a functioning pipe.
public func IsPipeLine()
{
	return GetAction() == "Connect";
}

// Returns whether this pipe is connected to an object.
public func IsConnectedTo(object obj)
{
	return GetActionTarget(0) == obj || GetActionTarget(1) == obj;
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

protected func LineBreak(bool no_msg)
{
	Sound("LineBreak");
	if (!no_msg) 
		BreakMessage();
	return;
}

private func BreakMessage()
{
	var line_end = GetActionTarget(0);
	if (line_end->GetID() != Pipe) 
		line_end = GetActionTarget(1);

	line_end->Message("$TxtPipeBroke$");
	return;
}

local ActMap = {
	Connect = {
		Prototype = Action,
		Name = "Connect",
		Procedure = DFA_CONNECT,
		NextAction = "Connect",
	},
};

local Name = "$Name$";
		
