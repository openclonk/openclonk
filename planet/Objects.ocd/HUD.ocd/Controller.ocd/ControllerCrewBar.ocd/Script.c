/**
	Controller Crew Bar

	Displays crew members in the top left corner.

	@authors Maikel, Clonkonaut
*/

/*
	crew_bars contains an array of proplists with the following attributes:
		ID: submenu ID. Unique in combination with the target == this
		shown: If the bar is currently shown

		Default bars are:
		0 - Health
		1 - Magic
		2 - Breath
*/

/*
	crew_warnings contains an array of integers, for proper positioning of crew warnings
	See IssueWarning()
*/

/*
	crew_displays contains an array of proplists with the following attributes:
		ID: submenu ID. Unique in combination with the target == this
		shown: If the menu is currently shown
	#### currently not in use
*/

// HUD margin and size in tenths of em.
static const GUI_Controller_CrewBar_IconSize = 30; // Used by the next clonk display + 1em for the bars (health, breath)
static const GUI_Controller_CrewBar_IconMargin = 6;
static const GUI_Controller_CrewBar_CursorSize = 40; // Cursor size is bigger
static const GUI_Controller_CrewBar_CursorMargin = 3;
static const GUI_Controller_CrewBar_BarSize = 3;
static const GUI_Controller_CrewBar_BarMargin = 3;

local crew_bars;
local crew_warnings;
//local crew_displays;

// To not have magic numbers
local crew_health_bar;
local crew_magic_bar;
local crew_breath_bar;
local crew_cursor_id;
local crew_next_id;
local crew_warning_id;

local crew_gui_menu;
local crew_gui_id;

// Shown when there are more than 3 crew members, clickable
local crew_plus_menu;
local crew_plus_id;

// Shown when the plus icon was clicked
local crew_info_menu;
local crew_info_id;

/* GUI creation */

// For custom HUD graphics overload the following function as deemed fit.

