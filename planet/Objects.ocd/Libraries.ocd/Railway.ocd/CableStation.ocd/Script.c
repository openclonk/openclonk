/**
	Cable Station
	Library for cable stations and crossings. This is included by 
	cable crossings, and should be included by structures which
	want to make use of the cable network.
	
	@author Randrian, Clonkonaut, Maikel
*/


/*-- State --*/

local is_station;

public func IsCableCrossing() { return true; }
public func IsCableStation() { return is_station; }



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

/* Local */


protected func Initialize()
{
	  SetAction("Wait");
	  SetGraphics(0, GetID(), 1, GFXOV_MODE_Base);
	  return;
}

// Every line connected to me
local aPath;
// Stores the next crossing (waypoint) to take when advancing to a certain final point
// Scheme: [Desired final point, Next waypoint to take, Distance (not airline!) until final point]
local aConnectionList;

/* Pathfinding for cable cars */

// Returns the waypoint to take next for the desired final point pEnd
public func GetNextWaypoint(object pEnd)
{
  if(!aConnectionList) return;
	for(var item in aConnectionList)
		if(item[0] == pEnd)
			return item[1];
	return false;
}

// Returns the distance (not airline!) for the desired final point pEnd
public func GetLengthToTarget(object pEnd)
{
	for(var item in aConnectionList)
		if(item[0] == pEnd)
			return item[2];
	return false;
}

/* Set up pathfinding information */

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
		obj = aPath[i]->GetOtherConnection(this);//EffectVar(0,this(),aPath[i]);
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
		obj = aPath[i]->GetOtherConnection(this);
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

public func GetList() { return aConnectionList; }

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
	if (GetLength(aPath) == 1 || bManualSetting)
	{
		if (!is_station)
		{
			SetGraphics("Station", CableCrossing, 2, GFXOV_MODE_Base);
			is_station = true;
		}
	}
	else if (is_station)
	{
		SetGraphics(nil, nil, 2, GFXOV_MODE_Base);
		is_station = false;
	}
}

local ActMap = {
	Active = {
		Prototype = Action,
		Name = "Active",
		Procedure = DFA_NONE,
		Length = 1,
		Delay = 1,
		FacetBase = 0,
		NextAction = "Active",
		StartCall = "TurnWheel",
	},
	Wait = {
		Prototype = Action,
		Name = "Wait",
		Procedure = DFA_NONE,
		Length = 1,
		Delay = 0,
		FacetBase = 0,
		NextAction = "Wait",
	},
};
local Name = "$Name$";