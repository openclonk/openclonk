/*--
	Windmill
	Authors: Ringwaul, Clonkonaut

	Crushes seeds into flour using power - is an own power producer too
--*/

#include Library_Ownable
#include Library_Producer
#include Library_PowerConsumer
#include Library_PowerProducer

local wind_anim;
local last_wind;

protected func Construction()
{
	SetProperty("MeshTransformation", Trans_Rotate(-30,0,1,0));
	return _inherited(...);
}

protected func Initialize()
{
	// Set initial position
	wind_anim = PlayAnimation("Spin", 5, Anim_Const(0), Anim_Const(1000));
	Wind2Turn();
	return _inherited(...);
}

func Wind2Turn()
{
	if(GetCon()  < 100) return;
	
	// Fade linearly in time until next timer call
	var start = 0;
	var end = GetAnimationLength("Spin");
	if(GetWind() < 0)
	{
		start = end;
		end = 0;
	}
	
	var power = Abs(GetWind());
	if(power < 5) power = 0;
	else power = Max(((power + 5) / 25), 1) * 50;
	
	if(last_wind != power)
	{
		last_wind = power;
		MakePowerProducer(last_wind);
	}
	
	// Number of frames for one revolution: the more wind the more revolutions per frame.
	if(last_wind == 0) last_wind = 1;
	var l = 18000/last_wind;

	// Note ending is irrelevant since this is called again after 35 frames
	if(l > 0)
	{
		SetAnimationPosition(wind_anim, Anim_Linear(GetAnimationPosition(wind_anim), start, end, l, ANIM_Loop));
	}
	else
	{
		SetAnimationPosition(wind_anim, Anim_Const(GetAnimationPosition(wind_anim)));
	}
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

protected func Collection()
{
	Sound("Clonk");
	return;
}

public func FxCrushingTimer(object target, proplist effect, int time)
{
	CreateParticle("Axe_WoodChip", -12, 40, 5 - Random(11), RandomX(6,13) * -1, 20, RGB(255,255,255), this);
	return 1;
}

public func OnProductEjection(object product)
{
	product->SetPosition(GetX() + 25, GetY() + 40);
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

func Definition(def) {
	SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(2000,0,7000),Trans_Rotate(-20,1,0,0),Trans_Rotate(30,0,1,0)), def);
}

local Name = "$Name$";
local Description = "$Description$";