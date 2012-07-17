/*-- Shipyard --*/

#include Library_Structure
#include Library_Ownable
#include Library_Producer

func Construction(object creator)
{
	SetAction("Default");
	return _inherited(creator, ...);
}

/*-- Production --*/

public func IsProduct(id product_id)
{
	return product_id->~IsShipyardProduct();
}

private func ProductionTime(id toProduce) { return 400; }
private func PowerNeed() { return 150; }

public func NeedRawMaterial(id rawmat_id)
{
	return true;
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