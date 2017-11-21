/*-- Inventor's Lab --*/

#include Library_Structure
#include Library_Ownable
#include Library_Producer
#include Library_LampPost

local hold_production;

public func LampPosition(id def) { return [GetCalcDir()*24,20]; }

func Construction(object creator)
{
	SetAction("Default");
	return _inherited(creator, ...);
}

public func IsHammerBuildable() { return true; }

/*-- Production --*/

public func IsProduct(id product_id)
{
	return product_id->~IsInventorProduct();
}

private func ProductionTime(id product) { return _inherited(product, ...) ?? 100; }
public func PowerNeed() { return 80; }

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
local ContainBlast = true;
local BlastIncinerate = 100;
local FireproofContainer = true;
local HitPoints = 70;
local Components = {Wood = 4, Metal = 3};