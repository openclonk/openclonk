/*-- Scaffold --*/

#include Library_Structure

local walls;

/* Initialization */

public func Construction()
{
	walls = [nil, nil, nil, nil, nil, nil, nil, nil];
	return _inherited(...);
}

public func IsHammerBuildable() { return true; }

func Initialize()
{
	return _inherited(...);
}

func Destruction()
{
	RemoveWalls();
	return _inherited(...);
}

func Incineration()
{
	RemoveWalls();
	return _inherited(...);
}

func RemoveWalls()
{
	for (var wall in walls)
	{
		if (wall) wall->RemoveObject();
	}
	return true;
}

/*-- Interaction --*/

public func HasInteractionMenu() { return true; }

public func GetExtensionMenuEntries(object clonk)
{
	// Add one menu entry that consists of a whole sub-menu.
	
	var control_entry_prototype = 
	{
		Style = GUI_NoCrop,
		BackgroundColor = {Std = 0, Hover = RGB(0, 100, 0)},
		OnMouseIn = GuiAction_SetTag("Hover"),
		OnMouseOut = GuiAction_SetTag("Std"),
		icon = 
		{
			Style = GUI_NoCrop,
			Symbol = ScaffoldWall
		}
	};
	
	var menu_entry = 
	{
		Left = "50% - 6em", Right = "50% + 6em",
		Top = "0em", Bottom = "12em",
		
		center_icon = 
		{
			Left = "4em", Right = "8em", Top = "4em", Bottom = "8em",
			Margin = "1em",
			Symbol = GetID()
		},
		
		reinforce_top = 
		{
			Left = "4em", Right = "8em", Top = "0em", Bottom = "4em",
			Prototype = control_entry_prototype, 
			Tooltip = "$Reinforcement$: $Top$",
			icon =
			{
				Prototype = control_entry_prototype.icon,
				GraphicsName = "Top",
				Top = "3.5em", Bottom = "7.5em"
			},
			OnClick = GuiAction_Call(this, "Reinforce", 0)
		},
		reinforce_bottom = 
		{
			Left = "4em", Right = "8em", Top = "8em", Bottom = "12em",
			Prototype = control_entry_prototype, 
			Tooltip = "$Reinforcement$: $Bottom$",
			icon = {Prototype = control_entry_prototype.icon, GraphicsName = "Bottom"},
			OnClick = GuiAction_Call(this, "Reinforce", 1)
		},
		reinforce_left = 
		{
			Left = "0em", Right = "4em", Top = "4em", Bottom = "8em",
			Prototype = control_entry_prototype, 
			Tooltip = "$Reinforcement$: $Left$",
			icon = 
			{
				Prototype = control_entry_prototype.icon,
				GraphicsName = "Left",
				Left = "3.5em", Right = "7.5em",
			},
			OnClick = GuiAction_Call(this, "Reinforce", 2)
		},
		reinforce_right = 
		{
			Left = "8em", Right = "12em", Top = "4em", Bottom = "8em",
			Prototype = control_entry_prototype, 
			Tooltip = "$Reinforcement$: $Right$",
			icon = {Prototype = control_entry_prototype.icon, GraphicsName = "Right"},
			OnClick = GuiAction_Call(this, "Reinforce", 3)
		},
		extend_left = 
		{
			Left = "0em", Right = "4em", Top = "8em", Bottom = "12em",
			Prototype = control_entry_prototype, 
			Tooltip = "$Extension$: $Left$",
			icon =  { Prototype = control_entry_prototype.icon, GraphicsName = "Bottom" },
			OnClick = GuiAction_Call(this, "Reinforce", 4)
		},
		extend_right = 
		{
			Left = "8em", Right = "12em", Top = "8em", Bottom = "12em",
			Prototype = control_entry_prototype, 
			Tooltip = "$Extension$: $Right$",
			icon =  { Prototype = control_entry_prototype.icon, GraphicsName = "Bottom" },
			OnClick = GuiAction_Call(this, "Reinforce", 5)
		},
		extend_topleft = 
		{
			Left = "0em", Right = "4em", Top = "0em", Bottom = "4em",
			Prototype = control_entry_prototype, 
			Tooltip = "$Extension$: $Top$ $Left$",
			icon = 
			{
				Prototype = control_entry_prototype.icon,
				GraphicsName = "Left",
				Left = "3.5em", Right = "7.5em",
			},
			OnClick = GuiAction_Call(this, "Reinforce", 6)
		},
		extend_topright = 
		{
			Left = "8em", Right = "12em", Top = "0em", Bottom = "4em",
			Prototype = control_entry_prototype, 
			Tooltip = "$Extension$: $Top$ $Right$",
			icon = 
			{
				Prototype = control_entry_prototype.icon,
				GraphicsName = "Right"
			},
			OnClick = GuiAction_Call(this, "Reinforce", 7)
		},
	};
	
	// Replace arrows with real symbols if the wall is already built in that direction.
	var subentries = [menu_entry.reinforce_top, menu_entry.reinforce_bottom,
					menu_entry.reinforce_left, menu_entry.reinforce_right,
					menu_entry.extend_left, menu_entry.extend_right,
					menu_entry.extend_topleft, menu_entry.extend_topright];
	for (var direction = 0; direction < 8; ++ direction)
	{
		if (!walls[direction]) continue;
		var entry = subentries[direction];
		entry.Tooltip = "$TearDown$";
		entry.cancel = 
		{
			Margin = "1em",
			Symbol = Icon_Cancel,
		};
	}
	
	return [{symbol = GetID(), custom = menu_entry}];
}

public func GetInteractionMenus(object clonk)
{
	var menus = _inherited(clonk, ...) ?? [];		
	var menu =
	{
		title = "$ReinforceScaffold$",
		entries_callback = this.GetExtensionMenuEntries,
		//callback = "OnPumpControl",
		callback_hover = "OnExtensionMenuHover",
		callback_target = this,
		BackgroundColor = RGB(0, 50, 50),
		Priority = 20
	};
	PushBack(menus, menu);
	return menus;
}

public func OnExtensionMenuHover(id symbol, string action, desc_menu_target, menu_id)
{
	GuiUpdateText("$DescExtend$", menu_id, 1, desc_menu_target);
}


public func Reinforce(int direction)
{
	if (walls[direction])
	{
		walls[direction]->RemoveObject();
	}
	else
	{
		var new_wall = CreateObject(ScaffoldWall, 0, 0, GetOwner());
		walls[direction] = new_wall;
		
		var callbacks = ["SetTop", "SetBottom", "SetLeft", "SetRight", "SetLeftExtension", "SetRightExtension", "SetTopLeftExtension", "SetTopRightExtension"];
		new_wall->Call(callbacks[direction], this);
	}
	
	UpdateInteractionMenus(this.GetExtensionMenuEntries);
	Sound("Hits::Materials::Wood::DullWoodHit1");
}

/* Destruction */

local ActMap = {
		Default = {
			Prototype = Action,
			Name = "Default",
			Procedure = DFA_NONE,
			Directions = 1,
			Length = 1,
			Delay = 0,
			FacetBase = 1,
			NextAction = "Default",
		},
};

func Definition(def) {
	
}

local Name = "$Name$";
local Description = "$Description$";
local ContainBlast = true;
local BlastIncinerate = 100;
local HitPoints = 30;
local Plane = 120;
local Components = {Wood = 1};
