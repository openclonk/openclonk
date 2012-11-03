// Elevator case back

func Initialize()
{
	//SetProperty("MeshTransformation", Trans_Mul(Trans_Scale(800), Trans_Translate(0, -2000)));
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