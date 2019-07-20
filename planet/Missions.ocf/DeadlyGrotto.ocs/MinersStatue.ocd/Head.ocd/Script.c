/*--
	Miner's statue part: Head
	Author: Sven2

	Missing part of the statue
--*/

protected func Hit(x, y)
{
	StonyObjectHit(x, y);
	return true;
}

public func Definition(proplist def)
{
}

local Collectible = false;
local Name = "$Name$";
local Description = "$Description$";
local Touchable = 1;

