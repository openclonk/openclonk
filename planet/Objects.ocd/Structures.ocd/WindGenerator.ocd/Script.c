/**
	Wind Generator 
	Converts wind into a steady power supply.
	
	@author Maikel	
*/

#include Library_Structure
#include Library_Ownable
#include Library_Flag
#include Library_PowerProducer

local DefaultFlagRadius = 90;

/*-- Initialization --*/

local last_power;
local wheel;

func TurnAnimation() { return "Turn"; }
func MinRevolutionTime() { return 4500; } // in frames

protected func Construction()
{
	// First initialize the libraries (especially the flag library).
	_inherited(...);
	// Rotate each wind generator a bit to make them look different.
	SetProperty("MeshTransformation", Trans_Mul(Trans_Rotate(RandomX(-15, 15), 0, 1, 0), Trans_Translate(1200, 0, 0)));
	return;
}

protected func Initialize()
{
	// First initialize the libraries (especially the flag library).
	_inherited(...);	
	// Create a helper object for the wheel.
	wheel = CreateObject(WindGenerator_Wheel, 0, 0, NO_OWNER);
	wheel->SetParent(this);
	// Start the animation for the wheel.
	PlayAnimation(TurnAnimation(), 5, wheel->Anim_R(0, GetAnimationLength(TurnAnimation())), Anim_Const(1000));
	// Initialize a regular check of the wheel's position and speed, also handles power updates.
	last_power = 0;
	AddTimer("Wind2Turn", 4);
	Wind2Turn();
	return;
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
public func OnPowerProductionStop(int amount)
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
	// Determine the current power production.	
	var power = 0;
	if (!wheel->Stuck() && !wheel->HasStopped())
	{		
		// Produced power ranges from 0 to 80 in steps of 10.
		power = Abs(wheel->GetRDir(MinRevolutionTime() / 90));
		power = BoundBy((10 * power + 60) / 125 * 10, 0, 80);
	}
	// Register the new power production if it changed.
	if (last_power != power)
	{
		last_power = power;
		RegisterPowerProduction(last_power);
	}
	// Adjust the wheel speed.
	var current_wind = GetWeightedWind();
	wheel->SetRDir(current_wind * 90, MinRevolutionTime());
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
