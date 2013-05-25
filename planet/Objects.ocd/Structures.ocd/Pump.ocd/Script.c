/*--
	Pump
	Author: Maikel, ST-DDT, Sven2
	
	Pumps liquids using drain and source pipes.
--*/

#include Library_Structure
#include Library_Ownable
#include Library_PowerConsumer
#include Library_PowerProducer

/** This object is a liquid pump, thus pipes can be connected. */
public func IsLiquidPump() { return true; }

protected func Initialize()
{
	turned_on = true;
	SetAction("Wait");
	AddTimer("CheckState",30);
	return;
}

/*-- Interaction --*/

local turned_on;

public func IsInteractable() { return GetCon() >= 100; }

public func GetInteractionMetaInfo(object clonk)
{
	if (turned_on)
		return { Description = "$MsgTurnOff$", IconName = nil, IconID = Icon_Stop };
	else
		return { Description = "$MsgTurnOn$", IconName = nil, IconID = Icon_Play };
}

/** Turn on or off. */
public func Interact(object clonk)
{
	turned_on = !turned_on;
	if(!turned_on) {
		SetAction("Wait");
	}
	else {
		SetAction("Pump");
	}
	CheckState();
	
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
	CheckState();
}

/*-- Power stuff --*/

func QueryWaivePowerRequest()
{
	// has less priority than other objects, but not too low
	return 10;
}

func OnNotEnoughPower()
{
	// assert: current action is "Pump" or "PumpWaitLiquid"
	SetAction("PumpWaitPower");
	CheckState();
	return _inherited(...);
}

func OnEnoughPower()
{
	// assert: current action is either PumpWaitPower or Wait
	SetAction("Pump");
	CheckState();
	return _inherited(...);
}

local aMaterials=["", 0]; //contained liquids
local pumpable_materials; // materials that can be pumped
local pump_amount; // mat amount pumped since last height check

/** PhaseCall of PumpWaitLiquid */
protected func PumpingWaitForLiquid()
{
	// Pump is ready; check for liquids to pump
	if(!source_pipe) return CheckCurrentState();
	if(!HasLiquidToPump()) return;
	// Check stuck drain
	if (aMaterials[1] && (aMaterials[0] != ""))
		if (!InsertMaterial(Material(aMaterials[0]), 0,0,0,0, nil, true))
			return;
	// OK; let's pump
	SetAction("Pump");
}

/** Returns object to which the liquid is pumped */
private func GetDrainObject()
{
	if (drain_pipe) return drain_pipe->GetConnectedObject(this) ?? this;
	return this;
}

/** Returns object to which the liquid is pumped */
private func GetSourceObject()
{
	if (source_pipe) return source_pipe->GetConnectedObject(this) ?? this;
	return this;
}

/** PhaseCall of Pump: Pump the liquid from the source to the drain pipe */
protected func Pumping()
{
	// at this point we can assert that we have power
	
	// something went wrong in the meantime?
	// let the central function handle that
	if (!source_pipe)
		return CheckState();
	
	// nothing to pump right now?
	if(!HasLiquidToPump())
	{
		SetAction("PumpWaitLiquid");
		
		return;
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
		else
			pump_amount += aMaterials[1];

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
		SetAction("PumpWaitLiquid");
	}
	// maybe add the possibility to empty pump (invaild mats?)
	return;
}

func CheckState()
{
	if(turned_on)
	{
		var is_fullcon = GetCon() >= 100;
		
		if(GetAction() == "Wait") // waiting: not consuming power
		{
			if(source_pipe && is_fullcon)
			{
				SetAction("Pump");
			}
		}
		else // not waiting: consuming power
		{
			if(!source_pipe || !is_fullcon)
			{
				SetAction("Wait");
			}
		}
	}
	UpdatePowerUsage();
}

/** Get current height the pump has to push liquids upwards (input.y - output.y) */
private func GetPumpHeight()
{
	var target = {X=GetDrainObject()->GetX(), Y=GetDrainObject()->GetY()};
	var source = {X=GetSourceObject()->GetX(), Y=GetSourceObject()->GetY()};
	
	// find Y position of surface of liquid that is pumped to target
	if (Global->GBackLiquid(source.X, source.Y))
	{
		var src_mat = Global->GetMaterial(source.X, source.Y);
		while (source.Y > 0 && src_mat == Global->GetMaterial(source.X, source.Y-1))
			--source.Y;
	}
	return source.Y - target.Y;
}

/** Recheck power usage/production for current pump height
	and make the pump a producer / consumer for the power system */
private func UpdatePowerUsage()
{
	var new_power;
	if(IsUsingPower())
		new_power = PumpHeight2Power(GetPumpHeight());
	else
		new_power = 0;
	
	// and update energy system
	if (new_power > 0)
	{
		UnmakePowerProducer();
		MakePowerConsumer(new_power);
	}
	else if (new_power <= 0)
	{
		UnmakePowerConsumer();
		MakePowerProducer(-new_power);
	}
}

/** Return whether the pump should be using power in the current state */
private func IsUsingPower()
{
	// does also not consume power if waiting (for source pipe)
	return turned_on && GetAction() != "Wait" && GetAction() != "PumpWaitLiquid";
}

/** Transform pump height (input.y - output.y) to required power */
private func PumpHeight2Power(int pump_height)
{
	// pumping downwards will only produce energy after an offset
	var power_offset = 35;
	// max power consumed/produced
	var max_power = 150;
	
	return BoundBy((pump_height + power_offset)/30*10, -max_power,max_power);
}

/** Returns whether there is liquid at the source pipe to pump */
private func HasLiquidToPump()
{
	// also return false if there is no source (pipe) connected
	if (!source_pipe)
		return false;
	var source = source_pipe->GetConnectedObject(this);
	if (!source)
		return false;
		
	if (!source->GBackLiquid())
		return false;
	// TODO: Account for pumping into buildings.
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
	States
	"Wait":				turned off or source pipe not connected
	"Pump":				currently working and consuming/producing power
	"PumpWaitLiquid":	waiting for liquid to pump
	"PumpWaitPower":	waiting for power

*/
local ActMap = {
	Pump = {
		Prototype = Action,
		Name = "Pump",
		Procedure = DFA_NONE,
		Directions = 2,
		FlipDir = 1,
		Length = 30,
		Delay = 3,
		FacetBase = 1,
		NextAction = "Pump",
		PhaseCall = "Pumping"
	},
	Wait = {
		Prototype = Action,
		Name = "Wait",
		Procedure = DFA_NONE,
		Directions = 2,
		FlipDir = 1,
		Length = 1,
		Delay = 60,
		FacetBase = 1,
		NextAction = "Wait",
	},
	PumpWaitPower = {
		Prototype = Action,
		Name = "PumpWaitPower",
		Procedure = DFA_NONE,
		Directions = 2,
		FlipDir = 1,
		Length = 1,
		Delay = 60,
		FacetBase = 1,
		NextAction = "PumpWaitPower",
	},
	PumpWaitLiquid = { // Pump waiting for liquid: Move slowly to indicate we're turned on
		Prototype = Action,
		Name = "PumpWaitLiquid",
		Procedure = DFA_NONE,
		Length = 30,
		Delay = 10,
		Directions = 2,
		FlipDir = 1,
		FacetBase = 1,
		NextAction = "PumpWaitLiquid",
		PhaseCall = "PumpingWaitForLiquid"
	}
};
