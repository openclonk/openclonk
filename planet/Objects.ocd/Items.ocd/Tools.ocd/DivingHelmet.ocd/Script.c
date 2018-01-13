/**
	Diving Helmet
	Connected to a pump it provides the user with air underwater.
	
	@author: pluto, Clonkonaut
*/

#include Library_Wearable

local air_pipe;

local custom_entry =
{
	Right = "100%", Bottom = "2em",
	BackgroundColor = {Std = 0, OnHover = 0x50ff0000},
	image = {Right = "2em"},
	text = {Left = "2em"}
};

/*-- Engine Callbacks --*/

func Hit()
{
	Sound("Hits::Materials::Metal::DullMetalHit?");
}

/*-- Callbacks --*/

// Called by a connected pump
public func OnAirPumped(object pump)
{
	var wearer = Contained();
	if (wearer && IsWorn())
	{
		// If nearly max breath just keep the level
		if (wearer->~GetBreath() >= wearer->~GetMaxBreath() - 10)
			wearer->DoBreath(3);
		else // Slowly fill up breath
			wearer->DoBreath(4);
	}
}

// Return whether the helmet needs air.
public func QueryAirNeed(object pump)
{
	var wearer = Contained();
	if (wearer && IsWorn())
		return wearer->~GetBreath() < wearer->~GetMaxBreath();
	return false;
}

public func OnPipeLengthChange()
{
	// Update usage bar for a possible carrier (the clonk).
	var carrier = Contained();
	if (carrier)
		carrier->~OnInventoryChange();
	return;
}

// Display the line length bar over the pipe icon.
public func GetInventoryIconOverlay()
{
	var pipe = GetConnectedPipe();
	if (!pipe) return;

	var percentage = 100 * pipe->GetPipeLength() / pipe.PipeMaxLength;
	var red = percentage * 255 / 100;
	var green = 255 - red;
	// Overlay a usage bar.
	var overlay = 
	{
		Bottom = "0.75em",
		Margin = ["0.1em", "0.25em"],
		BackgroundColor = RGB(0, 0, 0),
		margin = 
		{
			Margin = "0.05em",
			bar = 
			{
				BackgroundColor = RGB(red, green, 0),
				Right = Format("%d%%", percentage),
			}
		}
	};
	return overlay;
}

// It is assumed that only suitable pipes (drain / neutral / air pipes) are connected.
public func OnPipeConnect(object pipe)
{
	if (!pipe) return;

	air_pipe = FindObject(Find_ID(PipeLine), Find_Or(Find_ActionTarget(this), Find_ActionTarget2(this)));
	if (!air_pipe) return;
	pipe->SetAirPipe();
	pipe->Report("$MsgConnectedPipe$");
}

func OnPipeDisconnect(object pipe)
{
	if (!air_pipe || !pipe) return;

	var other = air_pipe->GetConnectedObject(this);
	if (!other || other->GetID() != Pump)
		pipe->SetNeutralPipe();
	pipe->SetDrainPipe();
	other->~CheckState();
}

// Called by the interaction menu (OnPipeControl)
public func DoConnectPipe(object pipe, string specific_pipe_state)
{
	if (!pipe) return;

	if (GetConnectedPipe())
		DoCutPipe(GetConnectedPipe());

	pipe->ConnectPipeTo(this, PIPE_STATE_Air);
}

// Called by the interaction menu (OnPipeControl)
public func DoCutPipe(object pipe)
{
	if (pipe)
		if (pipe->~GetPipeKit())
			pipe->GetPipeKit()->CutLineConnection(this);
}

/*-- Usage --*/

public func ControlUse(object clonk)
{
	if (IsWorn())
		TakeOff();
	else
		PutOn(clonk);

	return true;
}

public func CanConnectPipe()
{
	return !GetConnectedPipe();
}

public func GetConnectedPipe()
{
	if (!air_pipe) return;
	if (!air_pipe->IsConnectedTo(this)) return;
	return air_pipe;
}

