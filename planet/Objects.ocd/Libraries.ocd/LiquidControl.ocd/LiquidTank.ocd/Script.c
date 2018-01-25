/**
	Liquid Tank
	A structure that can contain liquids. Connecting pipes to the
	structure can be allowed, but this has to be implemented by the
	object. This is controlled with the callbacks
	 - QueryConnectPipe
	 - OnPipeConnect
	 - OnPipeDisconnect
	in that structure.
	
	@author Marky
*/

#include Library_LiquidContainer


static const LIBRARY_TANK_Menu_Action_Add_Drain = "adddrain";
static const LIBRARY_TANK_Menu_Action_Add_Source = "addsource";
static const LIBRARY_TANK_Menu_Action_Add_Neutral = "addneutral";
static const LIBRARY_TANK_Menu_Action_Cut_Drain = "cutdrain";
static const LIBRARY_TANK_Menu_Action_Cut_Source = "cutsource";
static const LIBRARY_TANK_Menu_Action_Cut_Neutral = "cutneutral";
static const LIBRARY_TANK_Menu_Action_Swap_SourceDrain = "swapsourcedrain";
static const LIBRARY_TANK_Menu_Action_Description = "description";


local lib_tank; // proplist for local variables


/*-- Callbacks --*/

public func Construction()
{
	lib_tank = {
		drain_pipe = nil,
		source_pipe = nil,
		neutral_pipe = nil,
		custom_entry = 
		{
			Right = "100%", Bottom = "2em",
			BackgroundColor = {Std = 0, OnHover = 0x50ff0000},
			image = {Right = "2em"},
			text = {Left = "2em"}
		},		
	};
	
	_inherited(...);
}

public func IsLiquidTank() { return true; }


/*-- Menu Entries --*/

public func HasInteractionMenu() { return true; }

public func GetInteractionMenus(object clonk)
{
	var menus = _inherited(clonk, ...) ?? [];
	
	if (CanConnectPipe())
	{
		var pipe_menu =
		{
			title = "$MenuPipeControl$",
			entries_callback = this.GetPipeControlMenuEntries,
			entries_callback_target = this,
			callback = "OnPipeControl",
			callback_hover = "OnPipeControlHover",
			callback_target = this,
			BackgroundColor = RGB(0, 50, 50),
			Priority = 30
		};
		PushBack(menus, pipe_menu);
	}
	return menus;
}

public func GetPipeControlMenuEntries(object clonk)
{
	var menu_entries = [];
	
	// Add info message about pipe control.
	PushBack(menu_entries, {symbol = this, extra_data = LIBRARY_TANK_Menu_Action_Description,
			custom =
			{
				Prototype = lib_tank.custom_entry,
				Bottom = "1.2em",
				Priority = -1,
				BackgroundColor = RGB(25, 100, 100),
				text = {Prototype = lib_tank.custom_entry.text, Text = "$MenuPipeControl$"},
				image = {Prototype = lib_tank.custom_entry.image, Symbol = Pipe}
			}});

	var source_pipe = FindAvailablePipe(clonk, Find_Func("IsSourcePipe"));
	var drain_pipe = FindAvailablePipe(clonk, Find_Func("IsDrainPipe"));
	var neutral_pipe = FindAvailablePipe(clonk, Find_Func("IsNeutralPipe"));

	if (GetSourcePipe())
	{
		if (!GetSourcePipe()->QueryCutLineConnection(this))
			PushBack(menu_entries, GetTankMenuEntry(Icon_Cancel, GetConnectedPipeMessage("$MsgCutSource$", GetSourcePipe()), 1, LIBRARY_TANK_Menu_Action_Cut_Source, RGB(102, 136, 34)));
	}
	else if (source_pipe)
		PushBack(menu_entries, GetTankMenuEntry(source_pipe, GetConnectedPipeMessage("$MsgConnectSource$", source_pipe), 1, LIBRARY_TANK_Menu_Action_Add_Source, RGB(102, 136, 34)));

	if (GetDrainPipe())
	{
		if (!GetDrainPipe()->QueryCutLineConnection(this))
			PushBack(menu_entries, GetTankMenuEntry(Icon_Cancel, GetConnectedPipeMessage("$MsgCutDrain$", GetDrainPipe()), 2, LIBRARY_TANK_Menu_Action_Cut_Drain, RGB(238, 102, 0)));
	}
	else if (drain_pipe)
		PushBack(menu_entries, GetTankMenuEntry(drain_pipe, GetConnectedPipeMessage("$MsgConnectDrain$", drain_pipe), 2, LIBRARY_TANK_Menu_Action_Add_Drain, RGB(238, 102, 0)));

	if (GetNeutralPipe())
	{
		if (!GetNeutralPipe()->QueryCutLineConnection(this))
			PushBack(menu_entries, GetTankMenuEntry(Icon_Cancel, GetConnectedPipeMessage("$MsgCutNeutral$", GetNeutralPipe()), 3, LIBRARY_TANK_Menu_Action_Cut_Neutral, RGB(80, 80, 120)));
	}
	else if (neutral_pipe)
		PushBack(menu_entries, GetTankMenuEntry(neutral_pipe, GetConnectedPipeMessage("$MsgConnectNeutral$", neutral_pipe), 3, LIBRARY_TANK_Menu_Action_Add_Neutral, RGB(80, 80, 120)));

	if (IsAllowedSwapSourceDrain())
		PushBack(menu_entries, GetTankMenuEntry(Icon_Swap, GetConnectedPipeMessage("$MsgSwapSourceDrain$", GetSourcePipe(), GetDrainPipe()), 10, LIBRARY_TANK_Menu_Action_Swap_SourceDrain, nil));

	return menu_entries;
}

