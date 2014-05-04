/*--- Gem of Power---*/

#include Ruby

local was_collected;

func Entrance()
{
	if (!was_collected)
	{
		was_collected = true;
		SetCategory(C4D_Object);
		GameCallEx("OnTreasureCollected", this);
	}
	return _inherited(...);
}

func QueryOnSell()
{
	GameCallEx("OnTreasureSold", this);
	return false; // allow sale
}

// returns the color of the gem (used for effects)
func GetGemColor()
{
	return RGB(0, Random(100), Random(100));
}

public func IsValuable(){ return true; }

local Name = "$Name$";
local Description = "$Description$";
local Collectible = 1;
local Rebuy = true;
local Plane = 510;
