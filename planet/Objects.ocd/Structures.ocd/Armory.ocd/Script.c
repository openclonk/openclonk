/*-- Armory --*/

#include Library_Structure
#include Library_Ownable
#include Library_Producer
#include Library_LampPost

local hold_production;

public func LampPosition(id def) { return [GetCalcDir()*28,4]; }

func Construction(object creator)
{
	SetAction("Default");
	this.MeshTransformation = Trans_Rotate(RandomX(0, 20), 0, 1, 0);
	return _inherited(creator, ...);
}

public func IsHammerBuildable() { return true; }

/*-- Production --*/

public func IsProduct(id product_id)
{
	return product_id->~IsArmoryProduct();
}

private func ProductionTime(id product) { return _inherited(product, ...) ?? 100; }
public func PowerNeed() { return 60; }

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
		CreateParticle("Fire", PV_Random(x-1, x+1), PV_Random(y-2, y+2), 0, PV_Random(-1, 0), PV_Random(18, 36), Particles_Fire(), 2);
}

func Definition(proplist def)
{
	def.PictureTransformation = Trans_Mul(Trans_Translate(7000, 0, 30000), Trans_Rotate(-10, 1, 0, 0), Trans_Rotate(25, 0, 1, 0));
	return _inherited(def, ...);
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
local Components = {Wood = 3, Metal = 2, Loam = 2};