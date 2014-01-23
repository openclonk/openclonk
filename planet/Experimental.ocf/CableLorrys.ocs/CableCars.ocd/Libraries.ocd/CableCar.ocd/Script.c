/**
	Cable Car
	Library object for the cable car.
	
	@author Randrian, Clonkonaut, Maikel
*/

//#appendto Lorry

local iMovementSpeed;

local pRailTarget;
local rail_direction; // 2 up the line, 1 down the line, 0 no movement
local rail_progress;
local rail_max_prog;
local rail_destination;

public func IsCableCar() { return true; }

public func GetRailTarget() { return pRailTarget; }

public func EngageRail(object pRailpoint)
{
	if (! pRailpoint->IsCableCrossing()) return false;

	var position = CreateArray(2);
	pRailpoint->GetCablePosition(position);
	SetPosition(position[0], position[1]);
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
	var target;
	if(rail_destination != pRailTarget)
	{
		if(target = pRailTarget->GetNextWaypoint(rail_destination))
			MoveTo(target);
		else
			DeliveryFailed();
	}
	else if (deliver_to) {
		if (deliver_to == pRailTarget)
			return DeliverObjects();
		if (pRailTarget->RequestObjects(this, deliver_id, deliver_amount))
			ReturnDelivery();
		else
			DeliveryFailed();
	}
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
	// Target the first or second action target?
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
	var origin = CreateArray(2), ending = CreateArray(2);
	rail->GetActionTarget(0)->GetCablePosition(origin);
	rail->GetActionTarget(1)->GetCablePosition(ending);
	rail_max_prog = Distance(origin[0], origin[1], ending[0], ending[1]);
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
		var position = CreateArray(2);
		pRailTarget->GetCablePosition(position);
		SetPosition(position[0], position[1]);
		rail_direction = 0;
		CrossingReached();
		return;
	}

	var prec = 100;
	var origin = CreateArray(2), ending = CreateArray(2);
	pRailTarget->GetActionTarget(start)->GetCablePosition(origin, prec);
	pRailTarget->GetActionTarget(end)->GetCablePosition(ending, prec);
	var x = origin[0] + (ending[0] - origin[0]) * rail_progress/rail_max_prog;
	var y = origin[1] + (ending[1] - origin[1]) * rail_progress/rail_max_prog;
	SetPosition(x, y, 1, prec);
}

/*-- Delivery --*/

local deliver_id, deliver_amount, deliver_to;

// Returns true when this car is not in move
public func IsAvailable()
{
	return !rail_destination;
}

public func AddDelivery(object from, object to, id object_id, int amount)
{
	deliver_id = object_id;
	deliver_amount = amount;
	deliver_to = to;
	SetDestination(from);
}

public func ReturnDelivery()
{
	SetDestination(deliver_to);
}

public func DeliverObjects()
{
	if (ContentsCount(deliver_id) < deliver_amount) return DeliveryFailed();
	for (var i = 0; i < deliver_amount; i++)
		FindContents(deliver_id)->Enter(pRailTarget);
	pRailTarget->DeliveryDone(deliver_id, deliver_amount);
	deliver_id = false;
	deliver_amount = false;
	deliver_to = false;
}

private func DeliveryFailed()
{

}