/**
	Cable Car
	Library object for the cable car.
	
	@author Randrian, Clonkonaut, Maikel
*/

local iMovementSpeed;

local pRailTarget;
local rail_direction; // 2 up the line, 1 down the line, 0 no movement
local rail_progress;
local rail_max_prog;
local rail_destination;

protected func Initialize()
{
	delivery_queue = [];
	AddEffect("ProcessDeliveryQueue", this, 100, 5, this);
	return _inherited(...);
}

public func IsCableCar() { return true; }

public func GetRailTarget() { return pRailTarget; }

public func EngageRail(object pRailpoint)
{
	if (! pRailpoint->IsCableCrossing()) return false;

	SetPosition(pRailpoint->GetX(), pRailpoint->GetY());
	SetSpeed(0,0);
	SetR(0);
	SetAction("OnRail");
	SetComDir(COMD_None);
	pRailTarget = pRailpoint;
	rail_direction = 0;
}

public func DisengageRail()
{
	pRailTarget = nil;
	rail_direction = 0;
	rail_progress = 0;
	rail_max_prog = 0;
	rail_destination = nil;
	Disengaged();
}

public func SetDestination(dest)
{
  if(GetType(dest) == C4V_Int)
  {
    dest = FindObjects(Find_Func("IsCableCrossing"))[dest];
  }
  rail_destination = dest;
  if(rail_direction == 0)
    CrossingReached();
}

private func Disengaged() {}

/* Destination selection */

public func SelectDestination(object select_clonk, object station)
{
	// Storage effect
	var effect = AddEffect("DestinationSelection", this, 1, 1, this);
	effect.select_clonk = select_clonk;
	effect.station = station;
	// Helping object
	effect.cablecar_sel = CreateObject(CableCar_Selector, 0,0, select_clonk->GetOwner());
	effect.cablecar_sel->FixTo(station);

	effect.engaging_station = station;
}

public func ShiftSelection(int direction, object selector)
{
	// Determine effect
	var effect = nil, temp_effect;
	for (var i = 0; temp_effect = GetEffect("DestinationSelection", this, i); i++)
		if (temp_effect.cablecar_sel == selector)
			effect = temp_effect;
	if (!effect) return false;

	var destinations = [];
	var connection_list = effect.engaging_station->GetDestinations();

	// Get every possible destination
	for (var targets in connection_list)
	{
		// Is this a valid destination?
		if (! targets[0]->IsCableStation()) continue;
		// Check ownership
		if (targets[0]->GetOwner() != effect.select_clonk->GetOwner() && targets[0]->GetOwner() != NO_OWNER) continue;
		// Save it
		destinations[GetLength(destinations)] = targets[0];
	}
	// Add current station, for it is a destination
	destinations[GetLength(destinations)] = effect.engaging_station;
	// Determine destination actually seen
	for (var i = 0; i < GetLength(destinations); i++)
		if (destinations[i] == effect.station)
			break;
	// Scale exceeded? Reset to current station
	if (i == GetLength(destinations)) i--;
	else { // Select the new destination
		i += direction;
		if (i < 0) i = GetLength(destinations) - 1;
		if (i >= GetLength(destinations)) i = 0;
	}
	// Set the view
	effect.station = destinations[i];
	effect.cablecar_sel->FixTo(effect.station);
	return true;
}

protected func FxDestinationSelectionTimer(object target, effect)
{
	// Check cancellation conditions
	if (! effect.select_clonk) return -1; // Clonk's gone
	if (! effect.select_clonk->GetAlive()) return -1; // Clonk's dead
	if (effect.select_clonk->GetActionTarget() != this) return -1; // Clonk's not grabbing anymore
	if (! effect.cablecar_sel) return -1; // Selector's gone
	if (! effect.engaging_station) return -1; // Engaging station's gone
	if (effect.engaging_station->OnFire()) return -1; // Engaging station's burning (destroyed)

	// Check view
	if (! effect.station) ShiftSelection(0, effect.cablecar_sel); // Current view target is gone
	if (effect.station->OnFire()) ShiftSelection(0, effect.cablecar_sel); // Current view target is burning (destroyed)
}

protected func FxDestinationSelectionStop(object target, effect, int reason, bool temp)
{
	if (temp) return;

	// Clonk's still alive? Reset Cursor
	if (effect.select_clonk && effect.select_clonk->GetAlive())
	{
		SetPlrView(effect.select_clonk->GetOwner(), effect.select_clonk);
		SetCursor(effect.select_clonk->GetOwner(), effect.select_clonk, true);
	}
	// Remove selector
	effect.cablecar_sel->RemoveObject();
}

