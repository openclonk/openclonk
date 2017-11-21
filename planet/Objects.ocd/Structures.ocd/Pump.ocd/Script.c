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
static const PUMP_Menu_Action_Material_Enable = "material_on";
static const PUMP_Menu_Action_Material_Disable = "material_off";


local animation; // animation handle

local switched_on; // controlled by Interaction. Indicates whether the user wants to pump or not

local powered; // whether the pump has enough power as a consumer, always true if producing
local power_used; // the amount of power currently consumed or (if negative) produced

local clog_count; // increased when the pump doesn't find liquid or can't insert it. When it reaches max_clog_count, it will put the pump into temporary idle mode.
local max_clog_count = 5; // note that even when max_clog_count is reached, the pump will search through offsets (but in idle mode)

local pump_materials; // list of materials which may be pumped.
local accepted_mat; // currently accepted material.

local stored_material_name; //contained liquid
local stored_material_amount;

/** This object is a liquid pump, thus pipes can be connected. */
public func IsLiquidPump() { return true; }
public func IsLiquidContainer() { return false; }
public func IsLiquidTank() { return false; }


// The pump is rather complex for players. If anything happened, tell it to the player via the interaction menu.
local last_status_message;

public func Construction()
{
	// Rotate at a 45 degree angle towards viewer and add a litte bit of Random
	this.MeshTransformation = Trans_Rotate(50 + RandomX(-10, 10), 0, 1, 0);
	InitMaterialSelection();
	return _inherited(...);
}

public func IsHammerBuildable() { return true; }

