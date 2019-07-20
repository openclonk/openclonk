/*-- Kitchen --*/

#include Library_Structure
#include Library_Ownable
#include Library_Producer
#include Library_LampPost

local hold_production;

public func LanternPosition(id def) { return [GetCalcDir()*19,-1]; }

func Construction(object creator)
{
	SetAction("Default");
	return _inherited(creator, ...);
}

public func IsHammerBuildable() { return true; }

/*-- Production --*/

func IsProduct(id product_id)
{
	return product_id->~IsKitchenProduct();
}

private func ProductionTime(id product) { return _inherited(product, ...) ?? 500; }
public func PowerNeed() { return 0; }

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
	if (!hold_production)
		Smoking();
}

private func Smoking()
{
	if (!Random(4)) Smoke(16 * GetCalcDir(),-14, 16);
	if (!Random(6)) Smoke(10 * GetCalcDir(),-14, 15 + Random(3));
	
	//Fire
	CreateParticle("Fire", 13 * GetCalcDir(), 16, PV_Random(-1, 1), PV_Random(-1, 1), PV_Random(18, 36), Particles_Fire(), 2);
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
		FacetBase = 1,
		NextAction = "Default",
	},
};

local Name = "$Name$";
local Description ="$Description$";
local ContainBlast = true;
local BlastIncinerate = 100;
local FireproofContainer = true;
local HitPoints = 70;
local Components = {Wood = 3, Rock = 2, Metal = 1};