/*-- Ropelbridge_Segment --*/

public func Initialize()
{
  SetSolidMask(50,0,2,8);
}

func GetLoadWeight()
{
  var weight = 50;
  for(obj in FindObjects(Find_AtRect(-5,-10,10,10), Find_Exclude(this), Find_NoContainer()))
    weight += obj->GetMass();
  return weight;
}

local Double;

func CreateDouble()
{
  if(!Double)
  {
    Double = CreateObject(GetID());
    Double.Plane = 1555;
    //Double->SetAction("Attach", this);
  }
}

local ActMap = {
Attach = {
  Prototype = Action,
  Name = "Attach",
  Procedure = DFA_ATTACH
},
};