/*
	Wheat
	Author: Clonkonaut

	Plant to get more seeds
*/

#include Library_Plant

func Construction()
{
	StartGrowth(this.growth);
	inherited(...);
}

protected func Initialize()
{
	AddEffect("WaterCheck", this, 2, 70, this);
}

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
			{
				water++;
				Log("water extracted at: %d,%d (%d,%d)", x,y,GetX()+x,GetY()+y);
			}
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

public func CanBeHarvested() { return true; }

local Name = "$Name$";
local Description = "$Description$";
local Collectible = 0;
local growth = 3;
local degrowth = -6;
local fastgrowth = 9;