func AssembleCrewBar()
{
	var cursor_margin = GUI_Controller_CrewBar_CursorMargin;
	var cursor_size = cursor_margin + GUI_Controller_CrewBar_CursorSize;

	var next_margin = cursor_size + GUI_Controller_CrewBar_IconMargin;
	var next_size = next_margin + GUI_Controller_CrewBar_IconSize;
	var next_y_margin = GUI_Controller_CrewBar_IconMargin;
	var next_y_size = next_y_margin + GUI_Controller_CrewBar_IconSize;

	crew_cursor_id = 1;
	crew_next_id = 10;
	crew_warning_id = 20;

	// --- WARNING ---
	// Crew plus menu and crew info menu will probably be removed!
	// Hereby marked deprecated!

	crew_plus_menu =
	{
		Target = this,
		Player = GetOwner(),
		Style = GUI_NoCrop | GUI_Multiple,
		Left = ToEmString(next_margin + GUI_Controller_CrewBar_IconSize / 2),
		Right = ToEmString(next_margin + GUI_Controller_CrewBar_IconSize * 3 / 2),
		Top = ToEmString(next_y_margin),
		Bottom = ToEmString(next_y_size - GUI_Controller_CrewBar_IconSize / 2),
		Symbol = Icon_Number,
		GraphicsName = "PlusGreen",
		Priority = 10,
		OnClick = GuiAction_Call(this, "OpenCrewInfo")
	};

	crew_info_menu =
	{
		Target = this,
		Style = GUI_NoCrop | GUI_Multiple | GUI_FitChildren,
		Left = ToEmString(next_margin),
		Right = ToEmString(next_margin + GUI_Controller_CrewBar_IconSize),
		Top = ToEmString(next_y_size + GUI_Controller_CrewBar_IconMargin*2),
		Decoration = GUI_MenuDecoInventoryHeader,
		Priority = 11
	};

	// --- SEE WARNING ABOVE ---

	return
	{
		Target = this,
		ID = 1,
		Style = GUI_Multiple | GUI_NoCrop | GUI_IgnoreMouse,
		cursor =
		{
			Target = this,
			Player = NO_OWNER, // will be shown once a crew member is recruited
			ID = crew_cursor_id,
			Style = GUI_NoCrop,
			Left = ToEmString(cursor_margin),
			Right = ToEmString(cursor_size),
			Top = ToEmString(cursor_margin),
			Bottom = Format("100%%%s", ToEmString(-cursor_margin)),
			portrait =
			{
				Target = this,
				Style = GUI_NoCrop,
				Left = "0%",
				Right = "100%",
				Top = "0%",
				Bottom = ToEmString(cursor_size),
				frame =
				{
					Target = this,
					Symbol = GUI_Controller_CrewBar,
					GraphicsName = "Selected",
					Priority = 2
				},
				picture = // Also the name
				{
					Target = this,
					Style = GUI_TextHCenter | GUI_TextBottom,
					Priority = 3
				},
				rank =
				{
					Target = this,
					Style = GUI_NoCrop,
					Right = ToEmString(cursor_size / 6),
					Bottom = ToEmString(cursor_size / 6),
					Symbol = Icon_Rank,
					Priority = 5,
					grade = {
						Target = this,
						Left = "-20%",
						Right = "80%",
						Top = "-20%",
						Bottom = "80%",
						Symbol = Icon_Rank,
						Priority = 6
					}
				},
				heart =
				{
					Target = this,
					Player = NO_OWNER,
					ID = crew_cursor_id + 1,
					Margin = ["5%"],
					Symbol = CrewHealIcon().Symbol,
					GraphicsName = CrewHealIcon().GraphicsName,
					Priority = 7
				}
			}
		},
		next_clonk =
		{
			Target = this,
			Player = NO_OWNER, // will be shown once at least two crew members are recruited
			ID = crew_next_id,
			Style = GUI_NoCrop,
			Left = ToEmString(next_margin),
			Right = ToEmString(next_size),
			Top = ToEmString(next_y_margin),
			Bottom = ToEmString(next_y_size),
			portrait =
			{
				Target = this,
				Style = GUI_NoCrop,
				frame =
				{
					Target = this,
					Symbol = GUI_Controller_CrewBar,
					Priority = 2
				},
				picture = // Also the name
				{
					Target = this,
					Style = GUI_TextHCenter | GUI_TextBottom,
					Priority = 3
				},
				rank =
				{
					Target = this,
					Style = GUI_NoCrop,
					Right = ToEmString(GUI_Controller_CrewBar_IconSize / 6),
					Bottom = ToEmString(GUI_Controller_CrewBar_IconSize / 6),
					Symbol = Icon_Rank,
					Priority = 5,
					grade = {
						Target = this,
						ID = 7,
						Left = "-20%",
						Right = "80%",
						Top = "-20%",
						Bottom = "80%",
						Symbol = Icon_Rank,
						Priority = 6
					}
				},
				heart =
				{
					Target = this,
					Player = NO_OWNER,
					ID = crew_next_id + 1,
					Margin = ["5%"],
					Symbol = CrewHealIcon().Symbol,
					GraphicsName = CrewHealIcon().GraphicsName,
					Priority = 7
				}
			},
			health =
			{
				Target = this,
				ID = crew_next_id + 2,
				Style = GUI_NoCrop,
				Top = "100%+0.2em",
				Bottom = "100%+0.45em",
				BackgroundColor = RGB(40, 40, 40),
				Priority = 7,
				fill =
				{
					Target = this,
					Style = GUI_NoCrop,
					Margin = ["0em", "0.1em"],
					BackgroundColor = RGB(160, 0, 0),
					Priority = 8
				},
				text =
				{
					Target = this,
					Top = "-0.5em",
					Bottom = "0.5em",
					Style = GUI_TextHCenter | GUI_TextVCenter,
					Text = "",
					Priority = 9
				}
			},
			breath =
			{
				Target = this,
				Player = NO_OWNER,
				ID = crew_next_id + 3,
				Top = "100%+0.6em",
				Bottom = "100%+0.85em",
				BackgroundColor = RGB(40, 40, 40),
				Priority = 7,
				fill =
				{
					Target = this,
					Style = GUI_NoCrop,
					Margin = ["0em", "0.1em"],
					BackgroundColor = RGB(0, 160, 160),
					Priority = 8
				}
			}
		},
		warning =// Displays warning signals for additional crew members
		{
			Target = this,
			Player = GetOwner(), // content will be added whenever something happens to a crew members not displayed
			ID = crew_warning_id,
			Style = GUI_NoCrop,
			Left = ToEmString(next_margin),
			Right = ToEmString(next_size),
			Top = ToEmString(next_y_size + GUI_Controller_CrewBar_IconMargin*2),
			Bottom = ToEmString(next_y_size + GUI_Controller_CrewBar_IconMargin*2 + 20),
			Priority = 10
		}
	};
}

func AddCrewBars()
{
	// Health bar
	crew_health_bar = AddCrewBar(RGB(160, 0, 0));
	// Magic bar
	crew_magic_bar = AddCrewBar(RGB(75, 75, 160));
	// Breath bar
	crew_breath_bar = AddCrewBar(RGB(0, 160, 160));
}

func CrewRecruitmentIcon()
{
	return Icon_Arrow;
}

