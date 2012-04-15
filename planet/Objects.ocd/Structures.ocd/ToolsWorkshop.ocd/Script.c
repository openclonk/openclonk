/*-- Tools workshop --*/

#include Library_Ownable
#include Library_Producer


local hold_production;

public func Initialize()
{
	// SetProperty("MeshTransformation", Trans_Rotate(RandomX(-30,30),0,1,0));
	hold_production = false;
	return _inherited(...);
}



/*-- Production --*/

public func IsProduct(id product_id)
{
	return product_id->~IsToolProduct();
}

private func ProductionTime(object toProduce) { return 150; }
private func PowerNeed() { return 100; }

public func NeedRawMaterial(id rawmat_id)
{
	return true;
}

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
	if(!hold_production)
		Smoking();
	return 1;
}

private func Smoking()
{
	if (Random(6)) Smoke(+16,-14,16);
	if (Random(8)) Smoke(10,-14,15+Random(3));
	return 1;
}

public func SetSign(id def)
{
	if (!def)
		return SetGraphics("", nil, 1, 4);
	SetGraphics("", def, 1, 4);
	SetObjDrawTransform(200, 0, 19500, 0, 200, 2500, 1);
}

local ActMap = {
	Build = {
		Prototype = Action,
		Name = "Build",
		Procedure = DFA_NONE,
		Length = 40,
		Delay = 1,
		FacetBase=1,
		NextAction = "Build",
		//Animation = "Turn",
		PhaseCall="Smoking",
	},
};
func Definition(def) {
	SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(2000,0,7000),Trans_Rotate(-20,1,0,0),Trans_Rotate(30,0,1,0)), def);
}
local Name = "$Name$";
local Description ="$Description$";
