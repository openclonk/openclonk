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
	effect.var0 = select_clonk;
	effect.var1 = station;
	// Helping object
	effect.var2 = CreateObject(CableCar_Selector, 0,0, select_clonk->GetOwner());
	effect.var2->FixTo(station);

	effect.var3 = station;
}

public func ShiftSelection(int direction, object selector)
{
	// Determine effect
	var effect = nil, temp_effect;
	for (var i = 0; temp_effect = GetEffect("DestinationSelection", this, i); i++)
		if (temp_effect.var2 == selector)
			effect = temp_effect;
	if (!effect) return false;

	var destinations = [];
	var connection_list = effect.var3->GetDestinations();

	// Get every possible destination
	for (var targets in connection_list)
	{
		// Is this a valid destination?
		if (! targets[0]->IsCableStation()) continue;
		// Check ownership
		if (targets[0]->GetOwner() != effect.var0->GetOwner() && targets[0]->GetOwner() != NO_OWNER) continue;
		// Save it
		destinations[GetLength(destinations)] = targets[0];
	}
	// Add current station, for it is a destination
	destinations[GetLength(destinations)] = effect.var3;
	// Determine destination actually seen
	for (var i = 0; i < GetLength(destinations); i++)
		if (destinations[i] == effect.var1)
			break;
	// Scale exceeded? Reset to current station
	if (i == GetLength(destinations)) i--;
	else { // Select the new destination
		i += direction;
		if (i < 0) i = GetLength(destinations) - 1;
		if (i >= GetLength(destinations)) i = 0;
	}
	// Set the view
	effect.var1 = destinations[i];
	effect.var2->FixTo(effect.var1);
	return true;
}

protected func FxDestinationSelectionTimer(object target, effect)
{
	// Check cancellation conditions
	if (! effect.var0) return -1; // Clonk's gone
	if (! effect.var0->GetAlive()) return -1; // Clonk's dead
	if (effect.var0->GetActionTarget() != this) return -1; // Clonk's not grabbing anymore
	if (! effect.var2) return -1; // Selector's gone
	if (! effect.var3) return -1; // Engaging station's gone
	if (effect.var3->OnFire()) return -1; // Engaging station's burning (destroyed)

	// Check view
	if (! effect.var1) ShiftSelection(0, effect.var2); // Current view target is gone
	if (effect.var1->OnFire()) ShiftSelection(0, effect.var2); // Current view target is burning (destroyed)
}

protected func FxDestinationSelectionStop(object target, effect, int reason, bool temp)
{
	if (temp) return;

	// Clonk's still alive? Reset Cursor
	if (effect.var0 && effect.var0->GetAlive())
	{
		SetPlrView(effect.var0->GetOwner(), effect.var0);
		SetCursor(effect.var0->GetOwner(), effect.var0, true);
	}
	// Remove selector
	effect.var2->RemoveObject();
}

public func AcceptSelection(object selector)
{
	// Determine effect
	var effect = nil, temp_effect;
	for (var i = 0; temp_effect = GetEffect("DestinationSelection", this, i); i++)
		if (temp_effect.var2 == selector)
			effect = temp_effect;
	if (!effect) return false;

	// Reset cursor
	SetPlrView(effect.var0->GetOwner(), effect.var0);
	SetCursor(effect.var0->GetOwner(), effect.var0, true);
	effect.var2->RemoveObject();
	// Ungrab & start!
	effect.var0->ObjectControl(effect.var0->GetOwner(), CON_Ungrab);
	SetDestination(effect.var1);
	RemoveEffect(nil, this, effect, true);
	return true;
}

public func AbortSelection(object selector)
{
	// Determine effect
	var effect = nil, temp_effect;
	for (var i = 0; temp_effect = GetEffect("DestinationSelection", this, i); i++)
		if (temp_effect.var2 == selector)
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


/*-- Network movement --*/

protected func Find_InNetwork(object station)
{

	return;
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
		for (var station in FindObjects(Find_Func("IsCableStation")))
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
		var car_cnt = ObjectCount(Find_Container(this), Find_ID(request.ObjID));
		if (request.Amount > car_cnt)
		{
			var req_cnt = request.Amount - car_cnt;
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
				// Remove request from queue. FIXME
				RemoveFromQueue(request);
			}
		}
	}
	return;
}

/**	
	Request this car for a delivery to a station.

*/
public func RequestDelivery(object station, id obj_id, int amount)
{
	// Check connection to station.
	
	// Check availability of delivery.
	
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
	return;
}

