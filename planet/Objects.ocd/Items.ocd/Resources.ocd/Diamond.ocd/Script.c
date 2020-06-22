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
public func QueryRejectRebuy() { return true; }

public func SaveScenarioObject(props, ...)
{
	// Do not save diamonds in sockets
	if (!inherited(props, ...)) return false;
	if (Contained() && Contained()->GetID() == Diamond_Socket) return false;
	return true;
}

local Name = "$Name$";
local Description = "$Description$";
local Collectible = 1;
