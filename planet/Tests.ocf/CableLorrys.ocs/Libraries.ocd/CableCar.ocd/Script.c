/*--
	Cable Car
	Authors: Randrian, Clonkonaut

	This is the basic movement functionality for every vehicles which wants to drive through the railway system
--*/

local iMovementSpeed;

local pRailTarget;
local rail_direction; // 2 up the line, 1 down the line, 0 no movement
local rail_progress;
local rail_max_prog;
local rail_destination;

public func EngageRail(object pRailpoint)
{
	if (! pRailpoint->IsCableCrossing()) return false;

	SetPosition(pRailpoint->GetX(), pRailpoint->GetY());
	SetAction("OnRail");
	pRailTarget = pRailpoint;
	rail_direction = 0;
}

public func SetDestination(dest)
{
  if(GetType(dest) == C4V_Int)
  {
    dest = FindObjects(Find_ID(CableCrossing))[dest];
  }
  rail_destination = dest;
  if(rail_direction == 0)
    CrossingReached();
}

/* Movement */

private func CrossingReached()
{
  var target;
  if(rail_destination != pRailTarget)
    if(target = pRailTarget->GetNextWaypoint(rail_destination))
      MoveTo(target);
}

private func MoveTo(dest)
{
	if(GetType(dest) == C4V_Int)
	{
		dest = FindObjects(Find_ID(CableCrossing))[dest];
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