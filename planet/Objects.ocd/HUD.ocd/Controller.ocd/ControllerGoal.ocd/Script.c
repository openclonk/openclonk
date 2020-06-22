/**
	ControllerGoal

	Shows the scenario goal in the top right corner, next to the wealth icon.
	Goal icon is clickable. On click displays a detailed goal description.

	@authors Maikel, Clonkonaut
*/

// HUD margin and size in tenths of em.
static const GUI_Controller_Goal_IconSize = 25;
static const GUI_Controller_Goal_IconMargin = 5;

// Does also use constants defined by ControllerWealth to ensure distance between both icons.

local goal_gui_menu;
local goal_gui_id;

local goal_info_id;
local goals;

/*-- Wealth Showing / Hiding --*/

public func ShiftGoal()
{
	if (goal_gui_id == nil) return;

	var offset = GUI_Controller_Wealth_IconSize + GUI_Controller_Wealth_IconMargin;
	var x_end = GUI_Controller_Goal_IconMargin + offset;
	var x_begin = GUI_Controller_Goal_IconMargin + GUI_Controller_Goal_IconSize + offset;

	var update = {
		Left = Format("100%%%s", ToEmString(-x_begin)),
		Right = Format("100%%%s", ToEmString(-x_end)),
	};

	GuiUpdate(update, goal_gui_id);
}

public func UnshiftGoal()
{
	if (goal_gui_id == nil) return;

	var x_end = GUI_Controller_Goal_IconMargin;
	var x_begin = GUI_Controller_Goal_IconMargin + GUI_Controller_Goal_IconSize;

	var update = {
		Left = Format("100%%%s", ToEmString(-x_begin)),
		Right = Format("100%%%s", ToEmString(-x_end)),
	};

	GuiUpdate(update, goal_gui_id);
}

/*-- Creation --*/

public func Construction()
{
	var y_begin = GUI_Controller_Goal_IconMargin;
	var y_end = y_begin + GUI_Controller_Goal_IconSize;
	// Also take into account the margin and size of the wealth HUD if shown
	var x_begin = y_end;
	var x_end = y_begin;
	if (this->~IsShowingWealth())
	{
		x_begin += GUI_Controller_Wealth_IconSize + GUI_Controller_Wealth_IconMargin;
		x_end += GUI_Controller_Wealth_IconSize + GUI_Controller_Wealth_IconMargin;
	}
	// See also ShiftGoal() / UnshiftGoal()

	goal_gui_menu = 
	{
		Target = this,
		Player = NO_OWNER, // The goal icon will become visible if OnGoalUpdate is called
		Style = GUI_Multiple | GUI_IgnoreMouse,
		Left = Format("100%%%s", ToEmString(-x_begin)),
		Right = Format("100%%%s", ToEmString(-x_end)),
		Top = ToEmString(y_begin),
		Bottom = ToEmString(y_end),
		Priority = 1,
		text = 
		{
			Style = GUI_TextHCenter | GUI_TextBottom,
			Text = nil,
			Priority = 3,
		},
	};
	goal_gui_id = GuiOpen(goal_gui_menu);

	return _inherited(...);
}

private func Destruction()
{
	GuiClose(goal_gui_id);
	if (goal_info_id) GuiClose(goal_info_id);

	_inherited(...);
}

/*-- Callbacks --*/

// Callback from the goal library: display this goal.
public func OnGoalUpdate(object goal)
{
	// Also notify the open info menu.
	OnGoalWindowUpdate(goal);
	// If there is no goal hide the menu
	if (!goal)
	{
		GuiUpdate({ Player = NO_OWNER }, goal_gui_id);

		return _inherited(goal, ...);
	}

	// Get current goal display settings
	var symbol = GetGoalSymbol(goal);
	var graphics = GetGoalGraphicsName(goal);
	var text = goal->~GetShortDescription(GetOwner());

	// Determine changes
	var update_symbol = goal_gui_menu.Symbol != symbol;
	var update_graphics = goal_gui_menu.GraphicsName != graphics;
	var update_text = goal_gui_menu.text.Text != text;

	// Only update if something has changed.
	if (update_symbol || update_graphics || update_text)
	{
		goal_gui_menu.text.Text = text;
		goal_gui_menu.Symbol = symbol;
		goal_gui_menu.GraphicsName = graphics;

		goal_gui_menu.Player = GetOwner();
		goal_gui_menu.Style = GUI_Multiple;
		// Also add an hover and mouse click element.
		goal_gui_menu.hover = 
		{
			Symbol = { Std = nil, OnHover = GUI_Controller_Goal},
			OnClick = GuiAction_Call(this, "OnGoalClick"),
			OnMouseIn = GuiAction_SetTag("OnHover"),
			OnMouseOut = GuiAction_SetTag("Std"),
			Priority = 2,
		};
		GuiUpdate(goal_gui_menu, goal_gui_id);
	}

	return _inherited(goal, ...);
}

