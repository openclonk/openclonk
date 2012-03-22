/*-- Cable line --*/

protected func Initialize()
{
	SetAction("Connect");
	SetVertexXY(0, GetX(), GetY());
	SetVertexXY(1, GetX(), GetY());
	SetProperty("LineColors", [RGB(80,80,80), RGB(80,80,80)]);
	iPhase = 0;
	iActiveCount = 0;
	return;
}

local prec, prec2;
local Layers;

public func UpdateDraw()
{
  prec = 100;
  prec2 = 1000/prec;
  var origin = CreateArray(2);
  var ending = CreateArray(2);
  GetActionTarget(0)->GetCablePosition(origin);
  GetActionTarget(1)->GetCablePosition(ending);
  SetPosition(origin[0], origin[1]);
  var Length = Distance(origin[0], origin[1], ending[0], ending[1])*prec;
  var pos = 0;
  var i = 0;
  GetActionTarget(0)->GetCablePosition(origin, prec);
  GetActionTarget(1)->GetCablePosition(ending, prec);
  var angle = Angle(origin[0], origin[1], ending[0], ending[1]);
//  Log("Length %d Angle %d", Length, angle);
  var xoff = 0;//Cos(angle, 6*prec);
  var yoff = 0;//Sin(angle, 6*prec);
  while(pos < Length)
  {
    SetGraphics("Line0", GetID(), i*2+1, GFXOV_MODE_Base);
    SetLineTransform(-angle, xoff+(ending[0]-origin[0])*pos/Length, yoff+(ending[1]-origin[1])*pos/Length, 1000, i*2+1, 1);
    SetGraphics("Line0", GetID(), i*2+2, GFXOV_MODE_Base);
    SetLineTransform(-angle, -xoff+(ending[0]-origin[0])*pos/Length, -yoff+(ending[1]-origin[1])*pos/Length, 1000, i*2+2, 1);
    SetClrModulation(RGB(255,255*pos/Length), i*2+1);
    SetClrModulation(RGB(255,255*pos/Length), i*2+2);
    i++;
    pos += 8*prec-10;
  }
  while(i <= Layers)
  {
    SetGraphics(nil, GetID(), i*2+1, GFXOV_MODE_Base);
    SetGraphics(nil, GetID(), i*2+2, GFXOV_MODE_Base);
    i++;
  }
  Layers = i;
}

protected func UpdateDraw2()
{
	// Breaking
	var Length = ObjectDistance(GetActionTarget(0), GetActionTarget(1));
	if (GetVertexNum() > 2 || Length > max_distance)
	{
		LineBreak();
		return RemoveObject();
	}

  prec = 100;
  prec2 = 1000/prec;
  SetPosition(GetActionTarget(0)->GetX(), GetActionTarget(0)->GetY());
  Length *= prec;
  var pos = 0;
  var i = 0;
  var startX = GetActionTarget(0)->GetX(prec), startY = GetActionTarget(0)->GetY(prec), endX = GetActionTarget(1)->GetX(prec), endY = GetActionTarget(1)->GetY(prec);
  var angle = Angle(startX, startY, endX, endY);
  var xoff = 0;//Cos(angle, 6*prec);
  var yoff = 0;//Sin(angle, 6*prec);
  while(pos < Length)
  {
    SetGraphics("Arrange", GetID(), i*2+1, GFXOV_MODE_Base);
    SetLineTransform(-angle, xoff+(endX-startX)*pos/Length, yoff+(endY-startY)*pos/Length, 1000, i*2+1, 1);
   // SetGraphics("Line0", GetID(), i*2+2, GFXOV_MODE_Base);
   // SetLineTransform(-angle, -xoff+(endX-startX)*pos/Length, -yoff+(endY-startY)*pos/Length, 1000, i*2+2, 1);
    i++;
    pos += 8*prec-10;
  }
  while(i <= Layers)
  {
    SetGraphics(nil, GetID(), i*2+1, GFXOV_MODE_Base);
    SetGraphics(nil, GetID(), i*2+2, GFXOV_MODE_Base);
    i++;
  }
  Layers = i;
}

local iPhase;

public func Active()
{
  var Name = Format("Line%d", iPhase);
  iPhase += 2;
  if(iPhase >= 8) iPhase = 0;
  for(var i = 0; i <= Layers; i++)
  {
    SetGraphics(Name, GetID(), i*2+1, 1);
    SetGraphics(Name, GetID(), i*2+2, 1);
  }
}

func SetLineTransform(int r, int xoff, int yoff, int length, int layer, int MirrorSegments) {
	if(!MirrorSegments) MirrorSegments = 1;
	var fsin=Sin(r, 1000), fcos=Cos(r, 1000);
	// set matrix values
	SetObjDrawTransform (
		+fcos*MirrorSegments, +fsin*length/1000, xoff*prec2,
		-fsin*MirrorSegments, +fcos*length/1000, yoff*prec2,layer
	);
}

// Returns true if this object is a functioning power line.
public func IsCableLine()
{
	return GetAction() == "Wait" || GetAction() == "Active";
}

public func GetOtherConnection(object obj)
{
  if(GetActionTarget(0) == obj) return GetActionTarget(1);
  if(GetActionTarget(1) == obj) return GetActionTarget(0);
}

// Returns whether this power line is connected to an object.
public func IsConnectedTo(object obj)
{
	return GetActionTarget(0) == obj || GetActionTarget(1) == obj;
}

// Returns the object which is connected to obj through this power line.
public func GetConnectedObject(object obj)
{
	if (GetActionTarget(0) == obj)
		return GetActionTarget(1);
	if (GetActionTarget(1) == obj)
		return GetActionTarget(0);
	return;
}

public func SetConnectedObjects(obj1, obj2)
{
  SetActionTargets(obj1, obj2);
  UpdateDraw();
  SetAction("Wait");
  obj1->AddCableConnection(this);
}

protected func LineBreak(bool no_msg)
{
	Sound("LineBreak");
	if (!no_msg) 
		BreakMessage();
	return;
}

private func BreakMessage()
{
	var line_end = GetActionTarget(0);
	if (line_end->GetID() != CableLorryReel) 
		line_end = GetActionTarget(1);
	if (line_end->Contained()) line_end = line_end->Contained();

	line_end->Message("$TxtLinebroke$");
	return;
}

local iActiveCount;

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

local ActMap = {
	Connect = {
		Prototype = Action,
		Name = "Connect",
		FacetBase = 0,
		Procedure = DFA_CONNECT,
		EndCall = "UpdateDraw2",
		Length = 1,
		Delay = 1,
		NextAction = "Connect",
	},
	Wait = {
		Prototype = Action,
		Name = "Wait",
		FacetBase = 0,
		Procedure = DFA_NONE,//CONNECT,
		NextAction = "Wait",
	},
	Active = {
		Prototype = Action,
		Name = "Active",
		Procedure = DFA_NONE,//CONNECT,
		Length = 1,
		Delay = 1,
		FacetBase = 0,
		EndCall = "Active",
		NextAction = "Active",
	},
};

local Name = "$Name$";
local max_distance = 200;	
