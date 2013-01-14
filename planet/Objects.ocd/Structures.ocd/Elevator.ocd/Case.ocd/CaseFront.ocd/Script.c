// Elevator case front

local Plane = 505;

func Initialize()
{
	//SetProperty("MeshTransformation", Trans_Scale(800));
}

func AttachTargetLost()
{
	RemoveObject();
}

local ActMap = {
	Attach = {
		Prototype = Action,
		Name = "Attach",
		Procedure = DFA_ATTACH,
		Directions = 1,
		FacetBase = 1,
		NextAction = "Attach"
	}
};