func CrewDeathIcon()
{
	return Icon_Skull;
}

func CrewHealIcon()
{
	return { Symbol = Icon_Heart, GraphicsName = "" };
}

func CrewFireDamageIcon()
{
	return { Symbol = Icon_Heart, GraphicsName = "OnFire" };
}

func CrewDamageIcon()
{
	return { Symbol = Icon_Heart, GraphicsName = "Broken" };
}

func CrewBreathIcon()
{
	return Icon_Bubbles;
}

/* Creation / Destruction */

private func Construction()
{
	crew_bars = [];
	crew_warnings = [];
//	crew_displays = [];

	crew_gui_menu = AssembleCrewBar();

	crew_gui_id = GuiOpen(crew_gui_menu);

	AddCrewBars();

	return _inherited(...);
}

private func Destruction()
{
	GuiClose(crew_gui_id);
	crew_gui_id = nil;
	crew_bars = [];
	if (crew_plus_id)
	{
		GuiClose(crew_plus_id);
		crew_plus_id = nil;
	}
	CloseCrewInfo();

	_inherited(...);
}

/* Callbacks */

public func OnCrewRecruitment(object clonk, int plr)
{
	UpdateCrewDisplay();
	IssueWarning(clonk, Icon_Arrow, "Right");

	return _inherited(clonk, plr, ...);
}

public func OnCrewDeRecruitment(object clonk, int plr)
{
	UpdateCrewDisplay();

	return _inherited(clonk, plr, ...);
}

public func OnCrewDeath(object clonk, int killer)
{
	UpdateCrewDisplay();

	var next_index = GetNextCrewIndex(GetCursorIndex());
	if (GetCursor(GetOwner()) != clonk && GetCrew(GetOwner(), next_index) != clonk)
		IssueWarning(this, CrewDeathIcon(), "", clonk->GetName()); // this for target because Clonk might get deleted

	return _inherited(clonk, killer, ...);
}

public func OnCrewDestruction(object clonk)
{
	UpdateCrewDisplay();

	return _inherited(clonk, ...);
}

public func OnCrewDisabled(object clonk)
{
	UpdateCrewDisplay();

	return _inherited(clonk, ...);
}

public func OnCrewEnabled(object clonk)
{
	UpdateCrewDisplay();

	return _inherited(clonk, ...);
}

public func OnCrewSelection(object clonk, bool unselect)
{
	UpdateCrewDisplay();

	return _inherited(clonk, unselect, ...);
}

public func OnCrewNameChange(object clonk)
{
	UpdateCrewDisplay();

	return _inherited(clonk, ...);
}

public func OnCrewRankChange(object clonk)
{
	UpdateCrewDisplay();

	return _inherited(clonk, ...);
}

public func OnCrewHealthChange(object clonk, int change, int cause, int caused_by)
{
	var health_phys = clonk->~GetMaxEnergy();
	if (health_phys && change != 0) // no false positives where change is zero
	{
		var health_val = clonk->GetEnergy();
		if (GetCursor(GetOwner()) == clonk)
		{
			// Show current health
			SetCrewBarValue(crew_health_bar, 1000 * health_val / health_phys, clonk->GetEnergy());
			// Show heart / broken heart
			var graphics = "";
			if (change < 0)
				graphics = "Broken";
			if (clonk->OnFire())
				graphics = "OnFire";
			Heartbeat(graphics, crew_cursor_id + 1);
		}
		else
		{
			// Clonk is next in line, show health changes
			var next_index = GetNextCrewIndex(GetCursorIndex());
			if (GetCrew(GetOwner(), next_index) == clonk)
			{
				SetNextHealthValue(1000 * health_val / health_phys, clonk->GetEnergy());
				var graphics = "";
				if (change < 0)
					graphics = "Broken";
				if (clonk->OnFire())
					graphics = "OnFire";
				Heartbeat(graphics, crew_next_id + 1);
			}
			else // Show a warning
			{
				var graphics = CrewHealIcon();
				if (change < 0)
					graphics = CrewDamageIcon();
				if (clonk->OnFire())
					graphics = CrewFireDamageIcon();
				IssueWarning(clonk, graphics.Symbol, graphics.GraphicsName);
			}
		}
	}

	return _inherited(clonk, change, cause, caused_by, ...);
}