public func GetTankMenuEntry(symbol, string text, int priority, extra_data, int clr)
{
	return {symbol = symbol, extra_data = extra_data, 
		custom =
		{
			Prototype = lib_tank.custom_entry,
			Priority = priority,
			text = {Prototype = lib_tank.custom_entry.text, Text = text},
			image = {Prototype = lib_tank.custom_entry.image, Symbol = symbol, BackgroundColor = clr},
		}};
}

public func OnPipeControlHover(symbol_or_object, string action, desc_menu_target, menu_id)
{
	var text = "";
	if (action == LIBRARY_TANK_Menu_Action_Add_Drain) text = GetConnectedPipeDescription("$DescConnectDrain$", symbol_or_object);
	else if (action == LIBRARY_TANK_Menu_Action_Cut_Drain) text = GetConnectedPipeDescription("$DescCutDrain$", GetDrainPipe());
	else if (action == LIBRARY_TANK_Menu_Action_Add_Source) text = GetConnectedPipeDescription("$DescConnectSource$", symbol_or_object);
	else if (action == LIBRARY_TANK_Menu_Action_Cut_Source) text = GetConnectedPipeDescription("$DescCutSource$", GetSourcePipe());
	else if (action == LIBRARY_TANK_Menu_Action_Add_Neutral) text = GetConnectedPipeDescription("$DescConnectNeutral$", symbol_or_object);
	else if (action == LIBRARY_TANK_Menu_Action_Cut_Neutral) text = GetConnectedPipeDescription("$DescCutNeutral$", GetNeutralPipe());
	else if (action == LIBRARY_TANK_Menu_Action_Swap_SourceDrain) text = GetConnectedPipeDescription("$DescSwapSourceDrain$", GetSourcePipe(), GetDrainPipe());
	else if (action == LIBRARY_TANK_Menu_Action_Description) text = this.Description;
	GuiUpdateText(text, menu_id, 1, desc_menu_target);
}

public func OnPipeControl(symbol_or_object, string action, bool alt)
{
	if (action == LIBRARY_TANK_Menu_Action_Add_Source)
		this->DoConnectPipe(symbol_or_object, PIPE_STATE_Source);
	else if (action == LIBRARY_TANK_Menu_Action_Cut_Source)
		this->DoCutPipe(GetSourcePipe());
	else if (action == LIBRARY_TANK_Menu_Action_Add_Drain)
		this->DoConnectPipe(symbol_or_object, PIPE_STATE_Drain);
	else if (action == LIBRARY_TANK_Menu_Action_Cut_Drain)
		this->DoCutPipe(GetDrainPipe());
	else if (action == LIBRARY_TANK_Menu_Action_Add_Neutral)
		this->DoConnectPipe(symbol_or_object, PIPE_STATE_Neutral);
	else if (action == LIBRARY_TANK_Menu_Action_Cut_Neutral)
		this->DoCutPipe(GetNeutralPipe());
	else if (action == LIBRARY_TANK_Menu_Action_Swap_SourceDrain)
		this->DoSwapSourceDrain(GetSourcePipe(), GetDrainPipe());

	UpdateInteractionMenus(this.GetPipeControlMenuEntries);
}

