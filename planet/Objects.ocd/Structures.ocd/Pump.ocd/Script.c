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
#include Library_Tank

static const PUMP_Menu_Action_Switch_On = "on";
static const PUMP_Menu_Action_Switch_Off = "off";
static const PUMP_Menu_Action_Description = "description";


local animation; // animation handle

local switched_on; // controlled by Interaction. Indicates whether the user wants to pump or not

local powered; // whether the pump has enough power as a consumer, always true if producing
local power_used; // the amount of power currently consumed or (if negative) produced

local clog_count; // increased when the pump doesn't find liquid or can't insert it. When it reaches max_clog_count, it will put the pump into temporary idle mode.
local max_clog_count = 5; // note that even when max_clog_count is reached, the pump will search through offsets (but in idle mode)

local stored_material_name; //contained liquid
local stored_material_amount;

/** This object is a liquid pump, thus pipes can be connected. */
public func IsLiquidPump() { return true; }
public func IsLiquidContainer() { return false; }
public func IsLiquidTank() { return false; }


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
	animation = PlayAnimation("pump", 5, Anim_Linear(GetAnimationPosition(animation), start, end, 35, ANIM_Loop));
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
	PushBack(menu_entries, {symbol = this, extra_data = PUMP_Menu_Action_Description,
			custom =
			{
				Prototype = custom_entry,
				Bottom = "1.2em",
				Priority = -1,
				BackgroundColor = RGB(25, 100, 100),
				text = {Prototype = custom_entry.text, Text = status},
				image = {Prototype = custom_entry.image, Symbol = Icon_Lightbulb, GraphicsName = lightbulb_graphics}
			}});
			
	// switch on and off
	if (!switched_on)
		PushBack(menu_entries, GetPumpMenuEntry(custom_entry, Icon_Play, "$MsgTurnOn$", 1, PUMP_Menu_Action_Switch_On));
	else
		PushBack(menu_entries, GetPumpMenuEntry(custom_entry, Icon_Stop, "$MsgTurnOff$", 1, PUMP_Menu_Action_Switch_Off));

	return menu_entries;
}

func GetPumpMenuEntry(proplist custom_entry, symbol, string text, int priority, extra_data)
{
	return {symbol = symbol, extra_data = extra_data, 
		custom =
		{
			Prototype = custom_entry,
			Priority = priority,
			text = {Prototype = custom_entry.text, Text = text},
			image = {Prototype = custom_entry.image, Symbol = symbol}
		}};
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
	if (action == PUMP_Menu_Action_Switch_On) text = "$DescTurnOn$";
	else if (action == PUMP_Menu_Action_Switch_Off) text = "$DescTurnOff$";
	else if (action == PUMP_Menu_Action_Description) text = this.Description;
	GuiUpdateText(text, menu_id, 1, desc_menu_target);
}


public func OnPumpControl(symbol_or_object, string action, bool alt)
{
	if (action == PUMP_Menu_Action_Switch_On || action == PUMP_Menu_Action_Switch_Off)
		ToggleOnOff(true);

	UpdateInteractionMenus(this.GetPumpControlMenuEntries);	
}

private func SetInfoMessage(string msg)
{
	if (last_status_message == msg) return;
	last_status_message = msg;
	UpdateInteractionMenus(this.GetPumpControlMenuEntries);
}

/*-- Pipe control --*/


func QueryConnectPipe(object pipe)
{
	if (GetDrainPipe() && GetSourcePipe())
	{
		pipe->Report("$MsgHasPipes$");
		return true;
	}
	else if (pipe->IsSourcePipe())
	{
		pipe->Report("$MsgSourcePipeProhibited$");
		return true;
	}
	else if (pipe->IsDrainPipe())
	{
		pipe->Report("$MsgDrainPipeProhibited$");
		return true;
	}
	return false;
}


func OnPipeConnect(object pipe, string specific_pipe_state)
{
	if (PIPE_STATE_Source == specific_pipe_state)
	{
		SetSourcePipe(pipe);
		pipe->SetSourcePipe();
		pipe->Report("$MsgCreatedSource$");
	}
	else if (PIPE_STATE_Drain == specific_pipe_state)
	{
		SetDrainPipe(pipe);
		pipe->SetDrainPipe();
		pipe->Report("$MsgCreatedDrain$");
	}
	else
	{
		// add a drain if we already connected a source pipe,
		// or if the line is already connected to a container
		var line = pipe->GetConnectedLine();
		var pump_target = !line || line->GetConnectedObject(this);
		if (pump_target) pump_target = pump_target->~IsLiquidContainer();		
		if (GetSourcePipe() || pump_target)
		{
			OnPipeConnect(pipe, PIPE_STATE_Drain);
		}
		// otherwise create a source first
		else
		{
			OnPipeConnect(pipe, PIPE_STATE_Source);
		}
	}
}


func OnPipeDisconnect(object pipe)
{
	pipe->SetNeutralPipe();		
	
	_inherited(pipe);
}


public func SetSourcePipe(object pipe)
{
	_inherited(pipe);
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

// TODO: these functions may be useful in the liquid tank, maybe move it to that library

/** Returns object to which the liquid is pumped */
private func GetDrainObject()
{
	if (GetDrainPipe()) return GetDrainPipe()->GetConnectedLine()->GetConnectedObject(this) ?? this;
	return this;
}

/** Returns object from which the liquid is pumped */
private func GetSourceObject()
{
	if (GetSourcePipe()) 
		return GetSourcePipe()->GetConnectedLine()->GetConnectedObject(this) ?? this;
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
	if (!GetSourcePipe()) 
		return;

	var pump_ok = true;
	
	// is empty? -> try to get liquid
	if (!stored_material_name)
	{
		// get new materials
		var source_obj = GetSourceObject();
		var mat = this->ExtractMaterialFromSource(source_obj, GetPumpSpeed() / 10);

		// no material to pump?
		if (mat)
		{
			stored_material_name = mat[0];
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
			if (this->InsertMaterialAtDrain(drain_obj, stored_material_name, 1))
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
			stored_material_name = nil;
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


// interface for the extraction logic
func ExtractMaterialFromSource(object source_obj, int amount)
{
	if (source_obj->~IsLiquidContainer())
	{
		return source_obj->RemoveLiquid(nil, amount, this);
	}
	else
	{
		var mat = source_obj->ExtractLiquidAmount(source_obj.ApertureOffsetX, source_obj.ApertureOffsetY, amount, true);
		if (mat)
			return [MaterialName(mat[0]), mat[1]];
		else
			return nil;
	}
}

// interface for the insertion logic
func InsertMaterialAtDrain(object drain_obj, string material_name, int amount)
{
	// insert material into containers, if possible
	if (drain_obj->~IsLiquidContainer())
	{
		amount -= drain_obj->PutLiquid(material_name, amount, this);
	}
	else
	{
		// convert to actual material, and insert remaining
		var material_index = Material(material_name);
		if (material_index != -1)
		{
			while (--amount >= 0)
				drain_obj->InsertMaterial(material_index, drain_obj.ApertureOffsetX, drain_obj.ApertureOffsetY);
		}
	}
	
	return amount <= 0;
}


/** Re check state and change the state if needed */
func CheckState()
{
	var is_fullcon = GetCon() >= 100;
	var can_pump = GetSourcePipe() && is_fullcon && switched_on;
	
	// can't pump at all -> wait
	if (!can_pump)
	{
		if (!GetSourcePipe() && switched_on)
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

// TODO: check usage of this, probably has to return true if the source is a container
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

// TODO: check usage of this, probably has to return true if the drain is a container
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
