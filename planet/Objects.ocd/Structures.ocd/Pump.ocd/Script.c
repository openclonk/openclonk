/**
	Pump
	Pumps liquids using drain and source pipes. Features include:
	+ switch on and off
	+ consume/produce a variable amount of power depending on the height of
	  source and drain
	
	@author Maikel, ST-DDT, Sven2, Newton
*/

#include Library_Structure
#include Library_Ownable
#include Library_PowerConsumer
#include Library_PowerProducer

local animation; // animation handle

local switched_on; // controlled by Interaction. Indicates whether the user wants to pump or not

local powered; // whether the pump has enough power as a consumer, always true if producing
local power_used; // the amount of power currently consumed or (if negative) produced

local stored_material_index; //contained liquid
local stored_material_amount;

local source_pipe;
local drain_pipe;

local clog_count; // increased when the pump doesn't find liquid or can't insert it. When it reaches max_clog_count, it will put the pump into temporary idle mode.
local max_clog_count = 5; // note that even when max_clog_count is reached, the pump will search through offsets (but in idle mode)

/** This object is a liquid pump, thus pipes can be connected. */
public func IsLiquidPump() { return true; }

// The pump is rather complex for players. If anything happened, tell it to the player via the interaction menu.
local last_status_message;

func Construction()
{
	// Rotate at a 45 degree angle towards viewer and add a litte bit of Random
	this.MeshTransformation = Trans_Rotate(50 + RandomX(-10, 10), 0, 1, 0);
	return _inherited(...);
}

func Initialize()
{
	switched_on = true;
	var start = 0;
	var end = GetAnimationLength("pump");
	animation = PlayAnimation("pump", 5, Anim_Linear(GetAnimationPosition(animation), start, end, 35, ANIM_Loop), Anim_Const(1000));
	SetState("Wait");
	return _inherited(...);
}


/*-- Interaction --*/

public func HasInteractionMenu() { return true; }

public func GetPumpControlMenuEntries(object clonk)
{
	var menu_entries = [];
	// default design of a control menu item
	var custom_entry = 
	{
		Right = "100%", Bottom = "2em",
		BackgroundColor = {Std = 0, OnHover = 0x50ff0000},
		image = {Right = "2em"},
		text = {Left = "2em"}
	};
	
	// Add info message about what is going on with the pump.
	var status = "$StateOk$";
	var lightbulb_graphics = "Green";
	if (last_status_message != nil)
	{
		status = last_status_message;
		lightbulb_graphics = "Red";
	}
	PushBack(menu_entries, {symbol = this, extra_data = "description",
			custom =
			{
				Prototype = custom_entry,
				Bottom = "1.2em",
				Priority = -1,
				BackgroundColor = RGB(25, 100, 100),
				text = {Prototype = custom_entry.text, Text = status},
				image = {Prototype = custom_entry.image, Symbol = Icon_Lightbulb, GraphicsName = lightbulb_graphics}
			}});
	
	if (!switched_on)
		PushBack(menu_entries, {symbol = Icon_Play, extra_data = "on", 
			custom =
			{
				Prototype = custom_entry,
				Priority = 1,
				text = {Prototype = custom_entry.text, Text = "$MsgTurnOn$"},
				image = {Prototype = custom_entry.image, Symbol = Icon_Play}
			}});
	else
		PushBack(menu_entries, {symbol = Icon_Stop, extra_data = "off", 
			custom =
			{
				Prototype = custom_entry,
				Priority = 1,
				text = {Prototype = custom_entry.text, Text = "$MsgTurnOff$"},
				image = {Prototype = custom_entry.image, Symbol = Icon_Stop}
			}});
	if (source_pipe)
		PushBack(menu_entries, {symbol = Icon_Cancel, extra_data = "cutsource", 
			custom =
			{
				Prototype = custom_entry,
				Priority = 2,
				text = {Prototype = custom_entry.text, Text = "$MsgCutSource$"},
				image = {Prototype = custom_entry.image, Symbol = Icon_Cancel}
			}});
	if (drain_pipe)
		PushBack(menu_entries, {symbol = Icon_Cancel, extra_data = "cutdrain", 
			custom =
			{
				Prototype = custom_entry,
				Priority = 3,
				text = {Prototype = custom_entry.text, Text = "$MsgCutDrain$"},
				image = {Prototype = custom_entry.image, Symbol = Icon_Cancel}
			}});
	return menu_entries;
}

