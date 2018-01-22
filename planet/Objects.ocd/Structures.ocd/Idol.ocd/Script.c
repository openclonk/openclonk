/**
	Idol
	A valuable piece of art.
	
	@author 
*/


#include Library_Ownable

static const IDOL_ITEM_LEFT = 1 << 0;
static const IDOL_ITEM_RIGHT = 1 << 1;

static const IDOL_BONE_LEFT = "Arm_L_Item";
static const IDOL_BONE_RIGHT = "Arm_R_Item";

local left_hand_item;
local right_hand_item;


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
			AsyncGet = "GetAction",
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
	
	// Ensure that the list of items exists
	if (!type.EditorProps.IdolItemL)
	{
		type.EditorProps.IdolItemL =
		{
			Type = "enum",
			Name = "$ChooseItemLeft$",
			EditorHelp = "$ChooseItemHelp$",
			Options = [],
			AsyncGet = "EditorGetItemLeft",
			Set = "EditorSetItemLeft",
		};
	}
	if (!type.EditorProps.IdolItemR)
	{
		type.EditorProps.IdolItemR =
		{
			Type = "enum",
			Name = "$ChooseItemRight$",
			EditorHelp = "$ChooseItemHelp$",
			Options = [],
			AsyncGet = "EditorGetItemRight",
			Set = "EditorSetItemRight",
		};
	}
	
	// Fill the list with items - this is currently implemented here, but should be implemented in the items.
	// For now, I did not want to clutter the items with code that might not be included in the long run
	var rotate180 = Trans_Rotate(180, 0, 0, 1);
	var scale = 1200;
	EditorAddStatueItem(nil);
	EditorAddStatueItem(Axe, rotate180, scale);
	EditorAddStatueItem(Club, rotate180, scale);
	EditorAddStatueItem(Sword, rotate180, scale);
	EditorAddStatueItem(Bow, rotate180, scale);
	EditorAddStatueItem(Javelin, rotate180, 1500, "Javelin");
	EditorAddStatueItem(Shield, Trans_Mul(Trans_Rotate(+90, 0, 0, 1), Trans_Rotate(+90, 1, 0, 0), Trans_Translate(0, -1000, 0)), scale, nil, IDOL_ITEM_LEFT);
	EditorAddStatueItem(Shield, Trans_Mul(Trans_Rotate(-90, 0, 0, 1), Trans_Rotate(-90, 1, 0, 0), Trans_Translate(0, -1000, 0)), scale, nil, IDOL_ITEM_RIGHT);
}

public func Initialize()
{
	SetAction("Default");
	left_hand_item = {};
	right_hand_item = {};
	return _inherited(...);
}

/*-- Custom editor actions --*/

private func EditorAddStatueItem(def item, array mesh_transform, int scale, string child_bone, int hands)
{
	hands = hands ?? (IDOL_ITEM_LEFT | IDOL_ITEM_RIGHT);
	child_bone = child_bone ?? "main";
	
	if (scale) mesh_transform = Trans_Mul(Trans_Scale(scale, scale, scale), mesh_transform);

	var item_info = {
		Type = item,
		Bone = child_bone,
		MeshTransformation = mesh_transform,
	};
	
	var name = "$ChooseItemNone$";
	var editor_help = nil;
	if (item)
	{
		name = item->GetName();
		editor_help = item.EditorHelp ?? item->~GetEditorHelp();
	}
	var option = {
		Name = name,
		EditorHelp = editor_help,
		Value = item_info,
	};

	if (hands & IDOL_ITEM_LEFT)
	{
		PushBack(this.EditorProps.IdolItemL.Options, option);
	}

	if (hands & IDOL_ITEM_RIGHT)
	{
		PushBack(this.EditorProps.IdolItemR.Options, option);
	}
}

private func EditorSetItemLeft(proplist item_info)
{
	EditorSetItem(item_info, IDOL_BONE_LEFT, left_hand_item);
}

private func EditorSetItemRight(proplist item_info)
{
	EditorSetItem(item_info, IDOL_BONE_RIGHT, right_hand_item);
}

private func EditorSetItem(proplist item_info, string parent_bone, proplist save_settings)
{
	if (save_settings.AttachedMesh)
	{
		DetachMesh(save_settings.AttachedMesh);
	}

	var submesh = AttachMesh(item_info.Type, parent_bone, item_info.Bone, item_info.MeshTransformation);
	
	save_settings.AttachedMesh = submesh;
	save_settings.Choice = item_info;
}

private func EditorGetItemLeft()
{
	return left_hand_item.Choice;
}

private func EditorGetItemRight()
{
	return right_hand_item.Choice;
}


/*-- Scenario saving --*/

public func SaveScenarioObject(props)
{
	if (!inherited(props, ...))
		return false;
	// Save action.
	SaveScenarioObjectAction(props);
	// Save hand items.
	if (left_hand_item.Choice)
		props->AddCall("IdolItemLeft", this, "EditorSetItemLeft", left_hand_item.Choice);
	if (right_hand_item.Choice)
		props->AddCall("IdolItemRight", this, "EditorSetItemRight", right_hand_item.Choice);
	return true;
}


/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Touchable = 1;
local Components = {GoldBar = 3};

public func IsHammerBuildable() { return true; }

public func NoConstructionFlip() { return true; }


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