public func OnCrewBreathChange(object clonk, int change)
{
	var breath_phys = clonk->~GetMaxBreath();
	if (breath_phys)
	{
		var breath_val = clonk->GetBreath();
		if (GetCursor(GetOwner()) == clonk)
		{
			// Hide bar if full breath
			if (breath_val == breath_phys)
				HideCrewBar(crew_breath_bar);
			// Else show bar!
			else
			{
				ShowCrewBar(crew_breath_bar);
				SetCrewBarValue(crew_breath_bar, 1000 * breath_val / breath_phys);
			}
		}
		else
		{
			// Clonk is next in line, show breath changes
			var next_index = GetNextCrewIndex(GetCursorIndex());
			if (GetCrew(GetOwner(), next_index) == clonk)
			{
				SetNextBreathValue(1000 * breath_val / breath_phys);
			}
			else if (change < 0 && breath_val < breath_phys) // Show a warning if decreasing
			{
				IssueWarning(clonk, Icon_Bubbles, "");
			}
		}
	}

	return _inherited(clonk, change, ...);
}

public func OnCrewMagicChange(object clonk, int change)
{
	var magic_phys = clonk->~GetMaxMagicEnergy();
	if (magic_phys)
	{
		var magic_val = clonk->GetMagicEnergy();
		if (GetCursor(GetOwner()) == clonk) // Only displayed for cursor
		{
			// Hide bar if no magic
			if (magic_val == 0)
				HideCrewBar(crew_magic_bar);
			// Else show bar!
			else
			{
				ShowCrewBar(crew_magic_bar);
				SetCrewBarValue(crew_magic_bar, 1000 * magic_val / magic_phys);
			}
		}
	}

	return _inherited(clonk, change, ...);
}

/* Display */

// Add a new crew member to the mini displays
// Unused for now, but we might want this in the future

/*
private func AddCrewDisplay(object clonk)
{
	var crew_index = 0;
	while (GetCrew(GetOwner(), crew_index) != clonk && crew_index < GetCrewCount(GetOwner()))
		crew_index++;
	// Something went wrong
	if (GetCrew(GetOwner(), crew_index) != clonk) return;

	var crew_margin = GUI_Controller_CrewBar_CursorMargin + GUI_Controller_CrewBar_CursorSize + GUI_Controller_CrewBar_IconMargin;
	var crew_top = GUI_Controller_CrewBar_IconMargin*2 + GUI_Controller_CrewBar_IconSize + 5;

	var new_display =
	{
		ID = 1000 + crew_index,
		shown = false
	};
	crew_displays[crew_index] = new_display;

	var health_phys = clonk->~GetMaxEnergy();
	var health_val = 0;
	if (health_phys)
	{
		health_val = 1000 * clonk->GetEnergy() / health_phys;
	}

	crew_gui_menu[Format("crew%d", crew_index)] =
	{
		Target = this,
		Player = NO_OWNER,
		ID = new_display.ID,
		Style = GUI_NoCrop,
		Left = ToEmString(crew_margin),
		Top = ToEmString(crew_top),
		Bottom = ToEmString(crew_top + 10),
		health =
		{
			Target = this,
			Style = GUI_NoCrop,
			Right = ToEmString(GUI_Controller_CrewBar_IconSize),
			Top = "0.2em",
			Bottom = "0.45em",
			BackgroundColor = RGB(40, 40, 40),
			Priority = 7,
			fill =
			{
				Target = this,
				Style = GUI_NoCrop,
				Right = ToPercentString(health_val),
				BackgroundColor = RGB(160, 0, 0),
				Priority = 1
			},
			text =
			{
				Target = this,
				Style = GUI_TextHCenter,
				Text = Format("<c dddd00>%d</c>", clonk->GetEnergy()),
				Priority = 2
			}
		},
		name =
		{
			Target = this,
			Left = ToEmString(GUI_Controller_CrewBar_IconSize),
			Style = GUI_TextLeft,
			Text = clonk->GetName()
		}
	};
	GuiUpdate(crew_gui_menu, crew_gui_id);
	UpdateCrewDisplay();
}*/

