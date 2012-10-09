/*-- 
	Energy bars
	Author: Maikel
	
	Displays an energy bar above an object, uses Library_Bars.
--*/


#include Library_Bars

private func EnergyBarWidth() { return 24; }
private func EnergyBarHeight() { return 6; }

protected func Initialize()
{
	return;
}

public func SetTarget(object target, int vtx)
{
	// Create energy bar.
	SetBarLayers(2, 0);
	SetBarOffset(0, - target->GetDefHeight() / 2, 0);
	SetBarDimensions(target->GetDefWidth(), EnergyBarHeight(), 0);
	// Attach energy bar to the object.
	SetAction("Attach", target);
	SetActionData(vtx);
	SetClrModulation(RGB(200, 0, 0), 3);
	UpdateEnergy();
	return;
}

protected func UpdateEnergy()
{
	var target = GetActionTarget();
	var hp = target.HitPoints - target->GetDamage();
	if (!hp)
		return RemoveObject();
	var max_hp = target.HitPoints;
	var promille;
	if (max_hp == 0) 
		promille = 0;
	else 
		promille = 1000 * hp / max_hp;
	SetBarProgress(promille, 0);	
	return;
}

protected func AttachTargetLost() { RemoveObject(); }

local Name = "$Name$";
local ActMap = {
	Attach = {
		Prototype = Action,
		Name = "Attach",
		Procedure = DFA_ATTACH,
		Length = 1,
		Delay = 1,
		X = 0,
		Y = 0,
		Wdt = 32,
		Hgt = 32,
		NextAction = "Attach",
		StartCall = "UpdateEnergy",
	},
};
