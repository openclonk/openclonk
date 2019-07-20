/*-- Hook --*/

local tower, rope;

/* Connection */

public func ControlUse(object clonk, int x, int y)
{
	// Search for objects to connect with!
	var connect = FindObjects(Find_Category(C4D_Vehicle), Find_AtPoint(), Find_Not(Find_Func("NoLiftTowerConnection")), Find_Not(Find_Func("IsEnvironment")));
	if (!GetLength(connect)) return true;
	if (GetLength(connect) == 1) return ConnectTo(connect[0]);

	var menu = clonk->CreateRingMenu(GetID(), this);
	for (var connect_object in connect)
		menu->AddItem(connect_object);
	menu->Show();
}

public func Selected(object menu, object selected)
{
	return ConnectTo(selected->GetSymbol());
}

public func ConnectTo(object connect)
{
	Hook();
/*	rope->BreakRope(true);
	SetRope(true);
	rope->Connect(tower, connect);*/
	rope->Reconnect(connect);
	AddEffect("Connecting", this, 1, 1, this, nil, connect);
	return true;
}

private func Hook()
{
	if (Contained()) Exit();
	this.Collectible = 0;
	SetCategory(C4D_StaticBack);
}
private func Unhook()
{
	this.Collectible = 1;
	SetCategory(C4D_Object);
}

private func FxConnectingStart(object target, effect, int temp, object connect_object)
{
	if (temp) return;
	effect.connection = connect_object;
}

private func FxConnectingTimer(object target, effect)
{
	if (!rope)
	{
		Unhook();
		return -1;
	}
	if (!effect.connection)
	{
		Unhook();
		rope->BreakRope(true);
		SetRope();
		return -1;
	}
	SetPosition(effect.connection->GetX(), effect.connection->GetY());
}

public func Connected()
{
	return GetEffect("Connecting", this);
}

public func IsInteractable(object clonk)
{
	return !this.Collectible && clonk->GetAction() == "Walk";
}

public func GetInteractionMetaInfo(object clonk)
{
	return { IconID = LiftTower_Hook, Description = "$Unhook$" };
}

public func Interact(object clonk)
{
	if (clonk->Collect(this))
	{
		RemoveEffect("Connecting", this);
		Unhook();
		rope->BreakRope(true);
		SetRope();
		return true;
	}
	return false;
}

/* Events */

protected func Hit()
{
	Sound("Hits::Materials::Metal::LightMetalHit?");
}

func Construction(object constructor)
{
	tower = constructor;
}

func Initialize()
{
	AddTimer("Rotation", 2);
}

func SetRope(bool no_connect)
{
	rope = CreateObjectAbove(LiftTower_Rope, 0, 0, NO_OWNER);
	if (!no_connect) rope->Connect(tower, this);
	tower->SetRope(rope);
	return rope;
}

public func Destruction()
{
	if (rope)
		rope->HookRemoved();
}

protected func Rotation()
{
	if (!rope) return;
	SetR(rope->GetHookAngle());
}

public func NoLiftTowerConnection() { return true; }

local Name = "$Name$";
local Description = "$Description$";
local Collectible = 1;
