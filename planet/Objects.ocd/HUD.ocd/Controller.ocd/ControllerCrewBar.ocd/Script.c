/**
	Controller Crew Bar
	Controlls the crew bar display.

	@author Maikel
*/

// TODO: static const for bar margins.
// TODO: Use old bars.

// HUD margin and size in tenths of em.
static const GUI_Controller_CrewBar_IconSize = 40;
static const GUI_Controller_CrewBar_IconMargin = 5;

// Local variables to keep track of the crew HUD menu.
local crew_gui_target;
local crew_gui_menu;
local crew_gui_id;


/*-- Construction & Destruction --*/

public func Construction()
{
	var plr = GetOwner();
	// Create the crew HUD menu.
	CreateCrewHUDMenu(plr);
	return _inherited(...);
}

public func Destruction()
{
	// This also closes the crew HUD menu.
	if (crew_gui_target)
		crew_gui_target->RemoveObject();
	return _inherited(...);
}


/*-- Callbacks --*/

public func OnCrewRecruitment(object clonk, int plr)
{
	RemoveCrewHUDMenu();
	CreateCrewHUDMenu(plr);
	return _inherited(clonk, plr, ...);
}

public func OnCrewDeRecruitment(object clonk, int plr)
{
	RemoveCrewHUDMenu();
	CreateCrewHUDMenu(plr);
	return _inherited(clonk, plr, ...);
}

public func OnCrewDeath(object clonk, int killer)
{
	RemoveCrewHUDMenu();
	CreateCrewHUDMenu(clonk->GetOwner());
	return _inherited(clonk, killer, ...);
}

public func OnCrewDestruction(object clonk)
{
	RemoveCrewHUDMenu();
	CreateCrewHUDMenu(clonk->GetOwner());
	return _inherited(clonk, ...);
}

public func OnCrewDisabled(object clonk)
{
	RemoveCrewHUDMenu();
	CreateCrewHUDMenu(clonk->GetOwner());
	return _inherited(clonk, ...);
}

public func OnCrewEnabled(object clonk)
{
	RemoveCrewHUDMenu();
	CreateCrewHUDMenu(clonk->GetOwner());
	return _inherited(clonk, ...);
}

public func OnCrewSelection(object clonk, bool unselect)
{
	CrewHUDMenuUpdateSelection(clonk, unselect);
	return _inherited(clonk, unselect, ...);
}

public func OnCrewNameChange(object clonk)
{
	CrewHUDMenuUpdateName(clonk);
	return _inherited(clonk, ...);
}

public func OnCrewRankChange(object clonk)
{
	CrewHUDMenuUpdateRank(clonk);
	return _inherited(clonk, ...);
}

public func OnCrewHealthChange(object clonk)
{
	var health_phys = clonk->GetMaxEnergy();
	var health_val = clonk->GetEnergy();
	var health_ratio = 0;
	if (health_phys != 0) 
		health_ratio = 1000 * health_val / health_phys;
	CrewHUDMenuUpdateHealth(clonk, health_val, health_ratio);
	return _inherited(clonk, ...);
}

public func OnCrewBreathChange(object clonk)
{
	var breath_phys = clonk->GetMaxBreath();
	var breath_val = clonk->GetBreath();
	if (breath_phys == breath_val)
	{
		// Hide breath bar and material background.
		CrewHUDMenuHideBreath(clonk);
		CrewHUDMenuHideBackground(clonk);
		return _inherited(clonk, ...);
	}
	// Show breath bar.
	var breath_ratio = 0;
	if (breath_phys != 0) 
		breath_ratio = 1000 * breath_val / breath_phys;
	CrewHUDMenuShowBreath(clonk);
	CrewHUDMenuUpdateBreath(clonk, breath_val, breath_ratio);
	// Show material background.
	var clr = GetAverageTextureColor(clonk->GetTexture());
	CrewHUDMenuShowBackground(clonk, clr);
	CrewHUDMenuUpdateBackground(clonk, clr);
	return _inherited(clonk, ...);
}


/*-- Crew HUD --*/