// Update everything
private func UpdateCrewDisplay()
{
	var cursor = GetCursor(GetOwner());
	var cursor_index = GetCursorIndex();

	// No cursor: Don't display cursor information
	if (!cursor || !cursor->GetCrewEnabled())
	{
		if (crew_gui_menu.cursor.Player != NO_OWNER)
		{
			crew_gui_menu.cursor.Player = NO_OWNER;
			GuiUpdate(crew_gui_menu.cursor, crew_gui_id, 1, this);
		}
	}
	else
	{
		// Update cursor information
		var update =
		{
			Player = GetOwner(),
			portrait =
			{
				picture =
				{
					Symbol = cursor,
					Text = cursor->GetName()
				},
				rank =
				{
					GraphicsName = Format("%d", cursor->~GetRank() % 24),
					grade =
					{
						GraphicsName = Format("Upgrade%d", cursor->~GetRank() / 24)
					}
				}
			}
		};
		GuiUpdate(update, crew_gui_id, crew_cursor_id, this);

		// Health
		var health_phys = cursor->~GetMaxEnergy();

		if (health_phys)
		{
			var health_val = cursor->GetEnergy();
			ShowCrewBar(crew_health_bar);
			SetCrewBarValue(crew_health_bar, 1000 * health_val / health_phys, cursor->GetEnergy());
		}
		else
			HideCrewBar(crew_health_bar);

		// Magic
		var magic_phys = cursor->~GetMaxMagicEnergy();
		var magic_val = cursor->~GetMagicEnergy();

		if (magic_phys && magic_val > 0)
		{
			ShowCrewBar(crew_magic_bar);
			SetCrewBarValue(crew_magic_bar, 1000 * magic_val / magic_phys);
		}
		else
			HideCrewBar(crew_magic_bar);

		// Breath
		var breath_phys = cursor->~GetMaxBreath();
		var breath_val = cursor->~GetBreath();

		if (breath_phys && breath_val < breath_phys)
		{
			ShowCrewBar(crew_breath_bar);
			SetCrewBarValue(crew_breath_bar, 1000 * breath_val / breath_phys);
		}
		else
			HideCrewBar(crew_breath_bar);
	}

	// Display next crew member
	var next_index = GetNextCrewIndex(cursor_index);
	if (next_index != cursor_index)
	{
		crew_gui_menu.next_clonk.Player = GetOwner();
		var next_clonk = GetCrew(GetOwner(), next_index);

		var health_visible = NO_OWNER;
		var health_phys = next_clonk->~GetMaxEnergy();
		var health_val = 0;
		if (health_phys)
		{
			health_visible = GetOwner();
			health_val = 1000 * next_clonk->GetEnergy() / health_phys;
		}

		var breath_visible = NO_OWNER;
		var breath_phys = next_clonk->~GetMaxBreath();
		var breath_val = 1000;
		if (breath_phys && breath_val < breath_phys)
		{
			breath_visible = GetOwner();
			breath_val = 1000 * next_clonk->GetBreath() / breath_phys;
		}

		var update =
		{
			Player = GetOwner(),
			portrait =
			{
				picture =
				{
					Symbol = next_clonk,
					Text = next_clonk->GetName()
				},
				rank =
				{
					GraphicsName = Format("%d", next_clonk->~GetRank() % 24),
					grade =
					{
						GraphicsName = Format("Upgrade%d", next_clonk->~GetRank() / 24)
					}
				}
			},
			health =
			{
				Player = health_visible,
				fill =
				{
					Right = ToPercentString(health_val)
				},
				text =
				{
					Text = Format("<c dddd00>%d</c>", next_clonk->GetEnergy())
				}
			},
			breath =
			{
				Player = breath_visible,
				fill =
				{
					Right = ToPercentString(breath_val)
				}
			}
		};
		GuiUpdate(update, crew_gui_id, crew_next_id, this);
	}
	else
	{
		// Hide second clonk display
		if (crew_gui_menu.next_clonk.Player != NO_OWNER)
		{
			crew_gui_menu.next_clonk.Player = NO_OWNER;
			GuiUpdate(crew_gui_menu.next_clonk, crew_gui_id, crew_next_id, this);
		}
	}

	// More crew members? Show plus sign
	if (GetCrewCount(GetOwner()) > 2)
	{
		if (!crew_plus_id)
			crew_plus_id = GuiOpen(crew_plus_menu);
	}
	else if (crew_plus_id)
	{
		GuiClose(crew_plus_id);
		crew_plus_id = nil;
	}

	// Only to be used when AddCrewDisplay gets reactivated
/*
	// Hide all mini crew displays
	for (var i = 0; i < GetLength(crew_displays); i++)
	{
		if (!crew_displays[i]) continue;
		GuiUpdate({ Player = NO_OWNER }, crew_gui_id, crew_displays[i].ID, this);
		crew_displays[i].shown = false;
	}

	if (!cursor || cursor_index == nil) return;
	// Display all crew members not yet shown
	var top = GUI_Controller_CrewBar_IconMargin*2 + GUI_Controller_CrewBar_IconSize + 5;
	while (GetNextCrewIndex(next_index) != cursor_index)
	{
		next_index = GetNextCrewIndex(next_index);
		if (!crew_displays[next_index]) continue; // Shouldn't happen
		var update =
		{
			Player = GetOwner(),
			Top = ToEmString(top)
		};
		GuiUpdate(update, crew_gui_id, crew_displays[next_index].ID, this);
		crew_displays[next_index].shown = true;
		top += 12;
	}*/
}

