/*-- Tutorial arrow --*/

func AttachTargetLost() { RemoveObject(); }

global func TutArrowClear()
{
	for (arrow in FindObjects(Find_ID(TutorialArrow)))
		arrow->RemoveObject();
	return;
}

global func TutArrowShowPos(int x, int y, int angle, int dist)
{
	//TutArrowClear();
	if (angle == nil) angle = 135;
	if (dist == nil) dist = 16;
	var arrow = CreateObject(TutorialArrow, x, y);
	if (!arrow) return;
	dist += 8; // arrow size
	x -= Sin(angle, dist);
	y += Cos(angle, dist);
	arrow->SetAction("Show");
	arrow->SetPosition(x,y);
	arrow->SetR(angle);
	return arrow;
}

global func TutArrowShowTarget(object target, int angle, int dist)
{
	//TutArrowClear();
	if (angle == nil) angle = 135;
	if (dist == nil) dist = 16;
	var arrow = CreateObject(TutorialArrow, target->GetX(), target->GetY());
	if (!arrow) return;
	arrow->SetAction("Attach", target);
	arrow->SetR(angle);
	dist += 8; // arrow size
	arrow->SetVertex(0, VTX_Y, -dist, VTX_SetPermanentUpd);
	return arrow;
}


/*-- Proplist --*/

protected func Definition(def) 
{
	SetProperty("Name", "$Name$", def);
	SetProperty("ActMap", {
		Attach = {
			Prototype = Action,
			Name = "Attach",
			Procedure = DFA_ATTACH,
			Length = 20,
			Delay = 2,
			X = 0,
			Y = 0,
			Wdt = 40,
			Hgt = 20,
			NextAction = "Attach",
			Animation = "Spin",
		},  
		Show = {
			Prototype = Action,
			Name = "Show",
			Procedure = DFA_FLOAT,
			Length = 20,
			Delay = 2,
			X = 0,
			Y = 0,
			Wdt = 40,
			Hgt = 20,
			NextAction = "Show",
			Animation = "Bounce",
		},  
	}, def);
}