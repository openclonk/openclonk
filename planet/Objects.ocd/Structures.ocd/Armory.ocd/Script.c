/*-- Armory --*/

#include Library_Structure
#include Library_Ownable
#include Library_Producer

local hold_production;

func Construction(object creator)
{
	SetAction("Default");
	this.MeshTransformation = Trans_Rotate(RandomX(0, 20), 0, 1, 0);
	return _inherited(creator, ...);
}

/*-- Production --*/

public func IsProduct(id product_id)
{
	return product_id->~IsArmoryProduct();
}

private func ProductionTime(id toProduce) { return 100; }
private func PowerNeed() { return 100; }

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
	var x = 8;
	var y = -17;
	if (!Random(2))
		Smoke(x,y + 4,20);
	if(!Random(2))
		CreateParticle("Fire", x + RandomX(-1, 1), y + RandomX(-2, 2), 0, -1, 30 + Random(10), RGB(255, 255, 255), this);
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