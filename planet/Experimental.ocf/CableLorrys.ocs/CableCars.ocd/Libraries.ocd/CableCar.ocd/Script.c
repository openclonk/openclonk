/**
	Cable Car
	Library object for the cable car.

	@author Randrian, Clonkonaut, Maikel

	Cable cars must set up a movement speed using SetCableSpeed(x);
	Cable cars handle movement on their own. In order to move along a rail, cable cars must call DoMovement() regularly.
	E.g. using AddTimer("DoMovement", 1);
*/


// The cable car is the consumer, but relays to the crossings for power supply.
#include Library_PowerConsumer


// The speed with which the car travels along rails
local lib_ccar_speed;
// The rail (cable or crossing) that is currently traveled along or stayed at
local lib_ccar_rail;
// The direction (0 or 1) the rail is travelled along, translates into the cable's action targets (of which there should be two!)
local lib_ccar_direction;
// The travel progress on the current rail
local lib_ccar_progress;
// The length of the rail
local lib_ccar_max_progress;
// This target point for pathfinding
local lib_ccar_destination;
// Current delivery the car is on, array: [starting station, target station, requested objects, amount]
local lib_ccar_delivery;
// Whether the cable car has power currently and the network crossing it is being provided by.
local lib_ccar_has_power;
local lib_ccar_power_crossing;
local lib_ccar_requesting_power;


/*-- Overloads --*/

// Overload these functions as you feel fit

// Called after the car is attached to a rail
public func Engaged() {}

// Called after the car is detached from the rail
public func Disengaged() {}

// To offset the position on the cable from the object's center
// position is a 2-value-array [x, y]
// prec is nil or a value to multiply your calculations with
public func GetCableOffset(array position, int prec) {}

// To add custom interaction menu entries after the regular cable car entries
// custom_entry is a prototype for proper spacing of buttons
// Use priorities > 2000 just to be sure
public func GetCableCarExtraMenuEntries(array menu_entries, proplist custom_entry, object clonk) {}

// Whenever movement is about to begin
// Movement data like lib_ccar_direction is still nil at this moment
public func OnStart() {}

// Whenever the car stops its movement
// failed is true if the movement to a destination was cancelled (usually because the path broke in the meantime)
public func OnStop(bool failed) {}


/*-- Interface --*/

// Sets the speed of the cable car
public func SetCableSpeed(int value)
{
	lib_ccar_speed = value;
}

// Returns the speed of this cable car.
public func GetCableSpeed() { return lib_ccar_speed; }

// Positioning of the car along the cable
// This should be called regularly, see header comment!
public func DoMovement()
{
	if (!GetRailTarget()) return;
	if (lib_ccar_destination == nil) return;
	if (lib_ccar_direction == nil) return;
	if (!lib_ccar_has_power)
	{
		if (!lib_ccar_requesting_power)
		{
			RegisterPowerRequest(GetNeededPower());
			lib_ccar_requesting_power = true;
		}
		else
		{
			CheckPowerCrossing();
		}
		return;
	}

	var start = 1;
	var end = 0;
	if (lib_ccar_direction == 1)
	{
		start = 0;
		end = 1;
	}

	lib_ccar_progress += lib_ccar_speed;
	var position = CreateArray(2);
	if (lib_ccar_progress >= lib_ccar_max_progress)
	{
		lib_ccar_rail->~Deactivation(1);
		lib_ccar_rail->~RemoveHangingCableCar(this);
		lib_ccar_rail = lib_ccar_rail->GetActionTarget(end);
		lib_ccar_rail->GetCablePosition(position);
		GetCableOffset(position);
		SetPosition(position[0], position[1]);
		lib_ccar_direction = nil;
		CrossingReached();
		// Reached crossing, thus unregister power request.
		UnregisterPowerRequest();
		lib_ccar_has_power = false;
		lib_ccar_requesting_power = false;
		return;
	}

	var prec = 100;
	var origin = CreateArray(2), ending = CreateArray(2);
	lib_ccar_rail->GetActionTarget(start)->GetCablePosition(origin, prec);
	lib_ccar_rail->GetActionTarget(end)->GetCablePosition(ending, prec);
	position[0] = origin[0] + (ending[0] - origin[0]) * lib_ccar_progress / lib_ccar_max_progress;
	position[1] = origin[1] + (ending[1] - origin[1]) * lib_ccar_progress / lib_ccar_max_progress;
	GetCableOffset(position, prec);
	SetPosition(position[0], position[1], 1, prec);
}


/*-- Status --*/

public func IsCableCar() { return true; }

