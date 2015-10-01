/*--- Diamond ---*/

public func Hit()
{
	Sound("GlassHit?");
	return 1;
}

public func IsValuable() { return true; }

local Name = "$Name$";
local Description = "$Description$";
local Collectible = 1;
local Rebuy = true;