public func Initialize()
{
	switched_on = true;
	var start = 0;
	var end = GetAnimationLength("pump");
	animation = PlayAnimation("pump", 5, Anim_Linear(GetAnimationPosition(animation), start, end, 35, ANIM_Loop));
	SetState("Wait");
	// Let the fast pump rule know it has been created.
	for (var rule in FindObjects(Find_ID(Rule_FastPump)))
		rule->~OnPumpCreation(this);
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

public func GetPumpMenuEntry(proplist custom_entry, symbol, string text, int priority, extra_data)
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
	var menus = _inherited(clonk, ...) ?? [];		
	var control_menu =
	{
		title = "$Control$",
		entries_callback = this.GetPumpControlMenuEntries,
		callback = "OnPumpControl",
		callback_hover = "OnPumpControlHover",
		callback_target = this,
		BackgroundColor = RGB(0, 50, 50),
		Priority = 20
	};
	PushBack(menus, control_menu);
	var materials_menu =
	{
		title = "$PumpMaterials$",
		entries_callback = this.GetPumpMaterialsMenuEntries,
		callback = "OnPumpMaterials",
		callback_hover = "OnPumpMaterialsHover",
		callback_target = this,
		BackgroundColor = RGB(0, 50, 50),
		Priority = 25
	};
	PushBack(menus, materials_menu);
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

public func QueryConnectPipe(object pipe)
{
	if (GetDrainPipe() && GetSourcePipe())
	{
		pipe->Report("$MsgHasPipes$");
		return true;
	}
	else if (pipe->IsSourcePipe() && GetSourcePipe())
	{
		pipe->Report("$MsgSourcePipeProhibited$");
		return true;
	}
	else if (pipe->IsDrainPipe() && GetDrainPipe())
	{
		pipe->Report("$MsgDrainPipeProhibited$");
		return true;
	}
	else if (pipe->IsAirPipe() && GetDrainPipe())
	{
		pipe->Report("$MsgAirPipeProhibited$");
		return true;
	}
	return false;
}


public func OnPipeConnect(object pipe, string specific_pipe_state)
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
	else if (PIPE_STATE_Air == specific_pipe_state)
	{
		// Air pipes take up the place of the drain
		SetDrainPipe(pipe);
		pipe->Report("$MsgCreatedAirDrain$");
	}
	else
	{
		// add a drain if we already connected a source pipe,
		// or if the line is already connected to a container
		var line = pipe->GetConnectedLine();
		var pump_target = !line || line->GetConnectedObject(this);
		if (pump_target) pump_target = pump_target->~IsLiquidContainer();
		if (line->IsAirPipe())
		{
			OnPipeConnect(pipe, PIPE_STATE_Air);
		}
		else if (GetSourcePipe() || pump_target)
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

public func OnPipeDisconnect(object pipe)
{
	_inherited(pipe, ...);

	if (!pipe->IsAirPipe())
		pipe->SetNeutralPipe();
	else // Stop pumping to prevent errors from Pumping()
		CheckState();
}

public func SetSourcePipe(object pipe)
{
	_inherited(pipe, ...);
	CheckState();
}

public func IsAirPipeConnected()
{
	if (!GetDrainPipe())
		return false;
	return GetDrainPipe()->~IsAirPipe();
}

/*-- Power stuff --*/

public func GetConsumerPriority()
{
	if (IsAirPipeConnected())
		return 125;	
	return 25;
}

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
	var drain = GetDrainPipe();
	if (drain && drain->GetConnectedLine())
		return drain->GetConnectedLine()->GetConnectedObject(this) ?? this;
	return this;
}

/** Returns object from which the liquid is pumped */
private func GetSourceObject()
{
	var source = GetSourcePipe();
	if (source && source->GetConnectedLine())
		return source->GetConnectedLine()->GetConnectedObject(this) ?? this;
	return this;
}

/** PhaseCall of Pump: Pump the liquid from the source to the drain pipe */
protected func Pumping()
{
	// at this point we can assert that we have power
	
	// something went wrong in the meantime?
	// let the central function handle that on next check
	if (!GetSourcePipe() && !IsAirPipeConnected()) 
		return;
		
	// Get the drain object.
	var drain_obj = GetDrainObject();

	// Don't do anything special if pumping air but inform the drain object.
	if (IsAirPipeConnected())
	{
		if (!GetAirSourceOk() || !GetAirDrainOk())
			return SetState("WaitForLiquid");
		if (drain_obj)
			drain_obj->~OnAirPumped(this);
		return;
	}

	var pump_ok = true;
	
	// is empty? -> try to get liquid
	if (!stored_material_name)
	{
		// get new materials
		var source_obj = GetSourceObject();
		var mat = this->ExtractMaterialFromSource(source_obj, this.PumpSpeed / 10);

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
		// Put into wait state if no liquid could be pumped for a while or if the drain has no aperture, i.e. is a liquid tank.
		if (++clog_count >= max_clog_count || !drain_obj->~HasAperture())
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
		return source_obj->RemoveLiquid(accepted_mat, amount, this);
	}
	else
	{
		var mat = source_obj->ExtractLiquidAmount(source_obj.ApertureOffsetX, source_obj.ApertureOffsetY, amount, true);
		if (mat)
			return [MaterialName(mat[0]), mat[1]];
	}
	return nil;
}

// interface for the insertion logic
public func InsertMaterialAtDrain(object drain_obj, string material_name, int amount)
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
		if (material_index == -1 && material_name != nil)
			material_index = Material(GetDefinition(material_name)->GetLiquidMaterial());
		if (material_index != -1)
		{
			while (amount > 0 && drain_obj->InsertMaterial(material_index, drain_obj.ApertureOffsetX, drain_obj.ApertureOffsetY))
				amount--;
		}
	}
	return amount <= 0;
}


