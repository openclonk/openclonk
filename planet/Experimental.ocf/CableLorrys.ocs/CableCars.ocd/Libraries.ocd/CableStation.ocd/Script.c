/**
	Cable Station
	Library for cable stations and crossings. This is included by
	cable crossings, and should be included by structures which
	want to make use of the cable network.
	
	@author Randrian, Clonkonaut, Maikel
*/

/*--- Overloads ---*/

// Overload these functions as you feel fit

// This function is called whenever a change in the cable network occured, i.e. destinations have been added / removed.
private func DestinationsUpdated() { }

// Called by cable lines whenever a car starts travelling along a connected cable.
// Can be used to start animation or sounds or similar.
// count is a value indicating the amount of activations.
public func CableActivation(int count) { }

// Called likewise as Activation() whenever a car leaves the cable.
// count is a value indicating the amount of deactivations (e.g. a cable with more than one car broke).
public func CableDeactivation(int count) { }

// Called by arriving cable cars if this station is the final stop
public func OnCableCarArrival(object car) { }

// Called by cable cars if it stopped at this station (usually because of a problem)
public func OnCableCarStopped(object car) { }

// Called by departing cable cars if it just starts a new journey
public func OnCableCarDeparture(object car) { }

// Called by a cable car that has been hooked up to the rail at this station
public func OnCableCarEngaged(object car) { }

// Called by a cable car that has been taken off the rail at this station
public func OnCableCarDisengaged(object car) { }

// Called by a cable car that has been destroyed at this station
public func OnCableCarDestruction(object car) { }

// Called when a cable car wants to pick up resources for a delivery
public func OnCableCarPickUp(object car, proplist order) { }

// Called when a cable car with a requested delivery arrives
public func OnCableCarDelivery(object car, proplist order) { }

// Called by other stations to check if a certain object and amount are available for delivery at this station.
// Return true if there are means to collect the required amount.
public func IsAvailable(proplist order) { return false; }

// Called to get idle cable cars at this station.
public func GetIdleCars() { return []; }


/*--- Callbacks ---*/

// Be sure to always call these via _inherited(...);

public func Construction()
{
	destination_list = [];
	request_queue = [];
	return _inherited(...);
}

public func Destruction()
{
	// The connection with other stations is broken via the cable and the network updating is handled there.
	// So there is updating to be performed here.
	return _inherited(...);
}

/*--- Status ---*/

local lib_crossing_is_station;

/** This object is a cable crossing.
	E.g. checked by whatever object wants to connect a cable.
	Does not mean that there actually is a cable connected to this crossing.
*/
public func IsCableCrossing() { return true; }

/** This function should return true if this crossing is considered a station.
	A station is selectable as a target if a lorry is sent its way.
	Functional buildings should always be a station, the 'crossing' building only if set to.
*/
public func IsCableStation() { return lib_crossing_is_station; }

/*--- Interface ---*/

// For switching the station status
public func SetCableStation(bool station)
{
	lib_crossing_is_station = station;
}

// Returns the cable hookup position for proper positioning of a car along the line.
public func GetCablePosition(array coordinates, int prec)
{
	if (!prec)
		prec = 1;
	if (!coordinates)
		coordinates = [];
	coordinates[0] = GetX(prec);
	coordinates[1] = GetY(prec);
	if (this.LineAttach)
	{
		coordinates[0] += this.LineAttach[0] * prec;
		coordinates[1] += this.LineAttach[1] * prec;
	}
	return coordinates;
}

// Called by cable cars to retrieve selectable destinations for the destination selection menu.
// Returns the list of destinations sorted by shortest distance first.
public func GetDestinationList()
{
	var dest_list = [];
	for (var destination in destination_list)
		if (destination[const_finaldestination]->IsCableStation())
			PushBack(dest_list, [destination[const_finaldestination], destination[const_distance]]);
	SortArrayByArrayElement(dest_list, 1);
	for (var index = 0; index < GetLength(dest_list); index++)
		dest_list[index] = dest_list[index][0];
	return dest_list;
}

/*--- Maintaining the destination list ---*/

/* Functions:
	GetDestinations()
	AddCableConnection(object cable)
	AddCableDestinations(array new_list, object crossing)
	AddCableDestination(object new_destination, object crossing, int distance_add)
	ClearConnections()
	RenewConnections()
*/

/** Returns the destination array so it can be used by other crossings.
*/
public func GetDestinations()
{
	// This is a nested array, so ensure a proper deep copy is made.
	var deep_copy = [];
	for (var dest in destination_list)
		PushBack(deep_copy, dest[:]);
	return deep_copy;
}