public func RejectCableCarPickup() { return true; }

public func GetRailTarget() { return lib_ccar_rail; }

public func IsTravelling() { return lib_ccar_destination; }


/*-- Interaction --*/

// Provides an own interaction menu.
public func HasInteractionMenu() { return true; }

// Show settins in interaction menu
public func GetInteractionMenus(object clonk)
{
	var menus = _inherited(clonk, ...) ?? [];
	var cablecar_menu =
	{
		title = "$CableCarOptions$",
		entries_callback = this.GetCableCarMenuEntries,
		callback = nil,
		callback_hover = "OnCableCarHover",
		callback_target = this,
		BackgroundColor = RGB(0, 0, 50),
		Priority = 20
	};
	PushBack(menus, cablecar_menu);
	return menus;
}

public func GetCableCarMenuEntries(object clonk)
{
	var control_prototype =
	{
		BackgroundColor = { Std = 0, Selected = RGB(100, 30, 30) },
		OnMouseIn = GuiAction_SetTag("Selected"),
		OnMouseOut = GuiAction_SetTag("Std"),
		Right = "2em"
	};

	var custom_entry =
	{
		Right = "3em", Bottom = "2em",
		image = { Prototype = control_prototype },
		icon = { Left = "2em" }
	};

	var menu_entries = [];

	// Clickable buttons
	if (!GetRailTarget())
	{
		// Engaging onto a rail
		var stations = FindObjects(Find_AtPoint(), Find_Func("IsCableStation"));
		var i = 0;
		for (var station in stations)
		{
			var engage = new custom_entry {
				Priority = 1000 + i,
				Tooltip = "$TooltipEngage$",
				OnClick = GuiAction_Call(this, "EngageRail", station),
				image = { Prototype = custom_entry.image, Symbol = station },
				icon = { Prototype = custom_entry.icon, Symbol = Icon_LibraryCableCar, GraphicsName = "Engage" }
			};
			PushBack(menu_entries, { symbol = station, extra_data = "Engage", custom = engage });
			i++;
		}
		// No station present
		if (i == 0)
		{
			var search = {
				Priority = 1000,
				Right = "100%", Bottom = "2em",
				text = { Text = "$NoStation$", Style = GUI_TextVCenter | GUI_TextHCenter}
			};
			PushBack(menu_entries, { symbol = this, extra_data = "NoStation", custom = search });
		}
	}
	else
	{
		// Start the trip
		if (!IsTravelling())
		{
			var go = new custom_entry {
				Priority = 1000,
				Tooltip = "$TooltipGo$",
				OnClick = GuiAction_Call(this, "OpenDestinationSelection", clonk),
				image = { Prototype = custom_entry.image, Symbol = Icon_Play }
			};
			PushBack(menu_entries, { symbol = this, extra_data = "Go", custom = go });

			var disengage = new custom_entry {
				Priority = 1001,
				Tooltip = "$TooltipDisengage$",
				OnClick = GuiAction_Call(this, "DisengageRail"),
				image = { Prototype = custom_entry.image, Symbol = GetRailTarget() },
				icon = { Prototype = custom_entry.icon, Symbol = Icon_LibraryCableCar, GraphicsName = "Disengage" }
			};
			PushBack(menu_entries, { symbol = GetRailTarget(), extra_data = "Disengage", custom = disengage });
		}
	}
	// Add custom entries
	GetCableCarExtraMenuEntries(menu_entries, custom_entry, clonk);

	return menu_entries;
}

public func OnCableCarHover(symbol, extra_data, desc_menu_target, menu_id)
{
	if (symbol == nil) return;

	var text = "";
	if (extra_data == "Engage")
		text = Format("$DescEngage$", GetName(), symbol->GetName());
	if (extra_data == "Go")
		text = "$DescGo$";
	if (extra_data == "Disengage")
		text = Format("$DescDisengage$", GetName(), symbol->GetName());
	GuiUpdate({ Text = text }, menu_id, 1, desc_menu_target);
}


/*-- Travelling --*/

// Called when the network is updated.
public func OnRailNetworkUpdate()
{
	// Remove any power request so that the car can find a new source if needed.
	UnregisterPowerRequest();
	lib_ccar_has_power = false;
	lib_ccar_requesting_power = false;
	// The car may have been stuck on a request, continue it now.
	ContinueRequest();
	return;
}