private func CreateCrewHUDMenu(int plr)
{
	// Create a menu target.
	crew_gui_target = CreateObject(Dummy, AbsX(0), AbsY(0), plr);
	crew_gui_target.Visibility = VIS_Owner;	
	// Create the crew HUD menu.
	var margin = GUI_Controller_CrewBar_IconMargin;
	var size = GUI_Controller_CrewBar_IconSize;
	crew_gui_menu = 
	{
		Target = crew_gui_target,
		Style = GUI_Multiple | GUI_TextHCenter | GUI_TextBottom | GUI_NoCrop,
		Left = ToEmString(margin),
		Right = ToEmString(margin + size),
		Top = ToEmString(margin),
		Bottom = ToEmString(margin + size),
		OnClose = GuiAction_Call(this, "OnCrewHUDMenuClosed", plr),
	};
	// Add the player's crew members to the menu.
	var nr_added = 0;
	for (var index = 0; index < GetCrewCount(plr); ++index)
	{
		var crew = GetCrew(plr, index);
		if (!crew->GetCrewEnabled())
			continue;
		crew_gui_menu[Format("crew%d", index)] = AddCrewHUDMenuMember(crew, index, margin, size);
		// Also add a health monitor for each crew member.
		AddEffect("GUIHealthMonitor", crew, 100, 0, this);
		nr_added++;
	}
	// Adjust menu size to number of crew members.
	crew_gui_menu.Right = ToEmString((margin + size) * nr_added);
	crew_gui_id = GuiOpen(crew_gui_menu);
	return;
}

private func RemoveCrewHUDMenu()
{
	GuiClose(crew_gui_id);
	return;
}

public func OnCrewHUDMenuClosed(int plr)
{
	if (crew_gui_target)
		crew_gui_target->RemoveObject();
	crew_gui_menu = nil;
	crew_gui_id = nil;
	for (var index = 0; index < GetCrewCount(plr); ++index)
	{
		var crew = GetCrew(plr, index);
		RemoveEffect("GUIHealthMonitor", crew);
	}
	return;
}

