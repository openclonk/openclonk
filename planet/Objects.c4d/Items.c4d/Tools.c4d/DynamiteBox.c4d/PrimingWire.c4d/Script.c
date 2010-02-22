/*--- Priming wire ---*/

#strict 2

local fHasMessage;

protected func Initialize ()
{
  // Put the first to vertices on the actual position
  SetVertex(0,0,GetX()); SetVertex(0,1,GetY());
  SetVertex(1,0,GetX()); SetVertex(1,1,GetY());
}

public func Connect(pTarget1, pTarget2)
{
  SetAction("Connect", pTarget1, pTarget2);  
}

private func GetLineLength()
{
  var i = GetVertexNum()-1;
  var iDist = 0;
  while(i--)
  {
    // Calculate the length between the vertices
    iDist += Distance(GetVertex(i,0),GetVertex(i,1),GetVertex(i+1,0),GetVertex(i+1,1));
  }
  return iDist;
}

func Definition(def) {
  SetProperty("ActMap", {
		Connect = {
		Prototype = Action,
		Name = "Connect",
		Length = 0,
		Delay = 0,
		Procedure = DFA_CONNECT,
		NextAction = "Connect",
	},  }, def);
  SetProperty("Name", "$Name$", def);
}