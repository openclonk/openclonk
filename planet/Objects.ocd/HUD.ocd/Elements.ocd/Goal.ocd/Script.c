/**
	HUD Goal
	Displays the current goal in the HUD.
	
	@authors Newton
*/

local goal;
local id_menu;
local prop_menu;

protected func Initialize()
{
	// Set parallaxity
	this.Parallaxity = [0, 0];
	// Set visibility
	this.Visibility = VIS_Owner;
	return;
}

public func SetGoal(object to_goal)
{
	goal = to_goal;
	Update();
	return;
}

public func Update()
{
	if (!goal)
		return;	
	// Display short description under the goal.
	var hudinfo = goal->~GetShortDescription(GetOwner());
	if (hudinfo)
		CustomMessage(Format("@%s",hudinfo), this, GetOwner(), 0, 90);
	else
		CustomMessage("", this, GetOwner(), 0, 90);
	// Set goal graphics to current goal.
	SetGraphics(goal->GetGraphics(), goal->GetID(), 1, GFXOV_MODE_IngamePicture);
	return;
}

/* Goal display using GUI menus */

public func MouseSelection(int plr)
{
	if (plr != GetOwner())
		return;
	if (!goal)
		return;
	// Open the goal menu if not already open.
	if (!prop_menu)
		OpenGoalWindow();
	return;
}

private func OpenGoalWindow()
{
	var goals = FindObjects(Find_Category(C4D_Goal));
	var nr_goals = GetLength(goals);
	var menu_width = BoundBy(nr_goals * 4, 20, 40); // in em
	
	// Safety: there has to be at least one goal.
	if (nr_goals <= 0)
		return;
	
	// Main menu
	prop_menu =
	{
		Target = this,
		Style = GUI_Multiple,
		Decoration = GUI_MenuDeco,
		Left = Format("50%%-%dem", menu_width),
		Right = Format("50%%+%dem", menu_width),
		Top = "50%-8em",
		Bottom = "50%+16em",
		BackgroundColor = {Std = 0},
	};
	
	// Close button
	GuiAddCloseButton(prop_menu, this, "OnCloseButtonClick");
	
	// Text submenu
	var prop_text =
	{
		Target = this,
		ID = 1,
		Left = "0%",
		Right = "100%",
		Top = "0%+8em",
		Bottom = "100%",
		Text = "",
		BackgroundColor = {Std = 0},	
	};
	prop_menu.TextMenu = prop_text;
	
	// Goal icons: maximum number of 10 goals for now
	var prop_goals = [];
	for (var i = 0; i < Min(10, nr_goals); i++)
	{
		var prop_goal = Format("GoalMenu%d", i + 2);
		prop_goals[i] = GoalSubMenu(goals[i], i);	
		prop_menu[prop_goal] = prop_goals[i];
	}
	
	// Select first goal and its description.
	OnGoalGUIHover(goals[0]);

	id_menu = GuiOpen(prop_menu);
}

private func CloseGoalWindow()
{
	GuiClose(id_menu, nil, this);
	id_menu = nil;
	prop_menu = nil;
}

private func GoalSubMenu(object g, int nr, int size)
{
	if (size == nil)
		size = 8;
	
	// Create the goal submenu with id counting upwards from 2.
	var prop_goal = 
	{
		Target = this,
		ID = nr + 2,
		Left = Format("0%%+%dem", nr * size),
		Right = Format("0%%+%dem", (nr + 1) * size),
		Top = "0%",
		Bottom = Format("0%%+%dem", size),
		Symbol = g->GetID(),
		BackgroundColor = {Std = 0, Hover = 0x50ffffff},
		OnMouseIn = [GuiAction_SetTag("Hover"), GuiAction_Call(this, "OnGoalGUIHover", g)],
		OnMouseOut = GuiAction_SetTag("Std"),
	};
	// Indicate whether the goal is already fulfilled with a star.
	var prop_star = 
	{
		Target = this,
		Left = "100%-2em",
		Right = "100%",
		Top = "0%",
		Bottom = "0%+2em",
		Symbol = Icon_Ok,
		BackgroundColor = {Std = 0},	
	};
	if (g->~IsFulfilled())
		prop_goal.Star = prop_star;
	
	return prop_goal;
}

public func OnCloseButtonClick()
{
	CloseGoalWindow();
}

public func OnGoalGUIHover(object g)
{
	// change text to the current goal.
	var prop_text = prop_menu.TextMenu;
	prop_text.Text = g->~GetDescription(GetOwner());
	var id_text = prop_text.ID;
	GuiUpdate(prop_text, id_menu, id_text, this);
}
