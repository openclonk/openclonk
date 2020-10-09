/**
	Refinery Drain
	Oil must be pumped into this structure to be processed further.
	
	@author Maikel
*/

#include Library_Structure
#include Library_Ownable
#include Library_LiquidContainer
#include Library_PipeControl


/*-- Pipeline --*/

public func GetLiquidContainerMaxFillLevel(liquid_name)
{
	return 10**9;
}

public func IsLiquidContainerForMaterial(string liquid)
{
	return WildcardMatch("Oil", liquid);
}

public func QueryConnectPipe(object pipe, bool do_msg)
{
	if (pipe->IsDrainPipe() || pipe->IsNeutralPipe())
	{
		// Also check if already connected to this refinery drain.
		var line = pipe->GetConnectedLine();
		if (!line)
			return false;		
		if (!line->IsConnectedTo(this, true))
			return false;
	}
	if (do_msg)
		pipe->Report("$MsgPipeProhibited$");
	return true;
}

public func OnPipeConnect(object pipe, string specific_pipe_state)
{
	SetDrainPipe(pipe);
	pipe->SetDrainPipe();
	pipe->Report("$MsgConnectedPipe$");
}

public func OnPipeDisconnect(object pipe)
{
	var result = inherited(pipe, ...);
	// There may be still drain pipes connected, selected one as the current drain pipe.
	for (var line in FindObjects(Find_Func("IsConnectedTo", this)))
	{
		var new_pipe = line->GetPipeKit();
		if (new_pipe == pipe || !new_pipe->IsDrainPipe())
			continue;
		SetDrainPipe(new_pipe);
		break;
	}
	return result;
}


/*-- Interaction Interface --*/

public func GetOilAmount()
{
	var oil = FindObject(Find_ID(Oil), Find_Container(this));
	if (oil)
		return oil->GetStackCount() / 1000;
	return 0;
}

public func HasInteractionMenu() { return true; }

public func GetInteractionMenus(object clonk)
{
	var menus = _inherited(clonk, ...) ?? [];
	var oil_menu =
	{
		title = "$MsgOilOverview$",
		entries_callback = this.GetOilDisplayMenuEntries,
		callback_hover = "OnOilDisplayHover",
		callback_target = this,
		BackgroundColor = RGB(0, 50, 50),
		Priority = 20
	};
	PushBack(menus, oil_menu);
	return menus;
}

public func GetOilDisplayMenuEntries(object clonk)
{
	return 
	[{
		symbol = Oil,
		extra_data = "oil",
		custom = {
			Style = GUI_FitChildren | GUI_TextVCenter | GUI_TextLeft,
			Bottom = "1.1em",
			BackgroundColor = {Std = 0, OnHover = 0x50ff0000},
			Priority = 1,
			Text = Format("$MsgOilPumped$", GetOilAmount())
		}
	}];
}

public func OnOilDisplayHover(id symbol, string extra_data, desc_menu_target, menu_id)
{
	GuiUpdateText(Format("$MsgOilDescription$", GetOilAmount()), menu_id, 1, desc_menu_target);
	return;
}


/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";