private func AddCrewHUDMenuMember(object crew, int index, int margin, int size)
{
	var health_phys = crew->GetMaxEnergy();
	var health_val = crew->GetEnergy();
	var health_ratio = 0;
	if (health_phys != 0) 
		health_ratio = 1000 * health_val / health_phys;
	var breath_phys = crew->GetMaxBreath();
	var breath_val = crew->GetBreath();
	var breath_ratio = 0;
	if (breath_phys != 0) 
		breath_ratio = 1000 * breath_val / breath_phys;
	
	// Create dummy objects for background and damage effects.
	var background_dummy = CreateObject(Dummy);
	background_dummy->Enter(crew_gui_target);
	background_dummy.Visibility = VIS_None;
	background_dummy->SetGraphics("Background", GUI_Controller_CrewBar, GFX_Overlay, GFXOV_MODE_Picture);
	background_dummy->SetClrModulation(0x0, GFX_Overlay);
	var damage_dummy = CreateObject(Dummy);
	damage_dummy->Enter(crew_gui_target);
	damage_dummy.Visibility = VIS_None;
	damage_dummy->SetGraphics("Hurt", GUI_Controller_CrewBar, GFX_Overlay, GFXOV_MODE_Picture);
	damage_dummy->SetClrModulation(0x0, GFX_Overlay);
	
	// Selection.
	var selected = nil;
	if (GetCursor(GetOwner()) == crew)
		selected = "Selected";
		
	var crew_menu = 
	{
		// This part of the crew menu contains the background.
		Target = crew_gui_target,
		ID = 100 * (index + 1),
		Style = GUI_NoCrop,
		Left = ToEmString(margin * index + size * index),
		Right = ToEmString(margin * index + size * (index + 1)),
		Top = ToEmString(0),
		Bottom = ToEmString(size),
		// Background for in material showing.
		background = 
		{
			Target = crew_gui_target,
			ID = 100 * (index + 1) + 1,
			Symbol = background_dummy,
			Priority = 1,
		},
		// The frame around the crew.
		frame = 
		{
			Target = crew_gui_target,
			ID = 100 * (index + 1) + 2,
			Symbol = GUI_Controller_CrewBar,
			GraphicsName = { Std = selected, OnHover = "Focussed" },
			OnMouseIn = GuiAction_SetTag("OnHover"),
			OnMouseOut = GuiAction_SetTag("Std"),
			OnClick = GuiAction_Call(this, "CrewHUDMenuOnSelection", crew),
			Priority = 2,
		},
		// Contains the clonk and its name.
		crew = {
			Target = crew_gui_target,
			ID = 100 * (index + 1) + 3,
			Symbol = crew,
			Text = crew->GetName(),
			Tooltip = Format("$TxtSelect$", Format("%s %s", crew->GetObjCoreRankName(), crew->GetName())),
			Style = GUI_TextHCenter | GUI_TextBottom,
			Priority = 3,
			damage = 
			{
				Target = crew_gui_target,
				ID = 100 * (index + 1) + 4,
				Symbol = damage_dummy,
				Priority = 4,
			}
		},
		// Contains the crew number.
		number = {
			Target = crew_gui_target,
			ID = 100 * (index + 1) + 5,
			Left = Format("100%%%s", ToEmString(-size / 4)),
			Bottom = ToEmString(size / 4),
			Symbol = Icon_Number,
			GraphicsName = Format("%d", index + 1),
			Priority = 5,
		},
		// Contains the rank symbol.
		rank = {
			Target = crew_gui_target,
			ID = 100 * (index + 1) + 6,
			Style = GUI_NoCrop,
			Right = ToEmString(size / 6),
			Bottom = ToEmString(size / 6),
			Symbol = Icon_Rank,
			GraphicsName = Format("%d", crew->GetRank() % 24),
			Priority = 5,
			grade = {
				Target = crew_gui_target,
				ID = 100 * (index + 1) + 7,
				Left = "-20%",
				Right = "80%",
				Top = "-20%",
				Bottom = "80%",
				Symbol = Icon_Rank,
				GraphicsName = Format("Upgrade%d", crew->GetRank() / 24),
				Priority = 6,
			},
		},
		// Contains the health bar.
		health = {
			Target = crew_gui_target,
			ID = 100 * (index + 1) + 8,
			Style = GUI_NoCrop,
			Left = "0%",
			Right = "100%",
			Top = "100%+0.2em",
			Bottom = "100%+0.65em",
			BackgroundColor = RGB(40, 40, 40),
			Priority = 6,
			value = 
			{
				Target = crew_gui_target,
				ID = 100 * (index + 1) + 9,
				Style = GUI_NoCrop,
				Left = "0.1em",
				Right = Format("%s%s", ToPercentString(health_ratio), ToEmString((500 - health_ratio) / 250)),
				Margin = ["0em", "0.1em"],
				BackgroundColor = RGB(160, 0, 0),
				Priority = 7,
				text = 
				{
					Target = crew_gui_target,
					ID = 100 * (index + 1) + 10,
					Top = "-0.45em",
					Style = GUI_TextHCenter,
					Text = Format("<c dddd00>%d</c>", health_val),
					Priority = 8,
				},
			},
		},
		// Contains the breath bar.
		breath = {
			Target = crew_gui_target,
			ID = 100 * (index + 1) + 11,
			Style = GUI_NoCrop,
			Left = "0%",
			Right = "100%",
			Top = "100%+0.75em",
			Bottom = "100%+1.2em",
			Priority = 6,
			value = 
			{
				Target = crew_gui_target,
				ID = 100 * (index + 1) + 12,
				Left = "0.2em",
				Right = Format("%s%s", ToPercentString(breath_ratio), ToEmString((500 - breath_ratio) / 250)),
				Margin = ["0em", "0.1em"],
				Priority = 7,
			},
		},
	};
	return crew_menu;
}


/*-- Crew Selection --*/

private func CrewHUDMenuOnSelection(object clonk)
{
	if (!clonk || (!clonk->GetCrewEnabled()))
		return;

	// Stop previously selected crew and set new cursor.
	var plr = GetOwner();
	StopSelected(plr);
	SetCursor(plr, clonk);
	return;
}


/*-- Crew HUD: Updates --*/

// Returns the submenu for this clonk, or nil if not found.
private func CrewHUDMenuFindCrewMenu(object clonk)
{
	var index = 0, crew_menu;
	while ((crew_menu = crew_gui_menu[Format("crew%d", index)]))
	{
		if (crew_menu.crew.Symbol == clonk)
			break;
		index++;
	}
	if (crew_menu)
		return [crew_menu, Format("crew%d", index)];
	return;
}

private func CrewHUDMenuUpdateName(object clonk)
{
	var crew_menu = CrewHUDMenuFindCrewMenu(clonk);
	if (!crew_menu)
		return;
	var name_menu = crew_menu[0].crew;
	name_menu.Text = clonk->GetName();
	name_menu.Tooltip = Format("$TxtSelect$", Format("%s %s", clonk->GetObjCoreRankName(), clonk->GetName()));
	GuiUpdate(name_menu, crew_gui_id, name_menu.ID, name_menu.Target);
	return;
}

