/**
	Power Display
	Include this library into a structure that should display
	the status of the power network in the interaction menu.
	
	@author Maikel
*/


// This object is a power display.
public func IsPowerDisplay() { return true; }


/*-- Interaction Menu --*/

public func HasInteractionMenu() { return true; }

public func GetInteractionMenus(object clonk)
{
	var menus = _inherited(clonk, ...) ?? [];
	// Only add a power menu if the structure is a flagpole (Library_Flag).
	// And only if a power network is already initialized for this object.
	if (this->~IsFlagpole() && this->GetPowerHelper())
	{
		var power_menu =
		{
			title = "$MsgPowerOverview$",
			entries_callback = this.GetPowerDisplayMenuEntries,
			callback_hover = "OnPowerDisplayHover",
			callback_target = this,
			BackgroundColor = RGB(0, 50, 50),
			Priority = 20
		};
		PushBack(menus, power_menu);
	}
	return menus;
}

public func GetPowerDisplayMenuEntries(object clonk)
{
	var menu_entries = [];
	// Get the power network for this object.
	var power_network = this->GetPowerHelper();
	if (!power_network)
		return menu_entries;
	
	// If the no power need rule is active just state that.
	if (FindObject(Find_ID(Rule_NoPowerNeed)))
	{
		var entry = 
		{
			Style = GUI_FitChildren,
			Bottom = "1.1em",
			BackgroundColor = {Std = 0, OnHover = 0x50ff0000},
			Priority = 0,
			text = 
			{
				Style = GUI_TextVCenter | GUI_TextLeft,
				Text = "$MsgPowerNoNeed$"		
			}
		};
		PushBack(menu_entries, {symbol = Icon_Lightbulb, extra_data = "nopowerneed", custom = entry});
		return menu_entries;	
	} 
	
	// Get all the power data.
	var power_production_current = power_network->GetActivePowerAvailable(true) / 10;
	var power_production_capacity = power_network->GetBarePowerAvailable() / 10;
	var power_consumption_current = power_network->GetPowerConsumption(true) / 10;
	var power_consumption_need = power_network->GetPowerConsumptionNeed() / 10;
	var power_stored = power_network->GetStoredPower();
	var power_stored_capacity = power_network->GetStoredPowerCapacity();
	
	var entry_prototype = 
	{
		Style = GUI_FitChildren | GUI_TextVCenter | GUI_TextLeft,
		Bottom = "1.1em",
		BackgroundColor = {Std = 0, OnHover = 0x50ff0000}
	};
	
	// Show power production.
	var entry = 
	{
		Prototype = entry_prototype,
		Priority = 0,
		Text = Format("$MsgPowerProduction$ %d {{Icon_Lightbulb}} ($MsgPowerProductionCapacity$ %d {{Icon_Lightbulb}})", power_production_current, power_production_capacity)
	};
	PushBack(menu_entries, {symbol = Icon_Lightbulb, extra_data = "production", custom = entry});
	
	// Show power consumption.
	var entry = 
	{
		Prototype = entry_prototype,
		Priority = 1,
		Text = Format("$MsgPowerConsumption$ %d {{Icon_Lightbulb}} ($MsgPowerConsumptionDemand$ %d {{Icon_Lightbulb}})", power_consumption_current, power_consumption_need)
	};
	PushBack(menu_entries, {symbol = Icon_Lightbulb, extra_data = "consumption", custom = entry});
	
	// Show power storage.
	var entry = 
	{
		Prototype = entry_prototype,
		Priority = 2,
		Text = Format("$MsgPowerStored$ %s {{Icon_Lightbulb}} ($MsgPowerStoredCapacity$ %s {{Icon_Lightbulb}})", GetStoredPowerString(power_stored), GetStoredPowerString(power_stored_capacity))
	};
	PushBack(menu_entries, {symbol = Icon_Lightbulb, extra_data = "storage", custom = entry});
	return menu_entries;
}

// Update the hover info display of the interaction menu.
public func OnPowerDisplayHover(id symbol, string extra_data, desc_menu_target, menu_id)
{
	var text = "";
	// Get the power network for this object.
	var power_network = this->GetPowerHelper();
	if (power_network)
	{
		var power_production_current = power_network->GetActivePowerAvailable(true) / 10;
		var power_consumption_current = power_network->GetPowerConsumption(true) / 10;
		// Display the info dependent on which item is being hovered.
		if (extra_data == "production")
		{
			var power_production_capacity = power_network->GetBarePowerAvailable() / 10;
			text = Format("$DescPowerProduction$", power_production_capacity, power_production_current);
		}
		else if (extra_data == "consumption")
		{
			var power_consumption_need = power_network->GetPowerConsumptionNeed() / 10;
			text = Format("$DescPowerConsumption$", power_consumption_need, power_consumption_current);
		}
		else if (extra_data == "storage")
		{
			var power_stored = GetStoredPowerString(power_network->GetStoredPower());
			var power_stored_capacity = GetStoredPowerString(power_network->GetStoredPowerCapacity());
			text = Format("$DescPowerStored$", power_stored, power_stored_capacity, power_stored);
			var over_production = power_production_current - power_consumption_current;
			if (over_production > 0)
				text = Format("%s $DescPowerOverproduction$", text, over_production);
			else if (over_production < 0)
				text = Format("%s $DescPowerUnderproduction$", text, -over_production);
			
		}
		else if (extra_data == "nopowerneed")
		{
			text = "$DescPowerNoNeed$";
		}
	}
	GuiUpdateText(text, menu_id, 1, desc_menu_target);
	return;
}

private func GetStoredPowerString(int stored_power)
{
	// Show power per bulb per minute.
	stored_power /= (36 * 60);
	var unit = stored_power / 10;
	var decimal = stored_power % 10;
	return Format("%d.%d", unit, decimal);
}

// Called when the power balance of this network changed.
public func OnPowerBalanceChange(object network)
{
	// Update the interaction menus.
	UpdateInteractionMenus(this.GetPowerDisplayMenuEntries);
	return;
}