/** Re check state and change the state if needed */
public func CheckState()
{
	var is_fullcon = GetCon() >= 100;
	// The pump can work without source if it needs to supply air.
	var can_pump = (GetSourcePipe() || IsAirPipeConnected()) && is_fullcon && switched_on;
	
	// Can't pump at all -> wait.
	if (!can_pump)
	{
		if (!GetSourcePipe() && switched_on)
			SetInfoMessage("$StateNoSource$");
		SetState("Wait");
	}
	else if(IsAirPipeConnected())
	{
		if (!GetAirSourceOk())
		{
			SetInfoMessage("$StateNoAir$");
			SetState("WaitForLiquid");
		}
		else if (!GetAirDrainOk())
		{
			SetInfoMessage("$StateNoAirNeed$");
			SetState("WaitForLiquid");
		}
		else
		{
			// Can pump, has air but has no power -> wait for power.
			if (!powered)
			{
				SetInfoMessage("$StateNoPower$");
				SetState("WaitForPower");
			}
			else
			{
				SetInfoMessage();
				clog_count = 0;
				SetState("Pump");
			}
			UpdatePowerUsage();
		}
	}
	else
	{
		// Can pump but has no liquid or can't dispense liquid -> wait.
		var source_mat = GetLiquidSourceMaterial();
		var source_ok = IsInMaterialSelection(source_mat);
		var drain_ok = GetLiquidDrainOk(source_mat);
		if (!source_ok || !drain_ok)
		{
			accepted_mat = nil;
			if (!source_ok)
				SetInfoMessage("$StateNoInput$");
			else if (!drain_ok)
				SetInfoMessage("$StateNoOutput$");
			SetState("WaitForLiquid");
		}
		else
		{
			accepted_mat = source_mat;
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
	// Pumped air always requires 20 power.
	if (IsAirPipeConnected())
		used_power = 20;
	return used_power;
}

// Returns whether there is liquid at the source pipe to pump.
private func GetLiquidSourceMaterial()
{
	// Get the source object and check whether there is liquid.
	var source_obj = GetSourceObject();
	if (!source_obj)
		return;
	// The source is a liquid container: check which material will be supplied.	
	if (source_obj->~IsLiquidContainer())
	{
		var liquid = source_obj->HasLiquid(pump_materials);
		if (liquid)
			return liquid->GetLiquidType();
		return;
	}	
	var is_liquid = source_obj->GBackLiquid(source_obj.ApertureOffsetX, source_obj.ApertureOffsetY);
	var liquid = MaterialName(source_obj->GetMaterial(source_obj.ApertureOffsetX, source_obj.ApertureOffsetY));
	if (!is_liquid)
	{
		// Try different offsets, so we can resume pumping after clog because 1px of earth was dropped on the source pipe.
		source_obj->~CycleApertureOffset(this); 
		return;
	}
	return liquid;
}

// Returns whether the drain pipe is free or the liquid container accepts the given material.
private func GetLiquidDrainOk(string liquid)
{
	if (!liquid)
		return false;
	var drain_obj = GetDrainObject();
	if (drain_obj->~HasAperture())
	{
		var material_index = Material(liquid);
		if (material_index == -1 && liquid != nil)
			material_index = Material(GetDefinition(liquid)->GetLiquidMaterial());
		if (!drain_obj->CanInsertMaterial(material_index, drain_obj.ApertureOffsetX, drain_obj.ApertureOffsetY))
		{
			drain_obj->~CycleApertureOffset(this); // try different offsets, so we can resume pumping after clog because 1px of earth was dropped on the source pipe
			return false;
		}
	}
	else if (drain_obj->~IsLiquidContainer())
	{
		if (!drain_obj->AcceptsLiquid(liquid, 1))
			return false;
	}
	return true;
}

// Returns whether the source (or alternatively the pump itself) is in free air.
public func GetAirSourceOk()
{
	var source_obj = GetSourceObject();
	if (!source_obj)
		return !GBackSemiSolid();
	var is_air = !source_obj->GBackSemiSolid(source_obj.ApertureOffsetX, source_obj.ApertureOffsetY);
	if (!is_air)
		source_obj->~CycleApertureOffset(this);
	return is_air;
}

// Returns whether the other side of air drain is in need of air.
public func GetAirDrainOk()
{
	var drain_obj = GetDrainObject();
	if (!drain_obj)
		return false;
	return drain_obj->~QueryAirNeed(this);
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


/*-- Material Selection --*/

private func InitMaterialSelection()
{
	// Add all liquids to the list of ones allowed to pump.
	pump_materials = [];
	var index = 0, def;
	while (def = GetDefinition(index++))
		if (def->~IsLiquid() && def != Library_Liquid)
			PushBack(pump_materials, def);
	// Accepted mat defaults to nil.
	accepted_mat = nil;		
	return;	
}

public func SetMaterialSelection(array mats)
{
	pump_materials = mats[:];
	return;
}

private func RemoveFromMaterialSelection(id mat)
{
	// Remove all child materials (DuroLava for lava) as well
	var def, index;
	while (def = GetDefinition(index++))
	{
		if (def->~GetParentLiquidType() == mat)
		{
			RemoveFromMaterialSelection(def);
		}
	}
	return RemoveArrayValue(pump_materials, mat);
}

private func AddToMaterialSelection(id mat)
{
	// Add all child materials (DuroLava for lava) as well
	var def, index;
	while (def = GetDefinition(index++))
	{
		if (def->~GetParentLiquidType() == mat)
		{
			AddToMaterialSelection(def);
		}
	}
	if (IsValueInArray(pump_materials, mat))
		return;
	return PushBack(pump_materials, mat);
}

private func IsInMaterialSelection(/* any */ mat)
{
	if (GetType(mat) == C4V_Def)
		return IsValueInArray(pump_materials, mat);
	for (var def in pump_materials)
		if (def->GetLiquidType() == mat)
			return true;		
	return false;	
}

public func GetPumpMaterialsMenuEntries(object clonk)
{
	var menu_entries = [];
	// Add materials to the selection.
	// Ignore those with parent materials, because they will be added/removed together with the parent
	var index = 0, def;
	while (def = GetDefinition(index++))
	{
		if (def->~IsLiquid() && def != Library_Liquid && !def->~GetParentLiquidType())
		{
			var act = PUMP_Menu_Action_Material_Disable;
			var status = Icon_Ok;
			var enabled = IsInMaterialSelection(def);
			if (!enabled)
			{
				act = PUMP_Menu_Action_Material_Enable;
				status = Icon_Cancel;
			}
			PushBack(menu_entries, 
				{symbol = def, extra_data = act, 
					custom =
					{
						Right = "2em", Bottom = "2em",
						BackgroundColor = {Std = 0, OnHover = 0x50ff0000},
						Priority = index,
						status = {Right = "1em", Top = "1em", Symbol = status},
						image = {Symbol = def}
				}}
			);
		}
	}
	return menu_entries;
}

public func OnPumpMaterialsHover(id symbol, string action, desc_menu_target, menu_id)
{
	var text = "";
	if (action == PUMP_Menu_Action_Material_Enable) text = Format("$MsgEnableMaterial$", symbol->GetName());
	else if (action == PUMP_Menu_Action_Material_Disable) text = Format("$MsgDisableMaterial$", symbol->GetName());
	else if (action == PUMP_Menu_Action_Description) text = this.Description;
	GuiUpdateText(text, menu_id, 1, desc_menu_target);
}

public func OnPumpMaterials(symbol_or_object, string action, bool alt)
{
	if (action == PUMP_Menu_Action_Material_Enable)
		AddToMaterialSelection(symbol_or_object);
	else if (action == PUMP_Menu_Action_Material_Disable)
		RemoveFromMaterialSelection(symbol_or_object);
	UpdateInteractionMenus(this.GetPumpMaterialsMenuEntries);	
}


/*-- Properties --*/

protected func Definition(def) 
{
	// for title image
	SetProperty("PictureTransformation", Trans_Rotate(50, 0, 1, 0), def);
	// for building preview
	SetProperty("MeshTransformation", Trans_Rotate(50, 0, 1, 0), def);
	return _inherited(def, ...);
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
local BlastIncinerate = 70;
local NoBurnDecay = true;
local HitPoints = 70;
local FireproofContainer = true;
local Components = {Wood = 1, Metal = 3};
// Pump speed in amount of pixels to pump per 30 frames.
local PumpSpeed = 50;
