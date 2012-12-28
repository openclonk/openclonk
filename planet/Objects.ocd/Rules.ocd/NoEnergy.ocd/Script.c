/*-- Power usage --*/

func Initialize()
{
	// we are not the first?
	if(ObjectCount(Find_ID(GetID())) != 1) return;
	
	// no power consumers yet, nothing to be done
	if(Library_Power_power_compounds == nil) return;
	
	// update all consumers forcefully
	var remember = [];
	
	// get all power compounds (bases)
	for(var base in Library_Power_power_compounds)
	{
		// iterate through all registered consumers & producers
		// these are not the sleeping power links!
		for(var i = GetLength(base.power_links) - 1; i >= 0; --i)
		{
			var obj_data = base.power_links[i];
			if(obj_data == nil) continue;
			// is a power producer?
			if(!obj_data.obj->~CurrentlyHasPower()) continue;
			if(obj_data.amount > 0) continue;
			
			// temporarily stop consuming power to make sure the callback order is correct
			obj_data.obj->~OnNotEnoughPower();
			
			// power consumer behavior
			var lpr = obj_data.obj.PowerConsumer_last_power_request;
			obj_data.obj.PowerConsumer_last_power_request = 0;
			var lpr_a = obj_data.obj.PowerConsumer_last_power_request_amount;
			obj_data.obj.PowerConsumer_last_power_request_amount = 0;
			
			// remove from list
			RemoveArrayIndexUnstable(base.power_links, i);
			
			// make new power consumer later
			PushBack(remember, {obj = obj_data.obj, amount = lpr_a});
		}
		
		// now awake the sleeping links (later)
		for(var i = GetLength(base.sleeping_links) - 1; i >= 0; --i)
		{
			var obj_data = base.sleeping_links[i];
			PushBack(remember, {obj = obj_data.obj, amount = obj_data.amount});
		}
		
		base.sleeping_links = [];
	}
	
	// let all remembered consumers consume again!
	for(var obj_data in remember)
	{
		if(obj_data == nil) continue;
		obj_data.obj->MakePowerConsumer(Abs(obj_data.amount));
	}
}

func Destruction()
{
	// we are not the last?
	if(ObjectCount(Find_ID(GetID())) != 1) return;
	
	// schedule the call so that the removal is complete
	Schedule(nil, "Rule_NoPowerNeed->DestructionEx()", 1);
}

func DestructionEx()
{
	// we were not the last?
	if(ObjectCount(Find_ID(Rule_NoPowerNeed)) != 0) return;
	
	// update consumers again
	// go through all existing consumers and see whether they are consuming atm
	for(var obj in FindObjects(Find_Func("IsPowerConsumer")))
	{
		var last_request = obj.PowerConsumer_last_power_request;
		var last_amount = obj.PowerConsumer_last_power_request_amount;

		// had not requested power aka "is not running"
		if(last_request == PowerConsumer_LPR_None) continue;
		// temporarily stop consuming power to make sure the callback order is correct
		obj->OnNotEnoughPower();
		
		obj.PowerConsumer_last_power_request = PowerConsumer_LPR_None;
		obj.PowerConsumer_last_power_request_amount = 0;
		obj->MakePowerConsumer(last_amount);
	}
}

protected func Activate(int iByPlayer)
{
	MessageWindow(GetProperty("Description"), iByPlayer);
	return true;
}

local Name = "$Name$";
local Description = "$Description$";
