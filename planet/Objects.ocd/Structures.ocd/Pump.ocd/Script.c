/*--
	Pump
	Author: Maikel, ST-DDT, Sven2
	
	Pumps liquids using drain and source pipes.
--*/

#include Library_Structure
#include Library_Ownable
#include Library_PowerConsumer
#include Library_PowerProducer

local current_pump_power; // Current power needed for pumping. Negative if power is being produced.

// This object is a liquid pump, thus pipes can be connected.
public func IsLiquidPump() { return true; }

public func Construction(object creator)
{
	return _inherited(creator, ...);
}

protected func Initialize()
{
	turned_on = true;
	SetAction("Wait");
	CheckCurrentState();
	return;
}

/*-- Interaction --*/

local turned_on;

public func IsInteractable() { return GetCon() >= 100; }

public func GetInteractionMetaInfo(object clonk)
{
	if (turned_on)
		return { Description = "$MsgTurnOff$", IconName = nil, IconID = nil };
	return { Description = "$MsgTurnOn$", IconName = nil, IconID = nil };
}

// On interaction the pump can be turned on or off.
public func Interact(object clonk)
{
	turned_on = !turned_on;
	CheckCurrentState();
	return true;
}

/*-- Pipe connection --*/

local source_pipe;
local drain_pipe;

// Set-Getters for source and drain pipe.
public func GetSource() { return source_pipe; }
public func SetDrain(object pipe) { drain_pipe = pipe; }
public func GetDrain() { return drain_pipe; }

public func SetSource(object pipe)
{
	source_pipe = pipe;
	CheckCurrentState();
}

func QueryWaivePowerRequest()
{
	// has less priority than other objects, but not too low
	return 10;
}

func OnNotEnoughPower()
{
	// assert: current action is "Pump" or "PumpWaitLiquid"
	SetActionKeepPhase("PumpWaitPower");
	return _inherited(...);
}

func OnEnoughPower()
{
	// assert: current action is either PumpWaitPower or Wait
	SetActionKeepPhase("Pump");
	return _inherited(...);
}

func PumpWaitPowerStart() { return AddTimer(this.PumpingWaitForPower, 60); }
func PumpWaitPowerStop() { return RemoveTimer(this.PumpingWaitForPower); }

func PumpingWaitForPower()
{
	// Waiting for power: Check pump height, because we might become a producer
	if (!source_pipe) return CheckCurrentState();
	if (last_source_y != (source_pipe->GetConnectedObject(this) ?? this)->GetY() || last_drain_y != GetDrainObject()->GetY()) UpdatePumpHeight(true);
}

local aMaterials=["", 0]; //contained liquids
local pumpable_materials; // materials that can be pumped
local last_source_y, last_drain_y;
local pump_amount; // mat amount pumped since last height check

protected func PumpingWaitForLiquid()
{
	// Pump is ready; check for liquids to pump
	if (!source_pipe) return CheckCurrentState();
	if(!HasLiquidToPump()) return;
	// Check stuck drain
	if (aMaterials[1] && (aMaterials[0] != "")) if (!InsertMaterial(Material(aMaterials[0]), 0,0,0,0, nil, true)) return;
	// OK; let's pump
	SetActionKeepPhase("Pump");
	UpdatePumpHeight();
}

private func GetDrainObject()
{
	if (drain_pipe) return drain_pipe->GetConnectedObject(this) ?? this;
	return this;
}

protected func Pumping()
{
	// at this point we can assert that we have power
	
	// something went wrong in the meantime?
	// let the central function handle that
	if (!source_pipe)
		return CheckCurrentState();
	
	// nothing to pump right now?
	if(!HasLiquidToPump())
	{
		UnmakePowerActuator();
		SetActionKeepPhase("PumpWaitLiquid");
		return;
	}
	
	// Recheck pump height
	if (pump_amount > 200 || last_source_y != (source_pipe->GetConnectedObject(this) ?? this)->GetY() || last_drain_y != GetDrainObject()->GetY())
	{
		UpdatePumpHeight(true);
		if (GetAction() != "Pump") return;
	}
	
	var pump_ok = true;
	
	// is empty?
	if ((aMaterials[1] == 0) || (aMaterials[0] == ""))
	{
		// get new materials
		aMaterials = source_pipe->GetLiquid(pumpable_materials, 5, this, true);
		// no material to pump?
		if ((aMaterials[0] == "") || (aMaterials[1] == 0))
			pump_ok = false;
	}
	if (pump_ok)
	{
		if (drain_pipe)
		{
			var pumped = BoundBy(drain_pipe->PutLiquid(aMaterials[0], aMaterials[1], this), 0, aMaterials[1]);
			aMaterials[1] -= pumped;
			// Drain is stuck?
			if (!pumped) pump_ok = false;
		}
		else
		{
			var i = Max(0, aMaterials[1]), itMaterial = Material(aMaterials[0]);
			while (i)
				if (InsertMaterial(itMaterial))
					i--;
				else
				{
					// Drain is stuck.
					pump_ok = false;
					break;
				}
			aMaterials[1] = i;
			if (!i) aMaterials[0] = "";
		}
	}
	if (!pump_ok)
	{
		// Couldn't pump. Probably drain stuck.
		UnmakePowerActuator();
		SetActionKeepPhase("PumpWaitLiquid");
	}
	// maybe add the possebility to empty pump (invaild mats?)
	return;
}