private func OnGoalClick()
{
	if (goal_info_id)
		CloseGoalWindow();
	else
		OpenGoalWindow();
}

/*-- Goal Info Menu --*/

private func OpenGoalWindow()
{
	goals = FindObjects(Find_Category(C4D_Goal));
	var nr_goals = GetLength(goals);
	var menu_width = BoundBy(nr_goals * 2, 10, 20); // in em

	// Safety: there has to be at least one goal.
	if (nr_goals <= 0)
		return;

	// Main menu
	var goal_info_menu =
	{
		Target = this,
		Player = GetOwner(),
		Decoration = GUI_MenuDeco,
		Left = Format("50%%-%dem", menu_width),
		Right = Format("50%%+%dem", menu_width),
		Top = "50%-4em",
		Bottom = "50%+8em",
		OnClose = GuiAction_Call(this, "OnGoalWindowClosed"),
	};

	// Close button
	GuiAddCloseButton(goal_info_menu, this, "OnCloseButtonClick");

	// Text submenu
	goal_info_menu.text =
	{
		Target = this,
		ID = 1,
		Left = "0%",
		Right = "100%",
		Top = "0%+4em",
		Bottom = "100%",
	};

	// Goal icons: maximum number of 10 goals for now
	for (var i = 0; i < Min(10, nr_goals); i++)
	{
		var menu = GoalSubMenu(goals[i], i);
		GuiAddSubwindow(menu, goal_info_menu);
	}
	
	goal_info_id = GuiOpen(goal_info_menu);
	
	// Select first goal and show its description.
	OnGoalGUIHover(goals[0]);
}

private func CloseGoalWindow()
{
	GuiClose(goal_info_id);
}

private func OnGoalWindowClosed()
{
	goal_info_id = nil;
}

private func GoalSubMenu(object goal, int nr, int size)
{
	if (size == nil)
		size = 4;

	var symbol = GetGoalSymbol(goal);
	var graphics = GetGoalGraphicsName(goal);
	// Create the goal submenu with id counting upwards from 2.
	var prop_goal = 
	{
		Target = this,
		ID = nr + 2,
		Left = Format("0%%+%dem", nr * size),
		Right = Format("0%%+%dem", (nr + 1) * size),
		Top = "0%",
		Bottom = Format("0%%+%dem", size),
		Symbol = symbol,
		GraphicsName = graphics,
		BackgroundColor = {Std = 0, Hover = 0x50ffffff},
		OnMouseIn = [GuiAction_SetTag("Hover"), GuiAction_Call(this, "OnGoalGUIHover", goal)],
		OnMouseOut = GuiAction_SetTag("Std"),
		fulfilled = nil
	};
	// Indicate whether the goal is already fulfilled
	if (goal->~IsFulfilled())
	{
		prop_goal.fulfilled = 
		{
			Target = this,
			Left = "100%-1em",
			Right = "100%",
			Top = "0%",
			Bottom = "0%+1em",
			Symbol = Icon_Ok,
		};
	}

	return prop_goal;
}

public func OnCloseButtonClick()
{
	CloseGoalWindow();
}

public func OnGoalGUIHover(object goal)
{
	if (!goal)
		return;
	// Change text to the current goal.
	var text = Format("<c ff0000>%s:</c> %s", goal->GetName(), goal->~GetDescription(GetOwner()));
	GuiUpdateText(text, goal_info_id, 1, this);
}

private func OnGoalWindowUpdate(object goal)
{
	if (!goal || !goal_info_id)
		return;

	var index = GetIndexOf(goals, goal);
	if (index == -1) return;

	var menu = GoalSubMenu(goal, index);
	// Update only very selectively. (To e.g. not reset the background/tag)
	var update = 
	{
		Symbol = menu.Symbol,
		GraphicsName = menu.GraphicsName,
		fulfilled = menu.fulfilled,
	};
	GuiUpdate(update, goal_info_id, menu.ID, menu.Target);
}

private func GetGoalSymbol(object goal)
{
	return goal->~GetPictureDefinition(GetOwner()) ?? goal->GetID();
}

private func GetGoalGraphicsName(object goal)
{
	return goal->~GetPictureName(GetOwner()) ?? goal->GetGraphics(GetOwner());
}
