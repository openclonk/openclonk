/*-- Cable car selector --*/

local cable_car;


protected func Construction(object constructor)
{
	cable_car = constructor;
	SetPlrViewRange(10);
	this["Visibility"] = VIS_Owner;
	AddEffect("Particles", this, 1, 2, this);
}

public func FixTo(object station)
{
	// no owner? -> panic
	if (GetOwner() == NO_OWNER) return RemoveObject();
	// Set owner's view
	SetPosition(station->GetX(), station->GetY());
	SetPlrView(GetOwner(), this);
	SetCursor(GetOwner(), this, true);
}

public func ObjectControl(int plr, int ctrl, int x, int y, int strength, bool repeat, bool release)
{
	if (release) return false;

	if (ctrl == CON_Left)
		return cable_car->ShiftSelection(-1, this);
	if (ctrl == CON_Right)
		return cable_car->ShiftSelection(+1, this);

	if (ctrl == CON_Use || ctrl == CON_Interact)
		return cable_car->AcceptSelection(this);
	if (ctrl == CON_UseAlt)
		return cable_car->AbortSelection(this);
}

protected func FxParticlesTimer(object target, effect, int time)
{
	var angle = time*10 % 360;
	CreateParticle("PSpark", Sin(angle, 13), -Cos(angle, 13), 0, 0, 16, RGBa(255, 255, 255, 150), this);
}