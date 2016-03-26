/*-- Cable line --*/

func Initialize()
{
	SetAction("Connect");
	SetVertexXY(0, GetX(), GetY());
	SetVertexXY(1, GetX(), GetY());
	SetProperty("LineColors", [RGB(20, 20, 50), RGB(20, 20, 50)]);
}

public func IsCableLine()
{
	return GetAction() == "Connect";
}

/** Returns whether this cable is connected to an object. */
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

public func SetConnectedObjects(obj1, obj2)
{
	SetActionTargets(obj1, obj2);
	obj1->AddCableConnection(this);
}

protected func LineBreak(bool no_msg)
{
	Sound("Objects::Connect");
	if (!no_msg) 
		BreakMessage();
	return;
}

private func BreakMessage()
{
	var line_end = GetActionTarget(0);
	if (line_end->GetID() != CableLorryReel) 
		line_end = GetActionTarget(1);
	if (line_end->Contained()) line_end = line_end->Contained();

	line_end->Message("$TxtLinebroke$");
	return;
}

public func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	SaveScenarioObjectAction(props);
	if (IsCableLine()) props->AddCall("Connection", this, "SetConnectedObjects", GetActionTarget(0), GetActionTarget(1));
	return true;
}

local ActMap = {
	Connect = {
		Prototype = Action,
		Name = "Connect",
		Procedure = DFA_CONNECT,
		NextAction = "Connect"
	}
};

local Name = "$Name$";