// Attach the car onto a crossing
public func EngageRail(object crossing, bool silent)
{
	if (!crossing->~IsCableCrossing())
		return false;

	var position = CreateArray(2);
	crossing->GetCablePosition(position);
	GetCableOffset(position);
	SetPosition(position[0], position[1]);
	SetSpeed(0, 0);
	SetR(0);
	SetRDir(0);
	SetComDir(COMD_None);
	lib_ccar_rail = crossing;
	lib_ccar_direction = nil;
	if (!silent)
		Sound("Objects::Connect");
	UpdateInteractionMenus(this.GetCableCarMenuEntries);

	Engaged();
	lib_ccar_rail->~OnCableCarEngaged(this);
}

// Detach the car from its current holding point (cable or crossing, does not matter)
public func DisengageRail()
{
	lib_ccar_rail = nil;
	lib_ccar_direction = nil;
	lib_ccar_progress = 0;
	lib_ccar_max_progress = 0;
	lib_ccar_destination = nil;
	UpdateInteractionMenus(this.GetCableCarMenuEntries);

	Disengaged();
	if (lib_ccar_rail)
		lib_ccar_rail->~OnCableCarDisengaged(this);
}

// Sets a target point for travelling and starts the movement process
public func SetDestination(dest)
{
	if (GetType(dest) == C4V_Int)
	{
		dest = FindObjects(Find_Func("IsCableCrossing"))[dest];
		if (!dest)
			return;
	}

	lib_ccar_destination = dest;
	if (lib_ccar_direction == nil)
	{
		CrossingReached();
		OnStart();
	}
}

// Whenever a crossing is reached it must be queried for the next crossing to go to
public func CrossingReached()
{
	if (lib_ccar_destination != lib_ccar_rail)
	{
		var target = lib_ccar_rail->GetNextWaypoint(lib_ccar_destination);
		if (target)
			MoveTo(target);
		else
			DestinationFailed();
	}
	// Destination reached
	else
	{
		DestinationReached();
	}
}

// When the current destination is reached
public func DestinationReached()
{
	lib_ccar_destination = nil;
	lib_ccar_direction = nil;
	lib_ccar_progress = 0;
	lib_ccar_max_progress = 0;

	OnStop(false);

	if (lib_ccar_rail)
	{
		if (lib_ccar_delivery)
			FinishedRequest(lib_ccar_rail);
		lib_ccar_rail->~OnCableCarArrival(this);
	}
}

// When the way to the current destination has vanished somehow
public func DestinationFailed()
{
	if (lib_ccar_rail)
		lib_ccar_rail->~OnCableCarStopped(this);
	
	lib_ccar_destination = nil;
	lib_ccar_direction = nil;
	lib_ccar_progress = 0;
	lib_ccar_max_progress = 0;

	OnStop(true);
}

public func Destruction()
{
	if (lib_ccar_rail)
		lib_ccar_rail->~OnCableCarDestruction(this);
}

public func MoveTo(object dest)
{
	// Find cable that is connected to both initial and final crossing.
	var rail = FindObject(Find_Func("IsConnectedTo", lib_ccar_rail), Find_Func("IsConnectedTo", dest));
	if (!rail)
		return DestinationFailed(); // Shouldn't happen

	// Notify crossing a cable car has left.
	if (lib_ccar_rail)
		lib_ccar_rail->~OnCableCarDeparture(this);

	// Target the first or second action target?
	if (rail->GetActionTarget(0) == dest)
		lib_ccar_direction = 0;
	else
		lib_ccar_direction = 1;
	lib_ccar_progress  = 0;
	var origin = CreateArray(2), ending = CreateArray(2);
	rail->GetActionTarget(0)->GetCablePosition(origin);
	rail->GetActionTarget(1)->GetCablePosition(ending);
	rail->~Activation(1);
	rail->~AddHangingCableCar(this);
	lib_ccar_max_progress = Distance(origin[0], origin[1], ending[0], ending[1]);
	lib_ccar_rail = rail;
	return;	
}


/*-- Destination selection --*/

public func OpenDestinationSelection(object clonk)
{
	if (!clonk) return;
	if (!GetRailTarget()) return;

	// Close interaction menu
	if (clonk->GetMenu())
		if (!clonk->TryCancelMenu())
			return;

	GUI_DestinationSelectionMenu->CreateFor(clonk, this, GetRailTarget());
}


/*-- Power consumption --*/

private func GetNeededPower() { return 10; }

public func GetConsumerPriority() { return 55; }

public func GetActualPowerConsumer()
{
	// The actual power consumer can be any crossing in the network. We want to find the most suitable one.
	// But only update it if there is currently no request being made, because otherwise it might unregister
	// in the wrong network.
	if (lib_ccar_rail && !lib_ccar_requesting_power)
	{
		var power_crossing = GetClosestCrossing();
		lib_ccar_power_crossing = GetBestPowerCrossing(power_crossing);
	}
	return lib_ccar_power_crossing;
}

