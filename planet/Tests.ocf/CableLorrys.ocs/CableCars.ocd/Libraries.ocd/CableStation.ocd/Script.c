/**
	Cable Station
	Library for cable stations and crossings. This is included by 
	cable crossings, and should be included by structures which
	want to make use of the cable network.
	
	@author Randrian, Clonkonaut, Maikel
*/

#appendto ToolsWorkshop
#appendto WoodenCabin

// Set to true to make this a station
local is_station;

/*-- State --*/

/** This object is a cable crossing
* E.g. checked by whatever object wants to connect a cable (so it does not mean there is a cable connected!)
*/
public func IsCableCrossing() { return true; }
/** Returns whether or not this object is a cable station
* A station is a possible (i.e. selectable) destination for cable cars (whereas normal crossings do not appear in the destination selection process)
*/
public func IsCableStation() { return is_station; }

// For setting up the cable
public func GetCablePosition(array coordinates, int prec)
{
	if (!prec) prec = 1;
	coordinates[0] = GetX(prec);
	coordinates[1] = GetY(prec);
	if (this->~GetCableXOffset()) coordinates[0] += this->~GetCableXOffset() * prec;
	if (this->~GetCableYOffset()) coordinates[1] += this->~GetCableYOffset() * prec;
}

/* Local */

// Stores the next crossing (waypoint) to take when advancing to a certain final point
// Scheme (2D array): [Desired final point, Next waypoint to take, Distance (not airline!) until final point]
local destination_list;
// According to this scheme, some constants for easy reading
local const_finaldestination; // :D
local const_nextwaypoint;
local const_distance;

protected func Initialize() // 
{
	const_finaldestination = 0;
	const_nextwaypoint = 1;
	const_distance = 2;
	destination_list = [];
	return _inherited(...);
}

/* Pathfinding for cable cars */

/** Returns the waypoint to take next for the desired final point \a end
* @param end The final destination for the information is queried
*/
public func GetNextWaypoint(object end)
{
  if (!destination_list) return nil;
	for (var item in destination_list)
	{
		if (!item) continue;
		if (item[const_finaldestination] == end)
			return item[const_nextwaypoint];
	}
	return nil;
}

/** Returns the actual traveling distance for the desired final point \a end
* This is not the airline distance but the length of all cables to take via traveling
* @param end The final destination for the information is queried
*/
public func GetLengthToTarget(object end)
{
	if (!destination_list) return nil;
	for (var item in destination_list)
	{
		if (!item) continue;
		if (item[const_finaldestination] == end)
			return item[const_distance];
	}
	return nil;
}

/** Returns the destination array
*/
public func GetDestinations()
{
	return destination_list[:];
}

/* Set up pathfinding information */

/** Adds a new connection via the cable \a cable to this crossing
* Does nothing if the other connected objects of the cable is not a cable crossing
* @param cable The newly connected cable
*/
public func AddCableConnection(object cable)
{
	// Failsafe
	if (!cable || ! cable->~IsCableLine())
		return false;
	// Line setup finished?
	var other_crossing = cable->~GetOtherConnection(this);
	if (! other_crossing->~IsCableCrossing())
		return false;
	// Acquire destinations of the other crossing, all these are now in reach
	AddCableDestinations(other_crossing->GetDestinations(), other_crossing);
	// Send own destinations, now in reach for the other one
	other_crossing->AddCableDestinations(destination_list[:], this);
	// Awesome, more power to the network!
	CheckRailStation();
	return true;
}

/** Adds a whole list of destinations to the crossing
* @param new_list The new destination list, formated like a crossing's normal destination list
* @param crossing The crossing where this list comes from
*/
public func AddCableDestinations(array new_list, object crossing)
{
	// Append crossing to the list
	if (crossing)
	{
		new_list[GetLength(new_list)] = [crossing, crossing];
		// This value is to be added to every distance
		var distance_add = ObjectDistance(crossing);
	}
	else
		return false; // Does not compute
	// Check every new destination
	for (var list_item in new_list)
	{
		if (!list_item) continue;
		// Destination is this
		if (list_item[const_finaldestination] == this) continue;
		// Check whether the destination is already in reach
		var handled = false;
		for (var i = 0, destination = false; i < GetLength(destination_list); i++)
		{
			if (!destination_list[i]) continue;
			destination = destination_list[i];
			if (destination[const_finaldestination] == list_item[const_finaldestination])
			{
				// Already known destination, check whether the new path is shorter
				handled = true;
				if (destination[const_distance] > list_item[const_distance] + distance_add)
				{
					// It is shorter, replace, route through crossing
					destination_list[i] = [list_item[const_finaldestination], crossing, list_item[const_distance] + distance_add];
					// Inform the destination
					list_item[const_finaldestination]->UpdateCableDestination(this, crossing, distance_add);
				}
			}
		}
		// Destination is replaced or to be dismissed (because the new path would be longer), do nothing
		if (handled) continue;
		// Destination is a new one, add to the list
		destination_list[GetLength(destination_list)] = [list_item[const_finaldestination], crossing, list_item[const_distance] + distance_add];
		// Add me to the new destination (the way to me is the same than to crossing)
		if (list_item[const_finaldestination] != crossing)
			list_item[const_finaldestination]->AddCableDestination(this, crossing, distance_add);
	}
	CheckRailStation();
	return true;
}

