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
// Rates the current efficiency of the wind generator from 0 to 100.
local last_wind_efficiency = nil;

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

public func IsHammerBuildable() { return true; }

protected func Initialize()
{
	// First initialize the libraries (especially the flag library).
	_inherited(...);	
	// Create a helper object for the wheel.
	wheel = CreateObject(WindGenerator_Wheel, 0, 0, NO_OWNER);
	wheel->SetParent(this);
	// Start the animation for the wheel.
	PlayAnimation(TurnAnimation(), 5, wheel->Anim_R(0, GetAnimationLength(TurnAnimation())));
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
	var weighted_wind_sum = 0;
	var max_wind = 0;
	var x_coords = [-150, -75, 0, 75, 150];
	var weights = [10, 25, 30, 25, 10]; // Should sum up to 100.
	var count = 5;
	for (var i = 0; i < count; ++i)
	{
		var raw_wind = GetWind(x_coords[i], -30);
		weighted_wind_sum += weights[i] * raw_wind;
		
		if (Abs(raw_wind) > max_wind) max_wind = Abs(raw_wind);
	}
	var wind_output = weighted_wind_sum / 100;
	if (max_wind != 0)
		last_wind_efficiency = 100 * Abs(wind_output) / max_wind;
	
	return wind_output;
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
		Sound(["Hits::Materials::Wood::WoodCreak?","Structures::HingeCreak?"][Random(2)], {custom_falloff_distance = 75});
	return;
}


/* Interaction */

// Provides an own interaction menu.
public func HasInteractionMenu() { return true; }

// Show hint about efficiency in the interaction menu.
public func GetInteractionMenus(object clonk)
{
	var menus = _inherited(clonk, ...) ?? [];
	var prod_menu =
	{
		title = "$Efficiency$",
		entries_callback = this.GetInfoMenuEntries,
		callback = nil,
		BackgroundColor = RGB(0, 0, 50),
		Priority = 20
	};
	PushBack(menus, prod_menu);
	
	return menus;
}

public func GetInfoMenuEntries()
{
	var text = "$EfficiencyUnknown$";
	if (last_wind_efficiency != nil)
	{
		var hint = "$EfficiencyGood$";
		if (last_wind_efficiency < 25)
			hint = "$EfficiencyBad$||$EfficiencyGeneral$";
		else if (last_wind_efficiency < 75)
			hint = "$EfficiencyMedium$||$EfficiencyGeneral$";
		text = Format("$Efficiency$: %3d%%|%s", last_wind_efficiency, hint); 
	}
	var info_text =
	{
		Right = "100%", Bottom = "8em",
		text = {Top = "2em", Text = text, Style = GUI_TextVCenter | GUI_TextHCenter},
		image = {Right = "2em", Bottom = "2em", Symbol = Icon_Lightbulb}
	};
	return [{symbol = this, custom = info_text}];
}

/*-- Properties --*/

protected func Definition(def) 
{
	SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(2000, 0, 7000), Trans_Rotate(-20, 1, 0, 0), Trans_Rotate(30, 0, 1, 0)), def);
	return _inherited(def, ...);
}

local Name = "$Name$";
local Description = "$Description$";
local BlastIncinerate = 20;
local ContactIncinerate = 5;
local NoBurnDecay = true;
local HitPoints = 50;
local Components = {Wood = 3, Metal = 1};