// Stores the next crossing (waypoint) to take when advancing to a certain final point. The list may not contain the crossing itself.
// Scheme (2D array): [Desired final point, Next waypoint to take, Distance (not airline!) until final point].
local destination_list;

// Constants for easier script reading
// These correspond to the aforementioned values of destination_list
local const_finaldestination = 0; // :D
local const_nextwaypoint = 1;
local const_distance = 2;

/** Adds a new connection via the cable \a cable to this crossing
	Does nothing if the other connected object of the cable is not a cable crossing
	@param cable The newly connected cable
*/
public func AddCableConnection(object cable)
{
	// Failsafe
	if (!cable || !cable->~IsCableLine())
		return false;
	// Line setup finished?
	var other_crossing = cable->~GetConnectedObject(this);
	if (!other_crossing || !other_crossing->~IsCableCrossing())
		return false;
	// Acquire destinations of the other crossing, all these are now in reach
	AddCableDestinations(other_crossing->GetDestinations(), other_crossing);
	// Send own destinations, now in reach for the other one
	other_crossing->AddCableDestinations(this->GetDestinations(), this);
	// Destinations have been updated for this crossing.
	DestinationsUpdated();
	return true;
}

public func RemoveCableConnection(object cable)
{
	// It is easiest to just update all connections.
	UpdateConnections();
	// Awesome, more power to the network!
	DestinationsUpdated();
	return true;
}

/** Adds a whole list of destinations to the crossing
* @param new_list The new destination list, formated like a crossing's normal destination list
* @param crossing The crossing where this list comes from
*/
public func AddCableDestinations(array new_list, object crossing, bool has_reversed)
{
	if (!crossing)
		return false;
	// Append crossing itself to the list with zero distance.
	new_list[GetLength(new_list)] = [crossing, crossing, 0];
	// This value is to be added to every distance
	var distance_add = ObjectDistance(crossing);
	// Check every new destination
	for (var list_item in new_list)
	{
		if (!list_item)
			continue;
		// Destination may not be this crossing.
		if (list_item[const_finaldestination] == this)
			continue;
		// Check whether the destination is already in already in the list.
		var handled = false;
		for (var i = 0, destination = nil; i < GetLength(destination_list); i++)
		{
			if (!destination_list[i])
				continue;
			destination = destination_list[i];
			if (destination[const_finaldestination] == list_item[const_finaldestination])
			{
				// Already known destination, check whether the new path is shorter
				handled = true;
				if (destination[const_distance] > list_item[const_distance] + distance_add)
				{
					// It is shorter, replace, route through crossing
					destination_list[i] = [list_item[const_finaldestination], crossing, list_item[const_distance] + distance_add];
					// Inform the destination.
					list_item[const_finaldestination]->UpdateCableDestination(this, crossing, distance_add);
				}
			}
		}
		
		// Destination is replaced or to be dismissed (because the new path would be longer), do nothing.
		if (handled)
			continue;
			
		// Destination is a new one, add to the list.
		AddCableDestination(list_item[const_finaldestination], crossing, list_item[const_distance] + distance_add);
		// Add me to the new destination (the way to me is the same than to crossing).
		if (list_item[const_finaldestination] != crossing && !has_reversed)
		{
			// This new crossing may have a bunch of other connections that need to be explored and potentially added.
			// The new crossing is connected to this crossing via the crossing specified as the parameter to this function.
			// So for the new destinations we want to add, we need to add the distance to the crossing.
			var add_destinations = list_item[const_finaldestination]->GetDestinations();
			var add_dest_distance = 0;
			for (var dest in crossing->GetDestinations())
				if (dest[const_finaldestination] == list_item[const_finaldestination])
					add_dest_distance = dest[const_distance];
			for (var dest in add_destinations)
				dest[const_distance] += add_dest_distance;
			this->AddCableDestinations(add_destinations, crossing, has_reversed);
			
			// However, this crossing and its destinations must also be added in reverse to the new crossing found.
			var reverse_destinations = this->GetDestinations();
			PushBack(reverse_destinations, [this, crossing, 0]);
			var reverse_crossing = nil;
			for (var dest in list_item[const_finaldestination]->GetDestinations())
				if (dest[const_finaldestination] == crossing)
					reverse_crossing = dest[const_nextwaypoint];
			if (reverse_crossing == nil)
				return FatalError("AddCableDestinations(): reverse_crossing not found.");			
			var add_reverse_distance = distance_add;
			if (reverse_crossing != crossing)
			{
				for (var dest in reverse_crossing->GetDestinations())
					if (dest[const_finaldestination] == crossing)
						add_reverse_distance += dest[const_distance];
				if (add_reverse_distance == distance_add)
					return FatalError("AddCableDestinations(): reverse_distance not correct.");
			}
			for (var dest in reverse_destinations)
				dest[const_distance] += add_reverse_distance;
			list_item[const_finaldestination]->AddCableDestinations(reverse_destinations, reverse_crossing, true);
		}
	}
	DestinationsUpdated();
	return true;
}

