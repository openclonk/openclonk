/*-- Power line --*/

protected func Initialize()
{
	SetAction("Connect");
	SetVertexXY(0, GetX(), GetY());
	SetVertexXY(1, GetX(), GetY());
	SetPosition(0, 0);
}

// Returns true if this object is a functioning power line.
public func IsPowerLine()
{
	return GetAction() == "Connect";
}

// Returns whether this power line is connected to pObject.
public func IsConnectedTo(object pObject)
{
	return GetActionTarget(0) == pObject || GetActionTarget(1) == pObject;
}

// Returns the object which is connected to pObject through this power line.
public func GetConnectedObject(object pObject)
{
	if(GetActionTarget(0) == pObject)
		return GetActionTarget(1);
	if(GetActionTarget(1) == pObject) 
		return GetActionTarget(0);		
	return;
}

protected func LineBreak(bool fNoMsg)
{
	Sound("LineBreak");
	if(!fNoMsg) BreakMessage();
}

private func BreakMessage()
{
	var pLine = GetActionTarget(0);
	if(pLine->GetID() != CableReel) pLine = GetActionTarget(1);

	Message("$TxtLinebroke$", pLine);
}

func Definition(def) {
	SetProperty("ActMap", {
Connect = {
Prototype = Action,
Name = "Connect",
Length = 0,
Delay = 0,
Procedure = DFA_CONNECT,
NextAction = "Connect",
},  }, def);
	SetProperty("Name", "$Name$", def);
}