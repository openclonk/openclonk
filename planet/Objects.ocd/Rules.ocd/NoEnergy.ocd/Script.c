/**
	No Energy Rule
	If this rule is activated, power consumers do not need energy supply 
	anymore to operate.
	
	@author Zapper, Maikel
*/


protected func Initialize()
{
	// Don't do anything if this is not the first rule of this type.
	if (ObjectCount(Find_ID(Rule_NoPowerNeed)) > 1) 
		return;
	
	// If there are no power consumers yet, there is nothing to be done.
	if (LIB_POWR_Networks == nil) 
		return;
	
	// Update all consumers forcefully.
	var remember = [];
	
	// Get all power compounds (bases).
	for (var base in LIB_POWR_Networks)
	{
		// Iterate through all registered consumers & producers, these are not the sleeping power links.
		for (var i = GetLength(base.power_links) - 1; i >= 0; --i)
		{
			var obj_data = base.power_links[i];
			if (obj_data == nil) 
				continue;
			// If it is a power producer, continue to next link.
			if (!obj_data.obj->~CurrentlyHasPower() || obj_data.amount > 0) 
				continue;
			
			// Temporarily stop consuming power to make sure the callback order is correct.
			obj_data.obj->~OnNotEnoughPower();
			
			// Power consumer behavior.
			obj_data.obj.last_request = 0;
			var lpr_a = obj_data.obj.last_amount;
			obj_data.obj.last_amount = 0;
			
			// Remove from list.
			RemoveArrayIndexUnstable(base.power_links, i);
			
			// Make new power consumer later.
			PushBack(remember, {obj = obj_data.obj, amount = lpr_a});
		}
		
		// Now awake the sleeping links, after the consumers.
		for (var i = GetLength(base.sleeping_links) - 1; i >= 0; --i)
		{
			var obj_data = base.sleeping_links[i];
			PushBack(remember, {obj = obj_data.obj, amount = obj_data.amount});
		}
		// Reset the sleeping links to an empty list.
		base.sleeping_links = [];
	}
	
	// Let all remembered consumers consume again!
	for (var obj_data in remember)
	{
		if (obj_data == nil) 
			continue;
		obj_data.obj->MakePowerConsumer(Abs(obj_data.amount));
	}
	return;
}

protected func Destruction()
{
	// If this is not the last copy of this rule do nothing. 
	if (ObjectCount(Find_ID(Rule_NoPowerNeed)) >= 1)
		return;
	
	// Schedule the a destruction definition call so that the removal is complete.
	Schedule(nil, "Rule_NoPowerNeed->DestructionEx()", 1);
	return;
}

public func DestructionEx()
{
	// Only perform resetting the links if there are no more rule objects active.
	if (ObjectCount(Find_ID(Rule_NoPowerNeed)) >= 1) 
		return;
	
	// Update all consumers again by going through all existing consumers.
	for (var obj in FindObjects(Find_Func("IsPowerConsumer")))
	{
		var request = obj.last_request;
		var amount = obj.last_amount;
		// Temporarily stop consuming power to make sure the callback order is correct.
		obj->OnNotEnoughPower();
		obj.last_amount = 0;
		obj->MakePowerConsumer(amount);
	}
	return;
}

protected func Activate(int plr)
{
	MessageWindow(GetProperty("Description"), plr);
	return true;
}

local Name = "$Name$";
local Description = "$Description$";