/** Adds a single destination \a new_destination. The link is \a crossing; \a crossing should already be known, otherwise it returns false.
* @param new_destination The destination to add
* @param crossing The crossing which links to the new destination
* @param distance_add The distance between crossing and new_destination
*/
public func AddCableDestination(object new_destination, object crossing, int distance)
{
	// Failsafes.
	if (!new_destination || !crossing)
		return;
	if (new_destination == this)
		return;
	if (!IsDirectlyConnectToCrossing(crossing))
		return FatalError(Format("destination to %v for %v is not connected to crossing link %v.", new_destination, this, crossing));
	// Add to destination list.
	PushBack(destination_list, [new_destination, crossing, distance]);
	DestinationsUpdated();
	return;
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
		if (!list_item)
			continue;
		if (list_item[const_finaldestination] == crossing)
			crossing_item = list_item;
		if (list_item[const_finaldestination] == known_destination)
			destination_item = i;
		i++;
	}
	// Failsafe
	if (!crossing_item || !destination_item)
		return false;
	// Save the updated path
	destination_list[destination_item][const_nextwaypoint] = crossing_item[const_nextwaypoint];
	destination_list[destination_item][const_distance] = crossing_item[const_distance] + distance_add;
	DestinationsUpdated();
	return true;
}

local clearing;

// Updates the network for this crossing, this is called when a cable to this crossing has changed.
// It first clears every waypoint from the network and then renews the whole information.
public func UpdateConnections()
{
	// Clear this crossing and then other connected crossings.
	ClearConnections();
	RenewConnections();
	return;
}

// Called automatically by UpdateConnections, see description there.
public func ClearConnections()
{
	if (clearing)
		return;
	clearing = true;
	destination_list = [];
	for (var connection in FindObjects(Find_Func("IsConnectedTo", this)))
	{
		if (!connection->~IsCableLine())
			continue;
		var other_crossing = connection->~GetConnectedObject(this);
		if (!other_crossing || !other_crossing->~IsCableCrossing())
			continue;
		other_crossing->ClearConnections();
	}
}

// Called automatically by UpdateConnections, see description there.
public func RenewConnections()
{
	if (!clearing)
		return;
	clearing = false;
	for (var connection in FindObjects(Find_Func("IsConnectedTo", this)))
	{
		if (!connection->~IsCableLine())
			continue;
		var other_crossing = connection->~GetConnectedObject(this);
		if (!other_crossing || !other_crossing->~IsCableCrossing())
			continue;
		// Acquire destinations of the other crossing, all these are now in reach
		AddCableDestinations(other_crossing->GetDestinations(), other_crossing);
		// Send own destinations, now in reach for the other one
		other_crossing->AddCableDestinations(this->GetDestinations(), this);
		// Destinations have been updated for this crossing.
		DestinationsUpdated();
		// Also update other connections.
		other_crossing->RenewConnections();
	}
}

// Returns whether there is a direct conenctions between this and the other crossing.
public func IsDirectlyConnectToCrossing(object other_crossing)
{
	for (var connection in FindObjects(Find_Func("IsConnectedTo", this)))
	{
		if (!connection->~IsCableLine())
			continue;
		if (other_crossing == connection->~GetConnectedObject(this))
			return true;
	}
	return false;
}


/*-- Pathfinding --*/

/* Functions:
	GetNextWaypoint(object end)
	GetLengthToTarget(object end)
*/

