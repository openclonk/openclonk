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

// Called by departing cable cars if it just starts a new journey
public func OnCableCarDeparture(object car) { }

// Called by a cable car that has been hooked up to the rail at this station
public func OnCableCarEngaged(object car) { }

// Called by a cable car that has been taken off the rail at this station
public func OnCableCarDisengaged(object car) { }

// Called when a cable car with a requested delivery arrives
public func OnCableCarDelivery(object car, id requested, int amount) { }

// Called by other stations to check if a certain object and amount are available for delivery at this station.
// Return true if there are means to collect the required amount.
public func IsAvailable(proplist requested, int amount)
{
	return false;
}

/*--- Callbacks ---*/

// Be sure to always call these via _inherited(...);

func Initialize()
{
	destination_list = [];
	request_queue = [];
	return _inherited(...);
}

/* Removes this crossing from the network
	It first clears every waypoint from the network and then renews the whole information.
	Optimisation welcome!
*/
func Destruction()
{
	for (var connection in FindObjects(Find_Func("IsConnectedTo", this)))
	{
		if (! connection->~IsCableLine()) continue;
		var other_crossing = connection->~GetConnectedObject(this);
		if (! other_crossing->~IsCableCrossing()) continue;
		other_crossing->ClearConnections(this);
	}
	for (var connection in FindObjects(Find_Func("IsConnectedTo", this)))
	{
		if (! connection->~IsCableLine()) continue;
		var other_crossing = connection->~GetConnectedObject(this);
		if (! other_crossing->~IsCableCrossing()) continue;
		other_crossing->RenewConnections(this);
	}
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
	if (!prec) prec = 1;
	coordinates[0] = GetX(prec);
	coordinates[1] = GetY(prec);
	if (this.LineAttach)
	{
		coordinates[0] += this.LineAttach[0] * prec;
		coordinates[1] += this.LineAttach[1] * prec;
	}
}

// Usually called by cable cars to retrieve selectable destinations for the destination selection menu.
// Returns an array of three objects and one int, one station before and one station after the middle one and the middle one.
// The int (fourth array value) is the overall amount of stations found.
// If middle is not a station (anymore), the first three found objects are returned.
public func GetDestinationList(object middle)
{
	var list = CreateArray();
	var ret = CreateArray(4);
	for (var destination in destination_list)
		if (destination[const_finaldestination]->IsCableStation())
			PushBack(list, destination[const_finaldestination]);
	if (GetLength(list) == 0) return ret;
	if (GetLength(list) == 1) return [nil, list[0], nil, 1];
	if (GetLength(list) == 2) return [list[0], list[1], nil, 2];
	if (GetLength(list) == 3) return [list[0], list[1], list[2], 3];

	var middle_index = GetIndexOf(list, middle);
	if (middle_index == -1) middle_index = 1;
	var left_index = middle_index - 1;
	if (left_index < 0) left_index = GetLength(list) - 1;
	var right_index = middle_index + 1;
	if (right_index >= GetLength(list)) right_index = 0;

	ret[0] = list[left_index];
	ret[1] = list[middle_index];
	ret[2] = list[right_index];
	ret[3] = GetLength(list);
	return ret;
}

/*--- Maintaining the destination list ---*/

/* Functions:
	GetDestinations()
	AddCableConnection(object cable)
	AddCableDestinations(array new_list, object crossing)
	AddCableDestination(object new_destination, object crossing, int distance_add)
	ClearConnections(object crossing)
	RenewConnections(object crossing)
*/

/** Returns the destination array so it can be used by other crossings.
*/
public func GetDestinations()
{
	return destination_list[:];
}

