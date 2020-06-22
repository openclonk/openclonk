/**
	Cable Crossing
	The standard crossing for the cable network.
	The crossing will automatically be a station if it is at the end of the cable line (i.e. only one cable connected).
	But the crossing can also manually be set to function as a station.

	@author Clonkonaut
*/

#include Library_CableStation
#include Library_Structure
#include Library_Ownable

// Animation
local turn_anim;

// Settings
local setting_dropoff = false;

// Combined building
local connected_building;

// Array of all cable cars currently idling at this station
local arrived_cars;

// Whether this crossing accepts resources.
local has_resource_chute;


public func Construction()
{
	SetAction("Default");
	return _inherited(...);
}

public func Initialize()
{
	turn_anim = PlayAnimation("Engine", 1, Anim_Const(0), Anim_Const(1000));
	arrived_cars = [];
	SetCategory(GetCategory() | C4D_StaticBack);
	return _inherited(...);
}

// Prevents the automatic change of the station status when manually set to station mode
local manual_setting = false;


/*-- Library functions: Cable Station --*/

public func DestinationsUpdated()
{
	// Do nothing if set manually
	if (manual_setting) return;
	
	var is_endpoint = GetLength(FindObjects(Find_Func("IsConnectedTo", this))) == 1;
	if (is_endpoint || connected_building)
		SetCableStation(true);
	else
		SetCableStation(false);
	CheckStationStatus();
	
	// Inform all cars at station about the update.
	for (var car in arrived_cars)
		if (car)
			car->~OnRailNetworkUpdate();
	// Also inform all cars on adjacent cables.
	for (var cable in FindObjects(Find_Func("IsConnectedTo", this)))
		cable->OnRailNetworkUpdate();
}

public func IsAvailable(proplist order)
{
	// Check resource chute for contents.
	if (has_resource_chute && ContentsCount(order.type) >= order.min_amount)
			return true;
	// Check cable cars idling at this station.
	if (GetLength(arrived_cars))
	{
		for (var car in arrived_cars)
		{
			if (car->~IsAvailable(order))
				return true;
		}
	}
	// TODO: check connected building.
	return false;
}

public func GetAvailableCableCar(proplist order, object requesting_station)
{
	// Check cars that are idling at this station
	var best;
	for (var car in arrived_cars)
	{
		if (!car->~IsReadyForDelivery(order, requesting_station))
			continue;
		if (!best)
			best = car;
		// A car might want to override the search for an available car, mainly because it holds the container
		// which holds the requested items
		else if (car->~OverridePriority(order, requesting_station, best))
			best = car;
	}
	if (best)
		return best;

	// Check cars that are idling at other crossings in the network.
	if (requesting_station != this)
	{
		// Find closest cars first.
		var destinations = GetDestinations();
		SortArrayByArrayElement(destinations, this.const_distance, false);
		for (var dest in destinations)
		{		
			var station = dest[this.const_finaldestination];
			var best = station->~GetAvailableCableCar(order, station);
			if (best)
				return best;
		}
	}
	return nil;
}

public func OnCableCarPickUp(object car, proplist order)
{
	car = car->~GetAttachedVehicle() ?? car;
	
	// Take from resource chute if available.
	if (has_resource_chute)
	{
		var amount = Max(order.min_amount, order.max_amount);
		while (amount > 0)
		{
			var item = FindContents(order.type);
			if (!item)
				break;
			item->Enter(car);
			amount--;
		}
	}
	
	// Take from connected building alternatively.
	if (connected_building)
	{
		// TODO: Do we even want this?
	}		
	return;
}

public func OnCableCarDelivery(object car, proplist order)
{
	// Transfer the requested material to the connected producer
	if (!connected_building)
		return;

	car = car->~GetAttachedVehicle() ?? car;
	var amount = Max(order.min_amount, order.max_amount);
	while (amount > 0)
	{
		var item = car->FindContents(order.type);
		if (!item)
			break;
		item->Enter(connected_building);
		amount--;
	}
	return;
}


/*-- Construction --*/

public func IsHammerBuildable() { return true; }

public func SetDir(int dir)
{
	if (GetDir() != dir)
		this.LineAttach = [-this.LineAttach[0], this.LineAttach[1]];
	return _inherited(dir, ...);
}

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
	return [0, other->GetObjHeight() / 2 - this->GetDefHeight() / 2];
}

public func CombineWith(object stick_to)
{
	if (!stick_to) return;

	if (stick_to->~AcceptsCableStationConnection())
	{
		connected_building = stick_to;
		stick_to->ConnectCableStation(this);
		SetCableStation(true);
		CheckStationStatus();
	}
}

/*-- Interaction --*/

// Provides an own interaction menu.
public func HasInteractionMenu() { return true; }