public func AcceptSelection(object selector)
{
	// Determine effect
	var effect = nil, temp_effect;
	for (var i = 0; temp_effect = GetEffect("DestinationSelection", this, i); i++)
		if (temp_effect.cablecar_sel == selector)
			effect = temp_effect;
	if (!effect) return false;

	// Reset cursor
	SetPlrView(effect.select_clonk->GetOwner(), effect.select_clonk);
	SetCursor(effect.select_clonk->GetOwner(), effect.select_clonk, true);
	effect.cablecar_sel->RemoveObject();
	// Ungrab & start!
	effect.select_clonk->ObjectControl(effect.select_clonk->GetOwner(), CON_Ungrab);
	SetDestination(effect.station);
	RemoveEffect(nil, this, effect, true);
	return true;
}

public func AbortSelection(object selector)
{
	// Determine effect
	var effect = nil, temp_effect;
	for (var i = 0; temp_effect = GetEffect("DestinationSelection", this, i); i++)
		if (temp_effect.cablecar_sel == selector)
			effect = temp_effect;
	if (!effect) return false;

	RemoveEffect(nil, this, effect);
}

/* Movement */

private func CrossingReached()
{
	// Callback to the delivery interface.
	if (pRailTarget->~IsCableStation())
		OnStationReached(pRailTarget);
	var target;
	if(rail_destination != pRailTarget)
		if(target = pRailTarget->GetNextWaypoint(rail_destination))
			MoveTo(target);
	return;
}

private func MoveTo(dest)
{
	if(GetType(dest) == C4V_Int)
	{
		dest = FindObjects(Find_Func("IsCableCrossing"))[dest];
	}
	var rail = 0;
	for(var test_rail in FindObjects(Find_Func("IsConnectedTo", pRailTarget)))
	{
		if(test_rail->IsConnectedTo(dest))
		{
			rail = test_rail;
			break;
		}
	}
	if(!rail)
	{
		Message("No Rail available!");
		return;
	}
	// Target the first or section action target?
	if(rail->GetActionTarget(0) == dest)
	{
		rail_direction = 1;
		rail_progress  = 0;
	}
	else
	{
		rail_direction = 2;
		rail_progress  = 0;
	}
	rail->GetActionTarget(0)->AddActive(0);
	rail->GetActionTarget(1)->AddActive(0);
	rail->AddActive(0);
	rail_max_prog = ObjectDistance(dest, pRailTarget);
	pRailTarget = rail;
}

protected func OnRail()
{
	if(rail_direction == 0 || rail_direction == nil) return;
	var start = 0;
	var end = 1;
	if(rail_direction == 1)
	{
		start = 1;
		end = 0;
	}

	rail_progress += iMovementSpeed;
	if(rail_progress >= rail_max_prog)
	{
		pRailTarget->GetActionTarget(0)->AddActive(1);
		pRailTarget->GetActionTarget(1)->AddActive(1);
		pRailTarget->AddActive(1);
		pRailTarget = pRailTarget->GetActionTarget(end);
		SetPosition(pRailTarget->GetX(), pRailTarget->GetY());
		rail_direction = 0;
		CrossingReached();
		return;
	}

	var prec = 100;
	var x = pRailTarget->GetActionTarget(start)->GetX(prec)+
			(pRailTarget->GetActionTarget(end)->GetX(prec)-pRailTarget->GetActionTarget(start)->GetX(prec))*rail_progress/rail_max_prog;
	var y = pRailTarget->GetActionTarget(start)->GetY(prec)+
			(pRailTarget->GetActionTarget(end)->GetY(prec)-pRailTarget->GetActionTarget(start)->GetY(prec))*rail_progress/rail_max_prog;
	SetPosition(x, y, 1, prec);
}


/*-- Network control --*/

/**
	Returns whether this lorry can reach the specified station.
*/
public func IsInNetwork(object station)
{
	var at_station = pRailTarget;
	if (at_station->~IsCableLine())
		at_station = at_station->GetActionTarget(0); // TODO: Traveling to which?
	if (!at_station)
		return false;
	return !!at_station->IsInNetwork(station);
}


// FindObject wrapper to find a station which this cable car can reach.
private func Find_CableStation()
{
	var at_station = pRailTarget;
	if (at_station->~IsCableLine())
		at_station = at_station->GetActionTarget(0); // TODO: Travelling to which?
	var station = [C4FO_Func, "IsCableStation"];
	var network = [C4FO_Func, "IsInNetwork", at_station];		
	return [C4FO_And, station, network];
}

/*-- Delivery queue --*/

local delivery_queue;

protected func FxProcessDeliveryQueueStart()
{

	return 1;
}

