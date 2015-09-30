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

local goal_info_menu;
local goal_info_id;

/* Creation */

public func Construction()
{
	var plr = GetOwner();

	var y_begin = GUI_Controller_Goal_IconMargin;
	var y_end = y_begin + GUI_Controller_Goal_IconSize;
	// Also take into account the margin and size of the wealth HUD.
	var x_begin = y_end + GUI_Controller_Wealth_IconSize + GUI_Controller_Wealth_IconMargin;
	var x_end = y_begin + GUI_Controller_Wealth_IconSize + GUI_Controller_Wealth_IconMargin;

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

/* Callbacks */

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
	// Only update if something has changed.
	if (goal_gui_menu.Symbol != goal->GetID() || goal_gui_menu.GraphicsName != goal->GetGraphics() || goal_gui_menu.text.Text != goal->~GetShortDescription(GetOwner()))
	{
		goal_gui_menu.text.Text = goal->~GetShortDescription(GetOwner());
		goal_gui_menu.Symbol = goal->GetID();
		goal_gui_menu.GraphicsName = goal->GetGraphics();

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
	if (goal_info_menu)
		CloseGoalWindow();
	else
		OpenGoalWindow();
}

/*-- Goal Info Menu --*/

private func OpenGoalWindow()
{
	var goals = FindObjects(Find_Category(C4D_Goal));
	var nr_goals = GetLength(goals);
	var menu_width = BoundBy(nr_goals * 2, 10, 20); // in em

	// Safety: there has to be at least one goal.
	if (nr_goals <= 0)
		return;

	// Main menu
	goal_info_menu =
	{
		Target = this,
		Player = GetOwner(),
		Decoration = GUI_MenuDeco,
		Left = Format("50%%-%dem", menu_width),
		Right = Format("50%%+%dem", menu_width),
		Top = "50%-4em",
		Bottom = "50%+8em",
		BackgroundColor = {Std = 0},
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
		Text = "",
		BackgroundColor = {Std = 0},
	};

	// Goal icons: maximum number of 10 goals for now
	var prop_goals = [];
	for (var i = 0; i < Min(10, nr_goals); i++)
	{
		var prop_goal = Format("goal%d", i);
		prop_goals[i] = GoalSubMenu(goals[i], i);
		goal_info_menu[prop_goal] = prop_goals[i];
	}

	// Select first goal and its description.
	OnGoalGUIHover(goals[0]);

	goal_info_id = GuiOpen(goal_info_menu);
}

private func CloseGoalWindow()
{
	GuiClose(goal_info_id);
}

private func OnGoalWindowClosed()
{
	goal_info_id = nil;
	goal_info_menu = nil;
}

private func GoalSubMenu(object goal, int nr, int size)
{
	if (size == nil)
		size = 4;

	// Create the goal submenu with id counting upwards from 2.
	var prop_goal = 
	{
		Target = this,
		ID = nr + 2,
		Left = Format("0%%+%dem", nr * size),
		Right = Format("0%%+%dem", (nr + 1) * size),
		Top = "0%",
		Bottom = Format("0%%+%dem", size),
		Symbol = goal->GetID(),
		GraphicsName = goal->GetGraphics(),
		BackgroundColor = {Std = 0, Hover = 0x50ffffff},
		OnMouseIn = [GuiAction_SetTag("Hover"), GuiAction_Call(this, "OnGoalGUIHover", goal)],
		OnMouseOut = GuiAction_SetTag("Std"),
		goal_object = goal,
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
			BackgroundColor = {Std = 0},
		};
	}

	return prop_goal;
}

public func OnCloseButtonClick()
{
	CloseGoalWindow();
	return;
}

public func OnGoalGUIHover(object goal)
{
	if (!goal) return;
	// change text to the current goal.
	var prop_text = goal_info_menu.text;
	prop_text.Text = goal->~GetDescription(GetOwner());
	var id_text = prop_text.ID;
	GuiUpdate(prop_text, goal_info_id, id_text, this);
}

private func OnGoalWindowUpdate(object goal)
{
	if (!goal)
		return;
	if (!goal_info_menu)
		return;

	var index = 0, goal_menu;
	while ((goal_menu = goal_info_menu[Format("goal%d", index)]))
	{
		if (goal_menu.goal_object == goal)
			break;
		index++;
	}
	if (!goal_menu)
		return;

	var prop_goal = Format("goal%d", index);
	goal_info_menu[prop_goal] = GoalSubMenu(goal, index);
	GuiUpdate(goal_info_menu[prop_goal], goal_info_id, goal_info_menu[prop_goal].ID, goal_info_menu[prop_goal].Target);
}