// Get current height the pump has to push liquids upwards (input.y - output.y)
func GetPumpHeight()
{
	var src = source_pipe->GetConnectedObject(this) ?? this, dst = GetDrainObject();
	var out_pos = {X=dst->GetX(), Y=dst->GetY()};
	dst->InsertMaterial(Material("Water"), 0,0,0,0, out_pos, true); // TODO assumes water material is loaded
	var src_x = src->GetX(), src_y = src->GetY();
	if (Global->GBackLiquid(src_x, src_y))
	{
		var src_mat = Global->GetMaterial(src_x, src_y);
		while (src_y>0 && src_mat == Global->GetMaterial(src_x, src_y-1)) --src_y;
	}
	return src_y - out_pos.Y;
}

// Recheck power usage/production for current pump height
func UpdatePumpHeight(bool unmake_actuator)
{
	pump_amount = 0;
	last_source_y = (source_pipe->GetConnectedObject(this) ?? this)->GetY();
	last_drain_y = GetDrainObject()->GetY();
	var new_power = PumpHeight2Power(GetPumpHeight());
	if (unmake_actuator)
		UnmakePowerActuator();
	else
		if (new_power == current_pump_power) return true;
	return MakePowerActuator(new_power);
}

// Makes this a power consumer or producer and sets appropriate waiting actions
func MakePowerActuator(int new_power)
{
	if (new_power > 0)
	{
		SetActionKeepPhase("PumpWaitPower");
		MakePowerConsumer(new_power);
	}
	else
	{
		SetActionKeepPhase("Pump");
		if (new_power < 0) MakePowerProducer(-new_power);
	}
	current_pump_power = new_power;
	return true;
}

func UnmakePowerActuator()
{
	if (current_pump_power > 0)
		UnmakePowerConsumer();
	else if (current_pump_power < 0)
		MakePowerProducer(0);
	return true;
}

// Transform pump height (input.y - output.y) to required power
func PumpHeight2Power(int pump_height)
{
	return BoundBy((pump_height + 35)/30*10, -150,150);
}

func CheckCurrentState()
{
	if(turned_on)
	{
		var has_source_pipe = !!source_pipe;
		var is_fullcon = GetCon() >= 100;
		
		if(GetAction() == "Wait") // waiting: not consuming power
		{
			if(has_source_pipe && is_fullcon)
			{
				UpdatePumpHeight();
				return;
			}
			else return; // waiting and no source pipe, keep waiting
		}
		else // not waiting: consuming power
		{
			if(!has_source_pipe || !is_fullcon)
			{
				UnmakePowerActuator();
				SetAction("Wait");
				return;
			}
			return;
		}
		// this point should not be reached
	}
	else // turned off
	{
		if(GetAction() != "Wait") // consuming power (except in PumpWaitLiquid condition)
		{
			UnmakePowerActuator();
			SetAction("Wait");
			return;
		}
		else // already waiting
			return;
	}
	FatalError("Not every case handled in Pump::CheckCurrentState");
}

// Returns whether the pump can pump some liquid.
private func HasLiquidToPump()
{
	// If there is no source pipe, return false.
	if (!source_pipe)
		return false;
	// If there is nothing to pump at the source return false.
	var source = source_pipe->GetConnectedObject(this);
	if (!source)
		return false;
	if (!source->GBackLiquid())
		return false;
	// TODO: Account for pumping into buildings.
	// Pumping is okay.
	return true;
}

/**
Set name or wildcard string of materials this pump can pump
@param to_val: Material that can be pumped. 0 or "*" for any material.
*/
public func SetPumpableMaterials(string to_val)
{
	pumpable_materials = to_val;
	return true;
}

local Name = "$Name$";
local Description = "$Description$";
local BlastIncinerate = 50;
local HitPoints = 70;

/*
	"Pump": the pump is currently working and consuming power
	"WillPump": the pump wants to work as soon as there is power
	"Wait": the pump has been turned off or some other need is not fulfilled (f.e. connected pipe)
*/
local ActMap = {
	Pump = {
		Prototype = Action,
		Name = "Pump",
		Procedure = DFA_NONE,
		Length = 30,
		Delay = 3,
		Directions = 2,
		FlipDir = 1,
		X = 0,
		Y = 0,
		Wdt = 28,
		Hgt = 32,
		NextAction = "Pump",
		StartCall = "CheckCurrentState",
		PhaseCall = "Pumping"
	},
	Wait = {
		Prototype = Action,
		Name = "Wait",
		Procedure = DFA_NONE,
		Length = 1,
		Delay = 60,
		Directions = 2,
		FlipDir = 1,
		X = 0,
		Y = 0,
		Wdt = 28,
		Hgt = 32,
		NextAction = "Wait",
		StartCall = "CheckCurrentState"
	},
	PumpWaitPower = {
		Prototype = Action,
		Name = "PumpWaitPower",
		Procedure = DFA_NONE,
		Length = 30,
		Delay = 0,
		Directions = 2,
		FlipDir = 1,
		X = 0,
		Y = 0,
		Wdt = 28,
		Hgt = 32,
		StartCall = "PumpWaitPowerStart",
		AbortCall = "PumpWaitPowerStop",
		NextAction = "PumpWaitPower"
	},
	PumpWaitLiquid = { // Pump waiting for liquid: Move slowly to indicate we're turned on
		Prototype = Action,
		Name = "PumpWaitLiquid",
		Procedure = DFA_NONE,
		Length = 30,
		Delay = 10,
		Directions = 2,
		FlipDir = 1,
		X = 0,
		Y = 0,
		Wdt = 28,
		Hgt = 32,
		NextAction = "PumpWaitLiquid",
		PhaseCall = "PumpingWaitForLiquid"
	}
};

// Set action while retaining phase from last action
func SetActionKeepPhase(string act)
{
	var phase = GetPhase();
	if (!SetAction(act)) return false;
	SetPhase(phase);
	return true;
}