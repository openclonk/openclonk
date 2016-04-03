/*-- Cable line --*/

func Initialize()
{
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
public func SetConnectedObjects(obj1, obj2)
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

/* Breaking */

func LineBreak(bool no_msg)
{
	Sound("Objects::Connect");
	if (GetActionTarget(0)) GetActionTarget(0)->~CableDeactivation(activations);
	if (GetActionTarget(1)) GetActionTarget(1)->~CableDeactivation(activations);
	if (!no_msg)
		BreakMessage();
}

func BreakMessage()
{
	var line_end = GetActionTarget(0);
	if (line_end->GetID() != CableLorryReel)
		line_end = GetActionTarget(1);
	if (line_end->Contained()) line_end = line_end->Contained();

	line_end->Message("$TxtLinebroke$");
}

/* Activation */

local activations = 0;

/** Called by cable cars whenever one proceeds onto this cable. Will be forwarded to connected objects.
	count increases the activation value. Stations will stop animation only if all activations are deactivated. */
public func Activation(int count)
{
	// Count must be > 0
	if (count < 1) return FatalError("Cable Line: Activation() was called with count < 1.");
	activations += count;
	if (GetActionTarget(0)) GetActionTarget(0)->~CableActivation(count);
	if (GetActionTarget(1)) GetActionTarget(1)->~CableActivation(count);
}

/** Called by cable cars whenever one proceeds off this cable. Will be forwarded to connected objects.
	count decreases the activation value. Stations will stop animation only if all activations are deactivated. */
public func Deactivation(int count)
{
	// Count must be > 0
	if (count < 1) return FatalError("Cable Line: Deactivation() was called with count < 1.");
	activations -= count;
	if (GetActionTarget(0)) GetActionTarget(0)->~CableDeactivation(count);
	if (GetActionTarget(1)) GetActionTarget(1)->~CableDeactivation(count);
}

/* Saving */

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