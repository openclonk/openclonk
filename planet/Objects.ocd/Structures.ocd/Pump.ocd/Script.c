/*--
	Pump
	Author: Maikel, ST-DDT
	
	Pumps liquids using drain and source pipes.
--*/

#include Library_Structure
#include Library_Ownable
#include Library_PowerConsumer

func NeededPower() { return 100; }

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
	// assert: current action is "Pump"
	SetAction("WillPump");
	return _inherited(...);
}

func OnEnoughPower()
{
	// assert: current action is either WillPump or Wait
	SetAction("Pump");
	return _inherited(...);
}

local aMaterials=["", 0]; //contained liquids
local pumpable_materials; // materials that can be pumped

protected func Pumping()
{
	// at this point we can assert that we have power
	
	// something went wrong in the meantime?
	// let the central function handle that
	if (!source_pipe)
		return CheckCurrentState();
	
	// not able to pump right now?
	if(!HasLiquidToPump()) return;
	
	// is empty?
	if ((aMaterials[1] == 0) || (aMaterials[0] == ""))
	{
		// get new materials
		aMaterials = source_pipe->GetLiquid(pumpable_materials, 5, this, true);
		// no material to pump?
		if ((aMaterials[0] == "") || (aMaterials[1] == 0))
			return;
	}
	if (drain_pipe)
		aMaterials[1] -= BoundBy(drain_pipe->PutLiquid(aMaterials[0], aMaterials[1], this), 0, aMaterials[1]);
	else
	{
		var i = Max(0, aMaterials[1]), itMaterial = Material(aMaterials[0]);
		while (i--)
			InsertMaterial(itMaterial);
		aMaterials = ["", 0];
	}
	// maybe add the possebility to empty pump (invaild mats?)
	return;	
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
				SetAction("WillPump");
				MakePowerConsumer(NeededPower());
				return;
			}
			else return; // waiting and no source pipe, keep waiting
		}
		else // not waiting: consuming power
		{
			if(!has_source_pipe || !is_fullcon)
			{
				UnmakePowerConsumer();
				SetAction("Wait");
				return;
			}
			return;
		}
		// this point should not be reached
	}
	else // turned off
	{
		if(GetAction() != "Wait") // consuming power
		{
			UnmakePowerConsumer();
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
	WillPump = {
		Prototype = Action,
		Name = "WillPump",
		Procedure = DFA_NONE,
		Length = 1,
		Delay = 60,
		Directions = 2,
		FlipDir = 1,
		X = 0,
		Y = 0,
		Wdt = 28,
		Hgt = 32,
		NextAction = "WillPump",
	}
};
