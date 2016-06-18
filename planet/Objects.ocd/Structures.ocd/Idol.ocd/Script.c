
#include Library_Ownable

local Name = "$Name$";
local Description = "$Description$";
local Touchable = 1;
local Components = {GoldBar = 3};

public func IsHammerBuildable() { return true; }

public func NoConstructionFlip() { return true; }