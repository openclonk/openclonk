
#include Library_Ownable

/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Touchable = 1;
local Components = {GoldBar = 3};

public func IsHammerBuildable() { return true; }

public func NoConstructionFlip() { return true; }


/*-- Engine callbacks --*/

public func Definition(proplist type)
{
	// Ensure that the poperties exist
	if (!type.EditorProps)
	{
		type.EditorProps = {};
	}

	// Ensure that the list of poses exists
	if (!type.EditorProps.IdolPoses)
	{
		type.EditorProps.IdolPoses =
		{
			Type = "enum",
			Name = "$ChoosePose$",
			EditorHelp = "$ChoosePoseHelp$",
			Options = [],
			Set = "SetAction",
		};
	}

	// Fill the list with poses
	for (var action in GetProperties(ActMap))
	{
		var option = {
			Name = ActMap[action].EditorName ?? ActMap[action].Name,
			EditorHelp = ActMap[action].EditorHelp ?? "$ChoosePoseHelp$",
			Value = ActMap[action].Name,
		};

		PushBack(type.EditorProps.IdolPoses.Options, option);
	}
}

public func Initialize()
{
	SetAction("Default");
	return _inherited(...);
}


/*-- Actions --*/

local ActMap = {
	Default = {
		Prototype = Action,
		Name = "Default",
		NextAction = "Default",
		Animation = "Pose01",
		Procedure = DFA_NONE,
		EditorName = "$PoseDefaultName$",
		EditorHelp = "$PoseDefaultHelp$",
	},

	ItemRightLow = {
		Prototype = Action,
		Name = "ItemRightLow",
		NextAction = "ItemRightLow",
		Animation = "Pose02",
		Procedure = DFA_NONE,
		EditorName = "$PoseItemRightLowName$",
		EditorHelp = "$PoseItemRightLowHelp$",
	},
	
	ItemRightHigh = {
		Prototype = Action,
		Name = "ItemRightHigh",
		NextAction = "ItemRightHigh",
		Animation = "Pose03",
		Procedure = DFA_NONE,
		EditorName = "$PoseItemRightHighName$",
		EditorHelp = "$PoseItemRightHighHelp$",
	},
	
	ItemLeftLow = {
		Prototype = Action,
		Name = "ItemLeftLow",
		NextAction = "ItemLeftLow",
		Animation = "Pose04",
		Procedure = DFA_NONE,
		EditorName = "$PoseItemLeftLowName$",
		EditorHelp = "$PoseItemLeftLowHelp$",
	},
	
	ItemLeftHigh = {
		Prototype = Action,
		Name = "ItemLeftHigh",
		NextAction = "ItemLeftHigh",
		Animation = "Pose05",
		Procedure = DFA_NONE,
		EditorName = "$PoseItemLeftHighName$",
		EditorHelp = "$PoseItemLeftHighHelp$",
	},
	
	ItemCentral = {
		Prototype = Action,
		Name = "ItemCentral",
		NextAction = "ItemCentral",
		Animation = "Pose06",
		Procedure = DFA_NONE,
		EditorName = "$PoseItemCentralName$",
		EditorHelp = "$PoseItemCentralHelp$",
	},
};
