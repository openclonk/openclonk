/*-- Steam engine --*/

#include Library_Ownable
#include Library_PowerProducer

local iFuelAmount;
local power_seconds;

func Construction()
{
	iFuelAmount = 0;
	power_seconds = 0;
	return _inherited(...);
}

func ContentsCheck()
{
	// Still active?
	if(!ActIdle())
		return true;

	// Still has some fuel?
	if(iFuelAmount) return SetAction("Work");
	
	var pFuel;
	// Search for new fuel
	if(pFuel = FindObject(Find_Container(this), Find_Func("IsFuel")))
	{
		iFuelAmount += pFuel->~GetFuelAmount();
		pFuel->RemoveObject();
		SetAction("Work");
		return 1;
	}

	//Ejects non fuel items immediately
	if(pFuel = FindObject(Find_Container(this), !Find_Func("IsFuel"))) {
	pFuel->Exit(-53,21, -20, -1, -1, -30);
	Sound("Chuff"); //I think a 'chuff' or 'metal clonk' sound could be good here -Ringwaul
	}

	return 0;
}

func ConsumeFuel()
{	
	if(iFuelAmount > 0)
	{
		// every fuel unit gives power for ten seconds
		power_seconds += 10;
		if(!GetEffect("CreatesPower", this))
			AddEffect("CreatesPower", this, 1, 36, this);
	}
	
	// All used up?
	if(!iFuelAmount)
	{
		SetAction("Idle");
		ContentsCheck();
	}
}

func FxCreatesPowerStart(target, effect, temp)
{
	if(temp) return;
	// fixed amount
	MakePowerProducer(300);
}

func FxCreatesPowerTimer(target, effect)
{
	if(power_seconds == 0) return -1;
	--power_seconds;
}

func FxCreatesPowerStop(target, effect, reason, temp)
{
	if(temp) return;
	// fixed amount
	MakePowerProducer(0);
}

local ActMap = {
Work = {
	Prototype = Action,
	Name = "Work",
	Procedure = DFA_NONE,
	Directions = 2,
	FlipDir = 1,
	Length = 20,
	Delay = 2,
	X = 0,
	Y = 0,
	Wdt = 110,
	Hgt = 80,
	NextAction = "Work",
	Animation = "Work",
	EndCall = "ConsumeFuel",
},
};
local Name = "$Name$";

func Definition(def) {
	SetProperty("MeshTransformation", Trans_Mul(Trans_Rotate(25,0,1,0), Trans_Scale(625)), def);
}