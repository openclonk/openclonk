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