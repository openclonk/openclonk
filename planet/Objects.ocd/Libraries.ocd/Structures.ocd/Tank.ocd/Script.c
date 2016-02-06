/* --- Tank --- */

#include Libary_LiquidContainer

/*
Author: ST-DDT, Marky
Import this to allow the structures to 
-fill liquids which has been pumped into the building into the internal contained 
-extract liquids from internal contained and pump it somewhere else
*/

static const LIBRARY_TANK_Menu_Action_Add_Drain = "adddrain";
static const LIBRARY_TANK_Menu_Action_Add_Source = "addsource";
static const LIBRARY_TANK_Menu_Action_Add_Neutral = "addneutral";
static const LIBRARY_TANK_Menu_Action_Cut_Drain = "cutdrain";
static const LIBRARY_TANK_Menu_Action_Cut_Source = "cutsource";
static const LIBRARY_TANK_Menu_Action_Cut_Neutral = "cutneutral";
static const LIBRARY_TANK_Menu_Action_Description = "description";


///**
//Extract liquid from this
//@param sznMaterial: Material to extract
//@param inMaxAmount: Max Amount of Material being extracted 
//@param pnPump: Object which extracts the liquid
//@param pnPipe: Pipe which extracts the liquid (connected to pnPump)
//@param bnWildcard: Usefull to extract random liquids; use '*' for sznMaterial for all Materials
//@return [irMaterial,irAmount]
//	-irMaterial: Material being extracted
//	-irAmount: Amount being extracted
//*/
//public func LiquidOutput(string sznMaterial, int inMaxAmount, object pnPump, object pnPipe, bool bnWildcard)
//{
//	//Search liquid to pump
//	if (bnWildcard)
//	{
//		if (WildcardMatch(szLiquid, sznMaterial))
//			sznMaterial = szLiquid;
//	}
//	//Wrong material?
//	if (szLiquid != sznMaterial)
//		return ["", 0];
//	inMaxAmount = Min(inMaxAmount, iLiquidAmount);
//	iLiquidAmount -= inMaxAmount;
//	return [szLiquid, inMaxAmount];
//}
//
///** 
//Insert liquid to this
//	@param sznMaterial: Material to insert
//	@param inMaxAmount: Max Amount of Material being inserted 
//	@param pnPump: Object which inserts the liquid
//	@param pnPipe: Pipe which inserts the liquid (connected to pnPump)
//	@return irAmount: The inserted amount
//*/
//public func LiquidInput(string sznMaterial, int inMaxAmount, object pnPump, object pnPipe)
//{
//	//wrong material?
//	if (szLiquid != sznMaterial)
//		return 0;
//	inMaxAmount = Min(MaxFillLevel() - iLiquidAmount, inMaxAmount);
//	iLiquidAmount += inMaxAmount;
//	return inMaxAmount;
//}

local lib_tank; // proplist for local variables

/* ---------- Callbacks ---------- */

func Construction()
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

func IsLiquidTank(){ return true;}

/* ---------- Menu Entries ---------- */

public func GetInteractionMenus(object clonk)
{
	var menus = _inherited() ?? [];
	
	if (CanConnectPipe())
	{
		var pipe_menu =
		{
			title = "$MenuPipeControl$",
			entries_callback = this.GetPipeControlMenuEntries,
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
		PushBack(menu_entries, GetTankMenuEntry(Icon_Cancel, "$MsgCutSource$", 1, LIBRARY_TANK_Menu_Action_Cut_Source));
	else if (source_pipe)
		PushBack(menu_entries, GetTankMenuEntry(source_pipe, "$MsgConnectSource$", 1, LIBRARY_TANK_Menu_Action_Add_Source));

	if (GetDrainPipe())
		PushBack(menu_entries, GetTankMenuEntry(Icon_Cancel, "$MsgCutDrain$", 2, LIBRARY_TANK_Menu_Action_Cut_Drain));
	else if (drain_pipe)
		PushBack(menu_entries, GetTankMenuEntry(drain_pipe, "$MsgConnectDrain$", 2, LIBRARY_TANK_Menu_Action_Add_Drain));

	if (GetNeutralPipe())
		PushBack(menu_entries, GetTankMenuEntry(Icon_Cancel, "$MsgCutNeutral$", 3, LIBRARY_TANK_Menu_Action_Cut_Neutral));
	else if (neutral_pipe)
		PushBack(menu_entries, GetTankMenuEntry(neutral_pipe, "$MsgConnectNeutral$", 3, LIBRARY_TANK_Menu_Action_Add_Neutral));

	return menu_entries;
}


func GetTankMenuEntry(symbol, string text, int priority, extra_data)
{
	return {symbol = symbol, extra_data = extra_data, 
		custom =
		{
			Prototype = lib_tank.custom_entry,
			Priority = priority,
			text = {Prototype = lib_tank.custom_entry.text, Text = text},
			image = {Prototype = lib_tank.custom_entry.image, Symbol = symbol}
		}};
}


public func OnPipeControlHover(id symbol, string action, desc_menu_target, menu_id)
{
	var text = "";
	if (action == LIBRARY_TANK_Menu_Action_Add_Drain) text = "$DescConnectDrain$";
	else if (action == LIBRARY_TANK_Menu_Action_Cut_Drain) text = "$DescCutDrain$";
	else if (action == LIBRARY_TANK_Menu_Action_Add_Source) text = "$DescConnectSource$";
	else if (action == LIBRARY_TANK_Menu_Action_Cut_Source) text = "$DescCutSource$";
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

	UpdateInteractionMenus(this.GetPipeControlMenuEntries);	
}


/* ---------- Handle connections ---------- */

func GetDrainPipe(){ return lib_tank.drain_pipe;}
func GetSourcePipe(){ return lib_tank.source_pipe;}
func GetNeutralPipe(){ return lib_tank.neutral_pipe;}

func SetDrainPipe(object drain_pipe)
{
	lib_tank.drain_pipe = drain_pipe;
	return lib_tank.drain_pipe;
}

func SetSourcePipe(object source_pipe)
{
	lib_tank.source_pipe = source_pipe;
	return lib_tank.source_pipe;
}

func SetNeutralPipe(object neutral_pipe)
{
	lib_tank.neutral_pipe = neutral_pipe;
	return lib_tank.neutral_pipe;
}


/* ---------- Menu callbacks ---------- */

func DoConnectPipe(object pipe, string specific_pipe_state)
{
	pipe->ConnectPipeTo(this, specific_pipe_state);
}

func DoCutPipe(object pipe)
{
	if (pipe)
	{
		pipe->CutLineConnection(this);		
	}
}

func FindAvailablePipe(object container, find_state)
{
	for (var pipe in FindObjects(Find_ID(Pipe), Find_Container(container), find_state))
	{
		if (!this->~QueryConnectPipe(pipe))
			return pipe;
	}
	return nil;
}

/* ---------- Pipe callbacks ---------- */

func CanConnectPipe(){ return true;}

func OnPipeDisconnect(object pipe)
{
	// pipe objects have to be reset!
	if (pipe == GetDrainPipe()) SetDrainPipe();
	if (pipe == GetSourcePipe()) SetSourcePipe();
	if (pipe == GetNeutralPipe()) SetNeutralPipe();
}
