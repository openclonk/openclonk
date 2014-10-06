/*-- Steam engine --*/

#include Library_Structure
#include Library_Ownable
#include Library_PowerProducer
#include Library_Flag

local DefaultFlagRadius = 200;

static const SteamEngine_produced_power = 300;

local iFuelAmount;
local power_seconds;

func Construction(object creator)
{
	iFuelAmount = 0;
	power_seconds = 0;

	SetAction("Default");
	AddTimer("ContentsCheck", 30);
	return _inherited(creator, ...);
}

public func IsContainer() { return true; }

func RejectCollect(id item, object obj)
{
	if (obj->~IsFuel())
		return false;
	return true;
}

func Collection(object obj, bool put)
{
	Sound("Clonk");
}

func ContentsCheck()
{
	//Ejects non fuel items immediately
	var fuel;
	if(fuel = FindObject(Find_Container(this), Find_Not(Find_Func("IsFuel")))) 
	{
		fuel->Exit(-53,21, -20, -1, -1, -30);
		Sound("Chuff"); //I think a 'chuff' or 'metal clonk' sound could be good here -Ringwaul
	}
	
	// Still active?
	if(GetAction() == "Work") return true;
	// or still warm water in the tank?!
	if(GetEffect("CreatesPower", this))
		return true;
	
	// not needed?
	if(GetPendingPowerAmount() == 0)
		return false;
	
	// Still has some fuel?
	if(iFuelAmount) return SetAction("Work");
	
	// Search for new fuel
	if(fuel = FindObject(Find_Container(this), Find_Func("IsFuel")))
	{
		iFuelAmount += fuel->~GetFuelAmount() / 2;
		fuel->RemoveObject();
		SetAction("Work");
		return true;
	}

	return false;
}

func ConsumeFuel()
{	
	if(iFuelAmount > 0)
	{
		// every fuel unit gives power for one second
		--iFuelAmount;
		power_seconds += 1;
		if(!GetEffect("CreatesPower", this))
			AddEffect("CreatesPower", this, 1, 36, this);
	}
	
	// All used up?
	if(!iFuelAmount || ((GetPendingPowerAmount() == 0) && (GetCurrentPowerBalance() >= SteamEngine_produced_power)))
	{
		SetAction("Default");
		ContentsCheck();
	}
}

func FxCreatesPowerStart(target, effect, temp)
{
	if(temp) return;
	// fixed amount
	MakePowerProducer(SteamEngine_produced_power);
	
	AddEffect("Smoking", this, 1, 5, this);
	Sound("SteamEngine", false, nil, nil, 1);
}

func FxCreatesPowerTimer(target, effect)
{
	if(power_seconds == 0) return -1;
	--power_seconds;
}

func FxCreatesPowerStop(target, effect, reason, temp)
{
	if(temp) return;
	// disable producer
	MakePowerProducer(0);
	
	if(GetEffect("Smoking", this))
		RemoveEffect("Smoking", this);
	Sound("SteamEngine", false, nil, nil, -1);
}

func FxSmokingTimer()
{
	Smoke(-20 * GetCalcDir(), -15, 10);
	return 1;
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
	Work = {
		Prototype = Action,
		Name = "Work",
		Procedure = DFA_NONE,
		Directions = 2,
		FlipDir = 1,
		Length = 20,
		Delay = 2,
		FacetBase = 1,
		NextAction = "Work",
		Animation = "Work",
		EndCall = "ConsumeFuel",
	},
};

func Definition(def) {
	SetProperty("MeshTransformation", Trans_Mul(Trans_Rotate(25,0,1,0), Trans_Scale(625)), def);
}
local BlastIncinerate = 130;
local HitPoints = 100;
local Name = "$Name$";
local Description = "$Description$";
