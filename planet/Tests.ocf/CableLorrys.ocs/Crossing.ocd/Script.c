/*--
	Cable Crossing
	Author: Randrian
--*/

/* Global */

global func WaypointsMakeList()
{
  var iterationLimit = ObjectCount(Find_ID(CableCrossing));
	for(var obj in FindObjects(Find_ID(CableCrossing)))
	{
		obj->ResetList();
		obj->AddNeighboursToList();
	}
	for(var i = 0; i < iterationLimit; i++)
		for(var obj in FindObjects(Find_ID(CableCrossing)))
			obj->AddNeighboursList();
}

/* Local */

// State
public func IsCableCrossing() { return 1; }
public func IsRailStation() { return GetLength(aPath) == 1; }

protected func Initialize()
{
  iRotation = 0;
  SetAction("Wait");
  SetGraphics(0, GetID(), 1, 1);
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

/* Animation stuff */

local iRotation;
local iActiveCount;
local bStation;
protected func TurnWheel()
{
  iRotation -= 4;
  var fsin=Sin(iRotation, 1000), fcos=Cos(iRotation, 1000);
  var xoff = 0;
  var yoff = 0;
  // set matrix values
  SetObjDrawTransform (
    +fcos, +fsin, (1000-fcos)*xoff - fsin*yoff,
    -fsin, +fcos, (1000-fcos)*yoff + fsin*xoff, 1
  );
}

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
	if (IsRailStation())
		if (!bStation)
		{
			SetGraphics("Station", CableCrossing, GFX_Overlay, GFXOV_MODE_Base);
			bStation = true;
		}
	else if (bStation)
	{
		SetGraphics(nil, CableCrossing, GFX_Overlay);
		bStation = false;
	}
}

local Name = "$Name$";
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