/* Bars (health, breath, ...) */

// Adds a new bar to the portrait.
// The bar is not shown until ShowCrewBar() is called. Bars will appear in order of creation.
private func AddCrewBar(int foreground, int background)
{
	if (!crew_gui_id) return;
	if (!foreground) return;
	if (!background) background = RGB(40, 40, 40);

	var y_begin = GUI_Controller_CrewBar_CursorMargin + GUI_Controller_CrewBar_CursorSize;
	var y_end = y_begin + GUI_Controller_CrewBar_BarSize;

	var new_bar = 
	{
		ID = 100 + GetLength(crew_bars),
		shown = false
	};
	PushBack(crew_bars, new_bar);

	// Will display the bar at the left screen border
	// Unwanted for now
/*
	var y_begin = GUI_Controller_CrewBar_CursorMargin + GUI_Controller_CrewBar_CursorSize;
	var y_end = GUI_Controller_CrewBar_CursorMargin;
	crew_gui_menu.cursor[Format("bar%d", new_bar.ID)] =
	{
		Target = this,
		Player = NO_OWNER,
		ID = new_bar.ID,
		Left = "0%",
		Right = Format("0%%%s", ToEmString(GUI_Controller_CrewBar_BarSize)),
		Top = Format("0%%%s", ToEmString(y_begin)),
		Bottom = Format("100%%%s", ToEmString(-y_end)),
		BackgroundColor = background,
		Priority = 3,
		fill =
		{
			Target = this,
			Margin = "0.1em",
			BackgroundColor = foreground,
			Priority = 4
		}
	};*/

	var crew_gui_bar_name = Format("bar%d", new_bar.ID);
	crew_gui_menu.cursor[crew_gui_bar_name] =
	{
		Target = this,
		Player = NO_OWNER,
		Style = GUI_NoCrop,
		ID = new_bar.ID,
		Left = "0%",
		Right = Format("0%%%s", ToEmString(GUI_Controller_CrewBar_CursorSize)),
		Top = ToEmString(y_begin),
		Bottom = ToEmString(y_end),
		BackgroundColor = background,
		Priority = 3,
		fill =
		{
			Target = this,
			Left = "0%",
			Margin = ["0em", "0.1em"],
			BackgroundColor = foreground,
			Priority = 4
		},
		text =
		{
			Target = this,
			Top = "-0.5em",
			Bottom = "0.5em",
			Style = GUI_TextHCenter | GUI_TextVCenter,
			Text = "",
			Priority = 5
		}
	};

	GuiUpdate(crew_gui_menu, crew_gui_id);
	
	// Prevent further calls to GuiUpdate on the main panel from overwriting values in the bars with their defaults
	crew_gui_menu.cursor[crew_gui_bar_name] = nil;
	
	UpdateCrewDisplay();

	return GetLength(crew_bars)-1;
}

// Shows the bar that was saved in crew_bars[bar]
private func ShowCrewBar(int bar)
{
	if (!crew_bars[bar]) return;
	if (crew_bars[bar].shown) return;
	if (GetOwner() == NO_OWNER) return;

	// Bars at left side of the screen
/*	var left = GUI_Controller_CrewBar_BarMargin;
	var i = 0;
	while (i < bar)
	{
		if (crew_bars[i] && crew_bars[i].shown)
			left += GUI_Controller_CrewBar_BarSize + GUI_Controller_CrewBar_BarMargin;
		i++;
	}
	var right = ToEmString(left + GUI_Controller_CrewBar_BarSize);
	left = ToEmString(left);

	GuiUpdate({ Player = GetOwner(), Left = left, Right = right }, crew_gui_id, crew_bars[bar].ID, this);*/

	var top = GUI_Controller_CrewBar_CursorMargin + GUI_Controller_CrewBar_CursorSize;
	var i = 0;
	while (i < bar)
	{
		if (crew_bars[i] && crew_bars[i].shown)
			top += GUI_Controller_CrewBar_BarSize + GUI_Controller_CrewBar_BarMargin;
		i++;
	}
	var bottom = ToEmString(top + GUI_Controller_CrewBar_BarSize);
	top = ToEmString(top);

	crew_bars[bar].shown = GuiUpdate({ Player = GetOwner(), Top = top, Bottom = bottom }, crew_gui_id, crew_bars[bar].ID, this);
}