// Show settings in interaction menu
public func GetInteractionMenus(object clonk)
{
	var menus = _inherited(clonk, ...) ?? [];
	// Crossing settings.
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
	// Upgrade options.
	if (!has_resource_chute)
	{
		var upgrade_menu =
		{
			title = "$StationUpgrades$",
			entries_callback = this.GetUpgradeMenuEntries,
			callback = "OnUpgradeSelected",
			callback_hover = "OnUpgradeHover",
			callback_target = this,
			BackgroundColor = RGB(0, 50, 0),
			Priority = 40
		};
		PushBack(menus, upgrade_menu);
	}
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

	// Clickable buttons.
	var station = new custom_entry
	{
		Priority = 1000,
		Tooltip = "$TooltipToggleStation$",
		OnClick = GuiAction_Call(this, "ToggleStation", false),
		image = { Prototype = custom_entry.image }
	};
	station.image.Symbol = CableCrossing_Icons;
	station.image.GraphicsName = "Station";
	PushBack(menu_entries, { symbol = CableCrossing_Icons, extra_data = "Station", custom = station });

	var drop_off = new custom_entry
	{
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

	GuiUpdate({Text = text}, menu_id, 1, desc_menu_target);
}

public func GetUpgradeMenuEntries()
{
	var menu = 
	{
		Bottom = "2em",
		entry = 
		{
			Bottom = "2em",
			BackgroundColor = { Std = 0, Selected = RGB(100, 30, 30) },
			OnMouseIn = GuiAction_SetTag("Selected"),
			OnMouseOut = GuiAction_SetTag("Std"),
			right =
			{
				Left = "2em + 0.2em",
				Right = "100% - 0.2em",
				Text = "$UpgradeChute$",
				Style = GUI_TextVCenter
			},
			symbol = 
			{
				Right = "2em",
				Symbol = CableCrossing_Icons
			}
		}
	};		
	return [{symbol = CableCrossing_Icons, extra_data = "upgrade", custom = menu}];
}

public func OnUpgradeHover(id symbol, string action, desc_menu_target, menu_id)
{
	var text = "$DescUpgradeChute$";	
	GuiUpdate({Text = text}, menu_id, 1, desc_menu_target);
}

public func OnUpgradeSelected(id symbol, string action, object cursor)
{
	var hammer = FindObject(Find_Container(cursor), Find_Func("IsConstructor"));
	
	if (!hammer)
	{
		PlayerMessage(cursor->GetOwner(), "$YouNeedAHammer$");
		Sound("UI::Click2", {player = cursor->GetOwner()});
		return;
	}
	
	var metal = FindObject(Find_Container(cursor), Find_ID(Metal));
	if (!metal)
	{
		PlayerMessage(cursor->GetOwner(), "$YouNeedMetal$");
		Sound("UI::Click2", {player = cursor->GetOwner()});
		return;
	}
	
	metal->RemoveObject();
	AddResourceChute();
	Sound("Structures::Repair");
	UpdateInteractionMenus();
	return;
}


/*-- Resource Chute --*/ 

public func AddResourceChute()
{
	// TODO: graphics.
	has_resource_chute = true;
	AddTimer("CheckResourceChute", 1);
	return;
}

public func CheckResourceChute()
{
	if (GetCon() < 100)
		return;
	for (var obj in FindObjects(Find_InRect(-13 + 13 * GetDir(), 3, 13, 13), Find_OCF(OCF_Collectible), Find_NoContainer(), Find_Layer(GetObjectLayer())))
		Collect(obj, true);
}

public func Collection(object obj, bool put)
{
	Sound("Objects::Clonk");
}

public func IsContainer() { return has_resource_chute; }

public func IsStorage() { return has_resource_chute; }

local MaxContentsCount = 100;

public func RejectCollect()
{
	if (ContentsCount() >= this.MaxContentsCount)
		return true;
	return false;
}


/*-- Settings --*/

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
	}
	else
	{
		setting_dropoff = false;
	}
	if (!silent)
		Sound("UI::Click2");
	CheckStationStatus();
}


/*-- Cable Car Management --*/

public func OnCableCarArrival(object car)
{
	// Apply all settings to the arriving car
	if (setting_dropoff)
		if (car)
			car->~DropContents(this);

	// Save the car
	PushBack(arrived_cars, car);
}

public func OnCableCarStopped(object car)
{
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

public func OnCableCarDestruction(object car)
{
	RemoveArrayValue(arrived_cars, car, true);
}

public func GetIdleCars() { return arrived_cars; }


/*-- Visuals --*/

public func CheckStationStatus()
{
	if (IsCableStation())
	{
		// In order of priority
		if (setting_dropoff)
			SetMeshMaterial("CableCarStation_SignDropOff", 1);
		else
			SetMeshMaterial("CableCarStation_SignStation", 1);
	}
	else
		SetMeshMaterial("CableCarStation_Sign", 1);
}

local activations = 0;

public func CableActivation(int count)
{
	if (activations <= 0)
		SetAnimationPosition(turn_anim, Anim_Linear(GetAnimationPosition(turn_anim), 0, GetAnimationLength("Engine"), 175, ANIM_Loop));
	activations += count;
}

public func CableDeactivation(int count)
{
	activations -= count;
	if (activations <= 0)
		SetAnimationPosition(turn_anim, Anim_Const(GetAnimationPosition(turn_anim)));
}


/*-- Saving --*/

public func SaveScenarioObject(props)
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


/*-- Properties --*/

local ActMap = {
	Default = {
		Prototype = Action,
		Name = "Default",
		Procedure = DFA_NONE,
		Directions = 2,
		FlipDir = 1,
		Length = 1,
		Delay = 0,
		FacetBase = 1,
		NextAction = "Default",
	}
};

local Name = "$Name$";
local BlastIncinerate = 90;
local HitPoints = 80;
local LineAttach = [8, -9];
local Components = {Metal = 1, Wood = 1};
local ContainBlast = true;
local FireproofContainer = true;
