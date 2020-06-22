/*-- Scaffold wall --*/

/* Initialization */

func SetLeft(object host)
{
	SetGraphics("Left");
	SetAction("Left", host);
	SetShape(0, 0, 4, 32);
	SetPosition(host->GetX()-16, host->GetY()-16);
	SetSolidMask(0, 0, 2, 32);
	SetVertexXY(0, 1, 1);
	return true;
}

func SetLeftExtension(object host)
{
	SetGraphics("Bottom");
	SetAction("Bottom", host);
	SetShape(0, 0, 32, 4);
	SetPosition(host->GetX()+13, host->GetY()+12);
	SetSolidMask(0, 0, 32, 4);
	SetVertexXY(0, 1 + 29,-27);
	SetHalfVehicleSolidMask(true);
	return true;
}

func SetRight(object host)
{
	SetGraphics("Right");
	SetAction("Right", host);
	SetShape(0, 0, 4, 32);
	SetPosition(host->GetX()+12, host->GetY()-16);
	SetSolidMask(0, 0, 2, 32, 2, 0);
	SetVertexXY(0,-27, 1);
	return true;
}

func SetRightExtension(object host)
{
	SetGraphics("Bottom");
	SetAction("Bottom", host);
	SetShape(0, 0, 32, 4);
	SetPosition(host->GetX()+13, host->GetY()+12);
	SetSolidMask(0, 0, 32, 4);
	SetVertexXY(0, 1-29,-27);
	SetHalfVehicleSolidMask(true);
	return true;
}

func SetTopRightExtension(object host)
{
	SetGraphics("Right");
	SetAction("Right", host);
	SetShape(0, 0, 4, 32);
	SetPosition(host->GetX()+15, host->GetY()-48);
	SetSolidMask(0, 0, 2, 32, 2, 0);
	SetVertexXY(0,-27, 1 + 32);
	return true;
}

func SetTopLeftExtension(object host)
{
	SetGraphics("Left");
	SetAction("Left", host);
	SetShape(0, 0, 4, 32);
	SetPosition(host->GetX()-16, host->GetY()-48);
	SetSolidMask(0, 0, 2, 32);
	SetVertexXY(0, 1, 1 + 32);
	return true;
}

func SetTop(object host)
{
	SetGraphics("Top");
	SetAction("Top", host);
	SetShape(0, 0, 32, 4);
	SetPosition(host->GetX()-16, host->GetY()-16);
	SetSolidMask(0, 0, 32, 4);
	SetVertexXY(0, 1, 1);
	SetHalfVehicleSolidMask(true);
	return true;
}

func SetBottom(object host)
{
	SetGraphics("Bottom");
	SetAction("Bottom", host);
	SetShape(0, 0, 32, 4);
	SetPosition(host->GetX()-16, host->GetY()+12);
	SetSolidMask(0, 0, 32, 4);
	SetVertexXY(0, 1,-27);
	SetHalfVehicleSolidMask(true);
	return true;
}


/* Destruction */

local ActMap = {
		Left = {
			Prototype = Action,
			Name = "Left",
			Procedure = DFA_ATTACH,
			Directions = 1,
			Length = 1,
			Delay = 0,
			FacetBase = 0,
			X = 0, Y = 0, Wdt = 4, Hgt = 32,
			NextAction = "Left",
		},
		Right = {
			Prototype = Action,
			Name = "Right",
			Procedure = DFA_ATTACH,
			Directions = 1,
			Length = 1,
			Delay = 0,
			FacetBase = 0,
			X = 0, Y = 0, Wdt = 4, Hgt = 32,
			NextAction = "Right",
		},
		Top = {
			Prototype = Action,
			Name = "Top",
			Procedure = DFA_ATTACH,
			Directions = 1,
			Length = 1,
			Delay = 0,
			FacetBase = 0,
			X = 0, Y = 0, Wdt = 32, Hgt = 4,
			NextAction = "Top",
		},
		Bottom = {
			Prototype = Action,
			Name = "Bottom",
			Procedure = DFA_ATTACH,
			Directions = 1,
			Length = 1,
			Delay = 0,
			FacetBase = 0,
			X = 0, Y = 0, Wdt = 32, Hgt = 4,
			NextAction = "Bottom",
		},
};

func Definition(def) {
	
}

local Name = "$Name$";
local Description = "$Description$";
local Plane = 121;