// Sets the fill status of the bar. value is between 0 and 1000
// Shows text_val before the bar if given
private func SetCrewBarValue(int bar, int value, int text_val)
{
	if (!crew_bars[bar]) return;
	value = BoundBy(value, 0, 1000);
	var plr = GetOwner();
	var bar_text = "";
	if (text_val) bar_text = Format("<c dddd00>%d</c>", text_val);
	// Displaying the fill with Top = 100% creates an unwanted scrollbar
	//if (value == 0) plr = NO_OWNER;

	GuiUpdate({ fill = { Player = plr, Right = ToPercentString(value) }, text = { Text = bar_text } }, crew_gui_id, crew_bars[bar].ID, this);
}

// Hides the bar that was saved in crew_bars[bar]
private func HideCrewBar(int bar)
{
	if (!crew_bars[bar]) return;
	if (!crew_bars[bar].shown) return;

	GuiUpdate({ Player = NO_OWNER }, crew_gui_id, crew_bars[bar].ID, this);
	crew_bars[bar].shown = false;

	// Update position of all following bars
	for (var i = bar; i < GetLength(crew_bars); i++)
		if (crew_bars[i].shown)
		{
			crew_bars[i].shown = false;
			ShowCrewBar(i);
		}
}

/* Crew indices */

// Returns the crew index of the cursor
private func GetCursorIndex()
{
	var cursor = GetCursor(GetOwner());
	if (!cursor) return;
	var cursor_index;
	for (var i = 0; i < GetCrewCount(GetOwner()); i++)
		if (GetCrew(GetOwner(), i) == cursor)
		{
			cursor_index = i;
			break;
		}
	if (!GetCrew(GetOwner(), cursor_index)) return;
	return cursor_index;
}

// Returns the next valid crew member index
// Wraps around, ignores dead and disabled crew members
private func GetNextCrewIndex(int start)
{
	// Endless loop protection
	if (!GetCrew(GetOwner(), start)) return;
	if (!GetCrew(GetOwner(), start)->GetAlive()) return;
	if (!GetCrew(GetOwner(), start)->GetCrewEnabled()) return;

	var index = start;
	do {
		index++;
		var crew = GetCrew(GetOwner(), index);
		if (crew && crew->GetAlive() && crew->GetCrewEnabled())
			break;
		if (index >= GetCrewCount(GetOwner()))
			index = -1;
	} while (true);
	return index;
}

/* Next clonk display */

// Sets the fill status of the next clonk health bar. Value is between 0 and 1000
private func SetNextHealthValue(int value, int text_val)
{
	value = BoundBy(value, 0, 1000);

	GuiUpdate({ fill = { Right = ToPercentString(value) }, text = { Text = Format("<c dddd00>%d</c>", text_val) } }, crew_gui_id, crew_next_id + 2, this);
}

// Sets the fill status of the next clonk breath bar. Value is between 0 and 1000
// Hides the bar if value == 1000
private func SetNextBreathValue(int value)
{
	value = BoundBy(value, 0, 1000);

	var plr = GetOwner();
	if (value == 1000) plr = NO_OWNER;

	GuiUpdate({ Player = plr, fill = { Right = ToPercentString(value) } }, crew_gui_id, crew_next_id + 3, this);
}

/* Warning effects */

// Shows a heart overlay icon in the portrait on energy changes
// graphics may either be "" or "Broken", id is the gui id to change
private func Heartbeat(string graphics, int id)
{
	// Already triggered
	for (var i = 0; i < GetEffectCount("Heartbeat", this); i++)
		if (GetEffect("Heartbeat", this, i).id == id)
			return;

	var update =
	{
		Player = GetOwner(),
		GraphicsName = graphics
	};

	GuiUpdate(update, crew_gui_id, id, this);
	// Remove heart after ~1 second
	var effect = AddEffect("Heartbeat", this, 1, 35, this);
	effect.id = id;
}

private func FxHeartbeatStop(object o, proplist effect)
{
	GuiUpdate({ Player = NO_OWNER }, crew_gui_id, effect.id, this);
}

// Shows various warnings regarding crew members not displayed (not cursor, not next cursor)
private func IssueWarning(object clonk, id icon, string graphics, string name)
{
	var next_warning_id = GetNextWarningID();
	var effect = AddEffect("IntHUDWarning", clonk, 2, 70, this, nil, icon, graphics, next_warning_id);
	// Make sure the exact same warning isn't already showing (effect gets rejected)
	if (!effect) return;
	if (!name) name = clonk->GetName();

	var next_warning =
	{
		Target = this,
		ID = 500 + next_warning_id, // let's hope there won't ever be more than 500 warnings
		Top = ToEmString(0),
		Bottom = ToEmString(10),
		symbol =
		{
			Symbol = icon,
			GraphicsName = graphics,
			Right = "20%"
		},
		text =
		{
			Style = GUI_TextLeft,
			Text = name,
			Left = "20%"
		}
	};

	// Save position of this warning
	crew_warnings[next_warning_id] = 0;
	// Show
	GuiUpdate({ _new_warning = next_warning }, crew_gui_id, crew_warning_id, this);

	// Lower all other crew warnings
	RepositionCrewWarnings(next_warning_id, 12);
}

