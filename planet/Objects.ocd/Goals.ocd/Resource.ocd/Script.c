/*-- 
	Resource Extraction
	Author: Maikel, Marky
	
	Player must extract the resources of the specified type, the
	tolerance of this goal is 5% currently, if no percentage of
	exploitation is specified.
	
	Additionally, the scenario designer can specify a percentage
	of material to be exploited, between 5% and 95%.
		
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

public func SetResource(string resource, int percent)
{
	var list_end = GetLength(resource_list);
	resource_list[list_end] = resource;
	
	// Assume that all objects, with 5% tolerance, have to be exploited if no percentage is specified 
	percent = BoundBy(percent ?? 95, 5, 95);

	var material = Material(resource);
	var exploitable_units = GetMaterialCount(material);
	var exploitable_objects = ExploitableObjectCount(exploitable_units, material);
	
	var target_objects = percent * exploitable_objects / 100;

	// Calculate 100 / 20 = 5% of the exploitable objects as tolerance
	tolerance_list[list_end] = Max(1, exploitable_objects - target_objects);
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
		props->AddCall("Goal", this, "SaveResource", Format("%v", resource_list[i]), tolerance_list[i]);
	return true;
}


/*-- Goal interface --*/

// The goal is fulfilled if all specified resource have been mined.
public func IsFulfilled()
{
	for (var i = 0; i < GetLength(resource_list); i++)
	{
		var material = Material(resource_list[i]);
		var exploitable_units = GetMaterialCount(material);
		var tolerance_units = ObjectCount2MaterialCount(tolerance_list[i], material);

		// Still solid material to be mined.
		if (exploitable_units == -1 || exploitable_units > tolerance_units)
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
			// Current data
			var material = Material(resource_list[i]);
			var exploitable_units = GetMaterialCount(material);
			var tolerance_units = ObjectCount2MaterialCount(tolerance_list[i], material);
			// Put the message together
			var icon = GetBlastID(material);
			var available_object_count = AvailableObjectCount(material);
			var exploitable_object_count = Max(0, ExploitableObjectCount(exploitable_units - tolerance_units, material));
			var add_msg = Format("$MsgGoalResource$", icon, exploitable_object_count, available_object_count);
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
			// Current data
			var material = Material(resource_list[i]);
			var exploitable_units = GetMaterialCount(material) * 10 / 11; // subtract some that gets lost on blasting
			var tolerance_units = ObjectCount2MaterialCount(tolerance_list[i], material);
			// Put the message together
			var icon = GetBlastID(material);
			var available_object_count = AvailableObjectCount(material);
			var exploitable_object_count = Max(0, ExploitableObjectCount(exploitable_units - tolerance_units));
			var add_msg = Format("$MsgGoalResource$", icon, exploitable_object_count, available_object_count);
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
		// Current data
		var material = Material(resource_list[i]);
		var exploitable_units = GetMaterialCount(material);
		var tolerance_units = ObjectCount2MaterialCount(tolerance_list[i], material);
		// Put the message together
		var icon = GetBlastID(material);
		var available_object_count = AvailableObjectCount(material);
		var exploitable_object_count = Max(0, ExploitableObjectCount(exploitable_units - tolerance_units, material));
		msg = Format("%s{{%i}}: %d ", msg, icon, exploitable_object_count + available_object_count);
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
