/*-- Shipyard --*/

#include Library_Structure
#include Library_Ownable
#include Library_Producer
#include Library_LampPost

local animWork;
local meshAttach;

public func LampPosition(id def) { return [GetCalcDir()*-6,30]; }

func Initialize()
{
	animWork = PlayAnimation("Work", 1, Anim_Const(0));
	return _inherited(...);
}

func Construction(object creator)
{
	SetAction("Wait");
	return _inherited(creator, ...);
}

public func IsHammerBuildable() { return true; }

/*-- Production --*/

public func IsProduct(id product_id)
{
	return product_id->~IsShipyardProduct();
}

private func ProductionTime(id product) { return _inherited(product, ...) ?? 400; }
public func PowerNeed() { return 80; }

private func FxIntWorkAnimTimer(object target, proplist effect, int timer)
{
	if(effect.paused == true) return 1;
	
//	Message("Working...");

	var tickAmount = 50;
	var animSpot = GetAnimationPosition(animWork);
	//-50 is to dodge around an engine crash. If it reaches (near) the end of the
	//animation, it dies for some reason. :(
	var animLength = GetAnimationLength("Work") - 50;
	
	//loop anim
	if(animSpot + tickAmount > animLength){
		SetAnimationPosition(animWork, Anim_Const(animSpot + tickAmount - animLength));
	}
	//otherwise, advance animation
	else SetAnimationPosition(animWork, Anim_Const(animSpot + tickAmount));
}

local workEffect;

public func OnProductionStart(id product)
{
	workEffect = AddEffect("IntWorkAnim", this, 1,1,this);
	return _inherited(product, ...);
}

public func OnProductionHold(id product)
{
	workEffect.paused = true;
	return _inherited(product, ...);
}

public func OnProductionContinued(id product)
{
	workEffect.paused = false;
	return _inherited(product, ...);
}

public func OnProductionFinish(id product)
{
	RemoveEffect(nil, this, workEffect);
	return _inherited(product, ...);
}

public func Definition(def)
{
	def.MeshTransformation = Trans_Mul(Trans_Scale(800), Trans_Translate(2000, 0, 0), Trans_Rotate(8, 0, 1, 0));
	def.PictureTransformation = Trans_Mul(Trans_Translate(0, -25000, 50000), Trans_Scale(600));
	return _inherited(def, ...);
}

local ActMap = {
	Wait = {
		Prototype = Action,
		Name = "Wait",
		Procedure = DFA_NONE,
		Directions = 2,
		FlipDir = 1,
		Length = 1,
		Delay = 0,
		FacetBase=1,
		NextAction = "Wait",
	},
};

local Name = "$Name$";
local Description ="$Description$";
local ContainBlast = true;
local BlastIncinerate = 100;
local FireproofContainer = true;
local HitPoints = 70;
local Components = {Wood = 4, Metal = 3};