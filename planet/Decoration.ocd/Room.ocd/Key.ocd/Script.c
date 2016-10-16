/**
	Key
	Use to open something.

	@author Maikel
*/


public func Hit()
{
	Sound("Hits::Materials::Metal::LightMetalHit?");
}

// Set the color of the key, may be corresponding to the color of a door for example.
public func SetColor(int color)
{
	return SetClrModulation(color);
}

public func IsKey() { return true; }


/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Collectible = true;
local BorderBound = C4D_Border_Sides;