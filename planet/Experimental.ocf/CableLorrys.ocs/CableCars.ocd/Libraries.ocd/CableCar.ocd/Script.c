/**
	Cable Car
	Library object for the cable car.

	@author Randrian, Clonkonaut, Maikel

	Cable cars must set up a movement speed using SetCableSpeed(x);
	Cable cars handle movement on their own. In order to move along a rail, cable cars must call DoMovement() regularly.
	E.g. using AddTimer("DoMovement", 1);
*/

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

/*--- Overloads ---*/

// Overload these functions as you feel fit

// Called after the car is attached to a rail
func Engaged() {}

// Called after the car is detached from the rail
func Disengaged() {}

// To offset the position on the cable from the object's center
// position is a 2-value-array [x,y]
// prec is nil or a value to multiply your calculations with
func GetCableOffset(array position, int prec) {}

// To add custom interaction menu entries after the regular cable car entries
// custom_entry is a prototype for proper spacing of buttons
// Use priorities > 2000 just to be sure
func GetCableCarExtraMenuEntries(array menu_entries, proplist custom_entry, object clonk) {}

// Whenever movement is about to begin
// Movement data like lib_ccar_direction is still nil at this moment
func OnStart() {}

// Whenever the car stops its movement
// failed is true if the movement to a destination was cancelled (usually because the path broke in the meantime)
func OnStop(bool failed) {}

/*--- Interface ---*/

// Sets the speed of the cable car
public func SetCableSpeed(int value)
{
	lib_ccar_speed = value;
}

// Positioning of the car along the cable
// This should be called regularly, see header comment!
public func DoMovement()
{
	if (!GetRailTarget()) return;
	if (lib_ccar_destination == nil) return;
	if (lib_ccar_direction == nil) return;

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
		lib_ccar_rail = lib_ccar_rail->GetActionTarget(end);
		lib_ccar_rail->GetCablePosition(position);
		GetCableOffset(position);
		SetPosition(position[0], position[1]);
		lib_ccar_direction = nil;
		CrossingReached();
		return;
	}

	var prec = 100;
	var origin = CreateArray(2), ending = CreateArray(2);
	lib_ccar_rail->GetActionTarget(start)->GetCablePosition(origin, prec);
	lib_ccar_rail->GetActionTarget(end)->GetCablePosition(ending, prec);
	position[0] = origin[0] + (ending[0] - origin[0]) * lib_ccar_progress/lib_ccar_max_progress;
	position[1] = origin[1] + (ending[1] - origin[1]) * lib_ccar_progress/lib_ccar_max_progress;
	GetCableOffset(position, prec);
	SetPosition(position[0], position[1], 1, prec);
}

/*--- Status ---*/

public func IsCableCar() { return true; }

public func GetRailTarget() { return lib_ccar_rail; }

public func IsTravelling() { return lib_ccar_destination; }

/* Interaction */

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
	} else {
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

	GuiUpdate({ Text = text }, menu_id, 1, desc_menu_target);
}

/*--- Travelling ---*/

// Attach the car onto a crossing
public func EngageRail(object crossing, bool silent)
{
	if (! crossing->~IsCableCrossing()) return false;

	var position = CreateArray(2);
	crossing->GetCablePosition(position);
	GetCableOffset(position);
	SetPosition(position[0], position[1]);
	SetSpeed(0,0);
	SetR(0);
	SetRDir(0);
	SetComDir(COMD_None);
	lib_ccar_rail = crossing;
	lib_ccar_direction = nil;
	if (!silent) Sound("Objects::Connect");
	UpdateInteractionMenus(this.GetCableCarMenuEntries);

	Engaged();
	lib_ccar_rail->OnCableCarEngaged(this);
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
	if (lib_ccar_rail) lib_ccar_rail->OnCableCarDisengaged(this);
}

// Sets a target point for travelling and starts the movement process
public func SetDestination(dest)
{
	if(GetType(dest) == C4V_Int)
	{
		dest = FindObjects(Find_Func("IsCableCrossing"))[dest];
		if (!dest) return;
	}

	lib_ccar_destination = dest;

	if(lib_ccar_direction == nil)
	{
		OnStart();
		CrossingReached();
		if (lib_ccar_rail)
			lib_ccar_rail->~OnCableCarDeparture(this);
	}
}

// Whenever a crossing is reached it must be queried for the next crossing to go to
func CrossingReached()
{
	var target;
	if(lib_ccar_destination != lib_ccar_rail)
	{
		if(target = lib_ccar_rail->GetNextWaypoint(lib_ccar_destination))
			MoveTo(target);
		else
			DestinationFailed();
	}
	// Destination reached
	else {
		DestinationReached();
	}
}

// When the current destination is reached
func DestinationReached()
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
		lib_ccar_rail->OnCableCarArrival(this);
	}
}

// When the way to the current destination has vanished somehow
func DestinationFailed()
{
	lib_ccar_destination = nil;
	lib_ccar_direction = nil;
	lib_ccar_progress = 0;
	lib_ccar_max_progress = 0;

	OnStop(true);
}

// Setup movement process
func MoveToIndex(int dest)
{
	var dest_obj = FindObjects(Find_Func("IsCableCrossing"))[dest];
	if (dest_obj) return MoveTo(dest_obj);
}

public func MoveTo(object dest)
{
	var rail = 0;
	for(var test_rail in FindObjects(Find_Func("IsConnectedTo", lib_ccar_rail)))
	{
		if(test_rail->IsConnectedTo(dest))
		{
			rail = test_rail;
			break;
		}
	}
	if(!rail)
		return DestinationFailed(); // Shouldn't happen

	// Target the first or second action target?
	if(rail->GetActionTarget(0) == dest)
		lib_ccar_direction = 0;
	else
		lib_ccar_direction = 1;
	lib_ccar_progress  = 0;
	var origin = CreateArray(2), ending = CreateArray(2);
	rail->GetActionTarget(0)->GetCablePosition(origin);
	rail->GetActionTarget(1)->GetCablePosition(ending);
	rail->~Activation(1);
	lib_ccar_max_progress = Distance(origin[0], origin[1], ending[0], ending[1]);
	lib_ccar_rail = rail;
}

/* Destination selection */

public func OpenDestinationSelection(object clonk)
{
	if (!clonk) return;
	if (!GetRailTarget()) return;

	var plr = clonk->GetOwner();
	// Close interaction menu
	if (clonk->GetMenu())
		if (!clonk->TryCancelMenu())
			return;

	GUI_DestinationSelectionMenu->CreateFor(clonk, this, GetRailTarget());
}

/*-- Delivery --*/

public func AddRequest(proplist requested, int amount, proplist target, proplist source)
{
	lib_ccar_delivery = [source, target, requested, amount];
	SetDestination(target);
}

func FinishedRequest(object station)
{
	if (station && lib_ccar_delivery)
		station->RequestArrived(this, lib_ccar_delivery[2], lib_ccar_delivery[3]);
	lib_ccar_delivery = nil;
}