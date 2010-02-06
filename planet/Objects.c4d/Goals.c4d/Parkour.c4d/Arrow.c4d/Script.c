/*-- Parkour arrow --*/

protected func Initialize()
{	
	this["Visibility"] = VIS_Owner;
	return;
}

/*-- Proplist --*/

func Definition(def) {
	SetProperty("Name", "$Name$", def);
	SetProperty("ActMap", {
		Show = {
			Prototype = Action,
			Name = "Show",
			Procedure = DFA_ATTACH,
			Length = 1,
			Delay = 0,
			X = 0,
			Y = 0,
			Wdt = 40,
			Hgt = 20,
			NextAction = "Show",
		},  
	}, def);
}