private func CrewHUDMenuUpdateRank(object clonk)
{
	var crew_menu = CrewHUDMenuFindCrewMenu(clonk);
	if (!crew_menu)
		return;
	// Update rank.
	var rank_menu = crew_menu[0].rank;
	rank_menu.GraphicsName = Format("%d", clonk->GetRank() % 24);
	rank_menu.grade.GraphicsName = Format("Upgrade%d", clonk->GetRank() / 24);
	GuiUpdate(rank_menu, crew_gui_id, rank_menu.ID, rank_menu.Target);
	// Also update tooltip rank.
	var name_menu = crew_menu[0].crew;
	name_menu.Tooltip = Format("$TxtSelect$", Format("%s %s", clonk->GetObjCoreRankName(), clonk->GetName()));
	GuiUpdate(name_menu, crew_gui_id, name_menu.ID, name_menu.Target);	
	return;
}

private func CrewHUDMenuUpdateSelection(object clonk, bool unselect)
{
	// Disable all previous selections.
	/*var index = 0, crew_menu;
	while ((crew_menu = crew_gui_menu[Format("crew%d", index)]))
	{
		crew_menu.frame.GraphicsName = nil;
		GuiUpdate(crew_menu.frame, crew_gui_id, crew_menu.frame.ID, crew_menu.frame.Target);
		index++;
	}*/
	// Update the selection of the given clonk.	
	var crew_menu = CrewHUDMenuFindCrewMenu(clonk);
	if (!crew_menu)
		return;
	var frame_menu = crew_menu[0].frame;
	frame_menu.GraphicsName = "Selected";
	if (unselect)
		frame_menu.GraphicsName = nil;
	GuiUpdate(frame_menu, crew_gui_id, frame_menu.ID, frame_menu.Target);
	return;
}

private func CrewHUDMenuUpdateHealth(object clonk, int health, int health_ratio)
{
	var crew_menu = CrewHUDMenuFindCrewMenu(clonk);
	if (!crew_menu)
		return;
	var health_menu = crew_menu[0].health;
	health_menu.value.Right = Format("%s%s", ToPercentString(health_ratio), ToEmString((500 - health_ratio) / 500));
	health_menu.value.text.Text = Format("<c dddd00>%d</c>", health);
	GuiUpdate(health_menu, crew_gui_id, health_menu.ID, health_menu.Target);
	return;
}

private func CrewHUDMenuUpdateBreath(object clonk, int breath, int breath_ratio)
{
	var crew_menu = CrewHUDMenuFindCrewMenu(clonk);
	if (!crew_menu)
		return;
	var breath_menu = crew_menu[0].breath;
	breath_menu.value.Right = Format("%s%s", ToPercentString(breath_ratio), ToEmString((500 - breath_ratio) / 500));
	GuiUpdate(breath_menu, crew_gui_id, breath_menu.ID, breath_menu.Target);
	return;
}

private func CrewHUDMenuShowBreath(object clonk)
{
	var crew_menu = CrewHUDMenuFindCrewMenu(clonk);
	if (!crew_menu)
		return;
	var breath_menu = crew_menu[0].breath;
	breath_menu.BackgroundColor = RGB(40, 40, 40);
	breath_menu.value.BackgroundColor = RGB(0, 160, 160);
	GuiUpdate(breath_menu, crew_gui_id, breath_menu.ID, breath_menu.Target);
	return;
}

private func CrewHUDMenuHideBreath(object clonk)
{
	var crew_menu = CrewHUDMenuFindCrewMenu(clonk);
	if (!crew_menu)
		return;
	var breath_menu = crew_menu[0].breath;
	breath_menu.BackgroundColor = nil;
	breath_menu.value.BackgroundColor = nil;
	GuiUpdate(breath_menu, crew_gui_id, breath_menu.ID, breath_menu.Target);
	return;
}


/*-- Health Monitor --*/

