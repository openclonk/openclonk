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
static const LIBRARY_TANK_Menu_Action_Cut_Drain = "cutdrain";
static const LIBRARY_TANK_Menu_Action_Cut_Source = "cutsource";
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

func Construction()
{
	lib_tank = {
		drain_pipe = nil,
		source_pipe = nil,
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

func CanConnectPipe(){ return this->CanConnectSourcePipe() || this->CanConnectDrainPipe();}

func CanConnectDrainPipe(){ return false;}
func CanConnectSourcePipe(){ return false;}

func QueryConnectDrainPipe()
{
	return !this->CanConnectDrainPipe() || GetDrainPipe();
}

func QueryConnectSourcePipe()
{
	return !this->CanConnectSourcePipe() || GetSourcePipe();
}


func GetDrainPipe(){ return lib_tank.drain_pipe;}
func GetSourcePipe(){ return lib_tank.source_pipe;}

func SetDrainPipe(object drain_pipe)
{
	if (!this->CanConnectDrainPipe()) FatalError("This object cannot have a drain pipe!");

	lib_tank.drain_pipe = drain_pipe;
	return lib_tank.drain_pipe;
}

func SetSourcePipe(object source_pipe)
{
	if (!this->CanConnectSourcePipe()) FatalError("This object cannot have a source pipe!");

	lib_tank.source_pipe = source_pipe;
	return lib_tank.source_pipe;
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
				text = {Prototype = lib_tank.custom_entry.text, Text = "$MsgPipeControl$"},
				image = {Prototype = lib_tank.custom_entry.image, Symbol = Pipe}
			}});

	var index = 0;

	if (GetSourcePipe())
		PushBack(menu_entries, GetTankMenuEntry(Icon_Cancel, "$MsgCutSource$", ++index, LIBRARY_TANK_Menu_Action_Cut_Source));

	for (var pipe in FindSourcePipes(clonk))
	{
		PushBack(menu_entries, GetTankMenuEntry(pipe, "$MsgConnectSource$", ++index, LIBRARY_TANK_Menu_Action_Add_Source));
	}
	
	if (GetDrainPipe())
		PushBack(menu_entries, GetTankMenuEntry(Icon_Cancel, "$MsgCutDrain$", ++index, LIBRARY_TANK_Menu_Action_Cut_Drain));

	for (var pipe in FindDrainPipes(clonk))
	{
		PushBack(menu_entries, GetTankMenuEntry(pipe, "$MsgConnectDrain$", ++index, LIBRARY_TANK_Menu_Action_Add_Drain));
	}


	//var entry_source_pipe = GetSourceMenuEntry(clonk);
	//var entry_drain_pipe = GetDrainMenuEntry(clonk);

	//if (entry_source_pipe) PushBack(menu_entries, entry_source_pipe);
	//if (entry_drain_pipe)  PushBack(menu_entries, entry_drain_pipe);

	return menu_entries;
}


func FindSourcePipes(object container)
{
	return FindObjects(Find_ID(Pipe), Find_Container(container), Find_Func("CanConnectAsSourcePipe"));
}


func FindDrainPipes(object container)
{
	return FindObjects(Find_ID(Pipe), Find_Container(container), Find_Func("CanConnectAsDrainPipe"));
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
		this->DoConnectSourcePipe(symbol_or_object);
	else if (action == LIBRARY_TANK_Menu_Action_Cut_Source)
		this->DoCutSourcePipe();
	else if (action == LIBRARY_TANK_Menu_Action_Add_Drain)
		this->DoConnectDrainPipe(symbol_or_object);
	else if (action == LIBRARY_TANK_Menu_Action_Cut_Drain)
		this->DoCutDrainPipe();

	UpdateInteractionMenus(this.GetPipeControlMenuEntries);	
}

func DoConnectSourcePipe(object pipe)
{
	pipe->ConnectTo(pipe->Contained(), this, PIPE_STATE_Source);
}

func DoConnectDrainPipe(object pipe)
{
	pipe->ConnectTo(pipe->Contained(), this, PIPE_STATE_Drain);
}


func DoCutSourcePipe()
{
	DoCutPipe(GetSourcePipe());
}

func DoCutDrainPipe()
{
	DoCutPipe(GetDrainPipe());
}

func DoCutPipe(object pipe)
{
	if (pipe) 
	{
		var pipe_kit = pipe->GetPipeKit();
		pipe_kit->CutConnection(this);
	}
}