protected func FxProcessDeliveryQueueTimer(object target, proplist effect)
{
	if (!pRailTarget)
		return 1;
	//LogDeliveryQueue();
	// Retrieve first request for objects from delivery queue.
	var request = delivery_queue[0];
	if (!request)
		return 1;
	// Check whether the requested contents are in this car.
	if (ObjectCount(Find_Container(this), Find_ID(request.ObjID)) < request.Amount)
	{
		// Not a sufficient amount of the requested objects, try to retrieve from other stations.
		// TODO: is station in network, find closest.
		if (!effect.Station)
			for (var station in FindObjects(Find_CableStation(), Find_Exclude(request.Station)))
			{
				// Check every station in network.
				// TODO: Move request down if not fulfillable.
				if (station->GetAvailableCount(request.ObjID) >= request.Amount)
				{
					//station->ReserveObjects(request.ObjID, request.Amount);
					//Log("Cable car %v moving to station %v for loading.", this, station);
					SetDestination(station);
					effect.Station = station;
					break;		
				}
			}
		// Request can't be handled, remove to back of queue.
		if (!effect.Station)
		{
			RemoveFromQueue(request); // Rather hacky this, but whatever.
			delivery_queue[GetLength(delivery_queue)] = request;			
		}
		
	}
	else
	{
		// Sufficient amount of requested objects, deliver at station.
		SetDestination(request.Station);
		effect.Station = nil;	
	}
	return 1;
}

/**
	Removes the specified request from the delivery queue.
*/
private func RemoveFromQueue(proplist request)
{
	var remove = false;
	var length = GetLength(delivery_queue);
	for (var i = 0; i < length; i++)
	{
		// Request found.
		if (request == delivery_queue[i])
			remove = true;
		// Move trailing requests up by one.
		if (remove)
			delivery_queue[i] = delivery_queue[i+1];
	}
	// Request found, reduce size by one.
	if (remove)
		SetLength(delivery_queue, length - 1);
	return;
}

/**
	Called from the movement section whenever a crossing of type station has been reached.
*/
private func OnStationReached(object station)
{
	Log("Station %v reached: Try loading and unloading.", station);
	// Check deliveries which need objects from the current station.
	for (var request in delivery_queue)
	{
		// Don't load from the station which requests.
		if (station == request.Station)
			continue;
		// Compensate for contents already in the car, not used already for another delivery.
		//var car_cnt = ObjectCount(Find_Container(this), Find_ID(request.ObjID));
		if (request.Amount > 0)
		{
			var req_cnt = request.Amount;
			var av_cnt = station->GetAvailableCount(request.ObjID);
			req_cnt = Min(req_cnt, av_cnt);
			if (req_cnt > 0)
				station->TransferFromStation(this, request.ObjID, req_cnt);			
		}	
	}
	
	// Check deliveries which need to unload at current station.
	for (var request in delivery_queue)
	{
		if (request.Station == station)
		{
			// Check if the requested objects are available in this car.
			if (ObjectCount(Find_Container(this), Find_ID(request.ObjID)) >= request.Amount)
			{
				station->TransferIntoStation(this, request.ObjID, request.Amount);	
				// Remove request from queue.
				RemoveFromQueue(request);
			}
		}
	}
	return;
}

/**	
	Request this car for a delivery to the specified station.
	@param station the place where the objects should be delivered.
	@param obj_id id of the requested objects.
	@param amount amount of the requested objects.
	@return \c true if the request can be processed, \c false otherwise.
*/
public func RequestDelivery(object station, id obj_id, int amount)
{
	//Log("CableCar: RequestDelivery, station %v, obj {{%i}}, amount %d", station, obj_id, amount);
	// Check connection to station.
	if (!IsInNetwork(station))
		return false;	
	// Check availability of delivery.
	var avl_amount = 0;
	for (var load_station in FindObjects(Find_CableStation()))
		avl_amount += load_station->GetAvailableCount(obj_id);
	// If not enough available, try to find a producer.
	if (avl_amount < amount)
	{
		var producer = FindObject(Find_CableStation(), Find_Func("CanProduceItem", obj_id));
		if (!producer)
			return false;
		producer->AddToQueue(obj_id, amount - avl_amount);		
	}	
	// Add request to delivery queue.
	var request = {Station = station, ObjID = obj_id, Amount = amount};
	delivery_queue[GetLength(delivery_queue)] = request;
	return true;
}

/** 
	Looks for a delivery in the delivery queue.

*/
public func FindDelivery(object station, id obj_id)
{
	for (var request in delivery_queue)
		if (request.Station == station && request.ObjID == obj_id)
			return request;
	return;
}

// For debug purposes
public func LogDeliveryQueue()
{
	Log("===Delivery queue of %v===", this);
	for (var request in delivery_queue)
	{
		Log("* Delivery %v: %d {{%i}} to station %v", request, request.Amount, request.ObjID, request.Station);
	}
	var station = GetEffect("ProcessDeliveryQueue", this).Station;
	Log("In Process: Delivery %v: %d {{%i}} from station %v to station %v", request, request.Amount, request.ObjID, station, request.Station);
	return;
}

