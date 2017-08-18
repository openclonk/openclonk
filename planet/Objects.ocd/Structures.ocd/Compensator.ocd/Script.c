/**
	Compensator
	A small structure which stores surplus energy available in a network.
	
	@author Zapper, Maikel
*/

#include Library_Structure
#include Library_Ownable
#include Library_PowerStorage
#include Library_Flag

local DefaultFlagRadius = 90;

// Storage power: the amount of power the storage can store or deliver when it
// operatores. 
public func GetStoragePower() { return 20; }

// Storage capacity: the amount of energy a power storage can store. The amount 
// is expressed in power frames.
public func GetStorageCapacity() { return 10800; } // 15 * 36 * 20

// Variables for displaying the charge.
local leftcharge, rightcharge, anim;

protected func Construction(object creator)
{
	anim = PlayAnimation("Charge", 1, Anim_Const(GetAnimationLength("Charge")), Anim_Const(1000));
	SetAction("Default");
	return _inherited(creator, ...);
}

public func IsHammerBuildable() { return true; }

protected func Initialize()
{
	leftcharge = CreateObjectAbove(Compensator_ChargeShower, 7 * GetCalcDir(), 10, NO_OWNER);
	leftcharge->Init(this);
	rightcharge = CreateObjectAbove(Compensator_ChargeShower, -6 * GetCalcDir(), 10, NO_OWNER);
	rightcharge->Init(this);
	return _inherited(...);
}

protected func Incineration(int caused_by)
{
	if (GetStoredPower() == 0)
		return Extinguish();
		
	// Set controller for correct kill tracing.
	SetController(caused_by);
	for (var i = 0; i < 2; ++i)
	{
		var x = -7 + 14 * i;
		var b = CreateObject(Compensator_BurningBattery, x, 6, NO_OWNER); 
		b->SetSpeed(-30 + 60 * i + RandomX(-10, 10), RandomX(-50, -30));
	}
	return Explode(30);
}

/*-- Animations & Effects --*/

private func OnStoredPowerChange()
{
	RefreshAnimationPosition();
}

private func RefreshAnimationPosition()
{
	var charge = (GetStoredPower() * 100) / GetStorageCapacity();
	/*var current = GetAnimationPosition(anim);
	var len = GetAnimationLength("Charge");
	SetAnimationPosition(anim, Anim_Linear(current, current, len - (charge * len) / 100, 35, ANIM_Hold));*/
	leftcharge->To(Min(charge, 50)*2);
	rightcharge->To(Max(0, charge-50)*2);
}

public func OnPowerProductionStart(int amount) 
{ 
	// Sparkle effect when releasing power.
	if (!GetEffect("Sparkle", this))
		AddEffect("Sparkle", this, 1, 1, this);
	return _inherited(amount, ...);
}

public func OnPowerProductionStop(int amount)
{
	if (GetEffect("Sparkle", this))
		RemoveEffect("Sparkle", this);
	return _inherited(...);
}

protected func FxSparkleTimer(object target, proplist effect, int time)
{
	effect.Interval *= 2;
	if (effect.Interval > 36 * 3) 
		return -1;
	CreateParticle("ElectroSpark", PV_Random(-3, 3), PV_Random(-15, -13), PV_Random(-5, 5), PV_Random(-5, 1), 10, Particles_ElectroSpark1(), 10);
	return 1;
}


/*-- Properties --*/

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

local Name = "$Name$";
local Description = "$Description$";
local BlastIncinerate = 1;
local HitPoints = 25;
local ContactIncinerate = 1;
local NoBurnDecay = true;
local Components = {Coal = 1, Metal = 1};
