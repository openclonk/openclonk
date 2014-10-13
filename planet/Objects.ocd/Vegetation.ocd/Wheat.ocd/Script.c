/*
	Wheat
	Author: Clonkonaut

	Easy crop for farming.
*/

#include Library_Plant
#include Library_Crop

private func SeedArea() { return 60; }
private func SeedChance() {	return 250; }
private func SeedAmount() { return 4; } // small seed area -> don't allow too many plants
private func SeedOffset() { return 20; }

protected func Construction()
{
	// Editable ActMap
	ActMap = { Prototype = this.Prototype.ActMap };
	StartGrowth(this.growth);
	return _inherited(...);
}

protected func Initialize()
{
	SetAction("Swing");

	AddEffect("WaterCheck", this, 2, 70, this);
	AddEffect("WindCheck", this, 3, 350, this);
}

/* Absorb water to grow faster */

protected func FxWaterCheckTimer(object obj, effect)
{
	// Fully grown
	if (GetCon() == 100) return -1;
	// Submerged
	if (InLiquid())
	{
		var degrowth = true;
		// Ignore minimum amount of water if small
		if (GetCon() < 20)
			if (!GBackLiquid(0,-5))
				degrowth = false;
		if (degrowth)
		{
			var grow_effect = GetEffect("IntGrowth", this);
			if (!grow_effect) { grow_effect = StartGrowth(this.degrowth); return; }
			if (grow_effect.growth == this.degrowth) return;
			grow_effect.growth = this.degrowth;
			return;
		}
	}
	// Decrease water amount
	if (effect.water)
		effect.water--;
	// Search for water
	var water = 0;
	for (var i = 0; i < GetObjWidth()+1; i++)
	{
		var y = (GetObjHeight()/2)+1;
		var x = i-(GetObjWidth()/2);
		if (!GBackSolid(x,y)) continue;
		while(GBackSolid(x,y) && y) --y;
		if (!y) continue;
		if (MaterialName(GetMaterial(x,y)) == "Water")
			if (ExtractLiquid(x,y))
				water++;
		if (water == 5) // maximum amount of water extracted in one check
			break;
	}
	// Fasten growth if needed
	effect.water += water;
	if (effect.water)
	{
		var grow_effect = GetEffect("IntGrowth", this);
		if (!grow_effect) { grow_effect = StartGrowth(this.fastgrowth); return; }
		if (grow_effect.growth == this.fastgrowth) return;
		grow_effect.growth = this.fastgrowth;
	}
	else
	{
		var grow_effect = GetEffect("IntGrowth", this);
		if (!grow_effect) { grow_effect = StartGrowth(this.growth); return; }
		if (grow_effect.growth == this.growth) return;
		grow_effect.growth = this.growth;
	}
}

/* Check the wind to adjust the swinging speed of the stalks */

protected func FxWindCheckStart(object obj, effect)
{
	if (Abs(GetWind()) < 25) effect.speed = 0;
	else if (Abs(GetWind()) == 100) effect.speed = 1;
	else effect.speed = 4 - Abs(GetWind())/25;
	SetSwingSpeed(effect.speed);
}

protected func FxWindCheckTimer(object obj, effect)
{
	var change = false;
	if (Abs(GetWind()) < 25 && effect.speed != 0)
	{
		effect.speed = 0;
		change = true;
	}
	if (Abs(GetWind()) == 100 && effect.speed != 1)
	{
		effect.speed = 1;
		change = true;
	}
	if (Inside(Abs(GetWind()), 25, 99) && 4 - Abs(GetWind())/25 != effect.speed)
	{
		effect.speed = 4 - Abs(GetWind())/25;
		change = true;
	}
	if (change)
	{
		SetSwingSpeed(effect.speed);
	}
}

// Sets the actions' delays
// Of course this does mean that: 0 = no movement, 1 = fastest movement, >1 = movements slows down
public func SetSwingSpeed(int delay)
{
	ActMap["Swing"] = { Prototype = ActMap["Swing"], Delay = delay };
	ActMap["Swing2"] = { Prototype = ActMap["Swing2"], Delay = delay };
	// Restart action
	var phase = GetPhase();
	SetAction(GetAction());
	SetPhase(phase);
}

/* Callbacks */

public func IsCrop() { return true; }

local Name = "$Name$";
local Description = "$Description$";
local Collectible = 0;
local growth = 3;
local degrowth = -6;
local fastgrowth = 9;

local ActMap = {
		Swing = {
			Prototype = Action,
			Name = "Swing",
			Procedure = DFA_NONE,
			Directions = 2,
			FlipDir = 1,
			X = 0,
			Y = 0,
			Wdt = 10,
			Hgt = 20,
			Delay = 1,
			Length = 18,
			NextAction = "Swing2"
		},
		Swing2 = {
			Prototype = Action,
			Name = "Swing",
			Procedure = DFA_NONE,
			Directions = 2,
			FlipDir = 1,
			X = 0,
			Y = 0,
			Wdt = 10,
			Hgt = 20,
			Delay = 1,
			Length = 18,
			Reverse = 1,
			NextAction = "Swing"
		}
};
