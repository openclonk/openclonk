/*--
	Windmill
	Authors: Ringwaul, Clonkonaut

	Crushes seeds into flour if there is wind or another power source.
--*/

#include Library_Producer

local wind_anim;

protected func Initialize()
{
	SetProperty("MeshTransformation", Trans_Rotate(RandomX(-15,15),0,1,0));
	wind_anim = PlayAnimation("Spin", 5, Anim_Const(0), Anim_Const(1000));
	// Set initial position
	Wind2Turn();
	return _inherited(...);
}

func Wind2Turn()
{
	// Fade linearly in time until next timer call
	var start = 0;
	var end = GetAnimationLength("Spin");
	if(GetWind() < 0)
	{
		start = end;
		end = 0;
	}

	// Number of frames for one revolution: the more wind the more revolutions per frame.
	var wind = Abs(GetWind());
	if(wind == 0) wind = 1;
	var l = 18000/wind;

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

// Overloaded from PowerConsumer
// As long as their is wind, the windmill has power
public func CheckPower()
{
	if (Abs(GetWind())) return true;
	return inherited(...);
}

/*-- Production --*/

private func IsProduct(id product_id)
{
	return product_id->~IsWindmillProduct();
}
private func ProductionTime() { return 290; }
private func PowerNeed(id product) { return 100; }

public func NeedsRawMaterial(id rawmat_id)
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
	CreateParticle("Axe_WoodChip", -5, 40, 5 - Random(11), RandomX(6,13) * -1, 20, RGB(255,255,255), this);
	return 1;
}

public func OnProductEjection(object product)
{
	product->SetPosition(GetX() + 30, GetY() + 40);
	product->SetSpeed(0, -17);
	product->SetR(30 - Random(59));
	Sound("Pop");
	return;
}

func Definition(def) {
	SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(2000,0,7000),Trans_Rotate(-20,1,0,0),Trans_Rotate(30,0,1,0)), def);
}
local Name = "$Name$";
