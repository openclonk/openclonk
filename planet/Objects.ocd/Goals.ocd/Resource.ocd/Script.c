/*-- 
	Resource Extraction
	Author: Maikel
	
	Player must extract the resources of the specified type, the
	tolerance of this goal is 5% currently.
		
	TODO: Expand to liquids and digable materials.
--*/


#include Library_Goal

local resource_list; // List of materials to be mined.
local tolerance_list; // List of ignorable amounts of available objects.

protected func Initialize()
{
	resource_list = [];
	tolerance_list = [];
	return inherited(...);
}

/*-- Resources --*/

public func SetResource(string resource)
{
	var list_end = GetLength(resource_list);
	resource_list[list_end] = resource;
	var material = Material(resource);
	var mat_cnt = GetMaterialCount(material);
	tolerance_list[list_end] = Max(1, ExploitableObjectCount(mat_cnt, material) / 20);
	return;
}

/*-- Scenario saving --*/

public func SaveResource(string resource, int tolerance_count)
{
	var list_end = GetLength(resource_list);
	resource_list[list_end] = resource;
	tolerance_list[list_end] = tolerance_count;
	return;
}

public func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	for (var i = 0; i < GetLength(resource_list); i++)
		props->AddCall("Goal", this, "SaveResource", resource_list[i], tolerance_list[i]);
	return true;
}


/*-- Goal interface --*/

// The goal is fulfilled if all specified resource have been mined.
public func IsFulfilled()
{
	for (var i = 0; i < GetLength(resource_list); i++)
	{
		var material = Material(resource_list[i]);
		var tol = tolerance_list[i];
		var mat_cnt = GetMaterialCount(material);
		// Still solid material to be mined.
		if (mat_cnt == -1 || mat_cnt > ObjectCount2MaterialCount(tol, material))
			return false; 
		// Still objects of material to be collected.
		if (AvailableObjectCount(material) > 0)
			return false; 
	}
	// Goal fulfilled.
	return true;
}

// Shows or hides a message window with information.
public func GetDescription(int plr)
{
	var message;
	if (IsFulfilled())
	{
		message = "$MsgGoalFulfilled$";		
	}
	else
	{
		message = "$MsgGoalExtraction$";
		for (var i = 0; i < GetLength(resource_list); i++)
		{
			var material = Material(resource_list[i]);
			var tol = tolerance_list[i];
			var mat_cnt = GetMaterialCount(material);
			var res_id = GetBlastID(material);
			var available_object_count = AvailableObjectCount(material);
			var add_msg = Format("$MsgGoalResource$", res_id, Max(0, ExploitableObjectCount(mat_cnt - ObjectCount2MaterialCount(tol, material))), available_object_count);
			message = Format("%s%s", message, add_msg);
		}
	}
	return message;
}

// Shows or hides a message window with information.
public func Activate(int plr)
{
	// If goal message open -> hide it.
	if (GetEffect("GoalMessage", this))
	{
		CustomMessage("", nil, plr, nil, nil, nil, nil, nil, MSG_HCenter);
		RemoveEffect("GoalMessage", this);
		return;
	}
	// Otherwise open a new message.
	AddEffect("GoalMessage", this, 100, 0, this);
	var message;
	if (IsFulfilled())
	{
		message = "@$MsgGoalFulfilled$";		
	}
	else
	{
		message = "@$MsgGoalExtraction$";
		for (var i = 0; i < GetLength(resource_list); i++)
		{
			var material = Material(resource_list[i]);
			var tol = tolerance_list[i];
			var mat_cnt = GetMaterialCount(material) * 10 / 11; // subtract some that gets lost on blasting
			var res_id = GetBlastID(material);
			var available_object_count = AvailableObjectCount(material);
			var add_msg = Format("$MsgGoalResource$", res_id, Max(0, ExploitableObjectCount(mat_cnt - ObjectCount2MaterialCount(tol, material))), available_object_count);
			message = Format("%s%s", message, add_msg);
		}
	}
	CustomMessage(message, nil, plr, 0, 16 + 64, 0xffffff, GUI_MenuDeco, this, MSG_HCenter);
	return;
}

protected func FxGoalMessageStart() {}

public func GetShortDescription(int plr)
{
	// Show resource image with total resource count.
	var msg = "";
	for (var i = 0; i < GetLength(resource_list); i++)
	{
		var material = Material(resource_list[i]);
		var tol = tolerance_list[i];
		var mat_cnt = GetMaterialCount(material);
		var res_id = GetBlastID(material);
		var available_object_count = AvailableObjectCount(material);
		msg = Format("%s{{%i}}: %d ", msg, res_id, Max(0, ExploitableObjectCount(mat_cnt - ObjectCount2MaterialCount(tol, material), material)) + available_object_count);
	}	
	return msg;
}

func GetBlastRatio(int material)
{
	return GetMaterialVal("Blast2ObjectRatio", "Material", material);
}

func GetBlastID(int material)
{
	return GetMaterialVal("Blast2Object", "Material", material);
}

/** Get number of objects that are lying around freely. */
func AvailableObjectCount(int material)
{
	return ObjectCount(Find_ID(GetBlastID(material)));
}

/** Gets the number of objects that can be exploited from a material, based on material count. */
func ExploitableObjectCount(int material_count, int material)
{
	return material_count / GetBlastRatio(material);
}

/** Converts a number of objects to the amount of material
    that would have to be exploited in order to receive that
    amount of objects. */
func ObjectCount2MaterialCount(int count, int material)
{
	return (2 * count + 1) * GetBlastRatio(material) / 2;
}

/*-- Proplist --*/

local Name = "$Name$";
