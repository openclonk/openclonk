/*--
	Cable Crossing
	The standard crossing for the cable network.
	The crossing will automatically be a station if it is at the end of the cable line (i.e. only one cable connected).
	But the crossing can also manually be set to function as a station.

	Author: Clonkonaut
--*/

#include Library_CableStation

// Animation
local turn_anim;

// Settings
local setting_dropoff = false;

// Combined building
local connected_building;

// Array of all cable cars currently idling at this station
local arrived_cars;

func Initialize()
{
	turn_anim = PlayAnimation("Engine", 1, Anim_Const(0), Anim_Const(1000));
	arrived_cars = [];
	SetCategory(GetCategory() | C4D_StaticBack);
	return _inherited(...);
}

// Prevents the automatic change of the station status when manually set to station mode
local manual_setting = false;

/* Library functions: Cable Station */

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

public func IsAvailable(proplist requested, int amount)
{
	// Check connected buildings for contents
	if (connected_building)
		if (connected_building->ContentsCount(requested) >= amount)
			return true;
	// Check cable cars idling at this station
	if (GetLength(arrived_cars))
	{
		for (var car in arrived_cars)
		{
			if (car->~IsAvailable(requested, amount))
				return true;
		}
	}
	return false;
}

public func GetAvailableCableCar(proplist requested, int amount, proplist requesting_station)
{
	// Check cars that are idling at this station
	var best;
	for (var car in arrived_cars)
	{
		if (!car->~IsReadyForDelivery(requested, amount, requesting_station))
			continue;
		if (!best)
			best = car;
		// A car might want to override the search for an available car, mainly because it holds the container
		// which holds the requested items
		else if (car->~OverridePriority(requested, amount, requesting_station, best))
			best = car;
	}
	if (best)
		return best;

	// Check cars that are idling at the requesting station
	if (requesting_station != this)
		if (best = requesting_station->~GetAvailableCableCar(requested, amount, requesting_station))
			return best;

	return nil;
}

public func OnCableCarDelivery(object car, id requested, int amount)
{
	// Transfer the requested material to the connected producer
	if (!connected_building)
		return;

	car = car->~GetAttachedVehicle() ?? car;

	for (var i = 0; i < amount; i++)
	{
		var item = car->FindContents(requested);
		if (!item)
			break;
		item->Enter(connected_building);
	}
}

/* Construction */

public func IsHammerBuildable() { return true; }

public func ConstructionCombineWith() { return "IsNoCableStationConnected"; }

public func ConstructionCombineDirection(object other)
{
	return CONSTRUCTION_STICK_Left | CONSTRUCTION_STICK_Right;
}

public func ConstructionCombineOffset(proplist other)
{
	if (!other)
		return;

	// Make sure the station preview is on the same ground level than the other building
	var ret = [0,0];
	ret[1] = other->GetObjHeight()/2 - this->GetDefHeight()/2;
	return ret;
}

public func CombineWith(object stick_to)
{
	if (!stick_to) return;

	if (stick_to->~IsProducer())
	{
		connected_building = stick_to;
		stick_to->ConnectCableStation(this);
	}
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
		OnClick = GuiAction_Call(this, "ToggleStation", false),
		image = { Prototype = custom_entry.image }
	};
	station.image.Symbol = CableCrossing_Icons;
	station.image.GraphicsName = "Station";
	PushBack(menu_entries, { symbol = CableCrossing_Icons, extra_data = "Station", custom = station });

	var drop_off = new custom_entry {
		Priority = 1001,
		Tooltip = "$TooltipToggleDropOff$",
		OnClick = GuiAction_Call(this, "ToggleDropOff", false),
		image = { Prototype = custom_entry.image }
	};
	drop_off.image.Symbol = CableCrossing_Icons;
	drop_off.image.GraphicsName = "DropOff";
	PushBack(menu_entries, { symbol = CableCrossing_Icons, extra_data = "DropOff", custom = drop_off });

	return menu_entries;
}

public func OnSettingsHover(symbol, extra_data, desc_menu_target, menu_id)
{
	if (symbol == nil) return;

	var text = "";
	if (extra_data == "Station")
		text = "$DescToggleStation$";
	if (extra_data == "DropOff")
		text = "$DescToggleDropOff$";

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

public func ToggleDropOff(bool silent)
{
	if (!setting_dropoff)
	{
		if (!IsCableStation())
			ToggleStation(true);
		setting_dropoff = true;
	} else {
		setting_dropoff = false;
	}
	if (!silent)
		Sound("UI::Click2");
	CheckStationStatus();
}

/* Cable Car Management */

public func OnCableCarArrival(object car)
{
	// Apply all settings to the arriving car
	if (setting_dropoff)
		if (car)
			car->~DropContents(this);

	// Save the car
	PushBack(arrived_cars, car);
}

public func OnCableCarDeparture(object car)
{
	RemoveArrayValue(arrived_cars, car, true);
}

public func OnCableCarEngaged(object car)
{
	PushBack(arrived_cars, car);
}

public func OnCableCarDisengaged(object car)
{
	RemoveArrayValue(arrived_cars, car, true);
}

/* Visuals */

func CheckStationStatus()
{
	if (IsCableStation())
	{
		// In order of priority
		if (setting_dropoff)
		{
			SetMeshMaterial("CableCarStation_SignDropOff", 1);
		} else {
			SetMeshMaterial("CableCarStation_SignStation", 1);
		}
	}
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

	if (connected_building)
		props->AddCall("Combination", this, "CombineWith", connected_building);
	return true;
}

public func SetManual() { manual_setting = true; return true; }

/* Properties */

local Name = "$Name$";
local BlastIncinerate = 50;
local LineAttach = [6,-9];