/** Returns the waypoint to take next for the desired final point \a end
* @param end The final destination for the information is queried
*/
public func GetNextWaypoint(object end)
{
	if (!destination_list)
		return nil;
	for (var item in destination_list)
	{
		if (!item)
			continue;
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
	if (!destination_list)
		return nil;
	for (var item in destination_list)
	{
		if (!item)
			continue;
		if (item[const_finaldestination] == end)
			return item[const_distance];
	}
	return nil;
}


/*-- Auto production --*/

local request_queue;

// Add a new acquiring order, this order will be delivered to this station and is a proplist with information.
// { type = <id of the object>, min_amount = <minimal amount to be delivered>, max_amount = <maximal amount to be delivered>}.
public func AddRequest(proplist order)
{
	if (!order || !order.type)
		return false;
	if (!order.min_amount)
		order.min_amount = 1;

	// First of all check if a similar request already is on the line
	// Similar requests will be dismissed, even if they are technically new
	for (var request in request_queue)
		if (request.type == order.type)
			if (request.min_amount == order.min_amount)
				return true; // The request is considered handled

	// Find source station which has the order's items.
	var source = CheckAvailability(order);
	if (!source)
		return false;

	// Find cable car that is at source station or closest to source station.
	var car = source->GetAvailableCableCar(order, this);
	if (!car)
		return false;

	// Great. Start working immediately
	PushBack(request_queue, order);
	car->AddRequest(order, this, source);
	return true;
}

// Check all connected stations for available objects.
public func CheckAvailability(proplist order)
{
	var nearest_station;
	var length_to;
	// Loop over all stations, including this one.
	var network_stations = [this];
	for (var station in GetDestinations())
		if (station)
			PushBack(network_stations, station[const_finaldestination]);
	for (var station in network_stations)
	{
		if (station->IsAvailable(order))
		{
			if (!nearest_station)
			{
				nearest_station = station;
				length_to = GetLengthToTarget(nearest_station);
			}
			else
			{
				// Storages (like chests) are always preferred over other producer because the items
				// might be stored in another producer to produce something.
				if (station->~IsStorage() && !nearest_station->~IsStorage())
				{
					nearest_station = station;
					length_to = GetLengthToTarget(nearest_station);
				}
				else if (nearest_station->~IsStorage() && !station->~IsStorage())
					continue;
				// Otherwise the shorter the path, the better.
				else if (GetLengthToTarget(station) < length_to)
				{
					nearest_station = station;
					length_to = GetLengthToTarget(nearest_station);
				}
			}
		}
	}
	return nearest_station;
}

// Check if there is a cable car ready for delivery
public func GetAvailableCableCar(proplist order, object requesting_station)
{
	
}

// A delivery needs to be picked up at this station.
public func RequestPickUp(object car, proplist order)
{
	OnCableCarPickUp(car, order);
}

// A delivery has arrived, remove it from the queue and handle the request
public func RequestArrived(object car, proplist order)
{
	if (!HasRequest(order))
		return;

	OnCableCarDelivery(car, order);
	RemoveRequest(order);
}

public func HasRequest(proplist order)
{
	for (var test_order in request_queue)
		if (test_order.type == order.type && test_order.min_amount == order.min_amount)
			return true;
	return false;
}

public func RemoveRequest(proplist order)
{
	for (var i = 0; i < GetLength(request_queue); i++)
	{
		if (request_queue[i].type != order.type)
			continue;
		if (request_queue[i].min_amount != order.min_amount)
			continue;
		break;
	}
	if (i >= GetLength(request_queue))
		return false;
	RemoveArrayIndex(request_queue, i, true);
	return true;
}


/*-- Misc Queries --*/

// Returns the closest object satisfying find_crit that can be found on the cables of this network.
// Returns {obj = <closest object>, station1 = <first station>, station2 = <second station>}.
public func FindObjectOnNetworkCables(array find_crit)
{
	var destinations = GetDestinations();
	SortArrayByArrayElement(destinations, this.const_distance, false);
	PushFront(destinations, [this, this, 0]);
	for (var dest in destinations)
	{		
		var station = dest[this.const_finaldestination];
		// Check all cables to other stations.
		for (var connection in FindObjects(Find_Func("IsConnectedTo", station)))
		{
			if (!connection->~IsCableLine())
				continue;
			var other_station = connection->~GetConnectedObject(station);
			if (!other_station || !other_station->~IsCableCrossing())
				continue;
			var sx = station->GetCablePosition()[0];
			var sy = station->GetCablePosition()[1];
			var ox = other_station->GetCablePosition()[0];
			var oy = other_station->GetCablePosition()[1];
			var obj = Global->FindObject(Find_OnLine(sx, sy, ox, oy), find_crit);
			if (obj)
				return [obj, station, other_station];
		}
	}
	return nil;
}

// Returns a free cable car satsfying find_crit on the network closest to this station.
public func FindCableCar(array find_crit)
{
	var destinations = GetDestinations();
	SortArrayByArrayElement(destinations, this.const_distance, false);
	PushFront(destinations, [this, this, 0]);
	for (var dest in destinations)
	{		
		var station = dest[this.const_finaldestination];
		var car = Global->FindObject(Find_InArray(station->GetIdleCars()), find_crit);
		if (car)
			return car;
	}
	return nil;
}
