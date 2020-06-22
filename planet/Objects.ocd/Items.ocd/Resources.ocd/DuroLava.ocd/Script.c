/**
	DuroLava
	Represents a single material pixel of non-solidifying lava.
	
	@author Sven2
*/

#include Lava

public func GetLiquidType() { return "DuroLava"; }
public func GetParentLiquidType() { return Lava; } // Combine with regular lava in menus such as e.g. the pump liquid selection

local Name="$Name$";
local Description="$Description$";