public func GetInteractionMenus(object clonk)
{
	var menus = _inherited() ?? [];		
	var prod_menu =
	{
		title = "$Control$",
		entries_callback = this.GetPumpControlMenuEntries,
		callback = "OnPumpControl",
		callback_hover = "OnPumpControlHover",
		callback_target = this,
		BackgroundColor = RGB(0, 50, 50),
		Priority = 20
	};
	PushBack(menus, prod_menu);
	return menus;
}

public func OnPumpControlHover(id symbol, string action, desc_menu_target, menu_id)
{
	var text = "";
	if (action == "on") text = "$DescTurnOn$";
	else if (action == "off") text = "$DescTurnOff$";
	else if (action == "cutdrain") text = "$DescCutDrain$";
	else if (action == "cutsource") text = "$DescCutSource$";
	else if (action == "description") text = this.Description;
	GuiUpdateText(text, menu_id, 1, desc_menu_target);
}

public func OnPumpControl(id symbol, string action, bool alt)
{
	if (action == "on" || action == "off")
		ToggleOnOff(true);
	else if (action == "cutsource" && source_pipe)
		source_pipe->RemoveObject();
	else if (action == "cutdrain" && drain_pipe)
		drain_pipe->RemoveObject();
	UpdateInteractionMenus(this.GetPumpControlMenuEntries);	
}

private func SetInfoMessage(string msg)
{
	if (last_status_message == msg) return;
	last_status_message = msg;
	UpdateInteractionMenus(this.GetPumpControlMenuEntries);
}

/*-- Pipe connection --*/

public func GetSource() { return source_pipe; }
public func SetDrain(object pipe) { drain_pipe = pipe; }
public func GetDrain() { return drain_pipe; }

public func SetSource(object pipe)
{
	source_pipe = pipe;
	CheckState();
}

/*-- Power stuff --*/

public func GetConsumerPriority() { return 25; }

public func GetProducerPriority() { return 100; }

public func IsSteadyPowerProducer() { return true; }

public func OnNotEnoughPower()
{
	powered = false;
	CheckState();
	return _inherited(...);
}

public func OnEnoughPower()
{
	powered = true;
	CheckState();
	return _inherited(...);
}

/** Returns object to which the liquid is pumped */
private func GetDrainObject()
{
	if (drain_pipe) return drain_pipe->GetConnectedObject(this) ?? this;
	return this;
}

/** Returns object from which the liquid is pumped */
private func GetSourceObject()
{
	if (source_pipe) 
		return source_pipe->GetConnectedObject(this) ?? this;
	return this;
}

/** Returns amount of pixels to pump per 30 frames */
public func GetPumpSpeed()
{
	return 50;
}

/** PhaseCall of Pump: Pump the liquid from the source to the drain pipe */
protected func Pumping()
{
	// at this point we can assert that we have power
	
	// something went wrong in the meantime?
	// let the central function handle that on next check
	if (!source_pipe) 
		return;
	
	var pump_ok = true;
	
	// is empty? -> try to get liquid
	if (!stored_material_amount)
	{
		// get new materials
		var source_obj = GetSourceObject();
		var mat = source_obj->ExtractLiquidAmount(source_obj.ApertureOffsetX, source_obj.ApertureOffsetY, GetPumpSpeed() / 10, true);
	
		// no material to pump?
		if (mat)
		{
			stored_material_index = mat[0];
			stored_material_amount = mat[1];
		}
		else
		{
			source_obj->~CycleApertureOffset(this); // try different offsets, so we don't stop pumping just because 1px of earth was dropped on the source pipe
			pump_ok = false;
		}
	}
	if (pump_ok)
	{
		var i = stored_material_amount;
		while (i > 0)
		{
			var drain_obj = GetDrainObject();
			if (GetDrainObject()->InsertMaterial(stored_material_index, drain_obj.ApertureOffsetX, drain_obj.ApertureOffsetY))
			{
				i--;
			}
			// Drain is stuck.
			else
			{
				drain_obj->~CycleApertureOffset(this); // try different offsets, so we don't stop pumping just because 1px of earth was dropped on the drain pipe
				pump_ok = false;
				break;
			}
		}
		
		stored_material_amount = i;
		if (stored_material_amount <= 0)
			stored_material_index = nil;
	}
	
	if (pump_ok)
	{
		clog_count = 0;
	}
	else
	{
		// Put into wait state if no liquid could be pumped for a while
		if (++clog_count >= max_clog_count)
		{
			SetState("WaitForLiquid");
		}
	}
	return;
}

