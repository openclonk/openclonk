/*--- Gold Bar ---*/

protected func Hit()
{
	Sound("MetalHit?");
	return 1;
}

public func IsFoundryProduct() { return true; }
public func IsValuable(){ return true; }

local Name = "$Name$";
local Description = "$Description$";
local Collectible = 1;
local Rebuy = true;
