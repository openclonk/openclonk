/**
	Health Tube
	Displays the health of the controlled clonk in the HUD.
	 
	@authors Mimmo_O, Clonkonaut
*/


// Current crew and energy
local crew, energy;
// Store additional tubes.
local btube, ftube;

protected func Initialize()
{
	// Set parallaxity
	this.Parallaxity = [0, 0];
	// Set visibility
	this.Visibility = VIS_Owner;
	return;
}

public func MakeTube()
{
	// Action and position.
	SetPosition(1, -1);
	SetAction("Swirl");
	
	// Store crew and current energy.
	crew = GetCursor(GetOwner());
	if (crew)
		energy = crew->GetEnergy();

	// Add additional tubes.
	btube = CreateObject(GUI_HealthTube);
	btube->SetPosition(1,-1);
	btube->SetGraphics("Back");
	
	ftube = CreateObject(GUI_HealthTube);
	ftube->SetPosition(1,-1);
	ftube->SetGraphics("Front");
	return;
}	

protected func Destruction()
{
	// Remove additional tubes.
	if (btube)
		btube->RemoveObject();
	if (ftube)
		ftube->RemoveObject();
	return;
}

public func Update()
{
	var cursor = GetCursor(GetOwner());
	if (!cursor)
		return;
	if (crew != cursor)
	{
		// New clonk.
		crew = cursor;
		energy = crew->GetEnergy();
		if (!crew->GetMaxEnergy())
			return;
		var promille =  1000 * energy / crew->GetMaxEnergy();
		UpdateTube(promille);
		UpdateNumber();
		RemoveEffect("UpdateTube", this);	
	}
	// Update on existing clonk.
	if (!GetEffect("UpdateTube", this))
		AddEffect("UpdateTube", this, 100, 1, this);
	return;
}

protected func FxUpdateTubeTimer(object target, proplist effect, int time)
{
	if (!crew->GetMaxEnergy())
		return -1;
		
	if (energy == crew->GetEnergy())
		return -1;

	if (Abs(energy - crew->GetEnergy()) > 6 )
		if (energy > crew->GetEnergy()) 
			energy -= 3;
		else
			energy += 3;
	else
		if (energy > crew->GetEnergy()) 
			energy -= 1;
		else
			energy += 1;

	var promille =  1000 * energy / crew->GetMaxEnergy();
	UpdateTube(promille);
	UpdateNumber();

	return 1;
}

private func UpdateNumber()
{
	CustomMessage(Format("@<c dd0000>%d</c>", energy), this, GetOwner(), 16, -72);
	return;
}

private func UpdateTube(int promille)
{
	var rot = - 210 - 69 * promille / 100; 
	var fsin = Sin(rot, 1000, 10);
	var fcos = Cos(rot, 1000, 10);
 	// Rotate via draw transform.
 	SetObjDrawTransform (+fcos, +fsin, 0, -fsin, +fcos, 0);
	return;
}

local ActMap = {
	Swirl = {
		Prototype = Action,
		Name = "Swirl",
		Procedure = DFA_NONE,
		Length = 25,
		Delay = 3,
		X = 0,
		Y = 0,
		Wdt = 450,
		Hgt = 225,
		NextAction = "Swirl",
	},
};