// Returns the closest crossing the car is connected to.
public func GetClosestCrossing()
{
	var power_crossing = lib_ccar_rail;
	if (!power_crossing->~IsCableCrossing())
		power_crossing = power_crossing->GetActionTarget(0) ?? power_crossing->GetActionTarget(1);
	return power_crossing;
}

// Returns the best crossing in the network of this crossing.
public func GetBestPowerCrossing(object crossing)
{
	// Get all connected crossings.
	var network_crossings = [crossing];
	for (var item in crossing->GetDestinations())
	{
		if (!item)
			continue;
		PushBack(network_crossings, item[Library_CableStation.const_finaldestination]);			
	}
	// Find the crossing with the most positive power balance.
	var best_crossing = crossing;
	var power_overflow = nil;
	for (var test_crossing in network_crossings)
	{
		var power_network = Library_Power->GetPowerNetwork(test_crossing);
		if (power_network->IsNeutralNetwork())
			continue;
		var overflow = power_network->GetBarePowerAvailable() - power_network->GetPowerConsumptionNeed();
		if (overflow > power_overflow || power_overflow == nil)
		{
			best_crossing = test_crossing;		
			power_overflow = overflow;
		}
	}
	return best_crossing;
}

// Checks whether the current crossing is optimal for powering this cable car.
public func CheckPowerCrossing()
{
	if (lib_ccar_requesting_power)
	{
		var power_crossing = GetClosestCrossing();
		if (lib_ccar_power_crossing != GetBestPowerCrossing(power_crossing))
		{
			// Unregister current power request such that a new crossing can be chosen.
			UnregisterPowerRequest();
			lib_ccar_has_power = false;
			lib_ccar_requesting_power = false;
		}
	}
	return;
}

public func OnNotEnoughPower(int amount, bool initial_call)
{
	lib_ccar_has_power = false;
	// Check the need for updating to a new crossing as the power source.
	if (!initial_call)
		CheckPowerCrossing();
	return _inherited(...);
}

public func OnEnoughPower(int amount)
{
	lib_ccar_has_power = true;
	// Do movement again because this frame there would be no movement otherwise.
	DoMovement();
	// The car may have been stuck on a request, continue it now.
	ContinueRequest();
	return _inherited(...);
}


/*-- Delivery --*/

public func AddRequest(proplist order, object target, object source)
{
	//Log("[%d]AddRequest(car %v at station %v) %v->%v->%v", FrameCounter(), this, lib_ccar_rail, source, order, target);
	lib_ccar_delivery = 
	{
		source = source,
		target = target,
		order = order
	};
	// First move to source if not already there.
	if (lib_ccar_rail != lib_ccar_delivery.source)
		return SetDestination(lib_ccar_delivery.source);
	// Was at source already so move to target directly.
	lib_ccar_delivery.source = nil;
	SetDestination(lib_ccar_delivery.target);
}

public func ContinueRequest()
{
	if (!lib_ccar_delivery)
		return;
	//Log("[%d]ContinueRequest(car %v currently at station %v) %v", FrameCounter(), this, lib_ccar_rail, lib_ccar_delivery.order);
	// Continue moving to source or target.
	SetDestination(lib_ccar_delivery.source ?? lib_ccar_delivery.target);
}

public func FinishedRequest(object station)
{
	if (!lib_ccar_delivery)
		return;
	//Log("[%d]FinishedRequest(car %v at station %v) %v", FrameCounter(), this, station, lib_ccar_delivery.order);
	// May have first arrived at the source station.
	if (lib_ccar_delivery.source && station == lib_ccar_delivery.source)
	{
		// Load requested objects and move to target.
		lib_ccar_delivery.source = nil;
		station->RequestPickUp(this, lib_ccar_delivery.order);
		SetDestination(lib_ccar_delivery.target);
		return;
	}
	// Arrived at delivery target station.
	if (station == lib_ccar_delivery.target)
	{
		station->RequestArrived(this, lib_ccar_delivery.order);
		lib_ccar_delivery = nil;
		return;
	}
}


/*-- Saving --*/

public func SaveScenarioObject(proplist props)
{
	if (!inherited(props, ...))
		 return false;
	if (lib_ccar_speed != nil) 
		props->AddCall("Speed", this, "SetCableSpeed", lib_ccar_speed);
	if (lib_ccar_rail)
		props->AddCall("Rail", this, "EngageRail", lib_ccar_rail, true);
	return true;
}