private func RemoveWarning(int warning_id)
{
	if (!crew_warnings) return;
	if (crew_warnings[warning_id] == nil || crew_warnings[warning_id] == -1) return;

	GuiClose(crew_gui_id, 500 + warning_id, this);
	
	crew_warnings[warning_id] = -1;
}

// Returns the first index from crew_warnings with value -1 or GetLength(crew_warnings)
private func GetNextWarningID()
{
	var ret = GetLength(crew_warnings);

	for (var i = 0; i < GetLength(crew_warnings); i++)
		if (crew_warnings[i] == -1)
		{
			ret = i;
			break;
		}
	return ret;
}

// Adjusts the Top value of all crew warnings except for except_index by change
private func RepositionCrewWarnings(int except_index, int change)
{
	for (var i = 0; i < GetLength(crew_warnings); i++)
	{
		if (except_index == i)
			continue;
		crew_warnings[i] += change;
		GuiUpdate({ Top = ToEmString(crew_warnings[i]), Bottom = ToEmString(crew_warnings[i] + 10) }, crew_gui_id, 500 + i, this);
	}
}

private func FxIntHUDWarningStart(object clonk, proplist effect, int temp, id icon, string graphic, int gui_id)
{
	if (temp) return;

	effect.icon = icon;
	effect.graphic = graphic;
	effect.id = gui_id;
}

// Reject effects with same parameters
private func FxIntHUDWarningEffect(string effect_name, object target, proplist effect, var1, var2)
{
	if (effect_name != "IntHUDWarning") return;

	// Check if all parameters are the same
	if (effect.icon == var1)
		if (effect.graphic == var2)
			// Reject
			return -1;
}

private func FxIntHUDWarningStop(object clonk, proplist effect, int reason, bool temp)
{
	if (temp) return;

	RemoveWarning(effect.id);
}

/* Crew info menu */

private func OpenCrewInfo()
{
	if (crew_info_id) return CloseCrewInfo();

	crew_info_id = GuiOpen(crew_info_menu);

	var y = 0;
	for (var i = 0; i < GetCrewCount(GetOwner()); i++)
	{
		var crew = GetCrew(GetOwner(), i);
		if (!crew) continue;
		var portrait = crew->~GetPortrait();
		var portraitgraphics = "";
		if (portrait)
		{
			portraitgraphics = portrait.Name;
			portrait = portrait.Source;
		}
		else
			portrait = crew;

		var new_sub =
		{
			Target = this,
			Top = ToEmString(y),
			Bottom = ToEmString(y + 20),
			Priority = 12,
			BackgroundColor = { Std = nil, Hover = RGB(200, 200, 200) },
			OnMouseIn = GuiAction_SetTag("Hover"),
			OnMouseOut = GuiAction_SetTag("Std"),
			OnClick = GuiAction_Call(this, "SelectClonk", i),
			portrait =
			{
				Symbol = portrait,
				GraphicsName = portraitgraphics,
				Right = "20%",
				Priority = 13
			},
			name =
			{
				Style = GUI_TextLeft,
				Text = crew->GetName(),
				Left = "20%",
				Bottom = ToEmString(10),
				Priority = 13
			},
			heart =
			{
				Symbol = CrewHealIcon().Symbol,
				GraphicsName = CrewHealIcon().GraphicsName,
				Left = "21%",
				Right = ToEmString(10),
				Top = ToEmString(10),
				Priority = 13
			},
			health =
			{
				Style = GUI_TextLeft,
				Text = Format("<c dddd00>%d</c>", crew->GetEnergy()),
				Left = Format("20%%%s", ToEmString(5)),
				Top = ToEmString(10),
				Priority = 13
			}
		};
		GuiUpdate({ _crew = new_sub }, crew_info_id);

		y += 20;
	}

	return true;
}

private func SelectClonk(int index)
{
	var clonk = GetCrew(GetOwner(), index);
	// Change allowed?
	if (clonk && clonk->GetCrewEnabled())
		SetCursor(GetOwner(), clonk);

	CloseCrewInfo();
}

private func CloseCrewInfo()
{
	if (!crew_info_id) return;

	GuiClose(crew_info_id);
	crew_info_id = nil;

	return true;
}