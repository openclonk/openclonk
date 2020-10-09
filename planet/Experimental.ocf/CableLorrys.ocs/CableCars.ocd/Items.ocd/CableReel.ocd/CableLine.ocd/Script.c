/**
	Cable line

	@author Clonkonaut	
*/

local hanging_cars;

public func Initialize()
{
	hanging_cars = [];
	SetAction("Connect");
	SetVertexXY(0, GetX(), GetY());
	SetVertexXY(1, GetX(), GetY());
	SetProperty("LineColors", [RGB(20, 20, 50), RGB(20, 20, 50)]);
}

public func IsCableLine() { return GetAction() == "Connect"; }

/** Returns whether this cable is connected to an object. */
public func IsConnectedTo(object obj)
{
	return GetActionTarget(0) == obj || GetActionTarget(1) == obj;
}

/** Connects this cable to obj1 and obj2. */
public func SetConnectedObjects(object obj1, object obj2)
{
	if (GetActionTarget(0)) GetActionTarget(0)->~CableDeactivation(activations);
	if (GetActionTarget(1)) GetActionTarget(1)->~CableDeactivation(activations);

	SetVertexXY(0, obj1->GetX(), obj1->GetY());
	SetVertexXY(1, obj2->GetX(), obj2->GetY());
	SetActionTargets(obj1, obj2);
	obj1->AddCableConnection(this);
}

/** Returns the object which is connected to obj through this cable. */
public func GetConnectedObject(object obj)
{
	if (GetActionTarget(0) == obj)
		return GetActionTarget(1);
	if (GetActionTarget(1) == obj)
		return GetActionTarget(0);
}


/*-- Cable Changes --*/

public func Destruction()
{
	if (GetActionTarget(0)) GetActionTarget(0)->~OnCableLineRemoval();
	if (GetActionTarget(1)) GetActionTarget(1)->~OnCableLineRemoval();
	return;
}

public func OnLineChange()
{
	// Notify action targets about line change.
	var act1 = GetActionTarget(0);
	var act2 = GetActionTarget(1);	
	if (act1) act1->~OnCableLengthChange(this);
	if (act2) act2->~OnCableLengthChange(this);
	// Break line if it is too long or if it is bend.
	if (GetVertexNum() > 2 || GetCableLength() > this.CableMaxLength)
	{
		OnLineBreak();
		RemoveObject();
	}
	return;
}

// Returns the length between all the vertices.
public func GetCableLength()
{
	var current_length = 0;
	for (var index = 0; index < GetVertexNum() - 1; index++)
		current_length += Distance(GetVertex(index, VTX_X), GetVertex(index, VTX_Y), GetVertex(index + 1, VTX_X), GetVertex(index + 1, VTX_Y));
	return current_length;
}


/*-- Breaking --*/

public func OnLineBreak(bool no_msg)
{
	Sound("Objects::LineSnap");
	var act1 = GetActionTarget(0);
	var act2 = GetActionTarget(1);
	
	if (!no_msg)
		BreakMessage();
	
	SetAction("Idle");
	if (act1)
	{
		act1->~CableDeactivation(activations);
		act1->~RemoveCableConnection(this);
	}
	if (act2)
	{
		act2->~CableDeactivation(activations);
		act2->~RemoveCableConnection(this);
	}
}

public func BreakMessage()
{
	var line_end = GetActionTarget(0);
	
	if (!line_end || line_end->GetID() != CableReel)
		line_end = GetActionTarget(1);
	if (line_end && line_end->Contained())
		line_end = line_end->Contained();
	if (line_end)
		line_end->Message("$MsgCableBroke$");
	return;
}


/*-- Activation --*/

local activations = 0;

/** Called by cable cars whenever one proceeds onto this cable. Will be forwarded to connected objects.
	count increases the activation value. Stations will stop animation only if all activations are deactivated. */
public func Activation(int count)
{
	// Count must be > 0
	if (count < 1)
		return FatalError("Cable Line: Activation() was called with count < 1.");
	activations += count;
	if (GetActionTarget(0)) GetActionTarget(0)->~CableActivation(count);
	if (GetActionTarget(1)) GetActionTarget(1)->~CableActivation(count);
}

/** Called by cable cars whenever one proceeds off this cable. Will be forwarded to connected objects.
	count decreases the activation value. Stations will stop animation only if all activations are deactivated. */
public func Deactivation(int count)
{
	// Count must be > 0
	if (count < 1)
		return FatalError("Cable Line: Deactivation() was called with count < 1.");
	activations -= count;
	if (GetActionTarget(0)) GetActionTarget(0)->~CableDeactivation(count);
	if (GetActionTarget(1)) GetActionTarget(1)->~CableDeactivation(count);
}


/*-- Cable Cars --*/

public func AddHangingCableCar(object car)
{
	PushBack(hanging_cars, car);
	return;
}

public func RemoveHangingCar(object car)
{
	RemoveArrayValue(hanging_cars, car, true);
	return;
}

public func OnRailNetworkUpdate()
{
	for (var car in hanging_cars)
		car->~OnRailNetworkUpdate();
	return;
}


/*-- Saving --*/

public func SaveScenarioObject(proplist props)
{
	if (!inherited(props, ...))
		 return false;
	SaveScenarioObjectAction(props);
	if (IsCableLine())
		props->AddCall("Connection", this, "SetConnectedObjects", GetActionTarget(0), GetActionTarget(1));
	return true;
}


/*-- Properties --*/

local ActMap = {
	Connect = {
		Prototype = Action,
		Name = "Connect",
		Procedure = DFA_CONNECT,
		NextAction = "Connect"
	}
};

local Name = "$Name$";
local CableMaxLength = 500;
