/*-- Chemical Lab --*/

#include Library_Structure
#include Library_Ownable
#include Library_Producer

local hold_production;

func Construction(object creator)
{
	SetAction("Default");
	return _inherited(creator, ...);
}

/*-- Production --*/

public func IsProduct(id product_id)
{
	return product_id->~IsChemicalProduct();
}

private func ProductionTime(id toProduce) { return 100; }
private func PowerNeed() { return 100; }

public func NeedRawMaterial(id rawmat_id)
{
	return true;
}

public func OnProductionStart(id product)
{
	AddEffect("Working", this, 100, 1, this);
	hold_production = false;
}

public func OnProductionHold(id product)
{
	hold_production = true;
}

public func OnProductionContinued(id product)
{
	hold_production = false;
}

public func OnProductionFinish(id product)
{
	RemoveEffect("Working", this);
}

protected func FxWorkingTimer()
{
	if(!hold_production)
		Smoking();
}

private func Smoking()
{
	if (!Random(4)) Smoke(16 * GetCalcDir(),-14,16);
	if (!Random(6)) Smoke(10 * GetCalcDir(),-14,15+Random(3));
}

local ActMap = {
	Default = {
		Prototype = Action,
		Name = "Default",
		Procedure = DFA_NONE,
		Directions = 2,
		FlipDir = 1,
		Length = 1,
		Delay = 0,
		FacetBase=1,
		NextAction = "Default",
	},
};

local Name = "$Name$";
local Description ="$Description$";
local BlastIncinerate = 100;
local HitPoints = 70;