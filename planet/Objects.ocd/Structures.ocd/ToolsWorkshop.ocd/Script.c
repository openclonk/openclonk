/*-- Tools workshop --*/

#include Library_Structure
#include Library_Ownable
#include Library_Producer
#include Library_LampPost

local hold_production;

public func LampPosition(id def) { return [GetCalcDir()*14,-6]; }

public func Construction(object creator)
{
	SetAction("Default");
	return _inherited(creator, ...);
}

public func IsHammerBuildable() { return true; }

public func Initialize()
{
	hold_production = false;
	return _inherited(...);
}

/*-- Production --*/

public func IsProduct(id product_id)
{
	return product_id->~IsToolProduct();
}

private func ProductionTime(id product) { return _inherited(product, ...) ?? 150; }
public func PowerNeed() { return 40; }

public func OnProductionStart(id product)
{
	SetSign(product);
	AddEffect("Working", this, 100, 1, this);
	hold_production = false;
	return;
}

public func OnProductionHold(id product)
{
	hold_production = true;
	return;
}

public func OnProductionContinued(id product)
{
	hold_production = false;
	return;
}

public func OnProductionFinish(id product)
{
	RemoveEffect("Working", this);
	SetSign(nil);
	return;
}

protected func FxWorkingTimer()
{
	if (!hold_production)
		Smoking();
	return 1;
}

private func Smoking()
{
	if (Random(6)) Smoke(16 * GetCalcDir(), -20, 10);
	if (Random(8)) Smoke(14 * GetCalcDir(), -20, 8 + Random(3));
	return 1;
}

public func SetSign(id def)
{
	if (!def)
		return SetGraphics("", nil, GFX_Overlay, 4);
	SetGraphics("", def, GFX_Overlay, 4);
	SetObjDrawTransform(200, 0, -19500 * GetCalcDir(), 0, 200, 2500, GFX_Overlay);
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
func Definition(def) {
	SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(2000, 0, 7000),Trans_Rotate(-20, 1, 0, 0),Trans_Rotate(30, 0, 1, 0)), def);
	return _inherited(def, ...);
}
local Name = "$Name$";
local Description ="$Description$";
local ContainBlast = true;
local BlastIncinerate = 100;
local FireproofContainer = true;
local HitPoints = 70;
local Components = {Wood = 3, Metal = 2};