/** Adds a single destination \a new_destination. The link is \a crossing; \a crossing should already be known, otherwise it returns false.
* @param new_destination The destination to add
* @param crossing The crossing which links to the new destination
* @param distance_add The distance between crossing and new_destination
*/
public func AddCableDestination(object new_destination, object crossing, int distance_add)
{
	// Failsafe
	if (!new_destination || !crossing) return false;
	if (new_destination == this) return false;
	// Find the entry of crossing to get the next waypoint and the distance
	var crossing_item;
	for (var list_item in destination_list)
	{
		if (!list_item) continue;
		if (list_item[const_finaldestination] == crossing)
		{
			crossing_item = list_item;
			break;
		}
	}
	// Failsafe²
	if (!crossing_item) return false;
	// Save the new destination
	destination_list[GetLength(destination_list)] = [new_destination, crossing_item[const_nextwaypoint], crossing_item[const_distance] + distance_add];
	CheckRailStation();
	return true;
}

/** Updates the path to \a known_destination via \a crossing (e.g. because the path is shorter through \a crossing)
* @param known_destination The destination to update
* @param crossing The crossing which links to the destination
* @param distance_add The distance between crossing and known_destination
*/
public func UpdateCableDestination(object known_destination, object crossing, int distance_add)
{
	// Failsafe
	if (!known_destination || !crossing) return false;
	if (known_destination == crossing) return false;
	// Find the entries of crossing and known_destination
	var crossing_item, destination_item, i = 0;
	for (var list_item in destination_list)
	{
		if (!list_item) continue;
		if (list_item[const_finaldestination] == crossing)
			crossing_item = list_item;
		if (list_item[const_finaldestination] == known_destination)
			destination_item = i;
		i++;
	}
	// Failsafe²
	if (!crossing_item || !destination_item) return false;
	// Save the updated path
	destination_list[destination_item][const_nextwaypoint] = crossing_item[const_nextwaypoint];
	destination_list[destination_item][const_distance] = crossing_item[const_distance] + distance_add;
	CheckRailStation();
	return true;
}

local clearing;

/* Called automatically by Destruction, see description there
* @param crossing The calling crossing
*/
public func ClearConnections(object crossing)
{
	if (clearing) return;
	clearing = true;
	destination_list = [];
	for (var connection in FindObjects(Find_Func("IsConnectedTo", this)))
	{
		if (! connection->~IsCableLine()) continue;
		var other_crossing = connection->~GetOtherConnection(this);
		if (! other_crossing->~IsCableCrossing()) continue;
		other_crossing->ClearConnections();
	}
}

/* Called automatically by Destruction, see description there
* @param crossing The calling crossing
*/
public func RenewConnections(object crossing)
{
	if (!clearing) return;
	clearing = false;
	for(var connection in FindObjects(Find_Func("IsConnectedTo", this)))
	{
		if (! connection->~IsCableLine()) continue;
		var other_crossing = connection->~GetOtherConnection(this);
		if (! other_crossing->~IsCableCrossing()) continue;
		if (other_crossing == crossing) continue;
		destination_list[GetLength(destination_list)] = [other_crossing, other_crossing, ObjectDistance(other_crossing)];
		AddCableDestinations(other_crossing->GetDestinations(), other_crossing);
	}
}

/* Station behaviour */

// Prevents the automatic change of the stations status when set to station mode
local bManualSetting;

// IsInteractable is stored in the appendto definitions!

// A clonk wants to change my station status
public func Interact(object pClonk)
{
	// Check ownership
	if (GetOwner() != NO_OWNER && GetOwner() != pClonk->GetOwner()) return false;
	// Clonk pushes a cable car?
	if (pClonk->GetActionTarget() && pClonk->GetActionTarget()->~IsCableCar())
	{
		var car = pClonk->GetActionTarget();
		// Disengage
		if (car->GetRailTarget() == this)
		{
			car->DisengageRail();
			return true;
		}
		// Engage
		car->EngageRail(this);
		car->SelectDestination(pClonk, this);
		return true;
	}
	// Change status
	if (is_station)
		bManualSetting = false;
	else
		bManualSetting = true;
	CheckRailStation();
	return _inherited(...);
}

