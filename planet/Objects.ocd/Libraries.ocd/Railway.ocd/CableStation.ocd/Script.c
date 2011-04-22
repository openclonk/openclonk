/**
	Cable Station
	Library for cable stations and crossings. This is included by 
	cable crossings, and should be included by structures which
	want to make use of the cable network.
	
	@author Randrian, Clonkonaut, Maikel
*/

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

/*
global func WaypointsMakeList()
{
	var iterationLimit = ObjectCount(Find_Func("IsCableCrossing"));
	for(var obj in FindObjects(Find_Func("IsCableCrossing")))
	{
		obj->ResetList();
		obj->AddNeighboursToList();
	}
	for(var i = 0; i < iterationLimit; i++)
		for(var obj in FindObjects(Find_Func("IsCableCrossing")))
			obj->AddNeighboursList();
}
*/

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
	reserved_list = {};
	destination_list = [];
	return _inherited(...);
}

/* Pathfinding for cable cars */

/** Returns the waypoint to take next for the desired final point \a end
* @param end The final destination for the information is queried
*/
public func GetNextWaypoint(object end)
{
  if(!destination_list) return nil;
	for(var item in destination_list)
		if(item[const_finaldestination] == end)
			return item[const_nextwaypoint];
	return nil;
}

/** Returns the actual traveling distance for the desired final point \a end
* This is not the airline distance but the length of all cables to take via traveling
* @param end The final destination for the information is queried
*/
public func GetLengthToTarget(object end)
{
	if (!destination_list) return nil;
	for(var item in destination_list)
		if(item[const_finaldestination] == end)
			return item[const_distance];
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
/** Removes a connection via the cable \a cable from this crossing
* Does nothing if the other connected objects of the cable is not a cable crossing
* @param cable The broken cable
*/
public func RemoveCableConnection(object cable)
{
	// Failsafe
	if (!cable || ! cable->~IsCableLine())
		return false;
	// Get other connection
	var other_crossing = cable->~GetOtherConnection(this);
	if (! other_crossing->~IsCableCrossing())
		return false;
	// Remove all connections
	RemoveCableDestinations(other_crossing);
	// Sad, less power to the network...
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
		// Destination is this
		if (list_item[const_finaldestination] == this) continue;
		// Check whether the destination is already in reach
		var handled = false;
		for (var i = 0, destination = false; i < GetLength(destination_list); i++)
		{
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

/** Removes the path to \a crossing and furthermore to every waypoint in reach through \a crossing but first tries to find alternate routes
* @param crossing The now probably unavailable crossing
*/
public func RemoveCableDestinations(object crossing)
{
	// Find the entry of crossing where crossing is the final destination
	for (var i = 0; i < GetLength(destination_list); i++)
	{
		if (destination_list[i][const_finaldestination] == crossing)
			break;
	}
	if (i >= GetLength(destination_list)) return false; // entry not found
	// Remove the entry
	destination_list[i] = nil;
	// Inform every connected waypoint
	for (var connection in FindObjects(Find_Func("IsConnectedTo", this)))
	{
		if (! connection->~IsCableLine()) continue;
		var other_crossing = connection->~GetOtherConnection(this);
		if (! other_crossing->~IsCableCrossing()) continue;
		other_crossing->RemoveCableDestination(crossing, this);
	}
}

/** Removes the destination \a to_remove, but only if the saved path leads through \a crossing
* If the route does not lead through crossing this checks whether the route is still intact
* @param to_remove The destination to remove
* @param crossing The crossing which linked to the old destination
*/
public func RemoveCableDestination(object to_remove, object crossing)
{
	// Find the entry of to_remove
	for (var i = 0; i < GetLength(destination_list); i++)
	{
		if (destination_list[i][const_finaldestination] == to_remove)
			break;
	}
	if (i >= GetLength(destination_list)) return false; // entry not found
	// Check if the path leads through crossing
	if (destination_list[i][const_nextwaypoint] == crossing)
	{
		// Remove the entry
		destination_list[i] = nil;
		// Inform every connected waypoint
		for (var connection in FindObjects(Find_Func("IsConnectedTo", this)))
		{
			if (! connection->~IsCableLine()) continue;
			var other_crossing = connection->~GetOtherConnection(this);
			if (! other_crossing->~IsCableCrossing()) continue;
			if (other_crossing == crossing) continue;
			other_crossing->RemoveCableDestination(crossing, this);
		}
	}
	else // It does not lead through crossing, check route
	{
		var waypoint = this;
		while(waypoint && waypoint != to_remove)
			waypoint = waypoint->GetNextWaypoint(to_remove);
		if (!waypoint) // This means that the path vanished anyway
			return RemoveCableDestination(to_remove, destination_list[i][const_nextwaypoint]);
		// We found an alternate route!
	}
	
}

/*
// Resets aPath, adds every connected line, reset aConnectionList
public func ResetList()
{
  aPath = [];
  for(var line in FindObjects(Find_Func("IsConnectedTo", this)))
    aPath[GetLength(aPath)] = line;
  aConnectionList = [];
}

// Preprocess in adding the whole waypoint list
// Fills aConnectionList with [Next waypoint in line, Next waypoint in line, Distance (airline) to next waypoint]
// Afterwards call AddNeighboursList to fill aConnectionList with the correct values
// --- It is not working without calling AddNeighboursList ---
public func AddNeighboursToList()
{
	var obj;
	for(var i = 0; i < GetLength(aPath); i++)
	{
		obj = aPath[i]->~GetOtherConnection(this);//EffectVar(0,this(),aPath[i]);
		aConnectionList[i] = [obj, obj, ObjectDistance(obj)];
	}
}

// Finishes the preprocessed values from AddNeighboursToList
// In a whole system rewrite Add NeighboursToList is called first
public func AddNeighboursList()
{
	var newList, obj, dist, pEnd;
	for(var i = 0; i < GetLength(aPath); i++)
	{
		obj = aPath[i]->~GetOtherConnection(this);
		dist = ObjectDistance(obj);
		// Get aConnectionList from the next waypoint in line
		newList = obj->GetList();
		for(var j = 0; j < GetLength(newList); j++)
		{
			// See where the path of the other waypoint ends
			pEnd = newList[j][0];
			// This is the end of that path, skip
			if(pEnd == this)
				continue;
			// See whether I've already stored a path with that final point
			for(var k = 0; k < GetLength(aConnectionList); k++)
				if(aConnectionList[k][0] == pEnd)
					break;
			// The path through the current next waypoint would be longer than my already stored path, skip
			if(k < GetLength(aConnectionList) && aConnectionList[k][2] < newList[j][2]+dist)
				continue;
			// Too possible cases: 1. new ending point which I didn't knew it was there
			// 2. The path through that next waypoint is superior (shorter) to what I thought was a good path (stupid me)
			aConnectionList[k] = [pEnd, obj, newList[j][2]+dist];
		}
	}
	// I want to check now whether I'm a railway station, upgrading me to awesome engaging point
	CheckRailStation();
}
*/

/* Station behaviour */

// Prevents the automatic change of the stations status when set to station mode
local bManualSetting;

// Appears in the bottom interaction bar
public func IsInteractable() { return true; }

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
	return true;
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

// Check whether I am a railway station
// If so, set up new graphics
// If not, disable graphics if needed
private func CheckRailStation()
{
	if (GetLength(FindObjects(Find_Func("IsConnectedTo", this))) == 1 || bManualSetting)
	{
		if (!is_station)
		{
			SetGraphics("Station", Library_CableStation, 2, GFXOV_MODE_Base);
			is_station = true;
		}
	}
	else if (is_station)
	{
		SetGraphics(nil, nil, 2, GFXOV_MODE_Base);
		is_station = false;
	}
}

/*-- Network control --*/

/**
	Determines whether this station is connected to the specified station.
	@param station cable station for which to check the connection.
	@return \c true if there is connection, \c false otherwise.
*/
public func IsInNetwork(object station)
{
	if (station == this)
		return true;
	return !!GetNextWaypoint(station);
}

// FindObject wrapper to find a cable car able to reach the specified station.
private func Find_CableCar(object station)
{
	if (!station)
		station = this;
	var car = [C4FO_Func, "IsCableCar"];
	var network = [C4FO_Func, "IsInNetwork", station];		
	return [C4FO_And, car, network];
}

// Would be nice to have some sorting function to determine which car is best available.
private func Sort_Network(object station)
{



}

/*-- Interface for object requests and deliveries --*/

// List to keep track of which objects already have been reserved by cable cars.
local reserved_list;

/**
	Requests the cable network to deliver the specified objects.
	@param obj_id id of the object requested.
	@param amount amount of requested objects.
	@return \c true if the requested object is available and added to network queue, \c false otherwise.
*/
public func RequestObject(id obj_id, int amount)
{
	// TODO: Complete implementation.
	// Find the most suited cable car for the delivery, i.e. near and small delivery queue.
	var car = FindObject(Find_CableCar());
	if (!car)
		return false;
	// Only request for a delivery if there is not already an equivalent.
	if (car->FindDelivery(this, obj_id))
		return false;		
	// Let the cable car check the network for the requested objects.
	// If there are sufficient objects in the network the delivery will be processed.
	return car->RequestDelivery(this, obj_id, amount);
}

/**
	Gives the number of available objects in this station.
	@param obj_id id of the object.
	@return the number of available objects of the specific type in this station.
*/
public func GetAvailableCount(id obj_id)
{
	// Determine number of objects in this station of the specified id.
	var count = ObjectCount(Find_Container(this), Find_ID(obj_id));
	// Reduce the count with the number of already reserved objects.
	if (reserved_list)
		count -= reserved_list[Format("%i",obj_id)];
	//Log("Available count requested for {{%i}} result: %d", obj_id, count);
	return count;
}

/**
	Let's the cable car reserve specific objects which are to be collected later on.
	
*/
public func ReserveObjects(id obj_id, int amount)
{
	// Only do so if enough objects are available.
	if (GetAvailableCount(obj_id) < amount)
		return false;
	// Add amount of objects to reserved list.
	//if (reserved_list[Format("%i",obj_id)] == nil)
	//	reserved_list[Format("%i",obj_id)] = amount;
	//else
	//	reserved_list[Format("%i",obj_id)] += amount;
	return true;
}


/**
	Called from the cable car to transfer objects from the car into the station.

*/
public func TransferIntoStation(object car, id obj_id, int amount)
{
	var cnt = 0;
	for (var obj in FindObjects(Find_Container(car), Find_ID(obj_id)))
	{
		// Transfer object.
		obj->Exit();
		obj->Enter(this);
		// Increase count and abort if amount has been reached.
		if (++cnt >= amount)
			break;		
	}
	// Visually display transfer.
	//CreateObject(CableStationTransfer)->ShowTransferOut(obj_id, cnt);
	Message("Transfered %d {{%i}} into station", cnt, obj_id);
	return;
}

/**
	Called from the cable car to transfer objects from the station into the car.

*/
public func TransferFromStation(object car, id obj_id, int amount)
{
	var cnt = 0;
	for (var obj in FindObjects(Find_Container(this), Find_ID(obj_id)))
	{
		// Transfer object.
		obj->Exit();
		obj->Enter(car);
		// Increase count and abort if amount has been reached.
		if (++cnt >= amount)
			break;		
	}
	// Visually display transfer.
	//CreateObject(CableStationTransfer)->ShowTransferIn(obj_id, cnt);
	Message("Transfered %d {{%i}} into car", cnt, obj_id);
	return;
}