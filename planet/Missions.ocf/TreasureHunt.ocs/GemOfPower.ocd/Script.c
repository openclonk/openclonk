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

// returns the color of the gem (used for effects)
func GetGemColor()
{
	return RGB(0, Random(100), Random(100));
}

local Name = "$Name$";
local Description = "$Description$";
local Collectible = 1;
local Rebuy = true;
local Plane = 510;
