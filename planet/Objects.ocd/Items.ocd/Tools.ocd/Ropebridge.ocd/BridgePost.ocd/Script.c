/*-- Ropebridge_Post --*/

local Double;

func Initialize()
{
  if(FindObject(Find_ID(GetID()), Find_Exclude(this), Find_AtPoint()))
    return; // I am just a double!
  if(!Double)
  {
    Double = CreateObject(GetID());
    Double.Plane = 600;
    Double->SetAction("Attach", this);
    Double->SetGraphics("Foreground", GetID());
  }
}

local ActMap = {
    Attach = {
      Prototype = Action,
      Name = "Attach",
      Procedure = DFA_ATTACH,
      FacetBase = 1,
    },
};
local Name = "$Name$";
local Description = "$Description$";
	