public func GetConnectedPipeMessage(string base_msg, object pipe1, object pipe2)
{
	var connected1 = (pipe1->GetConnectedObject(this) ?? pipe1->GetConnectedObject(pipe1)) ?? pipe1;
	var msg = Format("%s $MsgConnectedTo$", base_msg, connected1->GetID());
	if (pipe2)
	{
		var connected2 = (pipe2->GetConnectedObject(this) ?? pipe2->GetConnectedObject(pipe1)) ?? pipe2;
		msg = Format("%s $MsgConnectedToMultiple$", base_msg, connected1->GetID(), connected2->GetID());
	}
	return msg;
}

public func GetConnectedPipeDescription(string base_desc, object pipe1, object pipe2)
{
	var connected1 = (pipe1->GetConnectedObject(this) ?? pipe1->GetConnectedObject(pipe1)) ?? pipe1;
	var desc = Format("%s $DescConnectedTo$", base_desc, connected1->GetID());
	if (pipe2)
	{
		var connected2 = (pipe2->GetConnectedObject(this) ?? pipe2->GetConnectedObject(pipe1)) ?? pipe2;
		desc = Format("%s $DescConnectedToMultiple$", base_desc, connected1->GetID(), connected2->GetID());
	}
	return desc;
}

/*-- Handle Connections --*/

public func GetDrainPipe() { return lib_tank.drain_pipe;}
public func GetSourcePipe() { return lib_tank.source_pipe;}
public func GetNeutralPipe() { return lib_tank.neutral_pipe;}

public func SetDrainPipe(object drain_pipe)
{
	lib_tank.drain_pipe = drain_pipe;
	return lib_tank.drain_pipe;
}

public func SetSourcePipe(object source_pipe)
{
	lib_tank.source_pipe = source_pipe;
	return lib_tank.source_pipe;
}

public func SetNeutralPipe(object neutral_pipe)
{
	lib_tank.neutral_pipe = neutral_pipe;
	return lib_tank.neutral_pipe;
}


/*-- Menu Callbacks --*/

public func DoConnectPipe(object pipe, string specific_pipe_state)
{
	pipe->ConnectPipeTo(this, specific_pipe_state);
}

public func DoCutPipe(object pipe)
{
	if (pipe)
	{
		pipe->CutLineConnection(this);		
	}
}

public func IsAllowedSwapSourceDrain()
{
	if (!GetSourcePipe() || !GetDrainPipe())
		return false;
	var source_line = GetSourcePipe()->GetConnectedLine();
	var drain_line = GetDrainPipe()->GetConnectedLine();
	if (source_line && drain_line)
	{
		var source = source_line->GetConnectedObject(this);
		var drain = drain_line->GetConnectedObject(this);
		// Allow swapping if both ends are pipes.
		if (source->GetID() == Pipe && drain->GetID() == Pipe)
			return true;	
	}
	// TODO: Also allow swapping if non pipe objects are on the other end and accept the drain or source.
	return false;
}

public func DoSwapSourceDrain(object source, object drain)
{
	SetDrainPipe(source);
	SetSourcePipe(drain);
	source->SetDrainPipe();
	drain->SetSourcePipe();
	// TODO: Also change the state of the objects on the other end of the pipe.
}

public func FindAvailablePipe(object container, find_state)
{
	for (var pipe in FindObjects(Find_ID(Pipe), Find_Container(container), find_state))
	{
		if (this->~QueryConnectPipe(pipe, false))
			continue;
		// Because of the delayed entrance into the pipeline of 1 frame the pipe may still be considered available.
		// Therefore check also there there exists no connection of the line to this object.
		var line = pipe->GetConnectedLine();
		if (line && line->IsConnectedTo(this, true))
			continue;
		return pipe;		
	}
	return nil;
}


/*-- Pipe Callbacks --*/

public func CanConnectPipe(){ return true;}

public func OnPipeDisconnect(object pipe)
{
	// pipe objects have to be reset!
	if (pipe == GetDrainPipe()) SetDrainPipe();
	if (pipe == GetSourcePipe()) SetSourcePipe();
	if (pipe == GetNeutralPipe()) SetNeutralPipe();
}


/*-- Scenario Saving --*/

public func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	if (lib_tank.drain_pipe) props->AddCall("DrainPipe", this, "SetDrainPipe", lib_tank.drain_pipe);
	if (lib_tank.source_pipe) props->AddCall("SourcePipe", this, "SetSourcePipe", lib_tank.source_pipe);
	if (lib_tank.neutral_pipe) props->AddCall("NeutralPipe", this, "SetNeutralPipe", lib_tank.neutral_pipe);
	return true;
}
