/*--Helper --*/

func Initialize()
{
	return(1);
}

func Definition(def) {
	SetProperty("Name", "solid mask helper", def);
	
	SetProperty("ActMap", {

		Attach = {
			Prototype = Action,
			Name = "Attach",
			Procedure = DFA_ATTACH,
			Directions = 0,
			FlipDir = 0,
			Length = 1,
			Delay = 0,
			X = 0,
			Y = 0,
			FacetBase = 1,
		//	InLiquidAction = "Swim",
		}
	});
}

func OnAttachTargetLost()
{
	return this->RemoveObject();
}

local Plane = 300;
