/**
	Breath Tube
	Displays the breath of the controlled clonk in the HUD.
	 
	@authors Mimmo_O, Clonkonaut
*/ 

// Current crew and breath
local crew;
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
	
	// Store crew.
	crew = GetCursor(GetOwner());

	// Add additional tubes.
	//btube = CreateObject(GUI_HealthTube);
	//btube->SetPosition(1,-1);
	//btube->SetGraphics("Back");
	
	ftube = CreateObject(GUI_BreathTube);
	ftube->SetPosition(1, -1);
	ftube->SetGraphics("Front");
	
	// Hide tube.
	UpdateTube(1000);
	HideTube();
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
	// Add update on cursor.
	if (!GetEffect("UpdateTube", this))
		AddEffect("UpdateTube", this, 100, 1, this);
	return;
}

protected func FxUpdateTubeStart(object target, proplist effect, int temporary)
{
	if (temporary != 0)
		return 1;
		
	// Don't do anything if breath is full
	crew = GetCursor();
	if (!crew)
		return -1;
	if (crew->GetBreath() == crew->GetMaxBreath())
		return -1;
		
	// Load breath tube.
	LoadTube();

	return 1;
}

protected func FxUpdateTubeTimer(object target, proplist effect, int time)
{
	crew = GetCursor();
	if (!crew)
	{
		HideTube();
		return -1;
	}
	
	if (!crew->GetMaxBreath())
	{
		HideTube();
		return -1;
	}
		
	var breath = crew->GetBreath();
	var promille =  1000 * breath / crew->GetMaxBreath();
	UpdateTube(promille);
	
	// Full breath: remove effect and tube.
	if (breath == crew->GetMaxBreath())
	{		
		FadeTube();
		return -1;		
	}

	return 1;
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

private func LoadTube()
{
	SetR(0);
	if (btube) btube->SetR(0);
	if (ftube) ftube->SetR(0);
	return;
}

private func HideTube()
{
	SetR(110);
	if (btube) btube->SetR(110);
	if (ftube) ftube->SetR(110);
	return;
}

private func FadeTube()
{
	AddEffect("FadeTube", this, 100, 1, this);
	return;
}

protected func FxFadeTubeTimer(object target, proplist effect, int time)
{
	var rot = GetR() + 10;
	// Rotate tubes.
	SetR(rot);
	if (btube) btube->SetR(rot);
	if (ftube) ftube->SetR(rot);
	// Stop effect if done.
	if (rot >= 110)
		return -1;	
	return 1;
}

local ActMap = {
	Swirl = {
		Prototype = Action,
		Name = "Swirl",
		Procedure = DFA_NONE,
		Length = 25,
		Delay = 2,
		X = 0,
		Y = 0,
		Wdt = 500,
		Hgt = 250,
		NextAction = "Swirl",
	},
};
