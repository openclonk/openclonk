/*-- compensator --*/

#include Library_Structure
#include Library_Ownable
#include Library_PowerProducer
#include Library_PowerConsumer
#include Library_Flag

local DefaultFlagRadius = 90;

static const Compensator_max_seconds = 15;
static const Compensator_power_usage = 50;

local power_seconds;


local Name = "$Name$";
local Description = "$Description$";
local leftcharge, rightcharge, lastcharge;
local anim;

func Construction(object creator)
{
	power_seconds = 0;
	lastcharge = 0;
	
	anim = PlayAnimation("Charge", 1, Anim_Const(GetAnimationLength("Charge")), Anim_Const(1000));

	SetAction("Default");
	return _inherited(creator, ...);
}

func Initialize()
{
	leftcharge = CreateObject(Compensator_ChargeShower, 7 * GetCalcDir(), 10, NO_OWNER);
	leftcharge->Init(this);
	rightcharge = CreateObject(Compensator_ChargeShower, -6 * GetCalcDir(), 10, NO_OWNER);
	rightcharge->Init(this);
	AddTimer("EnergyCheck", 100);
	return _inherited(...);
}

func OnNotEnoughPower()
{
	// not enough power to sustain a battery - turn off
	if(GetEffect("ConsumePower", this))
		RemoveEffect("ConsumePower", this);
	
	ScheduleCall(this, "UnmakePowerConsumer", 1, 0);
	return _inherited(...);
}

// devour energy
func OnEnoughPower()
{
	if(!GetEffect("ConsumePower", this))
		AddEffect("ConsumePower", this, 1, 36, this);
	return _inherited(...);
}

func SetCharge(int to)
{
	power_seconds = BoundBy(to, 0, Compensator_max_seconds);
	RefreshAnimationPosition();
}

func RefreshAnimationPosition()
{
	var charge = (power_seconds * 100) / Compensator_max_seconds;
	/*var current = GetAnimationPosition(anim);
	var len = GetAnimationLength("Charge");
	SetAnimationPosition(anim, Anim_Linear(current, current, len - (charge * len) / 100, 35, ANIM_Hold));*/
	leftcharge->To(Min(charge, 50)*2);
	rightcharge->To(Max(0, charge-50)*2);
}	

func FxConsumePowerTimer(target, effect, time)
{
	++power_seconds;
	RefreshAnimationPosition();
	// fully charged?
	if(power_seconds >= Compensator_max_seconds)
	{
		UnmakePowerConsumer();
		return -1;
	}	
	return 1;
}

func EnergyCheck()
{
	if(GetCon() < 100) return;
	
	// consuming - everything is alright
	if(GetEffect("ConsumePower", this))
		return true;
	// producing - nothing to change either
	if(GetEffect("ProducePower", this))
		return true;
	
	// neutral compensators don't do anything
	if(GetOwner() == NO_OWNER) return false;
	
	// are we needed?
	if(power_seconds > 0)
	{
		var s = GetPendingPowerAmount();
		if(s > 0)
		{
			// turn on, start the machines!
			AddEffect("ProducePower", this, 1, 36, this);
			return true;
		}
	}
	
	// fully charged
	if(power_seconds >= Compensator_max_seconds)
		return false;
	
	// can we leech power?
	var p = GetCurrentPowerBalance();
	
	// we have some play here?
	if(p >= Compensator_power_usage)
	{
		MakePowerConsumer(Compensator_power_usage);
		return true;
	}
	
	return false;
}

func FxProducePowerStart(target, effect, temp)
{
	if(temp) return;
	MakePowerProducer(Compensator_power_usage);
	
	// todo: effects
	AddEffect("Sparkle", this, 1, 1, this);
}

func FxSparkleTimer(target, effect, time)
{
	effect.Interval *= 2;
	if(effect.Interval > 35*3) return -1;
	CreateParticle("StarSpark", PV_Random(-3, 3), PV_Random(-14, -10), PV_Random(-5, 5), PV_Random(-8, 0), 10, Particles_Magic(), 4);
}

func FxProducePowerTimer(target, effect, time)
{
	--power_seconds;
	RefreshAnimationPosition();
	if(power_seconds <= 0)
	{
		return -1;
	}
	
	// stop when not needed
	if((GetCurrentPowerBalance() >= Compensator_power_usage) && GetPendingPowerAmount() == 0)
		return -1;
		
	return 1;
}

func FxProducePowerStop(target, effect, reason, temp)
{
	if(temp) return;
	MakePowerProducer(0);
	
	if(GetEffect("Sparkle", this))
		RemoveEffect("Sparkle", this);
}

func Incineration()
{
	if(power_seconds == 0)
		return Extinguish();
	
	
	for(var i = 0; i < 2; ++i)
	{
		var x = -7 + 14 * i;
		var b = CreateObject(Compensator_BurningBattery, x, 6, NO_OWNER);
		b->SetController(GetController()); // killtracing

		b->SetSpeed(-30 + 60 * i + RandomX(-10, 10), RandomX(-50, -30));
	}
	
	Explode(30);
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
local BlastIncinerate = 1;
local HitPoints = 25;
local ContactIncinerate = 1;
