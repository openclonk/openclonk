
#include Library_Ownable

local Name = "$Name$";
local Description = "$Description$";
local Touchable = 1;
local Components = {GoldBar = 3};

public func IsHammerBuildable() { return true; }

public func NoConstructionFlip() { return true; }

public func Initialize()
{
	SetAction("Default");
	return _inherited(...);
}

local ActMap = {
	Default = {
		Prototype = Action,
		Name = "Default",
		NextAction = "Default",
		Animation = "Pose01",
		Procedure = DFA_NONE,
	},

	ItemRightLow = {
		Prototype = Action,
		Name = "ItemRightLow",
		NextAction = "ItemRightLow",
		Animation = "Pose02",
		Procedure = DFA_NONE,
	},
	
	ItemRightHigh = {
		Prototype = Action,
		Name = "ItemRightHigh",
		NextAction = "ItemRightHigh",
		Animation = "Pose03",
		Procedure = DFA_NONE,
	},
	
	ItemLeftLow = {
		Prototype = Action,
		Name = "ItemLeftLow",
		NextAction = "ItemLeftLow",
		Animation = "Pose04",
		Procedure = DFA_NONE,
	},
	
	ItemLeftHigh = {
		Prototype = Action,
		Name = "ItemLeftHigh",
		NextAction = "ItemLeftHigh",
		Animation = "Pose05",
		Procedure = DFA_NONE,
	},
	
	ItemCentral = {
		Prototype = Action,
		Name = "ItemCentral",
		NextAction = "ItemCentral",
		Animation = "Pose06",
		Procedure = DFA_NONE,
	},
};
