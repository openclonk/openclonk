/*-- Storm debug display --*/

func Initialize()
{
	SetAction("Connect");
}

func ShowData(array x, array y)
{
	while (RemoveVertex()) {}
	for (var i = 0; i<GetLength(x); ++i)
		AddVertex(x[i], y[i]);
}

local ActMap = {
	Connect = {
		Prototype = Action,
		Name = "Connect",
		Procedure = DFA_FLOAT,
		NextAction = "Connect"
	}
};

local Name = "$Name$";
		