public func FxGUIHealthMonitorDamage(object target, proplist effect, int damage, int cause)
{
	var change = Abs(damage) / ((target->GetMaxEnergy() * 10) || 1);
	// For really small changes, like fire or higher precision DoEnergy, set the change to a non zero value.
	if (change == 0)
		change = 3;
	
	// Create a health display effect and set intensity.
	if (!effect.worker)
	{
		effect.worker = AddEffect("GUIHealthDisplay", target, 100, 4, this);
		effect.worker.intensity = change + 20;
	}
	else
		effect.worker.intensity += change;
	effect.worker.intensity = BoundBy(effect.worker.intensity, 0, 80);
	
	// Also set the type of the health: damage, heal or fire.
	if (damage > 0)
		effect.worker.type = 1;
	else
		effect.worker.type = 0;
	if (cause == FX_Call_DmgFire || cause == FX_Call_EngFire)
		effect.worker.type = 2;
	// Just return the unchanged damage.
	return damage;
}

public func FxGUIHealthDisplayStart(object target, proplist effect, bool temp)
{
	if (temp) 
		return;
	CrewHUDMenuShowDamage(target);
	EffectCall(target, effect, "Timer");
	return FX_OK;
}

public func FxGUIHealthDisplayTimer(object target, proplist effect, int time)
{
	// Alpha of the effect depends on the intensity.
	var alpha = BoundBy(effect.intensity * 7, 20, 255);
	// Damage effect.
	if (effect.type == 0)
	{
		var r = Min(200 + effect.intensity, 255);
		CrewHUDMenuUpdateDamage(target, RGBa(r, 0, 0, alpha));
	}
	// Heal effect.
	else if (effect.type == 1)
	{
		var g = Min(100 + effect.intensity, 255);
		alpha = Min(alpha + 100, 255);
		CrewHUDMenuUpdateDamage(target, RGBa(50, g, 50, alpha));
	}
	// Fire effect.
	else if (effect.type == 2)
	{
		var r = Min(150 + effect.intensity, 255);
		var g = Min(effect.intensity, 100);
		alpha = Min(alpha + 100, 255);
		CrewHUDMenuUpdateDamage(target, RGBa(r, g, 0, alpha));
	}
	// Not set yet? might happen at the start sometimes. we just hide in that case.
	else
		CrewHUDMenuUpdateDamage(target, RGBa(0, 0, 0, 0));
	
	// Fade the effect, fade faster if huge numbers.
	effect.intensity -= 3;
	if (effect.intensity > 60)
		effect.intensity -= 2;
	
	// Remove the effect if the intensity is below zero.
	if (effect.intensity <= 0)
		return FX_Execute_Kill;
	return FX_OK;
}

public func FxGUIHealthDisplayStop(object target, proplist effect, int reason, bool temp)
{
	if (temp) 
		return FX_OK;
	CrewHUDMenuHideDamage(target);
	return FX_OK;
}

private func CrewHUDMenuShowDamage(object clonk, int color)
{
	var crew_menu = CrewHUDMenuFindCrewMenu(clonk);
	if (!crew_menu)
		return;
	crew_menu[0].crew.damage.Symbol->SetClrModulation(color, GFX_Overlay);
	return;
}

private func CrewHUDMenuUpdateDamage(object clonk, int new_color)
{
	var crew_menu = CrewHUDMenuFindCrewMenu(clonk);
	if (!crew_menu)
		return;
	crew_menu[0].crew.damage.Symbol->SetClrModulation(new_color, GFX_Overlay);
	return;
}

private func CrewHUDMenuHideDamage(object clonk)
{
	var crew_menu = CrewHUDMenuFindCrewMenu(clonk);
	if (!crew_menu)
		return;
	crew_menu[0].crew.damage.Symbol->SetClrModulation(0x0, GFX_Overlay);
	return;
}


/*-- In Material Background --*/

private func CrewHUDMenuShowBackground(object clonk, int color)
{
	var crew_menu = CrewHUDMenuFindCrewMenu(clonk);
	if (!crew_menu)
		return;
	crew_menu[0].background.Symbol->SetClrModulation(color, GFX_Overlay);
	return;
}

private func CrewHUDMenuUpdateBackground(object clonk, int new_color)
{
	var crew_menu = CrewHUDMenuFindCrewMenu(clonk);
	if (!crew_menu)
		return;
	crew_menu[0].background.Symbol->SetClrModulation(new_color, GFX_Overlay);
	return;
}

private func CrewHUDMenuHideBackground(object clonk)
{
	var crew_menu = CrewHUDMenuFindCrewMenu(clonk);
	if (!crew_menu)
		return;
	crew_menu[0].background.Symbol->SetClrModulation(0x0, GFX_Overlay);
	return;
}
