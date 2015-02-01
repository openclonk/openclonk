/**
	Compensator
	A small structure which stores surplus energy available in a network.
	
	@author Zapper, Maikel
*/

#include Library_Structure
#include Library_Ownable
#include Library_PowerProducer
#include Library_PowerConsumer
#include Library_Flag

local DefaultFlagRadius = 90;

// Power storage variables.
local stored_power;
static const POWR_COMP_PowerUsage = 20;
static const POWR_COMP_MaxStorage = 10800; // 15 * 36 * 20

// Variables for displaying the charge.
local leftcharge, rightcharge, anim;

protected func Construction(object creator)
{
	stored_power = 0;
	anim = PlayAnimation("Charge", 1, Anim_Const(GetAnimationLength("Charge")), Anim_Const(1000));
	SetAction("Default");
	return _inherited(creator, ...);
}

protected func Initialize()
{
	leftcharge = CreateObjectAbove(Compensator_ChargeShower, 7 * GetCalcDir(), 10, NO_OWNER);
	leftcharge->Init(this);
	rightcharge = CreateObjectAbove(Compensator_ChargeShower, -6 * GetCalcDir(), 10, NO_OWNER);
	rightcharge->Init(this);
	AddTimer("PowerCheck", 10);
	return _inherited(...);
}

protected func Incineration()
{
	if (stored_power == 0)
		return Extinguish();
	
	for (var i = 0; i < 2; ++i)
	{
		var x = -7 + 14 * i;
		var b = CreateObject(Compensator_BurningBattery, x, 6, NO_OWNER);
		// Set controller for correct kill tracing.
		b->SetController(GetController()); 
		b->SetSpeed(-30 + 60 * i + RandomX(-10, 10), RandomX(-50, -30));
	}
	return Explode(30);
}


/*-- Power --*/

public func SetCharge(int to)
{
	stored_power = BoundBy(to, 0, POWR_COMP_MaxStorage);
	RefreshAnimationPosition();
	return;
}

private func PowerCheck()
{
	// Not fully constructed or neutral compensators don't do anything.
	if (GetCon() < 100 || GetOwner() == NO_OWNER)
		return;
		
	// Always register as a producer when power is bigger than zero.
	if (stored_power > 0)
		RegisterPowerProduction(POWR_COMP_PowerUsage);
		
	// Always register as a consumer if power is below max.
	if (stored_power < POWR_COMP_MaxStorage)
		RegisterPowerRequest(POWR_COMP_PowerUsage);
	
	return;
}


/*-- Power Production --*/

// Produces power on demand, so not steady.
public func IsSteadyPowerProducer() { return false; }

// Producer priority depends on the amount of power that is stored.
public func GetProducerPriority() { return 50 * (2 * stored_power - POWR_COMP_MaxStorage) / POWR_COMP_MaxStorage; }

// Callback from the power library for production of power request.
public func OnPowerProductionStart(int amount) 
{ 
	// Start the production of power.
	if (!GetEffect("ProducePower", this))
		AddEffect("ProducePower", this, 1, 2, this);
	// Stop the consumption of power.
	if (GetEffect("ConsumePower", this))
	{
		RemoveEffect("ConsumePower", this);
		// Notify the power network.
		UnregisterPowerRequest();
	}
	return true;
}

// Callback from the power library requesting to stop power production.
public func OnPowerProductionStop()
{
	// Stop the production of power.
	if (GetEffect("ProducePower", this))
		RemoveEffect("ProducePower", this);
	return true;
}

protected func FxProducePowerStart(object target, proplist effect, int temp)
{
	if (temp) 
		return 1;
	// Set Interval to 2.
	effect.Interval = 2;	
	// Sparkle effect when releasing power.
	if (!GetEffect("Sparkle", this))
		AddEffect("Sparkle", this, 1, 1, this);
	return 1;
}

protected func FxProducePowerTimer(object target, proplist effect, time)
{
	// Increase the stored power.
	stored_power -= effect.Interval * POWR_COMP_PowerUsage;
	// Refresh the animation.
	RefreshAnimationPosition();
	// If stored power is zero then stop producing power.
	if (stored_power <= 0)
	{
		// Notify the power network.
		UnregisterPowerProduction();
		return -1;
	}
	return 1;
}

protected func FxProducePowerStop(object target, proplist effect, int reason, bool temp)
{
	if (temp) 
		return 1;
	if (GetEffect("Sparkle", this))
		RemoveEffect("Sparkle", this);
	return 1;
}


/*-- Power Consumption --*/

// It has a low consumer priority so that all other consumers are supplied first.
public func GetConsumerPriority() { return 0; }

// Callback from the power library saying there is enough power.
public func OnEnoughPower()
{
	// Start the consumption of power.
	if (!GetEffect("ConsumePower", this))
		AddEffect("ConsumePower", this, 1, 2, this);
	// Stop the production of power.
	if (GetEffect("ProducePower", this))
	{
		RemoveEffect("ProducePower", this);
		// Notify the power network.
		UnregisterPowerProduction();
	}
	return _inherited(...);
}

// Callback from the power library saying there is not enough power.
public func OnNotEnoughPower()
{
	if (GetEffect("ConsumePower", this))
		RemoveEffect("ConsumePower", this);
	return _inherited(...);
}

protected func FxConsumePowerStart(object target, proplist effect, int temp)
{
	if (temp) 
		return 1;
	// Set Interval to 2.
	effect.Interval = 2;	
	return 1;
}

protected func FxConsumePowerTimer(object target, proplist effect, int time)
{
	// Increase the stored power.
	stored_power += effect.Interval * POWR_COMP_PowerUsage;
	// Refresh the animation.
	RefreshAnimationPosition();
	// If fully charged remove this effect.
	if (stored_power >= POWR_COMP_MaxStorage)
	{
		// Notify the power network.
		UnregisterPowerRequest();
		return -1;
	}
	return 1;
}

protected func FxConsumePowerStop(object target, proplist effect, int reason, bool temp)
{
	if (temp) 
		return 1;
	return 1;
}


/*-- Animations & Effects --*/

private func RefreshAnimationPosition()
{
	var charge = (stored_power * 100) / POWR_COMP_MaxStorage;
	/*var current = GetAnimationPosition(anim);
	var len = GetAnimationLength("Charge");
	SetAnimationPosition(anim, Anim_Linear(current, current, len - (charge * len) / 100, 35, ANIM_Hold));*/
	leftcharge->To(Min(charge, 50)*2);
	rightcharge->To(Max(0, charge-50)*2);
}

protected func FxSparkleTimer(object target, proplist effect, int time)
{
	effect.Interval *= 2;
	if (effect.Interval > 35 * 3) 
		return -1;
	CreateParticle("StarSpark", PV_Random(-3, 3), PV_Random(-14, -10), PV_Random(-5, 5), PV_Random(-8, 0), 10, Particles_Magic(), 4);
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
