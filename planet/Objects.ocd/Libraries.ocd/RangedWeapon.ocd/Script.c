/**
	Library_RangedWeapon
	Adds functionality to overload weapon speed
	
	@author: Sven2
*/

local animation_set;
local speed_multiplier = 100;

// To be overloaded by weapon
local DefaultLoadTime = nil;
local DefaultLoadTime2 = nil;
local DefaultShootTime = nil;
local DefaultShootTime2 = nil;

public func IsRangedWeapon()
{
	return true; // identify this as a ranged weapon for the AI
}

public func Initialize(...)
{
	// Set initial animation speeds
	if (!animation_set) animation_set = {};
	SetSpeedMultiplier(speed_multiplier);
	return _inherited(...);
}

public func GetReloadTime()
{
	// Get weapon reload time after speed adjustments
	return animation_set.LoadTime;
}

public func GetShootTime()
{
	// Get shoot time after speed adjustments
	return animation_set.ShootTime;
}

public func SetSpeedMultiplier(int new_multiplier)
{
	// Updates the weapon speed
	if (GetType(DefaultLoadTime2)) animation_set.LoadTime2 = Max(1, DefaultLoadTime2 * 100 / new_multiplier);
	if (GetType(DefaultLoadTime)) animation_set.LoadTime = Max(1+animation_set.LoadTime2, DefaultLoadTime * 100 / new_multiplier);
	if (GetType(DefaultShootTime2)) animation_set.ShootTime2 = Max(1, DefaultShootTime2 * 100 / new_multiplier);
	if (GetType(DefaultShootTime)) animation_set.ShootTime = Max(1+animation_set.ShootTime2, DefaultShootTime * 100 / new_multiplier);
	// Remember setting
	this.speed_multiplier = new_multiplier;
}

public func Definition(def, ...)
{
	// Editor + Scenario saving
	_inherited(def, ...);
	if (!def.EditorProps) def.EditorProps = {};
	def.EditorProps.speed_multiplier = {
		Name = "$SpeedMult$",
		EditorHelp = "$SpeedMultHelp$",
		Set = "SetSpeedMultiplier",
		Type = "int",
		Min = 1,
		Max = 20000,
		Save = "SpeedMultiplier",
		};
}