public func GetInteractionMetaInfo(object clonk)
{
	if (is_station)
		return {IconID = Library_CableStation, IconName = "UnsetStation", Description = "$UnsetStationDesc$"};
	else
		return {IconID = Library_CableStation, IconName = "SetStation", Description = "$SetStationDesc$"};
}

/* Animation stuff */

/** Overload me to do wheel animation
	@return \c nil.
*/
protected func TurnWheel()
{
	/* EMPTY: Should be overloaded */
	return;
}

local iActiveCount;
// Start/End animation
// Call AddActive(0) if any next waypoint wants to start the animation (because a cable car is passing by)
// Call AddActive(1) if the cable car passed the line, stopping the animation
// Counts up to multiple animated connections
public func AddActive(fRemove)
{
  if(!fRemove)
   iActiveCount++;
  else
   iActiveCount--;
  if(iActiveCount <= 0 && GetAction() == "Active")
    SetAction("Wait");
  if(iActiveCount > 0  && GetAction() == "Wait")
    SetAction("Active");
}

/** Overload me to check station behaviour
	@return \c nil.
*/
private func CheckRailStation()
{
	/* EMPTY: Should be overloaded */
	return _inherited(...);
}

/* Destruction */

/* Removes this crossing from the network
It first clears every waypoint from the network and then renews the whole information.
Optimisation welcome!
*/
protected func Destruction()
{
	for (var connection in FindObjects(Find_Func("IsConnectedTo", this)))
	{
		if (! connection->~IsCableLine()) continue;
		var other_crossing = connection->~GetOtherConnection(this);
		if (! other_crossing->~IsCableCrossing()) continue;
		other_crossing->ClearConnections(this);
	}
	for (var connection in FindObjects(Find_Func("IsConnectedTo", this)))
	{
		if (! connection->~IsCableLine()) continue;
		var other_crossing = connection->~GetOtherConnection(this);
		if (! other_crossing->~IsCableCrossing()) continue;
		other_crossing->RenewConnections(this);
	}
}

/*-- Auto production --*/

public func CheckAcquire(id object_id, int amount)
{
	var container = false;
	for (var list_item in destination_list)
		if (list_item[const_finaldestination]->CheckAvailability(object_id, amount))
		{
			if (!container) container = list_item[const_finaldestination];
			else if (GetLengthToTarget(container) > GetLengthToTarget(list_item[const_finaldestination]))
				container = list_item[const_finaldestination];
		}
	return container;
}

public func CheckAcquiringCar(object target)
{
	var car = false;
	if (!target) target == this;
	for (var car_to_check in FindObjects(Find_Func("IsCableCar")))
	{
		if (car_to_check->GetRailTarget() == target)
		{
			if (car_to_check->IsAvailable())
				return car_to_check;
		}
		else if (car_to_check->GetRailTarget() == this)
			if (car_to_check->IsAvailable())
				car = car_to_check;
	}
	return car;
}

public func DoAcquire(id object_id, int amount)
{
	// Check if the objects are available
	var container = CheckAcquire(object_id, amount);
	if (!container) return false;
	// Check if a cable car is available
	var car = CheckAcquiringCar(container);
	if (!car) return false;
	container->AddReservation(object_id, amount);
	car->AddDelivery(container, object_id, amount);
	return true;
}

public func DeliveryDone(id object_id, int amount)
{
	// Nothing so far
}

/*-- Object requests and deliveries --*/

local reservations; // two dimensional array with ongoing deliveries: [[ID, amount]]

public func CheckAvailability(id object_id, int amount)
{
	if (ContentsCount(object_id) < amount) return false;
	var reserved = 0;
	for (var reservation in reservations)
		if (reservation[0] == object_id)
			reserved += reservation[1];
	if (ContentsCount(object_id) - reserved < amount)
		return false;
	return true;
}

public func AddReservation(id object_id, int amount)
{
	reservations[GetLength(reservations)] = [object_id, amount];
}

public func RemoveReservation(id object_id, int amount)
{
	var new_list = [], removed = false;
	for (var reservation in reservations)
	{
		if (reservation[0] == object_id && reservation[1] == amount && !removed)
		{
			removed = true;
			continue;
		}
		new_list[GetLength(new_list)] = reservation;
	}
	reservations = new_list;
}

public func AddRequest(id requested_id, int amount, id requested_object)
{
	if (!requested_id && !requested_object) return false;
	if (!amount) amount = 1;
}

public func RequestObjects(object requester, id request_id, int amount)
{
	if (ContentsCount(request_id) < amount)
		return false;
	for (var i = 0; i < amount; i++)
		FindContents(request_id)->Enter(requester);
	return true;
}