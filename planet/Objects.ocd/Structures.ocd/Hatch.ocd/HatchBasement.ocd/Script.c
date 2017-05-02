/**
	Basement
	Provides basements to structures, but can also be built as a single object.
	
	@author: Maikel
*/

#include Basement

public func IsHammerBuildable() { return false; }

/*-- Properties --*/

local Name = "$Name$";
local Description ="$Description$";
local HitPoints = 80;
local Plane = 190;
local Components = {Rock = 2};
