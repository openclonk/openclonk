/*--- Diamond ---*/

public func Hit()
{
	Sound("Hits::Materials::Glass::GlassHit?");
	return 1;
}

public func Place(int amount, proplist area, ...)
{
	// Diamonds naturally occur in sockets.
	return Diamond_Socket->Place(amount, area, ...);
}

public func IsValuable() { return true; }

local Name = "$Name$";
local Description = "$Description$";
local Collectible = 1;
