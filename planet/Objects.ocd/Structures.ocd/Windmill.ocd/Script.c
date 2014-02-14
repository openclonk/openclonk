/*--
	Windmill
	Authors: Ringwaul, Clonkonaut

	Crushes seeds into flour using power - is an own power producer too
--*/

#include Library_Structure
#include Library_Ownable
#include Library_Producer
#include Library_PowerConsumer
#include Library_PowerProducer
#include Library_Flag

local DefaultFlagRadius = 90;

local wind_anim;
local last_wind;

func TurnAnimation(){return "Spin";}
func MinRevolutionTime(){return 18000;} // in frames

protected func Construction(object creator)
{
	SetProperty("MeshTransformation", Trans_Rotate(-30,0,1,0));
	SetAction("Default");
	
	// uses functions of the wind generator
	this.Wind2TurnEx = WindGenerator.Wind2Turn;
	this.GetWeightedWind = WindGenerator.GetWeightedWind;	
	AddTimer("CollectionZone", 1);
	
	return _inherited(creator, ...);
}

protected func Initialize()
{
	// create wheel
	(this.wheel = CreateObject(WindGenerator_Wheel, 0, 0, NO_OWNER))->Set(this, 150);
	
	// Set initial position
	wind_anim = PlayAnimation(TurnAnimation(), 5, this.wheel->Anim_R(GetAnimationLength(TurnAnimation()), 0), Anim_Const(1000));
	return _inherited(...);
}

func Wind2Turn()
{	
	// dummy, uses the function of the WindGenerator
	this->Wind2TurnEx();
}


/*-- Production --*/

private func IgnoreKnowledge() { return true; }

private func IsProduct(id product_id)
{
	return product_id->~IsWindmillProduct();
}
private func ProductionTime(id toProduce) { return 290; }
private func PowerNeed() { return 75; }

public func NeedRawMaterial(id rawmat_id)
{
	if (rawmat_id == Seeds)
		return true;
	return false;
}

public func OnProductionStart(id product)
{
	AddEffect("Crushing", this, 100, 10, this);
	return;
}

public func OnProductionContinued(id product)
{
	AddEffect("Crushing", this, 100, 10, this);
	return;
}

public func OnProductionHold(id product)
{
	RemoveEffect("Crushing", this);
	return;
}

public func OnProductionFinish(id product)
{
	RemoveEffect("Crushing", this);
	return;
}	

// Timer, check for objects to collect in the designated collection zone
func CollectionZone()
{
	if (GetCon() < 100) return;

	if (!(FrameCounter() % 35)) Wind2Turn();

	for (var object in FindObjects(Find_InRect(-18 + 21 * GetDir(),35,15,15), Find_OCF(OCF_Collectible), Find_NoContainer(), Find_Layer(GetObjectLayer())))
		Collect(object);
}

protected func Collection()
{
	Sound("Clonk");
	return;
}

public func FxCrushingTimer(object target, proplist effect, int time)
{
	var dir = GetCalcDir();
	var particles =
	{
		Prototype = Particles_WoodChip(),
		R = 255,
		G = 200,
		B = 100
	};
	CreateParticle("Dust", PV_Random(11 * dir, 13 * dir), 40, PV_Random(-5, 5), PV_Random(-13, -6), PV_Random(36 * 3, 36 * 10), particles, 3);
	
	return 1;
}

public func OnProductEjection(object product)
{
	product->SetPosition(GetX() - 25 * GetCalcDir(), GetY() + 40);
	product->SetSpeed(0, -17);
	product->SetR(30 - Random(59));
	Sound("Pop");
	return;
}

protected func RejectCollect(id item, object collect)
{
	if(collect->~IsMillIngredient()) return false;
	else
		return true;
}

func IsInteractable() { return true; }

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
	SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(2000,0,7000),Trans_Rotate(-20,1,0,0),Trans_Rotate(30,0,1,0)), def);
}
local BlastIncinerate = 100;
local HitPoints = 70;
local Name = "$Name$";
local Description = "$Description$";