// Stores the next crossing (waypoint) to take when advancing to a certain final point
// Scheme (2D array): [Desired final point, Next waypoint to take, Distance (not airline!) until final point]
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
	if (!cable || ! cable->~IsCableLine())
		return false;
	// Line setup finished?
	var other_crossing = cable->~GetConnectedObject(this);
	if (! other_crossing->~IsCableCrossing())
		return false;
	// Acquire destinations of the other crossing, all these are now in reach
	AddCableDestinations(other_crossing->GetDestinations(), other_crossing);
	// Send own destinations, now in reach for the other one
	other_crossing->AddCableDestinations(destination_list[:], this);
	// Awesome, more power to the network!
	DestinationsUpdated();
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
	DestinationsUpdated();
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
	// Failsafe
	if (!crossing_item) return false;
	// Save the new destination
	destination_list[GetLength(destination_list)] = [new_destination, crossing_item[const_nextwaypoint], crossing_item[const_distance] + distance_add];
	DestinationsUpdated();
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
	// Failsafe
	if (!crossing_item || !destination_item) return false;
	// Save the updated path
	destination_list[destination_item][const_nextwaypoint] = crossing_item[const_nextwaypoint];
	destination_list[destination_item][const_distance] = crossing_item[const_distance] + distance_add;
	DestinationsUpdated();
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
		var other_crossing = connection->~GetConnectedObject(this);
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
		var other_crossing = connection->~GetConnectedObject(this);
		if (! other_crossing->~IsCableCrossing()) continue;
		if (other_crossing == crossing) continue;
		destination_list[GetLength(destination_list)] = [other_crossing, other_crossing, ObjectDistance(other_crossing)];
		AddCableDestinations(other_crossing->GetDestinations(), other_crossing);
	}
}

/*--- Pathfinding ---*/

/* Functions:
	GetNextWaypoint(object end)
	GetLengthToTarget(object end)
*/

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

/*-- Auto production --*/

local request_queue;

// Add a new acquiring order
public func AddRequest(proplist requested_id, int amount)
{
	if (!requested_id) return false;
	if (!amount) amount = 1;

	// First of all check if a similar request already is on the line
	// Similar requests will be dismissed, even if they are technically new
	for (var request in request_queue)
		if (request[0] == requested_id)
			if (request[1] == amount)
				return true; // The request is considered handled

	var source = CheckAvailability(requested_id, amount);
	if (!source)
		return false; // Items are not available

	var car = source->GetAvailableCableCar(requested_id, amount, this);
	if (!car)
		return false; // No cable car is available for delivery

	// Great. Start working immediately
	PushBack(request_queue, [requested_id, amount]);
	car->AddRequest(requested_id, amount, this, source);

	return true;
}

// Check all connected stations for available objects
func CheckAvailability(proplist requested, int amount)
{
	var nearest_station;
	var length_to;
	for (var destination in destination_list)
	{
		if (destination[const_finaldestination]->IsAvailable(requested, amount))
		{
			var station = destination[const_finaldestination];
			if (!nearest_station)
			{
				nearest_station = station;
				length_to = GetLengthToTarget(nearest_station);
			}
			else {
				// Storages (like chests) are always preferred over other producer because the items
				// might be stored in another producer to produce something
				if (station->~IsStorage() && !nearest_station->~IsStorage())
				{
					nearest_station = station;
					length_to = GetLengthToTarget(nearest_station);
				}
				else if (nearest_station->~IsStorage() && !station->~IsStorage())
					continue;
				// Otherwise the shorter the path, the better
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
public func GetAvailableCableCar(proplist requested, int amount, proplist requesting_station)
{
	
}

// A delivery has arrived, remove it from the queue and handle the request
public func RequestArrived(proplist car, proplist requested_id, int amount)
{
	if (!HasRequest(requested_id, amount))
		return;

	OnCableCarDelivery(car, requested_id, amount);
	RemoveRequest(requested_id, amount);
}

public func HasRequest(id requested, int amount)
{
	for (var i = 0; i < GetLength(request_queue); i++)
	{
		if (request_queue[i][0] != requested)
			continue;
		if (request_queue[i][1] != amount)
			continue;
		return true;
	}
	return false;
}

public func RemoveRequest(id requested, int amount)
{
	for (var i = 0; i < GetLength(request_queue); i++)
	{
		if (request_queue[i][0] != requested)
			continue;
		if (request_queue[i][1] != amount)
			continue;
		break;
	}
	if (i >= GetLength(request_queue))
		return false;
	RemoveArrayIndex(request_queue, i, true);
	return true;
}