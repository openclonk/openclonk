/*-- 
	Resource Extraction
	Author: Maikel
	
	Player must extract the resources of the specified type, the
	tolerance of this goal is 5% currently.
		
	TODO: Expand to liquids and digable materials.
--*/


#include Library_Goal

local resource_list; // List of materials to be mined.
local tolerance_list; // List of initial counts of materials.

protected func Initialize()
{
	resource_list = [];
	tolerance_list = [];
	return inherited(...);
}

/*-- Resources --*/

public func SetResource(string resource)
{
	var pos = GetLength(resource_list);
	resource_list[pos] = resource;
	var mat_cnt = GetMaterialCount(Material(resource));
	var blast_ratio = GetMaterialVal("Blast2ObjectRatio", "Material", Material(resource));
	tolerance_list[pos] = Max(1, mat_cnt / blast_ratio / 20);
	return;
}

/*-- Goal interface --*/

// The goal is fulfilled if all specified resource have been mined.
public func IsFulfilled()
{
	for (var i = 0; i < GetLength(resource_list); i++)
	{
		var mat = resource_list[i];
		var tol = tolerance_list[i];
		var mat_cnt = GetMaterialCount(Material(mat));
		var blast_ratio = GetMaterialVal("Blast2ObjectRatio", "Material", Material(mat));
		// Still solid material to be mined.
		if (mat_cnt == -1 || mat_cnt > (2*tol+1) * blast_ratio / 2)
			return false; 
		var res_id = GetMaterialVal("Blast2Object", "Material", Material(mat));
		// Still objects of material to be collected.
		if (ObjectCount(Find_ID(res_id)) > 0)
			return false; 
	}
	// Goal fulfilled.
	return true;
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
			var mat = resource_list[i];
			var tol = tolerance_list[i];
			var mat_cnt = GetMaterialCount(Material(mat));
			var res_id = GetMaterialVal("Blast2Object", "Material", Material(mat));
			var res_cnt = ObjectCount(Find_ID(res_id));
			var blast_ratio = GetMaterialVal("Blast2ObjectRatio", "Material", Material(mat));
			var add_msg = Format("$MsgGoalResource$", res_id, Max(0, (mat_cnt - (2*tol+1) * blast_ratio / 2) / blast_ratio), res_cnt);
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
		var mat = resource_list[i];
		var tol = tolerance_list[i];
		var mat_cnt = GetMaterialCount(Material(mat));
		var res_id = GetMaterialVal("Blast2Object", "Material", Material(mat));
		var res_cnt = ObjectCount(Find_ID(res_id));
		var blast_ratio = GetMaterialVal("Blast2ObjectRatio", "Material", Material(mat));
		msg = Format("%s{{%i}}: %d ", msg, res_id, Max(0, (mat_cnt - (2*tol+1) * blast_ratio / 2) / blast_ratio) + res_cnt);
	}	
	return msg;
}

/*-- Proplist --*/

local Name = "$Name$";
