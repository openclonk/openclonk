/*-- Airplane Reticle --*/

protected func Initialize()
{
	this["Visibility"] = VIS_Owner;
	AddEffect("IntRotate",this, 1, 1, this);
	return;
}

public func FxIntRotateTimer(object target, effect, int timer)
{
	if (!GetActionTarget()) RemoveObject();
	if (!target || !GetActionTarget()) return -1;
	SetR(GetActionTarget()->GetR());
}

/*-- Proplist --*/

protected func Definition(def)
{
	SetProperty("Name", "$Name$", def);
	SetProperty("ActMap", {
		Show = {
			Prototype = Action,
			Name = "Show",
			Procedure = DFA_ATTACH,
			Length = 1,
			Delay = 0,
			X = 0,
			Y = 0,
			Wdt = 40,
			Hgt = 20,
			NextAction = "Show",
		},
	}, def);
}