/** Re check state and change the state if needed */
func CheckState()
{
	var is_fullcon = GetCon() >= 100;
	var can_pump = source_pipe && is_fullcon && switched_on;
	
	// can't pump at all -> wait
	if (!can_pump)
	{
		if (!source_pipe && switched_on)
			SetInfoMessage("$StateNoSource$");
		SetState("Wait");
	}
	else
	{
		// can pump but has no liquid -> wait for liquid
		var source_ok = IsLiquidSourceOk();
		var drain_ok  = IsLiquidDrainOk();
		if (!source_ok || !drain_ok)
		{
			if (!source_ok)
				SetInfoMessage("$StateNoInput$");
			else if (!drain_ok)
				SetInfoMessage("$StateNoOutput$");
			SetState("WaitForLiquid");
		}
		else
		{
			// can pump, has liquid but has no power -> wait for power
			if (!powered)
			{
				SetInfoMessage("$StateNoPower$");
				SetState("WaitForPower");
			}
			// otherwise, pump! :-)
			else
			{
				SetInfoMessage();
				clog_count = 0;
				SetState("Pump");
			}
			
			// regularly update the power usage while pumping or waiting for power
			UpdatePowerUsage();
		}
	}
}

/** Get current height the pump has to push liquids upwards (input.y - output.y) */
private func GetPumpHeight()
{
	// compare each the surfaces of the bodies of liquid pumped
	
	// find Y position of surface of liquid that is pumped to target
	var source_obj = GetSourceObject();
	var source_x = source_obj.ApertureOffsetX;
	var source_y = source_obj.ApertureOffsetY;
	if (source_obj->GBackLiquid(source_x, source_y))
	{
		var src_mat = source_obj->GetMaterial(source_x, source_y);
		while (src_mat == source_obj->GetMaterial(source_x, source_y - 1))
			--source_y;
	}
	// same for target (use same function as if inserting)
	var target_pos = {X = 0, Y = 0};
	var drain_obj = GetDrainObject();
	drain_obj->CanInsertMaterial(Material("Water"), drain_obj.ApertureOffsetX, drain_obj.ApertureOffsetY, target_pos);
	return source_obj->GetY() + source_y - target_pos.Y;
}

/** Recheck power usage/production for current pump height
	and make the pump a producer / consumer for the power system */
private func UpdatePowerUsage()
{
	var new_power;
	if (IsUsingPower())
		new_power = PumpHeight2Power(GetPumpHeight());
	else
		new_power = 0;
	
	// do nothing if not necessary
	if (new_power == power_used)
	{
		// But still set powered to true if power_used was not positive.
		if (power_used <= 0)
			powered = true;
		return;
	}
	
	// and update energy system
	if (new_power > 0)
	{
		if (power_used < 0)
		{
			powered = false; // needed since the flag was set manually
			UnregisterPowerProduction();
		}
		RegisterPowerRequest(new_power);
	}
	else if (new_power < 0)
	{
		if (power_used > 0)
			UnregisterPowerRequest();
		RegisterPowerProduction(-new_power);
		powered = true; // when producing, we always have power
	}
	else // new_power == 0
	{
		if (power_used < 0) 
			UnregisterPowerProduction();
		else if (power_used > 0)
			UnregisterPowerRequest();
		powered = true;
	}
	
	power_used = new_power;
	return;
}

// Return whether the pump should be using power in the current state.
private func IsUsingPower()
{
	return switched_on && (GetAction() == "Pump" || GetAction() == "WaitForPower");
}

