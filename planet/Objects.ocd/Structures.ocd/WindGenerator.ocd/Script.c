/**
	Wind Generator 
	Converts wind into a steady power supply.
	
	@author Maikel	
*/

#include Library_Structure
#include Library_Ownable
#include Library_PowerProducer
#include Library_Flag

local DefaultFlagRadius = 90;

/*-- Initialization --*/

local wind_anim;
local last_power;
local wheel;

func TurnAnimation() { return "Turn"; }
func MinRevolutionTime() { return 4500; } // in frames

protected func Construction()
{
	SetProperty("MeshTransformation",Trans_Mul(Trans_Rotate(RandomX(-15,15),0,1,0), Trans_Translate(1200,0,0)));
	return _inherited(...);
}

protected func Initialize()
{
	// create wheel
	wheel = CreateObject(WindGenerator_Wheel, 0, 0, NO_OWNER);
	wheel->SetParent(this);

	// Start animation
	wind_anim = PlayAnimation(TurnAnimation(), 5, wheel->Anim_R(0, GetAnimationLength(TurnAnimation())), Anim_Const(1000));
	
	// Set initial position
	AddTimer("Wind2Turn", 4);
	Wind2Turn();
	return _inherited(...);
}

/*-- Power Production --*/

// Always produces power, irrespective of the demand.
public func IsSteadyPowerProducer() { return true; }

// High priority so that this drained first.
public func GetProducerPriority() { return 100; }

// Callback from the power library for production of power request.
public func OnPowerProductionStart(int amount) 
{ 
	// This is a steady producer, so it is already running.
	return true;
}

// Callback from the power library requesting to stop power production.
public func OnPowerProductionStop()
{
	// This is a steady producer, so don't stop anything.
	return true;
}

// Returns the wind weighted over several points.
private func GetWeightedWind()
{
	return (
		(10 * GetWind(-150, -30)) + 
		(25 * GetWind( -75, -30)) + 
		(30 * GetWind(   0, -30)) + 
		(25 * GetWind( +75, -30)) + 
		(10 * GetWind(+150, -30))
		) / 100;
}

// Turns wind into power and adjusts the power production accordingly.
public func Wind2Turn()
{
	// Only produce power if fully constructed.
	if (GetCon() < 100) 
		return;
	
	var current_wind = GetWeightedWind();
	var power = 0;
	
	if (wheel->Stuck() || wheel->HasStopped())
	{
		power = 0;
	}
	else
	{
		power = Abs(wheel->GetRDir(this->MinRevolutionTime() / 90));
		if (power < 5) 
			power = 0;
		else 
			power = Max((power + 5) / 25, 1) * 50;
	}

	// Register the new power production if it changed.
	if (last_power != power)
	{
		last_power = power;
		RegisterPowerProduction(last_power);
	}

	// Adjust the wheel speed.
	wheel->SetRDir(current_wind * 90, this->MinRevolutionTime());
	// Make some sounds.
	if (Abs(current_wind) >= 10 && Random(15 - Abs(current_wind / 10)) < 5)
		Sound(["WoodCreak?","HingeCreak?"][Random(2)], false, nil, nil, nil, 75);
	return;
}


/*-- Properties --*/

protected func Definition(def) 
{
	SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(2000, 0, 7000), Trans_Rotate(-20, 1, 0, 0), Trans_Rotate(30, 0, 1, 0)), def);
}

local Name = "$Name$";
local Description = "$Description$";
local BlastIncinerate = 60;
local ContactIncinerate = 5;
local HitPoints = 50;
