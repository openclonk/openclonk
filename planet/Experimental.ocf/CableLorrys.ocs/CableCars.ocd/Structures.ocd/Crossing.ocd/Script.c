/*--
	Cable Crossing
	The standard crossing for the cable network.
	The crossing will automatically be a station if it is at the end of the cable line (i.e. only one cable connected).
	But the crossing can also manually be set to function as a station.

	Author: Clonkonaut
--*/

#include Library_CableStation

local turn_anim;

func Initialize()
{
	turn_anim = PlayAnimation("Engine", 1, Anim_Const(0), Anim_Const(1000));
	return _inherited(...);
}

// Prevents the automatic change of the station status when manually set to station mode
local manual_setting = false;

// Called from the cable crossing library
public func DestinationsUpdated()
{
	// Do nothing if set manually
	if (manual_setting) return;

	if (GetLength(FindObjects(Find_Func("IsConnectedTo", this))) == 1)
		SetCableStation(true);
	else
		SetCableStation(false);
	CheckStationStatus();
}

/* Interaction */

// Provides an own interaction menu.
public func HasInteractionMenu() { return true; }

// Show settings in interaction menu
public func GetInteractionMenus(object clonk)
{
	var menus = _inherited(clonk, ...) ?? [];
	var crossing_menu =
	{
		title = "$StationSettings$",
		entries_callback = this.GetSettingsMenuEntries,
		callback = nil,
		callback_hover = "OnSettingsHover",
		callback_target = this,
		BackgroundColor = RGB(0, 0, 50),
		Priority = 20
	};
	PushBack(menus, crossing_menu);

	return menus;
}

public func GetSettingsMenuEntries()
{
	var control_prototype =
	{
		BackgroundColor = { Std = 0, Selected = RGB(100, 30, 30) },
		OnMouseIn = GuiAction_SetTag("Selected"),
		OnMouseOut = GuiAction_SetTag("Std")
	};

	var custom_entry =
	{
		Right = "3em", Bottom = "2em",
		image = { Prototype = control_prototype }
	};

	var menu_entries = [];

	// Clickable buttons
	var station = new custom_entry {
		Priority = 1000,
		Tooltip = "$TooltipToggleStation$",
		OnClick = GuiAction_Call(this, "ToggleStation", false)
	};
	station.image.Symbol = CableCrossing_Icons;
	station.image.GraphicsName = "Station";
	PushBack(menu_entries, { symbol = CableCrossing_Icons, extra_data = "Station", custom = station });

	return menu_entries;
}

public func OnSettingsHover(symbol, extra_data, desc_menu_target, menu_id)
{
	if (symbol == nil) return;

	var text = "";
	if (extra_data == "Station")
		text = "$DescToggleStation$";

	GuiUpdate({ Text = text }, menu_id, 1, desc_menu_target);
}

/* Settings */

public func ToggleStation(bool silent)
{
	SetCableStation(!IsCableStation());
	if (!manual_setting)
		manual_setting = true;
	if (!silent)
		Sound("UI::Click2");
	CheckStationStatus();
}

/* Visuals */

func CheckStationStatus()
{
	if (IsCableStation())
		SetMeshMaterial("CableCarStation_SignStation", 1);
	else
		SetMeshMaterial("CableCarStation_Sign", 1);
}

local activations = 0;

func CableActivation(int count)
{
	if (activations <= 0)
		SetAnimationPosition(turn_anim, Anim_Linear(GetAnimationPosition(turn_anim), 0, GetAnimationLength("Engine"), 175, ANIM_Loop));
	activations += count;
}

func CableDeactivation(int count)
{
	activations -= count;
	if (activations <= 0)
		SetAnimationPosition(turn_anim, Anim_Const(GetAnimationPosition(turn_anim)));
}

public func NoConstructionFlip() { return true; }

/* Saving */

func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	if (IsCableStation() && manual_setting)
		props->AddCall("StationSetting", this, "ToggleStation", true);
	if (!IsCableStation() && manual_setting)
		props->AddCall("ManualSetting", this, "SetManual");
	return true;
}

public func SetManual() { manual_setting = true; return true; }

local Name = "$Name$";
local BlastIncinerate = 50;
local LineAttach = [6,-9];