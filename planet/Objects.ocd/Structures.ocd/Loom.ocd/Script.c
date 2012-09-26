/*-- Tools workshop --*/

#include Library_Structure
#include Library_Ownable
#include Library_Producer


local hold_production;
local production_animation;

public func Construction(object creator)
{
	var r = 5; if(!Random(2)) r = -5;
	this.MeshTransformation = Trans_Mul(Trans_Rotate(r, 0, 1, 0), Trans_Rotate(-2, 1, 0, 0));
	SetAction("Default");
	production_animation = PlayAnimation("Working", 1, Anim_Const(0), Anim_Const(1000));
	return _inherited(creator, ...);
}

public func Initialize()
{
	hold_production = false;
	production_animation = -1;
	return _inherited(...);
}

/*-- Production --*/

public func IsProduct(id product_id)
{
	return product_id->~IsToolProduct();
}

private func ProductionTime(id toProduce) { return 150; }
private func PowerNeed() { return 50; }

public func NeedRawMaterial(id rawmat_id)
{
	return true;
}

public func OnProductionStart(id product)
{
	SetSign(product);
	AddEffect("Working", this, 100, 1, this);
	hold_production = false;
	ContinueProductionAnimation(true);
	return;
}

public func OnProductionHold(id product)
{
	hold_production = true;
	ContinueProductionAnimation(false);
	return;
}

public func OnProductionContinued(id product)
{
	hold_production = false;
	ContinueProductionAnimation(true);
	return;
}

public func OnProductionFinish(id product)
{
	RemoveEffect("Working", this);
	ContinueProductionAnimation(false);
	SetSign(nil);
	return;
}

protected func FxWorkingTimer()
{
	return 1;
}

func ContinueProductionAnimation(bool play)
{
	var pos = 0;
	if(production_animation != -1)
		pos = GetAnimationPosition(production_animation);
	else
		production_animation = PlayAnimation("Working", 1, Anim_Linear(pos, 0, GetAnimationLength("Working"), ProductionTime()/2, ANIM_Loop), Anim_Const(1000));
	if(play)
		SetAnimationPosition(production_animation, Anim_Linear(pos, 0, GetAnimationLength("Working"), ProductionTime()/2, ANIM_Loop));
	else SetAnimationPosition(production_animation, Anim_Const(pos));
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
		FacetBase=1,
		NextAction = "Default",
	},
};
func Definition(def) {
	SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(2000,0,7000),Trans_Rotate(-20,1,0,0),Trans_Rotate(30,0,1,0)), def);
}
local Name = "$Name$";
local Description ="$Description$";
local BlastIncinerate = 100;
local HitPoints = 40;