// Do not accept source pipes
public func QueryConnectPipe(object pipe, bool do_msg)
{
	if (GetConnectedPipe())
	{
		if (do_msg)
			pipe->Report("$MsgHasPipe$");
		return true;
	}
	if (pipe->IsSourcePipe())
	{
		if (do_msg)
			pipe->Report("$MsgPipeProhibited$");
		return true;
	}
	return false;
}


/*-- Interaction --*/

public func HasInteractionMenu() { return true; }

public func GetInteractionMenus(object clonk)
{
	var menus = _inherited(clonk, ...) ?? [];

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
	return menus;
}

public func GetPipeControlMenuEntries(object clonk)
{
	var menu_entries = [];

	// Add info message about pipe control.
	PushBack(menu_entries, {symbol = this, extra_data = "description",
			custom =
			{
				Prototype = custom_entry,
				Bottom = "1.2em",
				Priority = -1,
				BackgroundColor = RGB(25, 100, 100),
				text = {Prototype = custom_entry.text, Text = "$MenuPipeControl$"},
				image = {Prototype = custom_entry.image, Symbol = Pipe}
			}});

	var available_pipe = FindAvailablePipe(clonk);

	if (GetConnectedPipe())
		PushBack(menu_entries, GetHelmetMenuEntry(Icon_Cancel, "$MsgCutPipe$", 1, "cutpipe"));
	else if (available_pipe)
		PushBack(menu_entries, GetHelmetMenuEntry(available_pipe, "$MsgConnectPipe$", 1, "connectpipe"));

	return menu_entries;
}

func GetHelmetMenuEntry(symbol, string text, int priority, extra_data)
{
	return { symbol = symbol, extra_data = extra_data,
		custom =
		{
			Prototype = custom_entry,
			Priority = priority,
			text = {Prototype = custom_entry.text, Text = text},
			image = {Prototype = custom_entry.image, Symbol = symbol}
		}
	};
}

public func OnPipeControlHover(id symbol, string action, desc_menu_target, menu_id)
{
	var text = "";
	if (action == "cutpipe") text = "$DescCutPipe$";
	else if (action == "connectpipe") text = "$DescConnectPipe$";
	else if (action == "description") text = this.Description;
	GuiUpdateText(text, menu_id, 1, desc_menu_target);
}

public func OnPipeControl(symbol_or_object, string action, bool alt)
{
	if (action == "cutpipe")
		this->DoCutPipe(air_pipe);
	else if (action == "connectpipe")
		this->DoConnectPipe(symbol_or_object);

	UpdateInteractionMenus(this.GetPipeControlMenuEntries);	
}

func FindAvailablePipe(object container)
{
	for (var pipe in FindObjects(Find_ID(Pipe), Find_Container(container), Find_Or(Find_Func("IsAirPipe"), Find_Func("IsDrainPipe"), Find_Func("IsNeutralPipe"))))
		if (!this->~QueryConnectPipe(pipe, false))
			return pipe;
	return nil;
}

/*-- Production --*/

public func IsToolProduct() { return true; }

/*-- Display --*/

public func GetWearPlace()
{
	return WEARABLE_Head;
}

public func GetWearTransform()
{
	return Trans_Mul(Trans_Rotate(90, 0, 0, 1), Trans_Translate(0, -1000));
}

public func GetCarryMode(object clonk, bool secondary)
{
	if (IsWorn() || display_disabled)
		return CARRY_None;
	return CARRY_BothHands;
}

public func GetCarryPhase(object clonk)
{
	return 650;
}

public func GetCarryTransform(object clonk, bool secondary, bool no_hand, bool on_back)
{
	return Trans_Mul(Trans_Rotate(80, 0, 0, 1), Trans_Rotate(-90, 0, 1), Trans_Rotate(-45, 0, 0, 1), Trans_Translate(-1000, 4000));
}

func Definition(def)
{
	SetProperty("PictureTransformation", Trans_Mul(Trans_Rotate(45, 0, 1), Trans_Rotate(10, 0, 0, 1)),def);
}

/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Collectible = true;
local Components = {Wood = 1, Metal = 1};