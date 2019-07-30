protected func Initialize()
{
	//this.Plane = 400;
SetProperty("MeshTransformation", Trans_Mul(Trans_Rotate(15, 0, 10), Trans_Scale(200)));	
	//Trans_Translate(200, 0, 700),
}

func SaveScenarioObject() { return false; }

local ActMap = {
		Attach = {
			Prototype = Action,
			Name = "Attach",
			Procedure = DFA_ATTACH,
			Directions = 1,
			X = 0,
			Y = 0,
			Wdt = 64,
			Hgt = 54,
			NextAction = "Attach",
		},
};