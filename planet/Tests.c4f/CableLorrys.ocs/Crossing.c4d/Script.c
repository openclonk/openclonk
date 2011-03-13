/*--
	Cable Crossing
	Author: Randrian
--*/

func IsCableCrossing() { return 1; }

protected func Initialize()
{
  iRotation = 0;
  SetAction("Wait");
  SetGraphics(0, GetID(), 1, 1);
  return;
}


local aPath;

local aConnectionList;

public func GetNextWaypoint(pEnd)
{
  if(!aConnectionList) return;
	for(var item in aConnectionList)
		if(item[0] == pEnd)
			return item[1];
	return false;
}

public func GetLengthToTarget(pEnd)
{
	for(var item in aConnectionList)
		if(item[0] == pEnd)
			return item[2];
	return false;
}

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

public func ResetList()
{
  aPath = [];
  for(var line in FindObjects(Find_Func("IsConnectedTo", this)))
    aPath[GetLength(aPath)] = line;
  aConnectionList = [];
}

public func AddNeighboursToList()
{
	var obj;
	for(var i = 0; i < GetLength(aPath); i++)
	{
		obj = aPath[i]->GetOtherConnection(this);//EffectVar(0,this(),aPath[i]);
		aConnectionList[i] = [obj, obj, ObjectDistance(obj)];
	}
}

public func AddNeighboursList()
{
	var newList, obj, dist, pEnd;
	for(var i = 0; i < GetLength(aPath); i++)
	{
		obj = aPath[i]->GetOtherConnection(this);
		dist = ObjectDistance(obj);
		newList = obj->GetList();
		for(var j = 0; j < GetLength(newList); j++)
		{
			pEnd = newList[j][0];
			if(pEnd == this)
				continue;
			for(var k = 0; k < GetLength(aConnectionList); k++)
				if(aConnectionList[k][0] == pEnd)
					break;
			// Haben wir schon und ist so näher
			if(k < GetLength(aConnectionList) && aConnectionList[k][2] < newList[j][2]+dist)
				continue;
			aConnectionList[k] = [pEnd, obj, newList[j][2]+dist];
		}
	}
}

public func GetList() { return aConnectionList; }

local iRotation;
local iActiveCount;
func TurnWheel()
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

func AddActive(fRemove)
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