// Transform pump height (input.y - output.y) into required power.
private func PumpHeight2Power(int pump_height)
{
	// Pumping upwards will always cost the minimum energy.
	// Pumping downwards will only produce energy after an offset.
	var power_offset = 10;
	// Max power consumed / produced.
	var max_power = 60;
	// Calculate the used power in steps of ten, every 60 pixels represents ten units.
	var used_power = pump_height / 60 * 10;
	// If the pump height is positive then add the minimum energy.
	if (pump_height >= 0)
		used_power = Min(used_power + 10, max_power);
	// Pumping power downwards never costs energy, but only brings something if offset is overcome.
	else
		used_power = BoundBy(used_power + power_offset - 10, -max_power, 0);
	return used_power;
}

// Returns whether there is liquid at the source pipe to pump.
private func IsLiquidSourceOk()
{
	// source
	var source_obj = GetSourceObject();
	if(!source_obj->GBackLiquid(source_obj.ApertureOffsetX, source_obj.ApertureOffsetY))
	{
		source_obj->~CycleApertureOffset(this); // try different offsets, so we can resume pumping after clog because 1px of earth was dropped on the source pipe
		return false;
	}
	return true;
}

// Returns whether the drain pipe is free.
private func IsLiquidDrainOk()
{
	// target (test with the very popular liquid "water")
	var drain_obj = GetDrainObject();
	if(!drain_obj->CanInsertMaterial(Material("Water"),drain_obj.ApertureOffsetX, drain_obj.ApertureOffsetY))
	{
		drain_obj->~CycleApertureOffset(this); // try different offsets, so we can resume pumping after clog because 1px of earth was dropped on the source pipe
		return false;
	}
	return true;
}

// Set the state of the pump, retaining the animation position and updating the power usage.
private func SetState(string act)
{
	if (act == GetAction()) 
		return;

	// Set animation depending on the current action.
	var start = 0;
	var end = GetAnimationLength("pump");
	var anim_pos = GetAnimationPosition(animation);
	if (act == "Pump")
	{
		SetAnimationPosition(animation, Anim_Linear(anim_pos, start, end, 35, ANIM_Loop));
	}
	else if(act == "WaitForLiquid")
	{
		SetAnimationPosition(animation, Anim_Linear(anim_pos, start, end, 350, ANIM_Loop));
	}
	else
	{
		SetAnimationPosition(animation, Anim_Const(anim_pos));
	}
	
	// Deactivate power usage when not pumping.
	if (powered && (act == "Wait" || act == "WaitForLiquid"))
	{
		if (power_used < 0) 
			UnregisterPowerProduction();
		else if (power_used > 0) 
			UnregisterPowerRequest();
		
		power_used = 0;
		powered = false;
	}
	// Finally, set the action.
	SetAction(act);
}

/* Deactivates a running pump or vice-versa. */
func ToggleOnOff(bool no_menu_refresh)
{
	switched_on = !switched_on;
	CheckState();
	if (!switched_on)
		SetInfoMessage("$StateTurnedOff$");
	if (!no_menu_refresh)
		UpdateInteractionMenus(this.GetPumpControlMenuEntries);
}

/*-- Properties --*/

protected func Definition(def) 
{
	// for title image
	SetProperty("PictureTransformation", Trans_Rotate(50, 0, 1, 0), def);
	// for building preview
	SetProperty("MeshTransformation", Trans_Rotate(50, 0, 1, 0), def);
}

/*
	States
	"Wait":				turned off or source pipe not connected
	"WaitForPower":		turned on but no power (does consume power)
	"WaitForLiquid":	turned on but no liquid (does not consume power)
	"Pump":				currently working and consuming/producing power
*/
local ActMap = {
	Pump = {
		Prototype = Action,
		Name = "Pump",
		Length = 30,
		Delay = 3,
		Sound = "Structures::Pumpjack",
		NextAction = "Pump",
		StartCall = "CheckState",
		PhaseCall = "Pumping"
	},
	Wait = {
		Prototype = Action,
		Name = "Wait",
		Delay = 90,
		NextAction = "Wait",
		EndCall = "CheckState"
	},
	WaitForPower = {
		Prototype = Action,
		Name = "WaitForPower",
		Delay = 30,
		NextAction = "WaitForPower",
		EndCall = "CheckState"
	},
	WaitForLiquid = {
		Prototype = Action,
		Name = "WaitForLiquid",
		Delay = 30,
		NextAction = "WaitForLiquid",
		EndCall = "CheckState"
	}
};

local Name = "$Name$";
local Description = "$Description$";
local BlastIncinerate = 50;
local HitPoints = 70;
